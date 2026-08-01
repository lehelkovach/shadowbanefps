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
class ASBBotController;

/**
 * Server-authoritative match flow for the 20-minute conquest siege pilot.
 * See docs/game-design.md §5, §8, §9. Bot populate + admin spectate: §12.1 / docs/BOTS_AND_ADMIN.md.
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

	/** URL/option driven: ?Bots=8 fills both sides for admin spectate playtests. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Bots")
	int32 AutoSpawnBots = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Rules|Bots")
	int32 MaxBotsPerTeam = 5;

	/** Human joins as free-camera admin spectator (URL ?AdminSpectate=1). */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Admin")
	bool bForceAdminSpectate = false;

	/** URL ?Mode=FFA (or Deathmatch) — open dogfood deathmatch, no siege win. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Mode")
	bool bFreeForAll = false;

	UFUNCTION(BlueprintPure, Category = "Siege|Mode")
	bool IsFreeForAll() const { return bFreeForAll; }

	UFUNCTION(BlueprintCallable, Category = "Siege|Bots")
	int32 SpawnBots(int32 TotalBots);

	UFUNCTION(BlueprintCallable, Category = "Siege|Bots")
	ASBBotController* SpawnOneBot(ESBTeam Team);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void NotifyFinalObjectiveCompleted();

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void AdvanceConquestStage(ESBConquestStage NewStage);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	bool RequestSelectArchetype(ASBPlayerState* PlayerState, USBCharacterArchetype* Archetype);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void NotifyPlayerKilled(ASBPlayerState* Victim, ASBPlayerState* Killer, FName KillingPowerId = NAME_None);

	void ScheduleRespawn(APlayerController* PC);
	void ScheduleBotRespawn(ASBBotController* Bot);

	/** Returns a roster entry by index, or null. */
	UFUNCTION(BlueprintPure, Category = "Siege")
	USBCharacterArchetype* GetRosterArchetype(int32 Index) const;

	/** Spawns (or respawns) the player using their selected archetype. */
	bool SpawnPlayerFromController(APlayerController* PC);

	/** Shared spawn path for humans and bots. */
	bool SpawnCharacterForController(AController* Controller);

	/** Team pad for recall / fountain return (same pads as spawn). */
	bool GetSpawnTransformFor(const ASBPlayerState* PS, FVector& OutLocation, FRotator& OutRotation) const;

	/** Match telemetry sink (design doc §12). Valid after StartMatch. */
	UFUNCTION(BlueprintPure, Category = "Siege|Telemetry")
	USBMatchTelemetry* GetTelemetry() const { return Telemetry; }

	UFUNCTION(BlueprintPure, Category = "Siege|Bots")
	int32 GetActiveBotCount() const { return ActiveBots.Num(); }

protected:
	virtual void BeginPlay() override;

	void EnsureRoster();
	void EnsureCitadel();
	void StartMatch();
	void UpdatePhaseForElapsed();
	void UpdateInnerKeepStageFromPressure();
	void EndMatch(ESBMatchResult Result);
	void ParseLaunchOptions(const FString& Options);
	void MaybeSpawnConfiguredBots();

	bool CanTeamUseArchetype(ESBTeam Team, const USBCharacterArchetype* Archetype, const ASBPlayerState* Ignoring = nullptr) const;
	ESBTeam PickTeamForNewPlayer() const;
	USBCharacterArchetype* FindDefaultArchetypeForTeam(ESBTeam Team) const;
	USBCharacterArchetype* PickBotArchetype(ESBTeam Team) const;
	ASBSpawnPoint* FindSpawnPoint(ESBTeam Team, const USBCharacterArchetype* Archetype) const;

	UPROPERTY(Transient)
	TObjectPtr<ASBSiegeGameState> SiegeState = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ASBBrokenCitadelBuilder> CitadelBuilder = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USBMatchTelemetry> Telemetry = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASBBotController>> ActiveBots;

	FTimerHandle MatchTimerHandle;
	FTimerHandle PhaseTickHandle;

	double RegulationDeadline = 0.0;
	bool bInOvertime = false;
	ESBMatchPhase LastLoggedPhase = ESBMatchPhase::WaitingToStart;

private:
	void HandleRegulationExpired();
	void TickPhase();
};
