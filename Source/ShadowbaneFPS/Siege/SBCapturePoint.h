// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/SBTypes.h"
#include "SBCapturePoint.generated.h"

class UBoxComponent;
class ASBPlayerState;

/**
 * A conquest zone (the pilot's key example is the Courtyard, design doc §7).
 * When attackers hold it uncontested for CaptureSeconds, the zone flips and the
 * GameMode advances the conquest stage, moving the attacker forward spawn up and
 * pushing the defender spawn inward.
 */
UCLASS()
class SHADOWBANEFPS_API ASBCapturePoint : public AActor
{
	GENERATED_BODY()

public:
	ASBCapturePoint();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetCaptureProgress() const { return CaptureProgress; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	bool IsCaptured() const { return bCaptured; }

protected:
	/** The stage to set on the GameMode when this point is captured. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege")
	ESBConquestStage StageOnCapture = ESBConquestStage::Courtyard;

	/** Seconds of uncontested attacker presence required to capture. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege", meta = (ClampMin = "1.0"))
	float CaptureSeconds = 12.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siege")
	TObjectPtr<UBoxComponent> Zone;

	UPROPERTY(ReplicatedUsing = OnRep_Captured, BlueprintReadOnly, Category = "Siege")
	bool bCaptured = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	float CaptureProgress = 0.f;

	virtual void BeginPlay() override;

	/** Server-only: counts live players of each team currently inside the zone. */
	void CountOccupants(int32& OutAttackers, int32& OutDefenders) const;

	UFUNCTION()
	void OnRep_Captured();
};
