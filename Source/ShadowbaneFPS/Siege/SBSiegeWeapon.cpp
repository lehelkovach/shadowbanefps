// Copyright shadowbanefps.

#include "SBSiegeWeapon.h"
#include "SBDestructibleStructure.h"
#include "Characters/SBCharacter.h"
#include "Core/SBPlayerState.h"
#include "Core/SBLog.h"
#include "Art/SBPlaceholderArt.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

ASBSiegeWeapon::ASBSiegeWeapon()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	SetRootComponent(FrameMesh);
	FrameMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FrameMesh->SetCollisionProfileName(TEXT("BlockAll"));
	FrameMesh->SetRelativeScale3D(FVector(2.2f, 1.2f, 1.1f));

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(FrameMesh);
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetRelativeLocation(FVector(80.f, 0.f, 20.f));
	HeadMesh->SetRelativeScale3D(FVector(1.4f, 0.55f, 0.55f));

	UseVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("UseVolume"));
	UseVolume->SetupAttachment(FrameMesh);
	UseVolume->SetBoxExtent(FVector(180.f, 140.f, 120.f));
	UseVolume->SetRelativeLocation(FVector(-40.f, 0.f, 40.f));
	UseVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UseVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	UseVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		FrameMesh->SetStaticMesh(Cube.Object);
		HeadMesh->SetStaticMesh(Cube.Object);
	}
}

void ASBSiegeWeapon::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	ApplyVisuals();
}

void ASBSiegeWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASBSiegeWeapon, Health);
	DOREPLIFETIME(ASBSiegeWeapon, bCrewed);
}

void ASBSiegeWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || Health <= 0.f)
	{
		return;
	}

	if (CrewCharacter)
	{
		const float DistSq = FVector::DistSquared(CrewCharacter->GetActorLocation(), GetActorLocation());
		if (!IsValid(CrewCharacter) || CrewCharacter->GetHealth() <= 0.f || DistSq > FMath::Square(350.f))
		{
			ReleaseCrew();
			return;
		}

		PulseCooldown -= DeltaSeconds;
		if (PulseCooldown <= 0.f)
		{
			PulseCooldown = PulseInterval;
			PulseAttack();
		}

		// Creep the ram toward the gate while crewed (Omaha push).
		if (ASBDestructibleStructure* Target = FindTargetStructure())
		{
			const FVector ToGate = Target->GetActorLocation() - GetActorLocation();
			const float Dist2D = FVector(ToGate.X, ToGate.Y, 0.f).Size();
			if (Dist2D > 220.f)
			{
				const FVector Step = FVector(ToGate.X, ToGate.Y, 0.f).GetSafeNormal() * 160.f * DeltaSeconds;
				SetActorLocation(GetActorLocation() + Step, true);
			}
		}
	}
}

bool ASBSiegeWeapon::TryCrew(ASBCharacter* Character)
{
	if (!HasAuthority() || !Character || Health <= 0.f)
	{
		return false;
	}

	if (CrewCharacter && CrewCharacter != Character)
	{
		return false;
	}

	if (ASBPlayerState* PS = Character->GetPlayerState<ASBPlayerState>())
	{
		if (PS->GetTeam() != ESBTeam::Attackers)
		{
			return false;
		}
	}

	const float DistSq = FVector::DistSquared(Character->GetActorLocation(), GetActorLocation());
	if (DistSq > FMath::Square(280.f))
	{
		return false;
	}

	if (CrewCharacter == Character)
	{
		ReleaseCrew();
		return true;
	}

	CrewCharacter = Character;
	bCrewed = true;
	PulseCooldown = 0.35f;
	OnRep_Crewed();
	UE_LOG(LogShadowbaneCombat, Log, TEXT("Siege weapon crewed by %s"), *Character->GetName());
	return true;
}

void ASBSiegeWeapon::ReleaseCrew()
{
	if (!HasAuthority())
	{
		return;
	}
	CrewCharacter = nullptr;
	bCrewed = false;
	OnRep_Crewed();
}

void ASBSiegeWeapon::ApplyWeaponDamage(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f || Health <= 0.f)
	{
		return;
	}

	Health = FMath::Max(0.f, Health - Amount);
	OnRep_Health();
	if (Health <= 0.f)
	{
		ReleaseCrew();
		UE_LOG(LogShadowbaneServer, Log, TEXT("Siege weapon destroyed: %s"), *GetName());
	}
}

void ASBSiegeWeapon::ConfigureWeapon(float InMaxHealth, float InDamagePerPulse, float InPulseInterval)
{
	MaxHealth = FMath::Max(1.f, InMaxHealth);
	Health = MaxHealth;
	DamagePerPulse = FMath::Max(1.f, InDamagePerPulse);
	PulseInterval = FMath::Max(0.2f, InPulseInterval);
	OnRep_Health();
}

ASBDestructibleStructure* ASBSiegeWeapon::FindTargetStructure() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ASBDestructibleStructure* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<ASBDestructibleStructure> It(World); It; ++It)
	{
		ASBDestructibleStructure* Struct = *It;
		if (!Struct || !Struct->Tags.Contains(TargetStructureTag))
		{
			continue;
		}
		if (Struct->GetState() == ESBStructureState::Destroyed)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Struct->GetActorLocation(), GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Struct;
		}
	}
	return Best;
}

void ASBSiegeWeapon::PulseAttack()
{
	ASBDestructibleStructure* Target = FindTargetStructure();
	if (!Target)
	{
		return;
	}

	// Must be near the gate to actually pound it (push the ram up).
	if (FVector::DistSquared(Target->GetActorLocation(), GetActorLocation()) > FMath::Square(900.f))
	{
		return;
	}

	FName PowerId = FName(TEXT("BatteringRam"));
	FName ArchId = NAME_None;
	AController* InstigatorCtrl = nullptr;
	if (CrewCharacter)
	{
		InstigatorCtrl = CrewCharacter->GetController();
		if (ASBPlayerState* PS = CrewCharacter->GetPlayerState<ASBPlayerState>())
		{
			ArchId = PS->GetSelectedArchetypeId();
		}
	}
	Target->ApplyStructureDamage(DamagePerPulse, InstigatorCtrl, ArchId, PowerId);
	UE_LOG(LogShadowbaneCombat, Verbose, TEXT("Ram pulse %.0f -> %s"), DamagePerPulse, *Target->GetName());
}

void ASBSiegeWeapon::OnRep_Health()
{
	ApplyVisuals();
}

void ASBSiegeWeapon::OnRep_Crewed()
{
	ApplyVisuals();
}

void ASBSiegeWeapon::ApplyVisuals()
{
	const bool bAlive = Health > 0.f;
	if (FrameMesh)
	{
		FrameMesh->SetVisibility(bAlive);
		FrameMesh->SetCollisionEnabled(bAlive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		USBPlaceholderArt::ApplySolidColor(FrameMesh,
			bCrewed ? FLinearColor(0.55f, 0.35f, 0.15f) : FLinearColor(0.4f, 0.28f, 0.16f));
	}
	if (HeadMesh)
	{
		HeadMesh->SetVisibility(bAlive);
		USBPlaceholderArt::ApplySolidColor(HeadMesh,
			bCrewed ? FLinearColor(0.75f, 0.75f, 0.7f) : FLinearColor(0.55f, 0.55f, 0.5f));
	}
}
