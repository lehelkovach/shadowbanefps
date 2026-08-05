# Create DefaultSlot AnimMontages from existing AS_MM_* swing sequences
# (Steel B/C, Serath A/B/C — sequences already imported; montages were skipped).
#
# Run via:
#   powershell -ExecutionPolicy Bypass -File .\scripts\Create-MannySwingMontagesFromSeq.ps1
# Or with Editor open: Window → Execute Python Script → this file

import unreal

DEST = "/Game/Characters/Mannequins/Animations/Combat"
SKELETON_PATH = "/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"
DEFAULT_SLOT = "DefaultSlot"

# (sequence_name, montage_name) — only creates montage if sequence exists
MONTAGE_FROM_SEQ = [
    ("AS_MM_SteelSwing_B", "AM_MM_SteelSwing_B"),
    ("AS_MM_SteelSwing_C", "AM_MM_SteelSwing_C"),
    ("AS_MM_SerathSwing_A", "AM_MM_SerathSwing_A"),
    ("AS_MM_SerathSwing_B", "AM_MM_SerathSwing_B"),
    ("AS_MM_SerathSwing_C", "AM_MM_SerathSwing_C"),
]


def read_montage_slots(montage):
    names = []
    try:
        result = unreal.AnimationLibrary.get_montage_slot_names(montage)
        if result is not None:
            names = [str(n) for n in result]
            if names:
                return names
    except Exception:
        pass
    try:
        tracks = montage.get_editor_property("slot_anim_tracks")
        for t in tracks or []:
            names.append(str(t.get_editor_property("slot_name")))
    except Exception:
        pass
    return names


def force_default_slot(montage) -> bool:
    slot = unreal.Name(DEFAULT_SLOT)
    wrote = False
    try:
        tracks = list(montage.get_editor_property("slot_anim_tracks") or [])
        if not tracks and hasattr(montage, "add_slot"):
            try:
                montage.add_slot(slot)
                tracks = list(montage.get_editor_property("slot_anim_tracks") or [])
                wrote = True
            except Exception as ex:
                unreal.log_warning(f"add_slot empty: {ex}")

        new_tracks = []
        for t in tracks:
            try:
                t.set_editor_property("slot_name", slot)
                new_tracks.append(t)
                wrote = True
            except Exception:
                new_tracks.append(t)

        if new_tracks:
            try:
                montage.set_editor_property("slot_anim_tracks", new_tracks)
                wrote = True
            except Exception as ex:
                unreal.log_warning(f"set slot_anim_tracks: {ex}")
    except Exception as ex:
        unreal.log_warning(f"force_default_slot: {ex}")

    try:
        skel = montage.get_editor_property("skeleton")
        if skel:
            for meth in ("register_slot_node", "RegisterSlotNode", "add_slot"):
                if hasattr(skel, meth):
                    try:
                        getattr(skel, meth)(slot)
                        break
                    except Exception:
                        pass
    except Exception:
        pass

    montage.modify()
    return wrote


def create_montage(anim, skeleton, montage_name: str):
    mont_path = f"{DEST}/{montage_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(mont_path):
        unreal.EditorAssetLibrary.delete_asset(mont_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mont_factory = unreal.AnimMontageFactory()
    mont_factory.set_editor_property("target_skeleton", skeleton)
    mont_factory.set_editor_property("source_animation", anim)
    montage = asset_tools.create_asset(montage_name, DEST, unreal.AnimMontage, mont_factory)
    if not montage:
        unreal.log_error(f"Failed to create montage {montage_name}")
        return None

    try:
        montage.blend_in_time = 0.05
        montage.blend_out_time = 0.15
    except Exception:
        pass

    force_default_slot(montage)
    unreal.EditorAssetLibrary.save_asset(mont_path)
    montage = unreal.EditorAssetLibrary.load_asset(mont_path)
    if montage:
        force_default_slot(montage)
        unreal.EditorAssetLibrary.save_asset(mont_path)

    slots = read_montage_slots(montage) if montage else []
    unreal.log(f"montage OK {montage_name} slots={slots} path={mont_path}")
    if DEFAULT_SLOT not in slots:
        unreal.log_warning(f"montage {montage_name} missing DefaultSlot — slots={slots}")
    return montage


def main():
    unreal.log("Create-MannySwingMontagesFromSeq: start")
    skeleton = unreal.EditorAssetLibrary.load_asset(SKELETON_PATH)
    if not skeleton:
        unreal.log_error(f"Missing skeleton {SKELETON_PATH}")
        raise RuntimeError("missing SK_Mannequin")

    ok = 0
    for seq_name, mont_name in MONTAGE_FROM_SEQ:
        seq_path = f"{DEST}/{seq_name}"
        anim = unreal.EditorAssetLibrary.load_asset(seq_path)
        if not anim:
            unreal.log_error(f"Missing sequence {seq_path} — skip {mont_name}")
            continue
        if not isinstance(anim, unreal.AnimSequence):
            unreal.log_error(f"{seq_name} is not AnimSequence — skip")
            continue
        mont = create_montage(anim, skeleton, mont_name)
        if mont:
            ok += 1

    unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)
    unreal.log(
        f"Create-MannySwingMontagesFromSeq: OK montages={ok}/{len(MONTAGE_FROM_SEQ)} dest={DEST}"
    )


if __name__ == "__main__":
    main()
