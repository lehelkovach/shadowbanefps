// Copyright shadowbanefps.

#include "SBSiegeGameState.h"
#include "Net/UnrealNetwork.h"

ASBSiegeGameState::ASBSiegeGameState()
{
	SetNetUpdateFrequency(10.0f);
}

void ASBSiegeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASBSiegeGameState, Phase);
	DOREPLIFETIME(ASBSiegeGameState, ConquestStage);
	DOREPLIFETIME(ASBSiegeGameState, Result);
	DOREPLIFETIME(ASBSiegeGameState, FinalObjectiveProgress);
	DOREPLIFETIME(ASBSiegeGameState, RegulationDeadlineServerTime);
}

float ASBSiegeGameState::GetRemainingRegulationSeconds() const
{
	// GetServerWorldTimeSeconds() is synchronized on clients, so this reads the
	// same countdown everywhere without per-tick replication of the clock.
	const double Now = GetServerWorldTimeSeconds();
	return FMath::Max(0.f, static_cast<float>(RegulationDeadlineServerTime - Now));
}

void ASBSiegeGameState::ServerSetPhase(ESBMatchPhase NewPhase)
{
	if (HasAuthority() && Phase != NewPhase)
	{
		Phase = NewPhase;
		OnRep_Phase(); // Fire locally on the listen/dedicated server too.
	}
}

void ASBSiegeGameState::ServerSetConquestStage(ESBConquestStage NewStage)
{
	if (HasAuthority() && ConquestStage != NewStage)
	{
		ConquestStage = NewStage;
		OnRep_ConquestStage();
	}
}

void ASBSiegeGameState::OnRep_Phase()
{
	OnPhaseChanged.Broadcast(Phase);
}

void ASBSiegeGameState::OnRep_ConquestStage()
{
	OnConquestStageChanged.Broadcast(ConquestStage);
}
