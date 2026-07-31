# Import Kenney CC0 handaxe FBX into /Game/Art/Weapons
import unreal
import os

stage = r"C:\Users\Lehel\shadowbanefps\Content\Art\Weapons\Import"
dest = "/Game/Art/Weapons"

if not unreal.EditorAssetLibrary.does_directory_exist(dest):
    unreal.EditorAssetLibrary.make_directory(dest)

files = [
    os.path.join(stage, "SM_HandAxe.fbx"),
    os.path.join(stage, "SM_HandAxe_Upgraded.fbx"),
    os.path.join(stage, "Textures", "colormap.png"),
]

tasks = []
for path in files:
    if not os.path.isfile(path):
        unreal.log_warning("Missing: " + path)
        continue
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = dest
    task.replace_existing = True
    task.automated = True
    task.save = True
    # Prefer static mesh for FBX
    if path.lower().endswith(".fbx"):
        task.destination_name = os.path.splitext(os.path.basename(path))[0]
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for t in tasks:
    unreal.log("Imported paths: %s" % str(list(t.imported_object_paths)))

assets = unreal.EditorAssetLibrary.list_assets(dest, recursive=True, include_folder=False)
unreal.log("Weapons assets: " + ", ".join([str(a) for a in assets]))
unreal.EditorAssetLibrary.save_directory(dest, only_if_is_dirty=False, recursive=True)
