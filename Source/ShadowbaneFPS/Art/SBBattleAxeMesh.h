// Copyright shadowbanefps.
// Runtime crescent-blade battle axe (not a pickaxe / woodcutter). CC0 geometry.

#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;
class UMaterialInterface;

namespace SBBattleAxeMesh
{
	/** Builds a bearded / crescent battle-axe mesh into Mesh (section 0). Units = cm. */
	void Build(UProceduralMeshComponent* Mesh);

	/** Steel-ish opaque material for the axe head + haft tint. */
	UMaterialInterface* GetFallbackMaterial();
}
