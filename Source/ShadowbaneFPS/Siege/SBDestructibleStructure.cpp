// Copyright shadowbanefps.

#include "SBDestructibleStructure.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

ASBDestructibleStructure::ASBDestructibleStructure()
{
	bReplicates = true;
	SetReplicateMovement(false);
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetRelativeScale3D(FVector(1.5f, 8.f, 4.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
}

void ASBDestructibleStructure::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	if (HasAuthority())
	{
		RefreshState();
	}
	ApplyVisualState();
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
		OnRep_State();
	}
}

void ASBDestructibleStructure::OnRep_State()
{
	ApplyVisualState();
	OnStructureStateChanged.Broadcast(State);
}

void ASBDestructibleStructure::ApplyVisualState()
{
	if (!Mesh)
	{
		return;
	}

	switch (State)
	{
	case ESBStructureState::Intact:
		Mesh->SetVisibility(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetRelativeScale3D(FVector(1.5f, 8.f, 4.f));
		break;
	case ESBStructureState::Damaged:
		Mesh->SetVisibility(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetRelativeScale3D(FVector(1.2f, 8.f, 3.f));
		break;
	case ESBStructureState::Destroyed:
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
}
