// Copyright shadowbanefps.
// Soft content paths for optional hero skins (Paragon / Fab / Marketplace). Missing assets = silent fallback.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"

/**
 * Paragon Countess — Epic free Fab listing.
 * Skeleton is Paragon (NOT UE5 SK_Mannequin). Do not play AM_MM_AxeSwing_01 on it.
 *
 * Typical migrate root after Fab "Add to Project":
 *   Content/ParagonCountess/...  →  /Game/ParagonCountess/...
 * Optional copy/migrate into:
 *   Content/Art/Characters/Countess/...
 */
namespace SBHeroSkinPaths
{
	inline constexpr const TCHAR* CountessMeshCandidates[] = {
		TEXT("/Game/ParagonCountess/Characters/Heroes/Countess/Meshes/Countess.Countess"),
		TEXT("/Game/ParagonCountess/Characters/Heroes/Countess/Meshes/SK_Countess.SK_Countess"),
		TEXT("/Game/Art/Characters/Countess/Meshes/Countess.Countess"),
		TEXT("/Game/Art/Characters/Countess/SK_Countess.SK_Countess"),
	};

	inline constexpr const TCHAR* CountessAnimBPCandidates[] = {
		TEXT("/Game/ParagonCountess/Characters/Heroes/Countess/Countess_AnimBlueprint.Countess_AnimBlueprint_C"),
		TEXT("/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Countess_AnimBlueprint.Countess_AnimBlueprint_C"),
		TEXT("/Game/ParagonCountess/Characters/Heroes/Countess/Blueprints/Countess_AnimBlueprint.Countess_AnimBlueprint_C"),
		TEXT("/Game/Art/Characters/Countess/Countess_AnimBlueprint.Countess_AnimBlueprint_C"),
		TEXT("/Game/Art/Characters/Countess/ABP_Countess.ABP_Countess_C"),
	};

	/**
	 * ShadowKight (marketplace pack under Content/ShadowKight).
	 * Skeleton: ShadowknightUE_Skeleton — UE4-mannequin bone names (hand_r, etc.),
	 * NOT UE5 SK_Mannequin. Prefer ABP_SK_Melee (ThirdPerson loco + DefaultSlot,
	 * no Control Rig after the slot). Fallback Anim_ShadowKight then pack TP ABP.
	 * Melee swing: AM_SK_LibSwing_01 preferred, else AM_SK_SwordSwing_01 (not UE5 AM_MM_AxeSwing_01).
	 */
	inline constexpr const TCHAR* ShadowKightMeshCandidates[] = {
		TEXT("/Game/ShadowKight/Mesh/ShadowknightUE_SK.ShadowknightUE_SK"),
		TEXT("/Game/ShadowKight/Mesh/SK_ShadowknightUE.SK_ShadowknightUE"),
	};

	inline constexpr const TCHAR* ShadowKightAnimBPCandidates[] = {
		TEXT("/Game/ShadowKight/Animations/ABP_SK_Melee.ABP_SK_Melee_C"),
		TEXT("/Game/ShadowKight/Animations/Anim_ShadowKight.Anim_ShadowKight_C"),
		TEXT("/Game/ShadowKight/Mannequin/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP_C"),
	};

	inline USkeletalMesh* TryLoadCountessMesh()
	{
		for (const TCHAR* Path : CountessMeshCandidates)
		{
			if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, Path))
			{
				return Mesh;
			}
		}
		return nullptr;
	}

	inline UClass* TryLoadCountessAnimClass()
	{
		for (const TCHAR* Path : CountessAnimBPCandidates)
		{
			if (UClass* Cls = LoadClass<UAnimInstance>(nullptr, Path))
			{
				return Cls;
			}
		}
		return nullptr;
	}

	inline USkeletalMesh* TryLoadShadowKightMesh()
	{
		for (const TCHAR* Path : ShadowKightMeshCandidates)
		{
			if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, Path))
			{
				return Mesh;
			}
		}
		return nullptr;
	}

	inline UClass* TryLoadShadowKightAnimClass()
	{
		for (const TCHAR* Path : ShadowKightAnimBPCandidates)
		{
			if (UClass* Cls = LoadClass<UAnimInstance>(nullptr, Path))
			{
				return Cls;
			}
		}
		return nullptr;
	}
}
