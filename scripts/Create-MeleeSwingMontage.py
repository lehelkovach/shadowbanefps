# Bake a readable axe-swing AnimSequence + AnimMontage for SK_Mannequin.
# Source pose: MM_Idle (legs/torso stay grounded). Right-arm chain + light spine twist.
#
# Run via:
#   powershell -ExecutionPolicy Bypass -File .\scripts\Create-MeleeSwingMontage.ps1

import unreal
import math

DEST = "/Game/Characters/Mannequins/Animations/Combat"
SEQ_NAME = "AS_MM_AxeSwing_01"
MONTAGE_NAME = "AM_MM_AxeSwing_01"
IDLE_PATH = "/Game/Characters/Mannequins/Animations/Manny/MM_Idle.MM_Idle"
SKELETON_PATH = "/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"
MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"

FPS = 30
DURATION = 0.42
NUM_FRAMES = max(2, int(round(DURATION * FPS)) + 1)  # inclusive end ~13 keys

SWING_BONES = [
    "clavicle_r",
    "upperarm_r",
    "lowerarm_r",
    "hand_r",
    "spine_01",
    "spine_02",
    "spine_03",
    "clavicle_l",
    "upperarm_l",
    "lowerarm_l",
]


def smooth01(t: float) -> float:
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def lerp(a: float, b: float, u: float) -> float:
    return a + (b - a) * u


def swing_euler(bone: str, alpha: float):
    """Additive local-space degrees (pitch, yaw, roll) for one bone at swing alpha 0..1."""
    a = max(0.0, min(1.0, alpha))
    if a <= 0.001 or a >= 0.999:
        return (0.0, 0.0, 0.0)

    # Key poses: idle(0) -> windup(0.30) -> strike(0.55) -> idle(1)
    if a < 0.30:
        u = smooth01(a / 0.30)
        windup = {
            "clavicle_r": (-8.0, -18.0, 12.0),
            "upperarm_r": (-70.0, -45.0, 35.0),
            "lowerarm_r": (10.0, 5.0, -65.0),
            "hand_r": (20.0, -25.0, 15.0),
            "spine_03": (4.0, -12.0, 8.0),
            "spine_02": (2.0, -6.0, 4.0),
            "spine_01": (0.0, -3.0, 2.0),
            "clavicle_l": (4.0, 8.0, -6.0),
            "upperarm_l": (15.0, 20.0, -12.0),
            "lowerarm_l": (5.0, 0.0, -20.0),
        }
        p = windup.get(bone, (0.0, 0.0, 0.0))
        return (p[0] * u, p[1] * u, p[2] * u)

    if a < 0.55:
        u = smooth01((a - 0.30) / 0.25)
        power = u * u
        windup = {
            "clavicle_r": (-8.0, -18.0, 12.0),
            "upperarm_r": (-70.0, -45.0, 35.0),
            "lowerarm_r": (10.0, 5.0, -65.0),
            "hand_r": (20.0, -25.0, 15.0),
            "spine_03": (4.0, -12.0, 8.0),
            "spine_02": (2.0, -6.0, 4.0),
            "spine_01": (0.0, -3.0, 2.0),
            "clavicle_l": (4.0, 8.0, -6.0),
            "upperarm_l": (15.0, 20.0, -12.0),
            "lowerarm_l": (5.0, 0.0, -20.0),
        }
        strike = {
            "clavicle_r": (10.0, 28.0, -8.0),
            "upperarm_r": (45.0, 70.0, -20.0),
            "lowerarm_r": (-5.0, -10.0, -15.0),
            "hand_r": (-10.0, 30.0, -20.0),
            "spine_03": (-2.0, 18.0, -6.0),
            "spine_02": (-1.0, 10.0, -3.0),
            "spine_01": (0.0, 5.0, -1.0),
            "clavicle_l": (-2.0, -6.0, 4.0),
            "upperarm_l": (-5.0, -10.0, 8.0),
            "lowerarm_l": (0.0, 0.0, -5.0),
        }
        w = windup.get(bone, (0.0, 0.0, 0.0))
        s = strike.get(bone, (0.0, 0.0, 0.0))
        return (lerp(w[0], s[0], power), lerp(w[1], s[1], power), lerp(w[2], s[2], power))

    u = smooth01((a - 0.55) / 0.45)
    strike = {
        "clavicle_r": (10.0, 28.0, -8.0),
        "upperarm_r": (45.0, 70.0, -20.0),
        "lowerarm_r": (-5.0, -10.0, -15.0),
        "hand_r": (-10.0, 30.0, -20.0),
        "spine_03": (-2.0, 18.0, -6.0),
        "spine_02": (-1.0, 10.0, -3.0),
        "spine_01": (0.0, 5.0, -1.0),
        "clavicle_l": (-2.0, -6.0, 4.0),
        "upperarm_l": (-5.0, -10.0, 8.0),
        "lowerarm_l": (0.0, 0.0, -5.0),
    }
    s = strike.get(bone, (0.0, 0.0, 0.0))
    return (lerp(s[0], 0.0, u), lerp(s[1], 0.0, u), lerp(s[2], 0.0, u))


def quat_from_euler_deg(pitch, yaw, roll):
    return unreal.Quat.from_euler(unreal.Vector(pitch, yaw, roll))


def ensure_dir(path: str):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def delete_if_exists(object_path: str):
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        unreal.EditorAssetLibrary.delete_asset(object_path)


def main():
    unreal.log("Create-MeleeSwingMontage: start")
    idle = unreal.EditorAssetLibrary.load_asset(IDLE_PATH)
    skeleton = unreal.EditorAssetLibrary.load_asset(SKELETON_PATH)
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if not idle or not skeleton:
        unreal.log_error("Missing MM_Idle or SK_Mannequin")
        return

    ensure_dir(DEST)
    seq_path = f"{DEST}/{SEQ_NAME}"
    mont_path = f"{DEST}/{MONTAGE_NAME}"
    delete_if_exists(seq_path)
    delete_if_exists(mont_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    seq_factory = unreal.AnimSequenceFactory()
    seq_factory.set_editor_property("target_skeleton", skeleton)
    if mesh:
        seq_factory.set_editor_property("preview_skeletal_mesh", mesh)

    anim = asset_tools.create_asset(SEQ_NAME, DEST, unreal.AnimSequence, seq_factory)
    if not anim:
        unreal.log_error("Failed to create AnimSequence")
        return

    opts = unreal.AnimPoseEvaluationOptions()

    def eval_idle_pose():
        # UE5.5 Python: ScriptMethod returns the pose (no out-param).
        try:
            return unreal.AnimPoseExtensions.get_anim_pose_at_time(idle, 0.0, opts)
        except TypeError:
            pose = unreal.AnimPose()
            unreal.AnimPoseExtensions.get_anim_pose_at_time(idle, 0.0, opts, pose)
            return pose

    base_pose = eval_idle_pose()
    if not unreal.AnimPoseExtensions.is_valid(base_pose):
        # Fallback: skeleton ref pose.
        try:
            base_pose = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
        except TypeError:
            base_pose = unreal.AnimPose()
            unreal.AnimPoseExtensions.get_reference_pose(skeleton, base_pose)
    if not unreal.AnimPoseExtensions.is_valid(base_pose):
        unreal.log_error("Failed to evaluate base pose")
        return

    bone_names = unreal.AnimPoseExtensions.get_bone_names(base_pose)
    if bone_names is None:
        bone_names = []
        unreal.AnimPoseExtensions.get_bone_names(base_pose, bone_names)
    if not bone_names:
        unreal.log_error("Base pose has no bones")
        return

    # Pre-size key arrays per bone.
    pos_keys = {str(b): [None] * NUM_FRAMES for b in bone_names}
    rot_keys = {str(b): [None] * NUM_FRAMES for b in bone_names}
    scl_keys = {str(b): [None] * NUM_FRAMES for b in bone_names}
    swing_set = set(SWING_BONES)

    for frame in range(NUM_FRAMES):
        alpha = float(frame) / float(NUM_FRAMES - 1)
        pose = eval_idle_pose()
        if not unreal.AnimPoseExtensions.is_valid(pose):
            try:
                pose = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
            except TypeError:
                pose = unreal.AnimPose()
                unreal.AnimPoseExtensions.get_reference_pose(skeleton, pose)

        for bone in SWING_BONES:
            if bone not in swing_set:
                continue
            # Confirm bone exists
            try:
                local = unreal.AnimPoseExtensions.get_bone_pose(
                    pose, unreal.Name(bone), unreal.AnimPoseSpaces.LOCAL
                )
            except Exception:
                continue
            pitch, yaw, roll = swing_euler(bone, alpha)
            if abs(pitch) + abs(yaw) + abs(roll) < 0.001:
                continue
            add = unreal.Transform(
                unreal.Vector(0, 0, 0),
                unreal.Rotator(pitch, yaw, roll),
                unreal.Vector(1, 1, 1),
            )
            combined = add * local
            unreal.AnimPoseExtensions.set_bone_pose(
                pose, combined, unreal.Name(bone), unreal.AnimPoseSpaces.LOCAL
            )

        for b in bone_names:
            name = str(b)
            t = unreal.AnimPoseExtensions.get_bone_pose(
                pose, b, unreal.AnimPoseSpaces.LOCAL
            )
            pos_keys[name][frame] = unreal.Vector(t.translation.x, t.translation.y, t.translation.z)
            rot_keys[name][frame] = unreal.Quat(t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w)
            scl_keys[name][frame] = unreal.Vector(t.scale3d.x, t.scale3d.y, t.scale3d.z)

    controller = anim.controller
    if not controller:
        unreal.log_error("AnimSequence has no controller")
        return
    controller.open_bracket(unreal.Text("Bake melee axe swing"))
    controller.set_frame_rate(unreal.FrameRate(FPS, 1), False)
    # Number of frames in UE often means sample count; play length = (N)/fps or (N-1)/fps depending on version.
    controller.set_number_of_frames(unreal.FrameNumber(NUM_FRAMES), False)
    controller.remove_all_bone_tracks(False)

    for b in bone_names:
        name = str(b)
        controller.add_bone_curve(unreal.Name(name), False)
        ok = controller.set_bone_track_keys(
            unreal.Name(name),
            pos_keys[name],
            rot_keys[name],
            scl_keys[name],
            False,
        )
        if not ok:
            unreal.log_warning(f"Failed bone keys: {name}")

    controller.close_bracket()
    try:
        unreal.AnimationLibrary.finalize_bone_animation(anim)
    except Exception as ex:
        unreal.log_warning(f"finalize_bone_animation: {ex}")

    # Create montage from sequence (DefaultSlot).
    mont_factory = unreal.AnimMontageFactory()
    mont_factory.set_editor_property("target_skeleton", skeleton)
    mont_factory.set_editor_property("source_animation", anim)

    montage = asset_tools.create_asset(MONTAGE_NAME, DEST, unreal.AnimMontage, mont_factory)
    if not montage:
        unreal.log_error("Failed to create AnimMontage")
        unreal.EditorAssetLibrary.save_asset(seq_path)
        return

    # Blend times — snappy melee chop.
    try:
        montage.set_editor_property("blend_in", unreal.AlphaBlend(0.05))
    except Exception:
        pass
    try:
        montage.blend_in_time = 0.05
        montage.blend_out_time = 0.12
    except Exception:
        pass

    unreal.EditorAssetLibrary.save_asset(seq_path)
    unreal.EditorAssetLibrary.save_asset(mont_path)
    unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)

    slots = []
    try:
        unreal.AnimationLibrary.get_montage_slot_names(montage, slots)
    except Exception:
        slots = ["?"]
    unreal.log(
        f"Create-MeleeSwingMontage: OK seq={seq_path} montage={mont_path} frames={NUM_FRAMES} slots={slots}"
    )


if __name__ == "__main__":
    main()
