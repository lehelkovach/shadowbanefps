// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SBTypes.h"
#include "SBSiegeGameState.generated.h"

/** Fired on all clients when the match phase changes (UI, music, VO cues). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBOnPhaseChanged, ESBMatchPhase, NewPhase);

/** Fired on all clients when conquest ownership advances (front line moves). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBOnConquestStageChanged, ESBConquestStage, NewStage);

/**
 * Replicated authoritative match state that every client can read for HUD:
 * timer, phase, conquest stage, final-objective progress, result.
 * See design doc §5, §8, §11. Only server-safe / non-hidden data lives here.
 */
UCLASS()
class SHADOWBANEFPS_API ASBSiegeGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ASBSiegeGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Seconds left in regulation. Derived from the replicated deadline + server time. */
	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetRemainingRegulationSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Siege")
	ESBMatchPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	ESBConquestStage GetConquestStage() const { return ConquestStage; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	ESBMatchResult GetResult() const { return Result; }

	/** 0..1 progress on the inner-keep final objective (design doc §5). */
	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetFinalObjectiveProgress() const { return FinalObjectiveProgress; }

	// --- Server-authoritative setters (called by the GameMode) ---
	void ServerSetPhase(ESBMatchPhase NewPhase);
	void ServerSetConquestStage(ESBConquestStage NewStage);
	void ServerSetResult(ESBMatchResult NewResult) { if (HasAuthority()) { Result = NewResult; } }
	void ServerSetFinalObjectiveProgress(float NewProgress) { if (HasAuthority()) { FinalObjectiveProgress = FMath::Clamp(NewProgress, 0.f, 1.f); } }

	/** Server sets the server-world-time at which regulation ends. */
	void ServerSetRegulationDeadline(double InDeadlineServerTime) { if (HasAuthority()) { RegulationDeadlineServerTime = InDeadlineServerTime; } }

	UPROPERTY(BlueprintAssignable, Category = "Siege")
	FSBOnPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Siege")
	FSBOnConquestStageChanged OnConquestStageChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Phase, BlueprintReadOnly, Category = "Siege")
	ESBMatchPhase Phase = ESBMatchPhase::WaitingToStart;

	UPROPERTY(ReplicatedUsing = OnRep_ConquestStage, BlueprintReadOnly, Category = "Siege")
	ESBConquestStage ConquestStage = ESBConquestStage::OuterSiege;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	ESBMatchResult Result = ESBMatchResult::Undecided;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	float FinalObjectiveProgress = 0.f;

	/** GetServerWorldTimeSeconds() value at which regulation time expires. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	double RegulationDeadlineServerTime = 0.0;

	UFUNCTION()
	void OnRep_Phase();

	UFUNCTION()
	void OnRep_ConquestStage();
};
