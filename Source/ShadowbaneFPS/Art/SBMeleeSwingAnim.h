// Copyright shadowbanefps.
// Procedural axe / blade swing poses + skeletal arm deltas for hero meshes.

#pragma once

#include "CoreMinimal.h"

/** Evaluates a readable axe-chop / blade-swing pose over normalized time 0..1. */
namespace SBMeleeSwingAnim
{
	inline float Smooth01(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	/** Full swing length in seconds (windup + strike + recover). */
	inline constexpr float DurationSeconds = 0.42f;

	/**
	 * Preferred Manny/Quinn melee montages (SK_Mannequin / ABP_Manny DefaultSlot).
	 * Paragon→Manny Greystone/Steel FBX via scripts/Import-MannyMeleeFbx.py — before short AxeSwing.
	 */
	inline constexpr const TCHAR* GreystoneSwingMontageAPath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AM_MM_GreystoneSwing_A.AM_MM_GreystoneSwing_A");
	inline constexpr const TCHAR* GreystoneSwingSequenceAPath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AS_MM_GreystoneSwing_A.AS_MM_GreystoneSwing_A");
	inline constexpr const TCHAR* GreystoneSwingMontageBPath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AM_MM_GreystoneSwing_B.AM_MM_GreystoneSwing_B");
	inline constexpr const TCHAR* GreystoneSwingSequenceBPath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AS_MM_GreystoneSwing_B.AS_MM_GreystoneSwing_B");
	inline constexpr const TCHAR* GreystoneSwingMontageCPath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AM_MM_GreystoneSwing_C.AM_MM_GreystoneSwing_C");
	inline constexpr const TCHAR* GreystoneSwingSequenceCPath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AS_MM_GreystoneSwing_C.AS_MM_GreystoneSwing_C");
	inline constexpr const TCHAR* SteelSwingMontageAPath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AM_MM_SteelSwing_A.AM_MM_SteelSwing_A");
	inline constexpr const TCHAR* SteelSwingSequenceAPath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AS_MM_SteelSwing_A.AS_MM_SteelSwing_A");

	/** Short baked fallback (scripts/Create-MeleeSwingMontage.py). */
	inline constexpr const TCHAR* MeleeMontagePath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AM_MM_AxeSwing_01.AM_MM_AxeSwing_01");

	inline constexpr const TCHAR* MeleeSequencePath =
		TEXT("/Game/Characters/Mannequins/Animations/Combat/AS_MM_AxeSwing_01.AS_MM_AxeSwing_01");

	/** Greystone LMB combo A→B→C then reset to A — see sb.Melee.ComboWindowSec (post-anim). */
	inline constexpr const TCHAR* const GreystoneComboMontagePaths[] = {
		GreystoneSwingMontageAPath,
		GreystoneSwingMontageBPath,
		GreystoneSwingMontageCPath,
	};
	inline constexpr const TCHAR* const GreystoneComboSequencePaths[] = {
		GreystoneSwingSequenceAPath,
		GreystoneSwingSequenceBPath,
		GreystoneSwingSequenceCPath,
	};
	inline constexpr int32 GreystoneComboCount = 3;

	/** Preferred MM montage soft paths in load order (first hit wins / fallbacks). */
	inline constexpr const TCHAR* const PreferredMeleeMontagePaths[] = {
		GreystoneSwingMontageAPath,
		GreystoneSwingMontageBPath,
		GreystoneSwingMontageCPath,
		SteelSwingMontageAPath,
		MeleeMontagePath,
	};
	inline constexpr const TCHAR* const PreferredMeleeSequencePaths[] = {
		GreystoneSwingSequenceAPath,
		GreystoneSwingSequenceBPath,
		GreystoneSwingSequenceCPath,
		SteelSwingSequenceAPath,
		MeleeSequencePath,
	};

	/**
	 * ShadowKight / UE4-bone swing (ShadowknightUE_Skeleton).
	 * Preferred: IK-retargeted FreeAnimationLibrary Counter Attack
	 * (scripts/Create-ShadowKightLibSwing.py) — NOT UE5 AM_MM_AxeSwing_01.
	 * Fallback: baked AS/AM_SK_SwordSwing_01 (Create-ShadowKightSwingMontage.py).
	 */
	inline constexpr const TCHAR* ShadowKightLibSwingMontagePath =
		TEXT("/Game/ShadowKight/Animations/Combat/AM_SK_LibSwing_01.AM_SK_LibSwing_01");

	inline constexpr const TCHAR* ShadowKightLibSwingSequencePath =
		TEXT("/Game/ShadowKight/Animations/Combat/AS_SK_LibSwing_01.AS_SK_LibSwing_01");

	inline constexpr const TCHAR* ShadowKightMeleeMontagePath =
		TEXT("/Game/ShadowKight/Animations/Combat/AM_SK_SwordSwing_01.AM_SK_SwordSwing_01");

	inline constexpr const TCHAR* ShadowKightMeleeSequencePath =
		TEXT("/Game/ShadowKight/Animations/Combat/AS_SK_SwordSwing_01.AS_SK_SwordSwing_01");

	/**
	 * ShadowKight sword hold (optional looping DefaultSlot montage only).
	 * Never drive via PlayAnimation — that replaces the AnimBP and freezes loco.
	 * Baked via scripts/Create-ShadowKightHoldMontage.py.
	 */
	inline constexpr const TCHAR* ShadowKightMeleeHoldMontagePath =
		TEXT("/Game/ShadowKight/Animations/Combat/AM_SK_SwordHold_01.AM_SK_SwordHold_01");

	inline constexpr const TCHAR* ShadowKightMeleeHoldSequencePath =
		TEXT("/Game/ShadowKight/Animations/Combat/AS_SK_SwordHold_01.AS_SK_SwordHold_01");

	/**
	 * FPS arm / weapon pivot rotation for an overhead-to-side axe chop.
	 * Pitch = raise/lower, Yaw = across body, Roll = blade twist.
	 */
	inline FRotator EvalFpPivot(float Alpha01)
	{
		const float A = FMath::Clamp(Alpha01, 0.f, 1.f);
		const FRotator Idle(8.f, 18.f, 12.f);
		if (A <= 0.f)
		{
			return Idle;
		}
		if (A < 0.30f)
		{
			const float U = Smooth01(A / 0.30f);
			return FMath::Lerp(Idle, FRotator(-35.f, -55.f, 18.f), U);
		}
		if (A < 0.55f)
		{
			const float U = Smooth01((A - 0.30f) / 0.25f);
			const float Power = U * U;
			return FMath::Lerp(FRotator(-35.f, -55.f, 18.f), FRotator(28.f, 70.f, -8.f), Power);
		}
		const float U = Smooth01((A - 0.55f) / 0.45f);
		return FMath::Lerp(FRotator(28.f, 70.f, -8.f), Idle, U);
	}

	/** Third-person hand-mounted chop — bold arc (axe on TpWeaponPivot reads this). */
	inline FRotator EvalTpPivot(float Alpha01)
	{
		const float A = FMath::Clamp(Alpha01, 0.f, 1.f);
		const FRotator Idle(0.f, 0.f, 0.f);
		if (A <= 0.f)
		{
			return Idle;
		}
		if (A < 0.30f)
		{
			const float U = Smooth01(A / 0.30f);
			return FMath::Lerp(Idle, FRotator(-55.f, -70.f, 35.f), U);
		}
		if (A < 0.55f)
		{
			const float U = Smooth01((A - 0.30f) / 0.25f);
			const float Power = U * U;
			return FMath::Lerp(FRotator(-55.f, -70.f, 35.f), FRotator(50.f, 85.f, -20.f), Power);
		}
		const float U = Smooth01((A - 0.55f) / 0.45f);
		return FMath::Lerp(FRotator(50.f, 85.f, -20.f), Idle, U);
	}

	/**
	 * Optional additive on sword relative Rot (scaled by sb.Sword.ChopScale).
	 * Grip base is SwordLODS +Y → hand +X (RotY -90). Full pivot deltas send tip
	 * through the torso when Greystone already drives hand_r — keep ChopScale 0
	 * for montage-owned front arcs; raise slightly for axe/fallback polish.
	 */
	inline FRotator EvalTpSwordChop(float Alpha01)
	{
		return EvalTpPivot(Alpha01);
	}

	inline bool IsStrikeWindow(float Alpha01)
	{
		return Alpha01 >= 0.30f && Alpha01 <= 0.55f;
	}

	/** Additive local-space bone delta for procedural skeletal swing (montage fallback). */
	struct FBoneDelta
	{
		FName Bone;
		FRotator Euler;
	};

	inline void EvalSkeletalBoneDeltas(float Alpha01, TArray<FBoneDelta, TInlineAllocator<12>>& Out)
	{
		Out.Reset();
		const float A = FMath::Clamp(Alpha01, 0.f, 1.f);
		if (A <= 0.001f || A >= 0.999f)
		{
			return;
		}

		auto Push = [&Out](const TCHAR* Name, const FRotator& R)
		{
			if (R.IsNearlyZero())
			{
				return;
			}
			FBoneDelta& D = Out.AddDefaulted_GetRef();
			D.Bone = FName(Name);
			D.Euler = R;
		};

		const FRotator Windup_ClavR(-12.f, -28.f, 18.f);
		const FRotator Windup_UpperR(-95.f, -55.f, 40.f);
		const FRotator Windup_LowerR(15.f, 8.f, -80.f);
		const FRotator Windup_HandR(25.f, -30.f, 18.f);
		const FRotator Windup_Spine3(6.f, -16.f, 10.f);
		const FRotator Windup_Spine2(3.f, -8.f, 5.f);
		const FRotator Windup_UpperL(20.f, 25.f, -15.f);

		const FRotator Strike_ClavR(14.f, 36.f, -12.f);
		const FRotator Strike_UpperR(55.f, 85.f, -25.f);
		const FRotator Strike_LowerR(-8.f, -12.f, -20.f);
		const FRotator Strike_HandR(-12.f, 35.f, -25.f);
		const FRotator Strike_Spine3(-3.f, 22.f, -8.f);
		const FRotator Strike_Spine2(-2.f, 12.f, -4.f);
		const FRotator Strike_UpperL(-8.f, -14.f, 10.f);

		FRotator ClavR, UpperR, LowerR, HandR, Spine3, Spine2, UpperL;
		if (A < 0.30f)
		{
			const float U = Smooth01(A / 0.30f);
			ClavR = FMath::Lerp(FRotator::ZeroRotator, Windup_ClavR, U);
			UpperR = FMath::Lerp(FRotator::ZeroRotator, Windup_UpperR, U);
			LowerR = FMath::Lerp(FRotator::ZeroRotator, Windup_LowerR, U);
			HandR = FMath::Lerp(FRotator::ZeroRotator, Windup_HandR, U);
			Spine3 = FMath::Lerp(FRotator::ZeroRotator, Windup_Spine3, U);
			Spine2 = FMath::Lerp(FRotator::ZeroRotator, Windup_Spine2, U);
			UpperL = FMath::Lerp(FRotator::ZeroRotator, Windup_UpperL, U);
		}
		else if (A < 0.55f)
		{
			const float U = Smooth01((A - 0.30f) / 0.25f);
			const float Power = U * U;
			ClavR = FMath::Lerp(Windup_ClavR, Strike_ClavR, Power);
			UpperR = FMath::Lerp(Windup_UpperR, Strike_UpperR, Power);
			LowerR = FMath::Lerp(Windup_LowerR, Strike_LowerR, Power);
			HandR = FMath::Lerp(Windup_HandR, Strike_HandR, Power);
			Spine3 = FMath::Lerp(Windup_Spine3, Strike_Spine3, Power);
			Spine2 = FMath::Lerp(Windup_Spine2, Strike_Spine2, Power);
			UpperL = FMath::Lerp(Windup_UpperL, Strike_UpperL, Power);
		}
		else
		{
			const float U = Smooth01((A - 0.55f) / 0.45f);
			ClavR = FMath::Lerp(Strike_ClavR, FRotator::ZeroRotator, U);
			UpperR = FMath::Lerp(Strike_UpperR, FRotator::ZeroRotator, U);
			LowerR = FMath::Lerp(Strike_LowerR, FRotator::ZeroRotator, U);
			HandR = FMath::Lerp(Strike_HandR, FRotator::ZeroRotator, U);
			Spine3 = FMath::Lerp(Strike_Spine3, FRotator::ZeroRotator, U);
			Spine2 = FMath::Lerp(Strike_Spine2, FRotator::ZeroRotator, U);
			UpperL = FMath::Lerp(Strike_UpperL, FRotator::ZeroRotator, U);
		}

		Push(TEXT("clavicle_r"), ClavR);
		Push(TEXT("upperarm_r"), UpperR);
		Push(TEXT("lowerarm_r"), LowerR);
		Push(TEXT("hand_r"), HandR);
		Push(TEXT("spine_03"), Spine3);
		Push(TEXT("spine_02"), Spine2);
		Push(TEXT("upperarm_l"), UpperL);
	}
}
