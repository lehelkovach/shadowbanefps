# Import Kenney Castle Kit (CC0) pieces for Broken Citadel dressing.
# Source zip: Content/Art/Weapons/Raw/kenney-castle.zip (extracted to Raw/castle).
import unreal
import os

src = r"C:\Users\Lehel\shadowbanefps\Content\Art\Weapons\Raw\castle\Models\FBX format"
dest = "/Game/Art/Castle"
names = [
    "ground.fbx",
    "gate.fbx",
    "wall.fbx",  # may not exist in castle kit — skip if missing
    "tower-square-base.fbx",
    "tower-square-mid.fbx",
    "tower-square-roof.fbx",
    "stairs-stone.fbx",
    "rocks-large.fbx",
    "siege-ram.fbx",
    "siege-ballista.fbx",
    "bridge-straight.fbx",
    "flag-banner-long.fbx",
]

# Mini-dungeon walls as curtain fillers
dungeon = r"C:\Users\Lehel\shadowbanefps\Content\Art\Weapons\Raw\mini-dungeon\Models\FBX format"
dungeon_names = ["wall.fbx", "wall-opening.fbx", "floor.fbx", "gate.fbx", "column.fbx"]

if not unreal.EditorAssetLibrary.does_directory_exist(dest):
    unreal.EditorAssetLibrary.make_directory(dest)

tasks = []
for folder, file_list in ((src, names), (dungeon, dungeon_names)):
    for name in file_list:
        path = os.path.join(folder, name)
        if not os.path.isfile(path):
            unreal.log_warning("Skip missing: " + path)
            continue
        task = unreal.AssetImportTask()
        task.filename = path
        task.destination_path = dest
        task.destination_name = os.path.splitext(name)[0].replace("-", "_")
        task.replace_existing = True
        task.automated = True
        task.save = True
        tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    unreal.log("Imported: %s" % str(list(t.imported_object_paths)))
unreal.EditorAssetLibrary.save_directory(dest, only_if_is_dirty=False, recursive=True)
