// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "SBTypes.h"
#include "SBSpawnPoint.generated.h"

/**
 * Staged spawn for attackers/defenders. Availability changes with conquest stage
 * so the front line moves after courtyard capture (design doc §7, §9).
 */
UCLASS()
class SHADOWBANEFPS_API ASBSpawnPoint : public APlayerStart
{
	GENERATED_BODY()

public:
	ASBSpawnPoint(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege")
	ESBTeam Team = ESBTeam::Unassigned;

	/** Earliest conquest stage at which this spawn is usable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege")
	ESBConquestStage MinStage = ESBConquestStage::OuterSiege;

	/** Latest conquest stage at which this spawn remains usable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege")
	ESBConquestStage MaxStage = ESBConquestStage::InnerKeep;

	/** If true, preferred for heavy siege archetypes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege")
	bool bSiegeDeployment = false;

	bool IsAvailableFor(ESBTeam InTeam, ESBConquestStage Stage) const;
};
