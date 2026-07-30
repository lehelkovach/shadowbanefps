// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SBTypes.h"
#include "SBSiegeGameMode.generated.h"

class ASBSiegeGameState;
class ASBPlayerState;
class ASBPlayerController;
class ASBSpawnPoint;
class USBCharacterArchetype;
class ASBBrokenCitadelBuilder;
class USBMatchTelemetry;

/**
 * Server-authoritative match flow for the 20-minute conquest siege pilot.
 * See docs/game-design.md §5, §8, §9.
 */
UCLASS()
class SHADOWBANEFPS_API ASBSiegeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASBSiegeGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float RegulationSeconds = 20.f * 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float RespawnDelaySeconds = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float OvertimeClearConfirmSeconds = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float BreachPhaseElapsedSeconds = 6.f * 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float InnerAssaultPhaseElapsedSeconds = 12.f * 60.f;

	/** If empty at BeginPlay, the default pilot roster is generated in code. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Roster")
	TArray<TObjectPtr<USBCharacterArchetype>> Roster;

	UPROPERTY(EditDefaultsOnly, Category = "Rules|Map")
	bool bAutoBuildBrokenCitadel = true;

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void NotifyFinalObjectiveCompleted();

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void AdvanceConquestStage(ESBConquestStage NewStage);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	bool RequestSelectArchetype(ASBPlayerState* PlayerState, USBCharacterArchetype* Archetype);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void NotifyPlayerKilled(ASBPlayerState* Victim, ASBPlayerState* Killer, FName KillingPowerId = NAME_None);

	/** Returns a roster entry by index, or null. */
	UFUNCTION(BlueprintPure, Category = "Siege")
	USBCharacterArchetype* GetRosterArchetype(int32 Index) const;

	/** Spawns (or respawns) the player using their selected archetype. */
	bool SpawnPlayerFromController(APlayerController* PC);

	/** Match telemetry sink (design doc §12). Valid after StartMatch. */
	UFUNCTION(BlueprintPure, Category = "Siege|Telemetry")
	USBMatchTelemetry* GetTelemetry() const { return Telemetry; }

protected:
	virtual void BeginPlay() override;

	void EnsureRoster();
	void EnsureCitadel();
	void StartMatch();
	void UpdatePhaseForElapsed();
	void UpdateInnerKeepStageFromPressure();
	void EndMatch(ESBMatchResult Result);

	bool CanTeamUseArchetype(ESBTeam Team, const USBCharacterArchetype* Archetype, const ASBPlayerState* Ignoring = nullptr) const;
	ESBTeam PickTeamForNewPlayer() const;
	USBCharacterArchetype* FindDefaultArchetypeForTeam(ESBTeam Team) const;
	ASBSpawnPoint* FindSpawnPoint(ESBTeam Team, const USBCharacterArchetype* Archetype) const;

	void ScheduleRespawn(APlayerController* PC);

	UPROPERTY(Transient)
	TObjectPtr<ASBSiegeGameState> SiegeState = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ASBBrokenCitadelBuilder> CitadelBuilder = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USBMatchTelemetry> Telemetry = nullptr;

	FTimerHandle MatchTimerHandle;
	FTimerHandle PhaseTickHandle;

	double RegulationDeadline = 0.0;
	bool bInOvertime = false;
	ESBMatchPhase LastLoggedPhase = ESBMatchPhase::WaitingToStart;

private:
	void HandleRegulationExpired();
	void TickPhase();
};
