// Copyright shadowbanefps.

#include "SBPlayerState.h"
#include "SBSiegeGameState.h"
#include "Characters/SBCharacterArchetype.h"
#include "Net/UnrealNetwork.h"

ASBPlayerState::ASBPlayerState()
{
	SetNetUpdateFrequency(10.0f);
}

void ASBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASBPlayerState, Team);
	DOREPLIFETIME(ASBPlayerState, SelectedArchetypeId);
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
	if (!HasAuthority())
	{
		return;
	}

	SelectedArchetypeId = NewArchetype ? NewArchetype->ArchetypeId : NAME_None;
	CachedArchetype = NewArchetype;
	OnRep_ArchetypeId();
}

USBCharacterArchetype* ASBPlayerState::GetSelectedArchetype() const
{
	if (CachedArchetype && CachedArchetype->ArchetypeId == SelectedArchetypeId)
	{
		return CachedArchetype;
	}

	if (const ASBSiegeGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASBSiegeGameState>() : nullptr)
	{
		CachedArchetype = GS->FindArchetypeById(SelectedArchetypeId);
	}

	return CachedArchetype;
}

void ASBPlayerState::SetAlive(bool bNewAlive)
{
	if (HasAuthority())
	{
		bAlive = bNewAlive;
	}
}

void ASBPlayerState::OnRep_Team()
{
}

void ASBPlayerState::OnRep_ArchetypeId()
{
	CachedArchetype = nullptr;
	GetSelectedArchetype();
}
