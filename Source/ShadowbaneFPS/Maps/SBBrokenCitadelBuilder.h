// Copyright shadowbanefps.
//
// Runtime greybox for The Broken Citadel (design doc §6). Spawns geometry on
// every machine and replicated gameplay actors on the server so the pilot is
// playable before any .umap exists in Content/.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/SBTypes.h"
#include "SBBrokenCitadelBuilder.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class SHADOWBANEFPS_API ASBBrokenCitadelBuilder : public AActor
{
	GENERATED_BODY()

public:
	ASBBrokenCitadelBuilder();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Greybox")
	bool bBuildOnBeginPlay = true;

private:
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	bool bBuiltVisuals = false;
	bool bBuiltGameplay = false;

	void BuildVisualGeometry();
	void BuildGameplayActors();

	UStaticMeshComponent* AddBox(const FVector& Location, const FVector& Scale, const FLinearColor& Color, const FName& Name);
	void SpawnSpawnPoint(const FVector& Location, float Yaw, ESBTeam Team, ESBConquestStage MinStage, ESBConquestStage MaxStage, bool bSiege, const FName& Name);
};
