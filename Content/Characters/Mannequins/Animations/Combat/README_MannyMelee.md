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
| `AS_MM_GreystoneSwing_A/B/C` (+ RMB anim) | `AM_MM_GreystoneSwing_A/B/C` (+ RMB) |
| `AS_MM_SteelSwing_A` (+ B/C seq) | `AM_MM_SteelSwing_A` (+ B/C if baked) |
| `AS_MM_SerathSwing_A/B/C` | montages optional via `Create-MannySwingMontagesFromSeq.ps1` |

## Runtime combo (Manny, `sb.Hero.UseShadowKight 0`)

Greystone **A → B → C** on LMB (`AM_MM_GreystoneSwing_*`):

1. LMB → play **A** (~1.7s). No mid-montage interrupt.
2. After A **ends**, **0.5s** window (`sb.Melee.ComboWindowSec`) to press for **B**. Same after B for **C**.
3. Miss the window → next LMB is **A**. After **C** → next is always **A**.
4. **Bank LMB:** press during the anim → one queued continue; when the montage ends, auto-plays B/C (no extra click). Spam during anim still counts as one bank.
5. **RMB** while a swing is playing → cancel montage/chop, clear combo + bank + window (reset to A). RMB is not otherwise bound for melee; bow/aim uses LMB.

Fallback if Greystone assets missing: Preferred list → Steel A → `AM_MM_AxeSwing_01`. Emergency procedural chop if slot fails.

### Test

| Action | Expect |
|---|---|
| LMB | Greystone A |
| Spam LMB during A | A finishes → auto B |
| Wait >0.5s after A ends, LMB | A again |
| A end → LMB within 0.5s → B end → LMB within 0.5s | C, then next A |
| RMB mid-swing | Cancel, next LMB is A |
