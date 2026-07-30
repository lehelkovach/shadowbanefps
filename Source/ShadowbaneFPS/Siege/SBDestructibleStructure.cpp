// Copyright shadowbanefps.

#include "SBDestructibleStructure.h"
#include "Core/SBRulesLibrary.h"
#include "Core/SBLog.h"
#include "Core/SBSiegeGameMode.h"
#include "Core/SBMatchTelemetry.h"
#include "Core/SBPlayerState.h"
#include "Art/SBPlaceholderArt.h"
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

void ASBDestructibleStructure::ApplyStructureDamage(float Amount, AController* Instigator, FName AttackerArchetype, FName PowerId)
{
	if (!HasAuthority() || Amount <= 0.f || State == ESBStructureState::Destroyed)
	{
		return;
	}

	Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth);

	if (ASBSiegeGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASBSiegeGameMode>() : nullptr)
	{
		if (USBMatchTelemetry* Telemetry = GM->GetTelemetry())
		{
			const ASBPlayerState* KillerPS = Instigator ? Instigator->GetPlayerState<ASBPlayerState>() : nullptr;
			FSBCombatMetric Metric;
			Metric.AttackerName = KillerPS ? KillerPS->GetPlayerName() : TEXT("none");
			Metric.AttackerArchetype = !AttackerArchetype.IsNone() ? AttackerArchetype
				: (KillerPS ? KillerPS->GetSelectedArchetypeId() : NAME_None);
			Metric.AttackerTeam = KillerPS ? KillerPS->GetTeam() : ESBTeam::Unassigned;
			Metric.VictimName = Tags.Num() > 0 ? Tags[0].ToString() : GetName();
			Metric.VictimArchetype = FName(TEXT("Structure"));
			Metric.PowerId = PowerId.IsNone() ? FName(TEXT("StructureHit")) : PowerId;
			Metric.Amount = Amount;
			Metric.VictimHealthAfter = Health;
			Metric.bLethal = Health <= 0.f;
			Metric.Extra = TEXT("structure");
			Telemetry->RecordCombatDamage(Metric);
		}
	}

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
	const ESBStructureState NewState = USBRulesLibrary::ComputeStructureState(Pct, DamagedThreshold);

	if (NewState != State)
	{
		const ESBStructureState OldState = State;
		State = NewState;
		UE_LOG(LogShadowbaneServer, Log, TEXT("Structure %s state %s -> %s (hp=%.0f%%)"),
			*GetName(),
			*UEnum::GetValueAsString(OldState),
			*UEnum::GetValueAsString(NewState),
			Pct * 100.f);
		UE_LOG(LogShadowbane, Log, TEXT("Structure %s state %s -> %s (hp=%.0f%%)"),
			*GetName(),
			*UEnum::GetValueAsString(OldState),
			*UEnum::GetValueAsString(NewState),
			Pct * 100.f);

		if (HasAuthority())
		{
			if (ASBSiegeGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASBSiegeGameMode>() : nullptr)
			{
				if (USBMatchTelemetry* Telemetry = GM->GetTelemetry())
				{
					const FString Tag = Tags.Num() > 0 ? Tags[0].ToString() : GetName();
					if (NewState == ESBStructureState::Destroyed)
					{
						Telemetry->RecordStructure(ESBTelemetryEvent::StructureDestroyed, Tag, Pct);
					}
					else
					{
						Telemetry->RecordStructure(ESBTelemetryEvent::StructureDamaged, Tag, Pct);
					}
				}
			}
		}

		OnRep_State();
	}
}

void ASBDestructibleStructure::OnRep_State()
{
	UE_LOG(LogShadowbaneNet, Log, TEXT("OnRep_StructureState %s -> %s"),
		*GetName(), *UEnum::GetValueAsString(State));
	UE_LOG(LogShadowbaneClient, Log, TEXT("Client structure %s -> %s"),
		*GetName(), *UEnum::GetValueAsString(State));
	ApplyVisualState();
	OnStructureStateChanged.Broadcast(State);
}

void ASBDestructibleStructure::ApplyVisualState()
{
	if (!Mesh)
	{
		return;
	}

	USBPlaceholderArt::ApplySolidColor(Mesh, USBPlaceholderArt::StructureColor(State));

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
