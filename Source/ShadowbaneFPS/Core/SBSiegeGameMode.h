// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SBTypes.h"
#include "SBSiegeGameMode.generated.h"

class ASBSiegeGameState;
class ASBPlayerState;
class USBCharacterArchetype;

/**
 * Server-authoritative match flow for the 20-minute conquest siege pilot.
 *
 * Responsibilities (design doc §5, §8, §9):
 *  - Assign teams (attackers vs defenders).
 *  - Run the regulation clock and drive phase transitions for pacing/telemetry.
 *  - Advance conquest stages when the courtyard / inner keep are taken.
 *  - Track final-objective progress and resolve victory, including overtime.
 *  - Handle respawns and post-death character switching, enforcing roster rules.
 *
 * This class is intentionally logic-only so it can run headless on a Linux
 * dedicated server (OCI VM). Presentation is client-side. See docs/SETUP.md.
 */
UCLASS()
class SHADOWBANEFPS_API ASBSiegeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASBSiegeGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	// --- Tunables (design doc §1, §8, §9) ---

	/** Regulation length in seconds. Default 20 minutes. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float RegulationSeconds = 20.f * 60.f;

	/** Delay before a dead player may re-enter. See §9. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float RespawnDelaySeconds = 8.f;

	/** Extra confirmation window for defenders to clear the objective in overtime. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float OvertimeClearConfirmSeconds = 5.f;

	/** Phase 1 -> Phase 2 boundary (seconds elapsed). Pacing target only. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float BreachPhaseElapsedSeconds = 6.f * 60.f;

	/** Phase 2 -> Phase 3 boundary (seconds elapsed). Pacing target only. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Timing")
	float InnerAssaultPhaseElapsedSeconds = 12.f * 60.f;

	/** Curated pilot roster (~8-12 pre-built characters). See §3, §12. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules|Roster")
	TArray<TObjectPtr<USBCharacterArchetype>> Roster;

	// --- Match flow (server authority) ---

	/** Called when the objective interaction is completed by attackers (§5). */
	UFUNCTION(BlueprintCallable, Category = "Siege")
	void NotifyFinalObjectiveCompleted();

	/** Called when a conquest zone (e.g. the courtyard) is captured (§7). */
	UFUNCTION(BlueprintCallable, Category = "Siege")
	void AdvanceConquestStage(ESBConquestStage NewStage);

	/** Request a (re)deploy as an archetype. Validated against roster rules (§9). */
	UFUNCTION(BlueprintCallable, Category = "Siege")
	bool RequestSelectArchetype(ASBPlayerState* PlayerState, USBCharacterArchetype* Archetype);

	/** Report a player's death; starts their respawn timer (§9). */
	UFUNCTION(BlueprintCallable, Category = "Siege")
	void NotifyPlayerKilled(ASBPlayerState* Victim, ASBPlayerState* Killer);

protected:
	virtual void BeginPlay() override;

	void StartMatch();
	void UpdatePhaseForElapsed();
	void EndMatch(ESBMatchResult Result);

	/** Returns true if adding Archetype to Team would respect duplicate limits (§3). */
	bool CanTeamUseArchetype(ESBTeam Team, const USBCharacterArchetype* Archetype) const;

	/** Balances the next joiner onto the smaller side. */
	ESBTeam PickTeamForNewPlayer() const;

	UPROPERTY(Transient)
	TObjectPtr<ASBSiegeGameState> SiegeState = nullptr;

	FTimerHandle MatchTimerHandle;
	FTimerHandle PhaseTickHandle;

	/** Server-world-time at which regulation ends. */
	double RegulationDeadline = 0.0;

	/** True once overtime has begun so we don't restart it. */
	bool bInOvertime = false;

private:
	void HandleRegulationExpired();
	void TickPhase();
};
