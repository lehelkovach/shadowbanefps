// Copyright shadowbanefps.
// Built-in Shadowbane-race silhouettes from Engine BasicShapes (no Fab pack required).

#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;
class UCapsuleComponent;
class ACharacter;

/**
 * Applies a readable race silhouette (Human / Elf / High Elf / Nightshades / Dwarf / Minotaur / …)
 * using Engine BasicShapes. Real Fab/Sketchfab meshes can replace these later via Content/Art.
 */
namespace SBRaceSilhouette
{
	struct FParts
	{
		UStaticMeshComponent* Body = nullptr;
		UStaticMeshComponent* Head = nullptr;
		UStaticMeshComponent* FeatureL = nullptr; // horn / ear
		UStaticMeshComponent* FeatureR = nullptr;
		UStaticMeshComponent* FeatureExtra = nullptr; // snout / hood / beard slab
		UCapsuleComponent* Capsule = nullptr;
	};

	/** Race display name from USBCharacterArchetype::Race (case-insensitive). */
	void Apply(const FString& RaceName, const FParts& Parts, FLinearColor& InOutBodyTint);
}
