// Copyright shadowbanefps.

#include "SBSiegeGameMode.h"
#include "SBSiegeGameState.h"
#include "SBPlayerState.h"
#include "Characters/SBCharacterArchetype.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"

ASBSiegeGameMode::ASBSiegeGameMode()
{
	GameStateClass = ASBSiegeGameState::StaticClass();
	PlayerStateClass = ASBPlayerState::StaticClass();
	bUseSeamlessTravel = true;
}

void ASBSiegeGameMode::BeginPlay()
{
	Super::BeginPlay();

	SiegeState = GetGameState<ASBSiegeGameState>();

	// TODO: gate on lobby readiness. For the pilot scaffold we start immediately
	// so the flow is exercisable end-to-end in PIE / on the dedicated server.
	StartMatch();
}

void ASBSiegeGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ASBPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ASBPlayerState>() : nullptr)
	{
		PS->SetTeam(PickTeamForNewPlayer());
	}
}

void ASBSiegeGameMode::StartMatch()
{
	if (!SiegeState)
	{
		return;
	}

	RegulationDeadline = GetWorld()->GetTimeSeconds() + RegulationSeconds;
	// GameState uses server-world-time; align by using the same clock source.
	SiegeState->ServerSetRegulationDeadline(SiegeState->GetServerWorldTimeSeconds() + RegulationSeconds);
	SiegeState->ServerSetPhase(ESBMatchPhase::Recon);
	SiegeState->ServerSetConquestStage(ESBConquestStage::OuterSiege);

	GetWorldTimerManager().SetTimer(
		MatchTimerHandle, this, &ASBSiegeGameMode::HandleRegulationExpired, RegulationSeconds, false);

	// Lightweight pacing tick to move phases and drive telemetry checkpoints.
	GetWorldTimerManager().SetTimer(
		PhaseTickHandle, this, &ASBSiegeGameMode::TickPhase, 1.0f, true);
}

void ASBSiegeGameMode::TickPhase()
{
	UpdatePhaseForElapsed();
}

void ASBSiegeGameMode::UpdatePhaseForElapsed()
{
	if (!SiegeState || SiegeState->GetPhase() == ESBMatchPhase::Finished
		|| SiegeState->GetPhase() == ESBMatchPhase::Overtime)
	{
		return;
	}

	const float Elapsed = RegulationSeconds - SiegeState->GetRemainingRegulationSeconds();

	// Phase windows are pacing targets, not hard locks (design doc §8). A real
	// build should also advance Phase 2/3 when the courtyard / breach actually
	// change hands; here time is the floor.
	ESBMatchPhase Desired = ESBMatchPhase::Recon;
	if (Elapsed >= InnerAssaultPhaseElapsedSeconds)
	{
		Desired = ESBMatchPhase::InnerAssault;
	}
	else if (Elapsed >= BreachPhaseElapsedSeconds)
	{
		Desired = ESBMatchPhase::Breach;
	}

	if (Desired != SiegeState->GetPhase())
	{
		SiegeState->ServerSetPhase(Desired);
	}
}

void ASBSiegeGameMode::AdvanceConquestStage(ESBConquestStage NewStage)
{
	if (!HasAuthority() || !SiegeState)
	{
		return;
	}

	SiegeState->ServerSetConquestStage(NewStage);

	// Capturing the courtyard should also push pacing forward if we're behind.
	if (NewStage == ESBConquestStage::Courtyard && SiegeState->GetPhase() == ESBMatchPhase::Recon)
	{
		SiegeState->ServerSetPhase(ESBMatchPhase::Breach);
	}
	else if (NewStage == ESBConquestStage::InnerKeep && SiegeState->GetPhase() != ESBMatchPhase::Overtime)
	{
		SiegeState->ServerSetPhase(ESBMatchPhase::InnerAssault);
	}

	// TODO: move attacker forward spawn up and push the defender spawn inward
	// (design doc §7). Handled by the spawn manager once map spawns are tagged.
}

void ASBSiegeGameMode::NotifyFinalObjectiveCompleted()
{
	if (!HasAuthority())
	{
		return;
	}

	// Attackers completing the inner-keep objective wins immediately, in
	// regulation or overtime (design doc §5).
	if (SiegeState)
	{
		SiegeState->ServerSetFinalObjectiveProgress(1.f);
	}
	EndMatch(ESBMatchResult::AttackersWin);
}

void ASBSiegeGameMode::HandleRegulationExpired()
{
	if (!HasAuthority() || !SiegeState)
	{
		return;
	}

	// Overtime rule (design doc §5): if the final objective is actively contested
	// when the clock hits zero, play continues until attackers finish or defenders
	// clear it. We approximate "actively contested" with in-progress objective
	// state; the objective actor reports contest status.
	const bool bObjectiveContested = SiegeState->GetFinalObjectiveProgress() > 0.f
		&& SiegeState->GetFinalObjectiveProgress() < 1.f;

	if (bObjectiveContested && !bInOvertime)
	{
		bInOvertime = true;
		SiegeState->ServerSetPhase(ESBMatchPhase::Overtime);
		// The final-objective actor drives NotifyFinalObjectiveCompleted (attackers)
		// or, after a clear-confirm window, calls back to end for defenders.
		return;
	}

	// Time expired with no active contest: defenders hold.
	EndMatch(ESBMatchResult::DefendersWin);
}

void ASBSiegeGameMode::EndMatch(ESBMatchResult Result)
{
	if (!HasAuthority() || !SiegeState || SiegeState->GetPhase() == ESBMatchPhase::Finished)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(MatchTimerHandle);
	GetWorldTimerManager().ClearTimer(PhaseTickHandle);

	SiegeState->ServerSetResult(Result);
	SiegeState->ServerSetPhase(ESBMatchPhase::Finished);

	// TODO: freeze input, show scoreboard, flush telemetry (design doc §12).
}

bool ASBSiegeGameMode::RequestSelectArchetype(ASBPlayerState* PlayerState, USBCharacterArchetype* Archetype)
{
	if (!HasAuthority() || !PlayerState || !Archetype)
	{
		return false;
	}

	// Switching is only allowed while dead (or at a deployment point). See §9.
	if (PlayerState->IsAlive())
	{
		return false;
	}

	const ESBTeam Team = PlayerState->GetTeam();

	// Side eligibility (some archetypes are attacker/defender-only).
	if ((Team == ESBTeam::Attackers && !Archetype->bAttackerEligible)
		|| (Team == ESBTeam::Defenders && !Archetype->bDefenderEligible))
	{
		return false;
	}

	if (!CanTeamUseArchetype(Team, Archetype))
	{
		return false;
	}

	PlayerState->SetSelectedArchetype(Archetype);
	return true;
}

void ASBSiegeGameMode::NotifyPlayerKilled(ASBPlayerState* Victim, ASBPlayerState* /*Killer*/)
{
	if (!HasAuthority() || !Victim)
	{
		return;
	}

	Victim->SetAlive(false);

	// TODO: schedule respawn after RespawnDelaySeconds; on respawn spawn the
	// pawn for Victim->GetSelectedArchetype() at a valid staged/forward spawn
	// (design doc §9). Duplicate/side/spawn-tag rules enforced via RequestSelectArchetype.
}

bool ASBSiegeGameMode::CanTeamUseArchetype(ESBTeam Team, const USBCharacterArchetype* Archetype) const
{
	if (!Archetype || Archetype->PerTeamDuplicateLimit <= 0 || !GameState)
	{
		return true; // No limit configured (or no players yet).
	}

	int32 Count = 0;
	for (APlayerState* Base : GameState->PlayerArray)
	{
		const ASBPlayerState* PS = Cast<ASBPlayerState>(Base);
		if (PS && PS->GetTeam() == Team && PS->GetSelectedArchetype() == Archetype)
		{
			++Count;
		}
	}
	return Count < Archetype->PerTeamDuplicateLimit;
}

ESBTeam ASBSiegeGameMode::PickTeamForNewPlayer() const
{
	int32 Attackers = 0;
	int32 Defenders = 0;
	if (GameState)
	{
		for (APlayerState* Base : GameState->PlayerArray)
		{
			if (const ASBPlayerState* PS = Cast<ASBPlayerState>(Base))
			{
				if (PS->GetTeam() == ESBTeam::Attackers) { ++Attackers; }
				else if (PS->GetTeam() == ESBTeam::Defenders) { ++Defenders; }
			}
		}
	}
	return (Attackers <= Defenders) ? ESBTeam::Attackers : ESBTeam::Defenders;
}
