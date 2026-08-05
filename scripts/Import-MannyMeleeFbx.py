# Import Manny-retargeted Paragon melee FBX as AnimSequences on SK_Mannequin,
# then create DefaultSlot AnimMontages under /Game/Characters/Mannequins/Animations/Combat.
#
# Source FBX (already copied, do NOT un-park the 5k vault):
#   Content/Characters/Mannequins/Animations/Combat/Import_MannyFbx/*.FBX
#
# Run via:
#   powershell -ExecutionPolicy Bypass -File .\scripts\Import-MannyMeleeFbx.ps1
# Or with Editor open: Window → Execute Python Script → this file
# (prefer Editor closed so Content is not locked).

import unreal
import os

DEST = "/Game/Characters/Mannequins/Animations/Combat"
SKELETON_PATH = "/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"
MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"
ABP_PATH = "/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny"
DEFAULT_SLOT = "DefaultSlot"

# Absolute staging folder (FBX sit under Content only as import source).
FBX_DIR = r"C:\Users\Lehel\shadowbanefps\Content\Characters\Mannequins\Animations\Combat\Import_MannyFbx"

# (fbx_filename, anim_sequence_name, create_montage?, montage_name_or_None)
IMPORT_MAP = [
    ("GreystoneManny_Attack_PrimaryA.FBX", "AS_MM_GreystoneSwing_A", True, "AM_MM_GreystoneSwing_A"),
    ("GreystoneManny_Attack_PrimaryB.FBX", "AS_MM_GreystoneSwing_B", True, "AM_MM_GreystoneSwing_B"),
    ("GreystoneManny_Attack_PrimaryC.FBX", "AS_MM_GreystoneSwing_C", True, "AM_MM_GreystoneSwing_C"),
    ("GreystoneManny_Attack_RMB.FBX", "AS_MM_GreystoneSwing_RMB", True, "AM_MM_GreystoneSwing_RMB"),
    ("steelmanny_Steel_Attack_Melee_A.FBX", "AS_MM_SteelSwing_A", True, "AM_MM_SteelSwing_A"),
    ("steelmanny_Steel_Attack_Melee_B.FBX", "AS_MM_SteelSwing_B", True, "AM_MM_SteelSwing_B"),
    ("steelmanny_Steel_Attack_Melee_C.FBX", "AS_MM_SteelSwing_C", True, "AM_MM_SteelSwing_C"),
    ("SerathManny_Primary_Attack_A_Medium.FBX", "AS_MM_SerathSwing_A", True, "AM_MM_SerathSwing_A"),
    ("SerathManny_Primary_Attack_B_Medium.FBX", "AS_MM_SerathSwing_B", True, "AM_MM_SerathSwing_B"),
    ("SerathManny_Primary_Attack_C_Medium_120fps.FBX", "AS_MM_SerathSwing_C", True, "AM_MM_SerathSwing_C"),
]


def ensure_dir(path: str):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def delete_if_exists(object_path: str):
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        unreal.EditorAssetLibrary.delete_asset(object_path)


def soft_path(asset) -> str:
    if not asset:
        return "?"
    try:
        return asset.get_path_name()
    except Exception:
        return str(asset)


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
        arr = unreal.Array(unreal.Name)
        unreal.AnimationLibrary.get_montage_slot_names(montage, arr)
        names = [str(n) for n in arr]
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
        unreal.log(f"force_default_slot: track_count={len(tracks)} before={read_montage_slots(montage)}")
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
                try:
                    nt = unreal.SlotAnimationTrack()
                    nt.set_editor_property("slot_name", slot)
                    try:
                        nt.set_editor_property("anim_track", t.get_editor_property("anim_track"))
                    except Exception:
                        pass
                    new_tracks.append(nt)
                    wrote = True
                except Exception as ex2:
                    unreal.log_warning(f"rebuild SlotAnimationTrack: {ex2}")
                    new_tracks.append(t)

        if new_tracks:
            try:
                montage.set_editor_property("slot_anim_tracks", new_tracks)
                wrote = True
            except Exception as ex:
                unreal.log_warning(f"set slot_anim_tracks: {ex}")
    except Exception as ex:
        unreal.log_warning(f"force_default_slot tracks path: {ex}")

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


def make_fbx_anim_options(skeleton):
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", False)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("skeleton", skeleton)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property("original_import_type", unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("create_physics_asset", False)

    try:
        options.set_editor_property("anim_sequence_import_data", options.anim_sequence_import_data)
    except Exception:
        pass

    anim_data = options.get_editor_property("anim_sequence_import_data")
    if anim_data:
        for prop, val in (
            ("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME),
            ("import_meshes_in_bone_hierarchy", False),
            ("use_default_sample_rate", True),
            ("import_custom_attribute", False),
            ("delete_existing_custom_attribute_curves", True),
            ("delete_existing_non_curve_custom_attributes", True),
            ("import_bone_tracks", True),
            ("set_material_drive_parameter_on_custom_attribute", False),
            ("add_curve_metadata_to_skeleton", False),
            ("remove_redundant_keys", True),
            ("do_not_import_curve_with_zero", True),
        ):
            try:
                anim_data.set_editor_property(prop, val)
            except Exception:
                pass
    return options


def import_fbx_as_sequence(fbx_path: str, seq_name: str, skeleton) -> unreal.AnimSequence:
    ensure_dir(DEST)
    seq_path = f"{DEST}/{seq_name}"
    delete_if_exists(seq_path)

    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = DEST
    task.destination_name = seq_name
    task.replace_existing = True
    task.automated = True
    task.save = True
    task.options = make_fbx_anim_options(skeleton)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported = list(task.imported_object_paths or [])
    unreal.log(f"import {seq_name}: paths={imported}")

    anim = unreal.EditorAssetLibrary.load_asset(seq_path)
    if not anim and imported:
        # FBX importer may append _Anim or keep FBX basename.
        for p in imported:
            a = unreal.EditorAssetLibrary.load_asset(p)
            if a and isinstance(a, unreal.AnimSequence):
                # Rename/move to expected name if needed.
                if soft_path(a) != seq_path and soft_path(a).rstrip("/") != seq_path:
                    try:
                        moved = unreal.EditorAssetLibrary.rename_asset(soft_path(a), seq_path)
                        if moved:
                            anim = unreal.EditorAssetLibrary.load_asset(seq_path)
                    except Exception as ex:
                        unreal.log_warning(f"rename imported seq: {ex}")
                        anim = a
                else:
                    anim = a
                break

    if not anim:
        # Last chance: list DEST for anything matching basename.
        for p in unreal.EditorAssetLibrary.list_assets(DEST, recursive=False, include_folder=False):
            if seq_name in str(p):
                anim = unreal.EditorAssetLibrary.load_asset(p)
                if anim:
                    break

    if anim and not isinstance(anim, unreal.AnimSequence):
        unreal.log_warning(f"{seq_name} loaded but type={type(anim)}")
        return None

    if anim:
        try:
            sk = anim.get_editor_property("skeleton")
            unreal.log(
                f"seq OK {seq_name} skeleton={sk.get_name() if sk else '?'} len={anim.get_play_length():.3f}s"
            )
        except Exception:
            unreal.log(f"seq OK {seq_name}")
        unreal.EditorAssetLibrary.save_asset(seq_path)
    return anim


def create_montage(anim, skeleton, montage_name: str) -> unreal.AnimMontage:
    mont_path = f"{DEST}/{montage_name}"
    delete_if_exists(mont_path)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mont_factory = unreal.AnimMontageFactory()
    mont_factory.set_editor_property("target_skeleton", skeleton)
    mont_factory.set_editor_property("source_animation", anim)
    montage = asset_tools.create_asset(montage_name, DEST, unreal.AnimMontage, mont_factory)
    if not montage:
        unreal.log_error(f"Failed to create montage {montage_name}")
        return None

    try:
        montage.set_editor_property("blend_in", unreal.AlphaBlend(0.05))
    except Exception:
        pass
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


def sniff_abp_default_slot():
    """Best-effort: confirm ABP_Manny mentions DefaultSlot (needed for Montage_Play)."""
    abp = unreal.EditorAssetLibrary.load_asset(ABP_PATH)
    if not abp:
        unreal.log_warning(f"ABP_Manny missing at {ABP_PATH}")
        return
    try:
        # Soft string sniff via asset path / export is limited in -NullRHI; log presence.
        unreal.log(f"ABP_Manny loaded: {soft_path(abp)} class={abp.get_class().get_name()}")
    except Exception as ex:
        unreal.log_warning(f"ABP sniff: {ex}")
    try:
        skel = unreal.EditorAssetLibrary.load_asset(SKELETON_PATH)
        if skel:
            slot = unreal.Name(DEFAULT_SLOT)
            for meth in ("register_slot_node", "RegisterSlotNode", "add_slot"):
                if hasattr(skel, meth):
                    try:
                        getattr(skel, meth)(slot)
                        unreal.log(f"SK_Mannequin.{meth}(DefaultSlot) OK")
                        unreal.EditorAssetLibrary.save_asset(SKELETON_PATH)
                        break
                    except Exception as ex:
                        unreal.log_warning(f"skel.{meth}: {ex}")
    except Exception as ex:
        unreal.log_warning(f"register DefaultSlot on skeleton: {ex}")


def main():
    unreal.log("Import-MannyMeleeFbx: start")
    skeleton = unreal.EditorAssetLibrary.load_asset(SKELETON_PATH)
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if not skeleton:
        unreal.log_error(f"Missing skeleton {SKELETON_PATH}")
        raise RuntimeError("missing SK_Mannequin")
    unreal.log(f"skeleton={soft_path(skeleton)} mesh={soft_path(mesh)}")

    ensure_dir(DEST)
    sniff_abp_default_slot()

    ok_seq = 0
    ok_mont = 0
    for fbx_name, seq_name, want_mont, mont_name in IMPORT_MAP:
        fbx_path = os.path.join(FBX_DIR, fbx_name)
        if not os.path.isfile(fbx_path):
            unreal.log_error(f"Missing FBX: {fbx_path}")
            continue
        anim = import_fbx_as_sequence(fbx_path, seq_name, skeleton)
        if not anim:
            unreal.log_error(f"Import failed: {fbx_name} -> {seq_name}")
            continue
        ok_seq += 1
        if want_mont and mont_name:
            mont = create_montage(anim, skeleton, mont_name)
            if mont:
                ok_mont += 1

    unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)
    unreal.log(
        f"Import-MannyMeleeFbx: OK seq={ok_seq}/{len(IMPORT_MAP)} montages={ok_mont} dest={DEST}"
    )
    # Proof line for PS1 scrape:
    primary = f"{DEST}/AM_MM_GreystoneSwing_A"
    if unreal.EditorAssetLibrary.does_asset_exist(primary):
        unreal.log(f"Import-MannyMeleeFbx: PRIMARY_OK {primary}")
    else:
        unreal.log_error(f"Import-MannyMeleeFbx: PRIMARY_MISSING {primary}")


if __name__ == "__main__":
    main()
