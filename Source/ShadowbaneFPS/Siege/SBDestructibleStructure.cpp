// Copyright shadowbanefps.

#include "SBDestructibleStructure.h"
#include "Net/UnrealNetwork.h"

ASBDestructibleStructure::ASBDestructibleStructure()
{
	bReplicates = true;
	SetReplicateMovement(false);
	PrimaryActorTick.bCanEverTick = false;
}

void ASBDestructibleStructure::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	if (HasAuthority())
	{
		RefreshState();
	}
}

void ASBDestructibleStructure::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASBDestructibleStructure, State);
	DOREPLIFETIME(ASBDestructibleStructure, Health);
}

void ASBDestructibleStructure::ApplyStructureDamage(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f || State == ESBStructureState::Destroyed)
	{
		return;
	}

	Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth);
	RefreshState();
}

void ASBDestructibleStructure::Repair(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f)
	{
		return;
	}

	// A fully destroyed structure stays destroyed in the first pilot (design §7).
	if (State == ESBStructureState::Destroyed && bDestructionIsPermanent)
	{
		return;
	}

	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
	RefreshState();
}

void ASBDestructibleStructure::RefreshState()
{
	const float Pct = GetHealthPercent();

	ESBStructureState NewState = ESBStructureState::Intact;
	if (Pct <= 0.f)
	{
		NewState = ESBStructureState::Destroyed;
	}
	else if (Pct < DamagedThreshold)
	{
		NewState = ESBStructureState::Damaged;
	}

	if (NewState != State)
	{
		State = NewState;
		OnRep_State(); // Reflect on the server as well.
		// TODO: on Destroyed, open the associated path / disable the defense and
		// notify the GameMode so conquest routes update (design doc §7).
	}
}

void ASBDestructibleStructure::OnRep_State()
{
	OnStructureStateChanged.Broadcast(State);
}
