// Copyright shadowbanefps.

#include "SBPlayerState.h"
#include "Net/UnrealNetwork.h"

ASBPlayerState::ASBPlayerState()
{
	// Match state changes are infrequent; a modest update rate is plenty.
	SetNetUpdateFrequency(10.0f);
}

void ASBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASBPlayerState, Team);
	DOREPLIFETIME(ASBPlayerState, SelectedArchetype);
	DOREPLIFETIME(ASBPlayerState, bAlive);
}

void ASBPlayerState::SetTeam(ESBTeam NewTeam)
{
	if (HasAuthority() && Team != NewTeam)
	{
		Team = NewTeam;
		OnRep_Team();
	}
}

void ASBPlayerState::SetSelectedArchetype(USBCharacterArchetype* NewArchetype)
{
	if (HasAuthority())
	{
		SelectedArchetype = NewArchetype;
	}
}

void ASBPlayerState::OnRep_Team()
{
	// TODO: notify UI / team-colour setup when the local player's team resolves.
}
