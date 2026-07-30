// Copyright shadowbanefps.

#include "SBCharacter.h"
#include "SBCharacterArchetype.h"
#include "Art/SBPlaceholderArt.h"
#include "Core/SBPlayerState.h"
#include "Core/SBSiegeGameMode.h"
#include "Core/SBTypes.h"
#include "Core/SBLog.h"
#include "Core/SBRulesLibrary.h"
#include "Siege/SBDestructibleStructure.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

ASBCharacter::ASBCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->JumpZVelocity = 450.f;
	GetCharacterMovement()->AirControl = 0.25f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 280.f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bDoCollisionTest = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -20.f));
	BodyMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.6f));

	RuneDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RuneDisc"));
	RuneDisc->SetupAttachment(RootComponent);
	RuneDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RuneDisc->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	RuneDisc->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.08f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		RuneDisc->SetStaticMesh(CylinderMesh.Object);
	}
}

void ASBCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdatePlaceholderVisuals();
}

void ASBCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASBCharacter, Health);
	DOREPLIFETIME(ASBCharacter, MaxHealth);
	DOREPLIFETIME(ASBCharacter, Archetype);
}

void ASBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASBCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ASBCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ASBCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ASBCharacter::LookUp);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ASBCharacter::OnFirePressed);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ASBCharacter::OnInteractPressed);
	PlayerInputComponent->BindAction(TEXT("Ping"), IE_Pressed, this, &ASBCharacter::OnPingPressed);
}

void ASBCharacter::ApplyArchetype(USBCharacterArchetype* InArchetype)
{
	if (!HasAuthority() || !InArchetype)
	{
		return;
	}

	Archetype = InArchetype;
	MaxHealth = InArchetype->MaxHealth;
	Health = MaxHealth;
	AttackDamage = InArchetype->AttackDamage;
	AttackRange = InArchetype->AttackRange;
	AttackInterval = InArchetype->AttackInterval;
	StructureDamage = InArchetype->StructureDamage;
	HealPerSecond = InArchetype->HealPerSecond;
	RepairPerSecond = InArchetype->RepairPerSecond;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = InArchetype->MoveSpeed;
	}

	UE_LOG(LogShadowbane, Verbose, TEXT("Applied archetype %s hp=%.0f speed=%.0f dmg=%.0f siege=%.0f"),
		*InArchetype->ArchetypeId.ToString(), MaxHealth, InArchetype->MoveSpeed, AttackDamage, StructureDamage);

	UpdatePlaceholderVisuals();
}

void ASBCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		PerformSupportTick(DeltaSeconds);
	}

	if (RuneDisc)
	{
		const float Pulse = 0.55f + 0.05f * FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f);
		RuneDisc->SetRelativeScale3D(FVector(Pulse, Pulse, 0.08f));
	}
}

void ASBCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Value);
	}
}

void ASBCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Value);
	}
}

void ASBCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASBCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASBCharacter::OnFirePressed()
{
	ServerFire();
}

void ASBCharacter::OnInteractPressed()
{
	ServerInteract();
}

void ASBCharacter::OnPingPressed()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		const FVector End = CamLoc + CamRot.Vector() * 8000.f;

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SBPing), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Visibility, Params))
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 40.f, 12, FColor::Cyan, false, 3.f, 0, 2.f);
		}
	}
}

void ASBCharacter::ServerFire_Implementation()
{
	PerformFire();
}

void ASBCharacter::ServerInteract_Implementation()
{
	// Siege-device mount / explicit repair channel — next pass.
}

void ASBCharacter::PerformFire()
{
	if (!HasAuthority())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < AttackInterval)
	{
		return;
	}
	LastFireTime = Now;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector End = CamLoc + CamRot.Vector() * AttackRange;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SBFire), false, this);
	// Hit pawns and world geometry (gates / walls / greybox).
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Camera, Params);

	const FVector TraceEnd = bHit ? Hit.ImpactPoint : End;
	DrawDebugLine(GetWorld(), CamLoc, TraceEnd, FColor::Orange, false, 0.15f, 0, 1.5f);

	if (!bHit || !Hit.GetActor())
	{
		return;
	}

	if (ASBDestructibleStructure* Structure = Cast<ASBDestructibleStructure>(Hit.GetActor()))
	{
		Structure->ApplyStructureDamage(StructureDamage);
		return;
	}

	if (ASBCharacter* Target = Cast<ASBCharacter>(Hit.GetActor()))
	{
		const ASBPlayerState* MyPS = GetPlayerState<ASBPlayerState>();
		const ASBPlayerState* TheirPS = Target->GetPlayerState<ASBPlayerState>();
		if (MyPS && TheirPS && USBRulesLibrary::IsFriendlyFire(MyPS->GetTeam(), TheirPS->GetTeam()))
		{
			UE_LOG(LogShadowbaneCombat, Verbose, TEXT("Friendly fire blocked %s -> %s"),
				*GetName(), *Target->GetName());
			return;
		}

		Target->TakeDamage(AttackDamage, FDamageEvent(), GetController(), this);
	}
}

void ASBCharacter::PerformSupportTick(float DeltaSeconds)
{
	if (HealPerSecond <= 0.f && RepairPerSecond <= 0.f)
	{
		return;
	}

	const FVector Origin = GetActorLocation();

	if (HealPerSecond > 0.f)
	{
		const float HealRadiusSq = FMath::Square(450.f);
		for (TActorIterator<ASBCharacter> It(GetWorld()); It; ++It)
		{
			ASBCharacter* Other = *It;
			if (!Other || Other == this || Other->Health >= Other->MaxHealth)
			{
				continue;
			}

			const ASBPlayerState* MyPS = GetPlayerState<ASBPlayerState>();
			const ASBPlayerState* TheirPS = Other->GetPlayerState<ASBPlayerState>();
			if (!MyPS || !TheirPS || MyPS->GetTeam() != TheirPS->GetTeam())
			{
				continue;
			}

			if (FVector::DistSquared(Origin, Other->GetActorLocation()) <= HealRadiusSq)
			{
				Other->Health = FMath::Min(Other->MaxHealth, Other->Health + HealPerSecond * DeltaSeconds);
			}
		}
	}

	if (RepairPerSecond > 0.f)
	{
		const float RepairRadiusSq = FMath::Square(350.f);
		for (TActorIterator<ASBDestructibleStructure> It(GetWorld()); It; ++It)
		{
			ASBDestructibleStructure* Structure = *It;
			if (!Structure || Structure->GetState() == ESBStructureState::Destroyed)
			{
				continue;
			}
			if (FVector::DistSquared(Origin, Structure->GetActorLocation()) <= RepairRadiusSq)
			{
				Structure->Repair(RepairPerSecond * DeltaSeconds);
			}
		}
	}
}

float ASBCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || Health <= 0.f || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Applied);

	UE_LOG(LogShadowbaneCombat, Verbose, TEXT("%s took %.1f damage (hp %.0f/%.0f)"),
		*GetName(), Applied, Health, MaxHealth);

	if (Health <= 0.f)
	{
		Die(EventInstigator);
	}

	return Applied;
}

void ASBCharacter::Die(AController* KillerController)
{
	if (!HasAuthority())
	{
		return;
	}

	ASBPlayerState* VictimPS = GetPlayerState<ASBPlayerState>();
	ASBPlayerState* KillerPS = KillerController ? KillerController->GetPlayerState<ASBPlayerState>() : nullptr;

	UE_LOG(LogShadowbane, Log, TEXT("%s died (archetype=%s)"),
		*GetName(),
		Archetype ? *Archetype->ArchetypeId.ToString() : TEXT("none"));

	if (ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>())
	{
		GM->NotifyPlayerKilled(VictimPS, KillerPS);
	}

	DetachFromControllerPendingDestroy();
	SetLifeSpan(0.2f);
}

void ASBCharacter::OnRep_Health()
{
	// Hook for damage feedback / HUD pulse.
}

void ASBCharacter::OnRep_Archetype()
{
	UpdatePlaceholderVisuals();
}

void ASBCharacter::UpdatePlaceholderVisuals()
{
	const ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	const FLinearColor BodyColor = USBPlaceholderArt::TeamColor(PS ? PS->GetTeam() : ESBTeam::Unassigned);
	USBPlaceholderArt::ApplySolidColor(BodyMesh, BodyColor);

	const FSBPlaceholderIcon Icon = USBPlaceholderArt::MakeIcon(Archetype);
	USBPlaceholderArt::ApplySolidColor(RuneDisc, Icon.Tint);
}
