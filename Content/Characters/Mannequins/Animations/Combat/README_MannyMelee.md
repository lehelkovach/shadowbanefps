# Manny melee FBX import (Paragon→Manny Greystone / Steel / Serath)

Do **not** un-park `_ParkedContent` or re-import the 5k vault. Staging FBX already live at:

`Content/Characters/Mannequins/Animations/Combat/Import_MannyFbx/`

## Import + montages

Prefer Editor **closed** (locks Content):

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\Import-MannyMeleeFbx.ps1
```

If Editor must stay open: **Window → Execute Python Script** → `scripts/Import-MannyMeleeFbx.py`

Writes under `/Game/Characters/Mannequins/Animations/Combat` on skeleton `SK_Mannequin`:

| Sequence | Montage (DefaultSlot) |
|---|---|
| `AS_MM_GreystoneSwing_A/B/C` (+ RMB) | `AM_MM_GreystoneSwing_A/B/C` (+ RMB) |
| `AS_MM_SteelSwing_A` (+ B/C seq only) | `AM_MM_SteelSwing_A` |
| `AS_MM_SerathSwing_A/B/C` | (seq only) |

## Runtime (C++)

With `sb.Hero.UseShadowKight 0`, LMB prefers `AM_MM_GreystoneSwing_A` → B → C → `AM_MM_SteelSwing_A` → `AM_MM_AxeSwing_01` on `ABP_Manny` DefaultSlot. If montage/slot fails entirely, emergency procedural chop still fires so LMB is not dead.
