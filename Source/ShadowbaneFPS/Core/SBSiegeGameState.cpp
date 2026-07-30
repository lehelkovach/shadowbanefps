// Copyright shadowbanefps.

#include "SBSiegeGameState.h"
#include "Characters/SBCharacterArchetype.h"
#include "Characters/SBPilotRoster.h"
#include "Net/UnrealNetwork.h"

ASBSiegeGameState::ASBSiegeGameState()
{
	SetNetUpdateFrequency(10.0f);
}

void ASBSiegeGameState::BeginPlay()
{
	Super::BeginPlay();

	if (LocalRoster.Num() == 0)
	{
		USBPilotRoster::BuildDefaultRoster(this, LocalRoster);
	}
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
	const double Now = GetServerWorldTimeSeconds();
	return FMath::Max(0.f, static_cast<float>(RegulationDeadlineServerTime - Now));
}

USBCharacterArchetype* ASBSiegeGameState::FindArchetypeById(FName ArchetypeId) const
{
	if (ArchetypeId.IsNone())
	{
		return nullptr;
	}

	for (USBCharacterArchetype* Arch : LocalRoster)
	{
		if (Arch && Arch->ArchetypeId == ArchetypeId)
		{
			return Arch;
		}
	}
	return nullptr;
}

void ASBSiegeGameState::ServerSetPhase(ESBMatchPhase NewPhase)
{
	if (HasAuthority() && Phase != NewPhase)
	{
		Phase = NewPhase;
		OnRep_Phase();
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
