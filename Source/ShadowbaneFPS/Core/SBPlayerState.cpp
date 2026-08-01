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
	DOREPLIFETIME(ASBPlayerState, KillCount);
	DOREPLIFETIME(ASBPlayerState, DeathCount);
	DOREPLIFETIME(ASBPlayerState, bHasCreationVitals);
	DOREPLIFETIME(ASBPlayerState, CreationVitals);
	DOREPLIFETIME(ASBPlayerState, CreationRace);
	DOREPLIFETIME(ASBPlayerState, CreationBaseClass);
	DOREPLIFETIME(ASBPlayerState, CreationPrestige);
	DOREPLIFETIME(ASBPlayerState, CreationDiscipline);
}

void ASBPlayerState::SetTeam(ESBTeam NewTeam)
{
	if (HasAuthority() && Team != NewTeam)
	{
		Team = NewTeam;
		OnRep_Team();
	}
}

void ASBPlayerState::AddKill()
{
	if (HasAuthority())
	{
		++KillCount;
	}
}

void ASBPlayerState::AddDeath()
{
	if (HasAuthority())
	{
		++DeathCount;
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

void ASBPlayerState::SetCreationVitalsOverlay(const FSBCreationFpsVitals& Vitals, const FString& Race, const FString& Base, const FString& Prestige, const FString& Discipline)
{
	if (!HasAuthority())
	{
		return;
	}
	CreationVitals = Vitals;
	CreationRace = Race;
	CreationBaseClass = Base;
	CreationPrestige = Prestige;
	CreationDiscipline = Discipline;
	bHasCreationVitals = true;
}

bool ASBPlayerState::ConsumeCreationVitalsOverlay(FSBCreationFpsVitals& OutVitals)
{
	if (!bHasCreationVitals)
	{
		return false;
	}
	OutVitals = CreationVitals;
	// Keep overlay sticky across respawns of the same build (do not clear).
	return true;
}

FString ASBPlayerState::GetCreationSummary() const
{
	if (!bHasCreationVitals)
	{
		return FString();
	}
	if (CreationDiscipline.IsEmpty())
	{
		return FString::Printf(TEXT("%s %s / %s"), *CreationRace, *CreationBaseClass, *CreationPrestige);
	}
	return FString::Printf(TEXT("%s %s / %s [%s]"), *CreationRace, *CreationBaseClass, *CreationPrestige, *CreationDiscipline);
}

void ASBPlayerState::OnRep_Team()
{
}

void ASBPlayerState::OnRep_ArchetypeId()
{
	CachedArchetype = nullptr;
	GetSelectedArchetype();
}
