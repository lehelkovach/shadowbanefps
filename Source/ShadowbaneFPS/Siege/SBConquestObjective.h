// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SBConquestObjective.generated.h"

class UBoxComponent;
class ASBPlayerState;

/**
 * The final objective inside the inner keep: an interruptible capture/destruction
 * interaction (design doc §5). Attackers must maintain control long enough to
 * complete it; defenders interrupt by contesting the zone or eliminating the
 * interacting attackers. Progress may persist in segments or decay slowly.
 *
 * On completion it tells the GameMode the attackers win. It also feeds
 * GameState->FinalObjectiveProgress for the HUD and the overtime rule.
 */
UCLASS()
class SHADOWBANEFPS_API ASBConquestObjective : public AActor
{
	GENERATED_BODY()

public:
	ASBConquestObjective();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetProgress() const { return Progress; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	bool IsContested() const { return bContested; }

protected:
	/** Seconds of uncontested attacker control needed to complete the objective. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege", meta = (ClampMin = "1.0"))
	float CompleteSeconds = 30.f;

	/** How fast progress decays when attackers lose control (0 = it locks/persists). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege", meta = (ClampMin = "0.0"))
	float DecayRatePerSecond = 2.f;

	/** Only available once the conquest has reached the inner keep. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege")
	bool bRequireInnerKeepStage = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siege")
	TObjectPtr<UBoxComponent> InteractionZone;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	float Progress = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	bool bContested = false;

	virtual void BeginPlay() override;

	void CountOccupants(int32& OutAttackers, int32& OutDefenders) const;

	/** Pushes current 0..1 progress up to the GameState for HUD + overtime logic. */
	void PublishProgress() const;
};
