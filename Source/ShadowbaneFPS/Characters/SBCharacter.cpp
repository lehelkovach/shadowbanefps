// Copyright shadowbanefps.

#include "SBCharacter.h"
#include "SBCharacterArchetype.h"
#include "SBCharacterCalculator.h"
#include "Art/SBPlaceholderArt.h"
#include "Core/SBPlayerState.h"
#include "Core/SBSiegeGameMode.h"
#include "Core/SBTypes.h"
#include "Core/SBLog.h"
#include "Core/SBRulesLibrary.h"
#include "Core/SBMatchTelemetry.h"
#include "Siege/SBDestructibleStructure.h"
#include "Camera/CameraComponent.h"
#include "Engine/DamageEvents.h"
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
#include "Audio/SBCastShout.h"
#include "Core/SBClientDebug.h"
#include "Art/SBRaceSilhouette.h"
#include "Art/SBMeleeSwingAnim.h"
#include "Art/SBBattleAxeMesh.h"
#include "Siege/SBSiegeWeapon.h"
#include "UI/SBDamageFloat.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputActionValue.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "TimerManager.h"

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
	SpringArm->TargetArmLength = 320.f;
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
	BodyMesh->SetCastShadow(true);
	BodyMesh->SetOwnerNoSee(false);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(RootComponent);
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FeatureL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FeatureL"));
	FeatureL->SetupAttachment(RootComponent);
	FeatureL->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FeatureL->SetHiddenInGame(true);

	FeatureR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FeatureR"));
	FeatureR->SetupAttachment(RootComponent);
	FeatureR->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FeatureR->SetHiddenInGame(true);

	FeatureExtra = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FeatureExtra"));
	FeatureExtra->SetupAttachment(RootComponent);
	FeatureExtra->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FeatureExtra->SetHiddenInGame(true);

	RuneDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RuneDisc"));
	RuneDisc->SetupAttachment(RootComponent);
	RuneDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RuneDisc->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	RuneDisc->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.08f));

	HandL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandL"));
	HandL->SetupAttachment(RootComponent);
	HandL->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandL->SetHiddenInGame(true);

	HandR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandR"));
	HandR->SetupAttachment(RootComponent);
	HandR->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandR->SetHiddenInGame(true);

	// First-person axe (camera-mounted) + third-person axe (body) for peers/bots.
	FpWeaponPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FpWeaponPivot"));
	FpWeaponPivot->SetupAttachment(Camera);
	FpWeaponPivot->SetRelativeLocation(FVector(35.f, 28.f, -18.f));

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(FpWeaponPivot);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	WeaponMesh->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	WeaponMesh->SetRelativeScale3D(FVector(0.1f, 0.1f, 1.05f)); // haft
	WeaponMesh->SetHiddenInGame(true);

	AxeBladeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AxeBladeMesh"));
	AxeBladeMesh->SetupAttachment(FpWeaponPivot);
	AxeBladeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AxeBladeMesh->SetRelativeLocation(FVector(0.f, 8.f, 48.f));
	AxeBladeMesh->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
	AxeBladeMesh->SetRelativeScale3D(FVector(0.55f, 0.08f, 0.35f)); // axe head
	AxeBladeMesh->SetHiddenInGame(true);

	TpWeaponPivot = CreateDefaultSubobject<USceneComponent>(TEXT("TpWeaponPivot"));
	TpWeaponPivot->SetupAttachment(RootComponent);
	TpWeaponPivot->SetRelativeLocation(FVector(15.f, 35.f, 20.f));

	TpWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TpWeaponMesh"));
	TpWeaponMesh->SetupAttachment(TpWeaponPivot);
	TpWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TpWeaponMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 1.2f));
	TpWeaponMesh->SetHiddenInGame(true);

	TpAxeBladeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TpAxeBladeMesh"));
	TpAxeBladeMesh->SetupAttachment(TpWeaponPivot);
	TpAxeBladeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TpAxeBladeMesh->SetRelativeLocation(FVector(0.f, 10.f, 55.f));
	TpAxeBladeMesh->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
	TpAxeBladeMesh->SetRelativeScale3D(FVector(0.7f, 0.1f, 0.42f));
	TpAxeBladeMesh->SetHiddenInGame(true);

	TpBattleAxe = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TpBattleAxe"));
	TpBattleAxe->SetupAttachment(TpWeaponPivot);
	TpBattleAxe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TpBattleAxe->SetCastShadow(true);
	TpBattleAxe->SetHiddenInGame(true);

	// Prefer imported battle-axe mesh, then Kenney handaxe — never cube/procedural when available.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BattleAxeMesh(
		TEXT("/Game/Art/Weapons/SM_BattleAxe.SM_BattleAxe"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HandAxeMesh(
		TEXT("/Game/Art/Weapons/SM_HandAxe_Upgraded.SM_HandAxe_Upgraded"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HandAxeMeshFallback(
		TEXT("/Game/Art/Weapons/SM_HandAxe.SM_HandAxe"));
	if (BattleAxeMesh.Succeeded())
	{
		TpWeaponMesh->SetStaticMesh(BattleAxeMesh.Object);
	}
	else if (HandAxeMesh.Succeeded())
	{
		TpWeaponMesh->SetStaticMesh(HandAxeMesh.Object);
	}
	else if (HandAxeMeshFallback.Succeeded())
	{
		TpWeaponMesh->SetStaticMesh(HandAxeMeshFallback.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeMesh.Object);
		WeaponMesh->SetStaticMesh(CubeMesh.Object);
		AxeBladeMesh->SetStaticMesh(CubeMesh.Object);
		// Only use cubes for TP axe if no handaxe asset loaded.
		if (!TpWeaponMesh->GetStaticMesh())
		{
			TpWeaponMesh->SetStaticMesh(CubeMesh.Object);
			TpAxeBladeMesh->SetStaticMesh(CubeMesh.Object);
		}
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		HeadMesh->SetStaticMesh(SphereMesh.Object);
		HandL->SetStaticMesh(SphereMesh.Object);
		HandR->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		RuneDisc->SetStaticMesh(CylinderMesh.Object);
		FeatureL->SetStaticMesh(CylinderMesh.Object);
		FeatureR->SetStaticMesh(CylinderMesh.Object);
		FeatureExtra->SetStaticMesh(CylinderMesh.Object);
	}

	// Epic Third-Person mannequin (Content/Characters/Mannequins from TemplateResources).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannyABP(
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));

	if (USkeletalMeshComponent* Hero = GetMesh())
	{
		Hero->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Hero->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		Hero->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		Hero->bOwnerNoSee = false;
		Hero->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

		if (MannyMesh.Succeeded())
		{
			Hero->SetSkeletalMesh(MannyMesh.Object);
			bUsingHeroMesh = true;
		}
		if (MannyABP.Succeeded())
		{
			Hero->SetAnimInstanceClass(MannyABP.Class);
		}
		HeroMeshBaseRelativeRot = Hero->GetRelativeRotation();
	}

	ApplyMeleeIdlePose();
	if (bUsingHeroMesh)
	{
		SetPlaceholderBodyVisible(false);
	}
}

void ASBCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (TpBattleAxe && TpBattleAxe->GetNumSections() == 0)
	{
		SBBattleAxeMesh::Build(TpBattleAxe);
	}
	UpdatePlaceholderVisuals();
	EnsureMeleeSwingAnimAssets();
	BindHeroBoneSwingOverlay();

	// AnimBP / mesh init can stomp materials on the first frames — re-tint shortly after.
	if (UWorld* World = GetWorld())
	{
		FTimerHandle SkinTimer;
		World->GetTimerManager().SetTimer(SkinTimer, FTimerDelegate::CreateUObject(this, &ASBCharacter::ApplyHeroRaceMaterials), 0.15f, false);
	}
}

void ASBCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindHeroBoneSwingOverlay();
	Super::EndPlay(EndPlayReason);
}

void ASBCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureGameplayInputMode();
	EnsureEnhancedInputAssets();
	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (MoveMappingContext)
				{
					Subsys->AddMappingContext(MoveMappingContext, 1);
				}
			}
		}
	}
	ApplyHeroRaceMaterials();
}

void ASBCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	EnsureGameplayInputMode();
	EnsureEnhancedInputAssets();
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (MoveMappingContext)
				{
					Subsys->ClearAllMappings();
					Subsys->AddMappingContext(MoveMappingContext, 1);
				}
			}
		}
	}
	ApplyHeroRaceMaterials();
}

void ASBCharacter::EnsureGameplayInputMode()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	PC->bShowMouseCursor = false;
	FInputModeGameOnly Mode;
	PC->SetInputMode(Mode);
}

void ASBCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASBCharacter, Health);
	DOREPLIFETIME(ASBCharacter, MaxHealth);
	DOREPLIFETIME(ASBCharacter, Mana);
	DOREPLIFETIME(ASBCharacter, MaxMana);
	DOREPLIFETIME(ASBCharacter, Stamina);
	DOREPLIFETIME(ASBCharacter, MaxStamina);
	DOREPLIFETIME(ASBCharacter, bDead);
	DOREPLIFETIME(ASBCharacter, Archetype);
	DOREPLIFETIME(ASBCharacter, RaceName);
	DOREPLIFETIME(ASBCharacter, bUsesMelee);
	DOREPLIFETIME(ASBCharacter, bBowEquipped);
	DOREPLIFETIME(ASBCharacter, bCasting);
	DOREPLIFETIME(ASBCharacter, bRecalling);
}

void ASBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureEnhancedInputAssets();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (MoveMappingContext)
				{
					Subsys->AddMappingContext(MoveMappingContext, 0);
				}
			}
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// WASD + mouse are polled in Tick (PollKeyboardMovement / PollMouseLook) so they
		// keep working when Enhanced Input swallows legacy axes.
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (FireAction)
		{
			EIC->BindAction(FireAction, ETriggerEvent::Started, this, &ASBCharacter::OnFirePressed);
		}
	}

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ASBCharacter::OnFirePressed);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &ASBCharacter::OnFireReleased);
	PlayerInputComponent->BindAction(TEXT("ToggleBow"), IE_Pressed, this, &ASBCharacter::OnToggleBowPressed);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ASBCharacter::OnInteractPressed);
	PlayerInputComponent->BindAction(TEXT("Ping"), IE_Pressed, this, &ASBCharacter::OnPingPressed);

	EnsureGameplayInputMode();
}

void ASBCharacter::ApplyArchetype(USBCharacterArchetype* InArchetype)
{
	if (!HasAuthority() || !InArchetype)
	{
		return;
	}

	Archetype = InArchetype;
	RaceName = InArchetype->Race.ToString();
	MaxHealth = InArchetype->MaxHealth;
	Health = MaxHealth;
	MaxMana = InArchetype->MaxMana;
	Mana = MaxMana;
	MaxStamina = InArchetype->MaxStamina;
	Stamina = MaxStamina;
	ManaRegenPerSecond = InArchetype->ManaRegenPerSecond;
	StaminaRegenPerSecond = InArchetype->StaminaRegenPerSecond;
	bDead = false;
	AttackDamage = InArchetype->AttackDamage;
	AttackRange = InArchetype->AttackRange;
	AttackInterval = InArchetype->AttackInterval;
	StructureDamage = InArchetype->StructureDamage;
	HealPerSecond = InArchetype->HealPerSecond;
	RepairPerSecond = InArchetype->RepairPerSecond;
	bUsesMelee = InArchetype->bMeleeAttack;
	bBowEquipped = !bUsesMelee; // Rangers etc. start on bow; warriors start axe (Q to swap).

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = InArchetype->MoveSpeed;
	}

	UE_LOG(LogShadowbane, Verbose, TEXT("Applied archetype %s hp=%.0f mana=%.0f stam=%.0f speed=%.0f dmg=%.0f siege=%.0f melee=%d bow=%d"),
		*InArchetype->ArchetypeId.ToString(), MaxHealth, MaxMana, MaxStamina, InArchetype->MoveSpeed, AttackDamage, StructureDamage,
		bUsesMelee ? 1 : 0, bBowEquipped ? 1 : 0);

	UpdatePlaceholderVisuals();
	UpdateWeaponVisibility();
}

void ASBCharacter::ApplyCreationVitals(const FSBCreationFpsVitals& Vitals, const FString& OverlayRace)
{
	if (!HasAuthority())
	{
		return;
	}

	MaxHealth = Vitals.MaxHealth;
	Health = MaxHealth;
	MaxMana = Vitals.MaxMana;
	Mana = MaxMana;
	MaxStamina = Vitals.MaxStamina;
	Stamina = MaxStamina;
	ManaRegenPerSecond = Vitals.ManaRegen;
	StaminaRegenPerSecond = Vitals.StaminaRegen;
	AttackDamage = Vitals.AttackDamage;
	bUsesMelee = Vitals.bPreferMelee;
	bBowEquipped = !bUsesMelee;
	if (!OverlayRace.IsEmpty())
	{
		RaceName = OverlayRace;
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Vitals.MoveSpeed;
	}

	UpdatePlaceholderVisuals();
	UpdateWeaponVisibility();
}

void ASBCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		PerformSupportTick(DeltaSeconds);
		TickRecall(DeltaSeconds);
		TickSpellCast(DeltaSeconds);
		TickVitals(DeltaSeconds);
	}

	if (bRecalling)
	{
		const float PulseR = 0.7f + 0.35f * FMath::Sin(GetWorld()->GetTimeSeconds() * 10.f);
		DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 20.f), 70.f * PulseR, 16, FColor::Cyan, false, -1.f, 0, 2.f);
	}

	if (bCasting)
	{
		const float Pulse = 0.55f + 0.45f * (1.f - FMath::Clamp(CastTimeRemaining / FMath::Max(SpellCastDuration, 0.1f), 0.f, 1.f));
		DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 95.f), 40.f * Pulse, 12, FColor::Magenta, false, -1.f, 0, 2.f);
	}

	if (CastFlashRemaining > 0.f)
	{
		CastFlashRemaining = FMath::Max(0.f, CastFlashRemaining - DeltaSeconds);
		TickCastInvokeVisual(DeltaSeconds);
	}

	if (RuneDisc)
	{
		float Pulse = 0.55f + 0.05f * FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f);
		if (CastFlashRemaining > 0.f)
		{
			Pulse += 0.45f * (CastFlashRemaining / FMath::Max(CastInvokeDuration, KINDA_SMALL_NUMBER));
		}
		RuneDisc->SetRelativeScale3D(FVector(Pulse, Pulse, 0.08f));
	}

	if (WeaponMesh || TpWeaponMesh)
	{
		TickMeleeSwingVisual(DeltaSeconds);
	}

	if (IsLocallyControlled())
	{
		EnsureGameplayInputMode();
		PollKeyboardMovement();
		PollMouseLook();
	}

	// Soft catch if anyone falls off the pad into the void.
	if (HasAuthority() && !bDead)
	{
		const FVector Loc = GetActorLocation();
		if (Loc.Z < -80.f)
		{
			SetActorLocation(FVector(FMath::Clamp(Loc.X, -5000.f, 3500.f), FMath::Clamp(Loc.Y, -2000.f, 2000.f), 140.f),
				false, nullptr, ETeleportType::ResetPhysics);
			if (UCharacterMovementComponent* Move = GetCharacterMovement())
			{
				Move->StopMovementImmediately();
				Move->SetMovementMode(MOVE_Walking);
			}
		}
	}
}

void ASBCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.f)
	{
		if (bCasting)
		{
			// Shadowbane-style: stepping breaks the cast.
			if (HasAuthority())
			{
				CancelSpellCast(TEXT("move"));
			}
			else if (IsLocallyControlled())
			{
				bCasting = false;
			}
			return;
		}
		if (bRecalling && HasAuthority())
		{
			CancelRecall(TEXT("move"));
		}
		else if (bRecalling && IsLocallyControlled())
		{
			// Client predicts cancel; server confirms via velocity / next RPC.
			bRecalling = false;
		}
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Value);
	}
}

void ASBCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.f)
	{
		if (bCasting)
		{
			if (HasAuthority())
			{
				CancelSpellCast(TEXT("strafe"));
			}
			else if (IsLocallyControlled())
			{
				bCasting = false;
			}
			return;
		}
		if (bRecalling && HasAuthority())
		{
			CancelRecall(TEXT("strafe"));
		}
		else if (bRecalling && IsLocallyControlled())
		{
			bRecalling = false;
		}
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Value);
	}
}

void ASBCharacter::OnMoveForwardTriggered(const FInputActionValue& Value)
{
	MoveForward(Value.Get<float>());
}

void ASBCharacter::OnMoveRightTriggered(const FInputActionValue& Value)
{
	MoveRight(Value.Get<float>());
}

void ASBCharacter::OnLookTriggered(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	Turn(Axis.X);
	LookUp(Axis.Y);
}

void ASBCharacter::PollKeyboardMovement()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	float Forward = 0.f;
	float Right = 0.f;
	if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up)) { Forward += 1.f; }
	if (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down)) { Forward -= 1.f; }
	if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right)) { Right += 1.f; }
	if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left)) { Right -= 1.f; }

	if (Forward != 0.f)
	{
		MoveForward(Forward);
	}
	if (Right != 0.f)
	{
		MoveRight(Right);
	}
}

void ASBCharacter::PollMouseLook()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	float DX = 0.f;
	float DY = 0.f;
	PC->GetInputMouseDelta(DX, DY);
	if (DX == 0.f && DY == 0.f)
	{
		return;
	}

	// Match DefaultInput.ini MouseX/Y sensitivity (~0.07) with a usable floor.
	const float Sens = 0.12f;
	Turn(DX * Sens);
	LookUp(-DY * Sens);
}

void ASBCharacter::EnsureEnhancedInputAssets()
{
	if (MoveMappingContext)
	{
		return;
	}

	MoveForwardAction = NewObject<UInputAction>(this, TEXT("IA_SB_MoveForward"));
	MoveForwardAction->ValueType = EInputActionValueType::Axis1D;

	MoveRightAction = NewObject<UInputAction>(this, TEXT("IA_SB_MoveRight"));
	MoveRightAction->ValueType = EInputActionValueType::Axis1D;

	LookAction = NewObject<UInputAction>(this, TEXT("IA_SB_Look"));
	LookAction->ValueType = EInputActionValueType::Axis2D;

	JumpAction = NewObject<UInputAction>(this, TEXT("IA_SB_Jump"));
	JumpAction->ValueType = EInputActionValueType::Boolean;

	FireAction = NewObject<UInputAction>(this, TEXT("IA_SB_Fire"));
	FireAction->ValueType = EInputActionValueType::Boolean;

	MoveMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_SB_Default"));

	MoveMappingContext->MapKey(MoveForwardAction, EKeys::W);
	{
		FEnhancedActionKeyMapping& Back = MoveMappingContext->MapKey(MoveForwardAction, EKeys::S);
		UInputModifierNegate* Neg = NewObject<UInputModifierNegate>(this);
		Back.Modifiers.Add(Neg);
	}
	MoveMappingContext->MapKey(MoveRightAction, EKeys::D);
	{
		FEnhancedActionKeyMapping& Left = MoveMappingContext->MapKey(MoveRightAction, EKeys::A);
		UInputModifierNegate* Neg = NewObject<UInputModifierNegate>(this);
		Left.Modifiers.Add(Neg);
	}

	{
		FEnhancedActionKeyMapping& Mouse = MoveMappingContext->MapKey(LookAction, EKeys::Mouse2D);
		// Invert Y to match classic look (LookUp Scale=-1).
		UInputModifierNegate* NegY = NewObject<UInputModifierNegate>(this);
		NegY->bX = false;
		NegY->bY = true;
		NegY->bZ = false;
		Mouse.Modifiers.Add(NegY);
	}

	MoveMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MoveMappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
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

void ASBCharacter::BotFire()
{
	if (HasAuthority())
	{
		PerformAttack();
	}
}

void ASBCharacter::ServerFire_Implementation()
{
	PerformAttack();
}

void ASBCharacter::ServerInteract_Implementation()
{
	// Prefer crewing a nearby battering ram / siege weapon.
	ASBSiegeWeapon* BestWeapon = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<ASBSiegeWeapon> It(GetWorld()); It; ++It)
	{
		ASBSiegeWeapon* Weapon = *It;
		if (!Weapon)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Weapon->GetActorLocation(), GetActorLocation());
		if (DistSq < BestDistSq && DistSq <= FMath::Square(280.f))
		{
			BestDistSq = DistSq;
			BestWeapon = Weapon;
		}
	}
	if (BestWeapon)
	{
		BestWeapon->TryCrew(this);
	}
}

void ASBCharacter::PerformAttack()
{
	if (!HasAuthority() || bDead || Health <= 0.f)
	{
		return;
	}

	// Archers / melee keep full mobility. Only spells root.
	CancelRecall(TEXT("attack"));
	if (bCasting)
	{
		CancelSpellCast(TEXT("attack"));
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < AttackInterval)
	{
		return;
	}

	if (bUsesMelee && !bBowEquipped)
	{
		if (Stamina < MeleeStaminaCost)
		{
			return;
		}
		Stamina = FMath::Max(0.f, Stamina - MeleeStaminaCost);
		LastFireTime = Now;
		PerformMeleeSwing();
	}
	else
	{
		LastFireTime = Now;
		// Bow / ranged: shoot while moving — classic MMO archer fantasy.
		PerformHitscanFire();
	}
}

void ASBCharacter::MulticastMeleeSwing_Implementation()
{
	MeleeSwingAnimDuration = SBMeleeSwingAnim::DurationSeconds;
	MeleeSwingAnimRemaining = MeleeSwingAnimDuration;
	bSwingTrailValid = false;
	EnsureWeaponInHand();
	UpdateWeaponVisibility();
	PlayMeleeAttackAnimation();
}

void ASBCharacter::ServerCastSpell_Implementation(int32 SlotIndex)
{
	BeginSpellCast(SlotIndex);
}

void ASBCharacter::MulticastCastSpell_Implementation(int32 SlotIndex)
{
	CastFlashRemaining = CastInvokeDuration;
	TickCastInvokeVisual(0.f);
	SBCastShout::PlayAt(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 70.f));

	const FColor Colors[4] = { FColor::Magenta, FColor::Cyan, FColor::Orange, FColor::Emerald };
	const FColor Color = Colors[FMath::Clamp(SlotIndex, 0, 3)];
	DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 90.f), 55.f, 12, Color, false, 0.45f, 0, 3.f);

	if (IsLocallyControlled())
	{
		static const TCHAR* Names[4] = { TEXT("Bolt"), TEXT("Heal"), TEXT("Shock"), TEXT("Reinforce") };
		FSBClientDebug::PushMessage(
			FString::Printf(TEXT("Casting %s — stand still"), Names[FMath::Clamp(SlotIndex, 0, 3)]),
			SpellCastDuration + 0.4f);
	}
}

void ASBCharacter::MulticastCastInterrupted_Implementation()
{
	CastFlashRemaining = 0.f;
	if (HandL) { HandL->SetHiddenInGame(true); }
	if (HandR) { HandR->SetHiddenInGame(true); }
	if (IsLocallyControlled())
	{
		FSBClientDebug::PushMessage(TEXT("Cast interrupted"), 1.8f);
	}
}

void ASBCharacter::ClientCombatHint_Implementation(const FString& Message)
{
	FSBClientDebug::PushMessage(Message.IsEmpty() ? TEXT("(hint)") : Message, 2.f);
}

void ASBCharacter::MulticastShowDamage_Implementation(float Amount, FVector Loc, bool bHeal)
{
	SBDamageFloat::Push(GetWorld(), Loc, Amount, bHeal);
}

void ASBCharacter::PerformSpell(int32 SlotIndex)
{
	BeginSpellCast(SlotIndex);
}

void ASBCharacter::BeginSpellCast(int32 SlotIndex)
{
	if (!HasAuthority() || Health <= 0.f)
	{
		return;
	}

	if (SlotIndex < 0 || SlotIndex > 3)
	{
		return;
	}

	if (bCasting)
	{
		return;
	}

	CancelRecall(TEXT("spell"));

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastSpellTime < SpellCooldown)
	{
		return;
	}

	if (Mana < SpellManaCost)
	{
		ClientCombatHint(TEXT("Not enough mana"));
		return;
	}

	// Must already be nearly still to start (Shadowbane / fantasy mage rule).
	if (GetVelocity().Size2D() > CastMoveCancelSpeed)
	{
		ClientCombatHint(TEXT("Stand still to cast"));
		UE_LOG(LogShadowbaneCombat, Verbose, TEXT("%s cast rejected (moving)"), *GetName());
		return;
	}

	LastSpellTime = Now;
	Mana = FMath::Max(0.f, Mana - SpellManaCost);
	bCasting = true;
	PendingCastSlot = SlotIndex;
	CastTimeRemaining = SpellCastDuration;
	ApplyCastRoot(true);
	MulticastCastSpell(SlotIndex);
	UE_LOG(LogShadowbaneCombat, Log, TEXT("%s began cast slot %d (%.2fs)"), *GetName(), SlotIndex, SpellCastDuration);
}

void ASBCharacter::TickSpellCast(float DeltaSeconds)
{
	if (!bCasting)
	{
		return;
	}

	if (GetVelocity().Size2D() > CastMoveCancelSpeed)
	{
		CancelSpellCast(TEXT("velocity"));
		return;
	}

	CastTimeRemaining -= DeltaSeconds;
	if (CastTimeRemaining <= 0.f)
	{
		CompleteSpellCast();
	}
}

void ASBCharacter::CancelSpellCast(const TCHAR* Reason)
{
	if (!HasAuthority() || !bCasting)
	{
		return;
	}

	bCasting = false;
	PendingCastSlot = -1;
	CastTimeRemaining = 0.f;
	ApplyCastRoot(false);
	MulticastCastInterrupted();
	UE_LOG(LogShadowbaneCombat, Verbose, TEXT("%s cast cancelled (%s)"), *GetName(), Reason ? Reason : TEXT("?"));
}

void ASBCharacter::CompleteSpellCast()
{
	if (!HasAuthority() || !bCasting)
	{
		return;
	}

	const int32 Slot = PendingCastSlot;
	bCasting = false;
	PendingCastSlot = -1;
	CastTimeRemaining = 0.f;
	ApplyCastRoot(false);

	switch (Slot)
	{
	case 0: PerformSpellBolt(); break;
	case 1: PerformSpellHealPulse(); break;
	case 2: PerformSpellShockwave(); break;
	case 3: PerformSpellReinforce(); break;
	default: break;
	}

	UE_LOG(LogShadowbaneCombat, Log, TEXT("%s completed cast slot %d"), *GetName(), Slot);
}

void ASBCharacter::ApplyCastRoot(bool bRoot)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	if (bRoot)
	{
		WalkSpeedBeforeCast = Move->MaxWalkSpeed;
		Move->StopMovementImmediately();
		Move->MaxWalkSpeed = 0.f;
	}
	else
	{
		Move->MaxWalkSpeed = WalkSpeedBeforeCast > 0.f ? WalkSpeedBeforeCast : 600.f;
	}
}

void ASBCharacter::OnRep_Casting()
{
	ApplyCastRoot(bCasting);
	if (bCasting && IsLocallyControlled())
	{
		FSBClientDebug::PushMessage(TEXT("Casting — stand still"), SpellCastDuration);
	}
}

void ASBCharacter::PerformSpellBolt()
{
	FVector CamLoc;
	FRotator CamRot;
	if (AController* C = GetController())
	{
		C->GetPlayerViewPoint(CamLoc, CamRot);
	}
	else
	{
		CamLoc = GetActorLocation() + FVector(0.f, 0.f, 60.f);
		CamRot = GetActorRotation();
	}

	const float Range = FMath::Max(AttackRange, 1800.f);
	const FVector End = CamLoc + CamRot.Vector() * Range;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SBSpellBolt), false, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Camera, Params);
	const FVector TraceEnd = bHit ? Hit.ImpactPoint : End;
	DrawDebugLine(GetWorld(), CamLoc, TraceEnd, FColor::Magenta, false, 0.25f, 0, 2.5f);

	if (!bHit || !Hit.GetActor())
	{
		return;
	}

	const float SpellDamage = AttackDamage * 1.35f;
	if (ASBDestructibleStructure* Structure = Cast<ASBDestructibleStructure>(Hit.GetActor()))
	{
		Structure->ApplyStructureDamage(StructureDamage * 1.25f, GetController(),
			Archetype ? Archetype->ArchetypeId : NAME_None,
			FName(TEXT("SpellBolt_Structure")));
		return;
	}

	if (ASBCharacter* Target = Cast<ASBCharacter>(Hit.GetActor()))
	{
		const ASBPlayerState* MyPS = GetPlayerState<ASBPlayerState>();
		const ASBPlayerState* TheirPS = Target->GetPlayerState<ASBPlayerState>();
		if (MyPS && TheirPS && USBRulesLibrary::IsFriendlyFire(MyPS->GetTeam(), TheirPS->GetTeam()))
		{
			return;
		}
		Target->LastDamagePowerId = FName(TEXT("SpellBolt"));
		Target->TakeDamage(SpellDamage, FDamageEvent(), GetController(), this);
	}
}

void ASBCharacter::PerformSpellHealPulse()
{
	const float HealAmount = FMath::Max(22.f, MaxHealth * 0.18f);
	const float Radius = 450.f;
	DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 16, FColor::Cyan, false, 0.35f, 0, 2.f);

	for (TActorIterator<ASBCharacter> It(GetWorld()); It; ++It)
	{
		ASBCharacter* Ally = *It;
		if (!Ally || Ally->Health <= 0.f)
		{
			continue;
		}
		if (FVector::DistSquared(Ally->GetActorLocation(), GetActorLocation()) > Radius * Radius)
		{
			continue;
		}

		const ASBPlayerState* MyPS = GetPlayerState<ASBPlayerState>();
		const ASBPlayerState* TheirPS = Ally->GetPlayerState<ASBPlayerState>();
		if (MyPS && TheirPS && MyPS->GetTeam() != TheirPS->GetTeam() && Ally != this)
		{
			continue;
		}

		const float Before = Ally->Health;
		Ally->Health = FMath::Min(Ally->MaxHealth, Ally->Health + HealAmount);
		Ally->OnRep_Health();
		const float AppliedHeal = Ally->Health - Before;
		if (AppliedHeal > 0.f)
		{
			Ally->MulticastShowDamage(AppliedHeal, Ally->GetActorLocation() + FVector(0.f, 0.f, 60.f), true);
		}
		UE_LOG(LogShadowbaneCombat, Log, TEXT("%s SpellHeal +%.0f on %s (%.0f->%.0f)"),
			*GetName(), HealAmount, *Ally->GetName(), Before, Ally->Health);
	}
}

void ASBCharacter::PerformSpellShockwave()
{
	const float Radius = 380.f;
	const float SpellDamage = AttackDamage * 0.85f;
	DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 16, FColor::Orange, false, 0.3f, 0, 2.5f);

	for (TActorIterator<ASBCharacter> It(GetWorld()); It; ++It)
	{
		ASBCharacter* Target = *It;
		if (!Target || Target == this || Target->Health <= 0.f)
		{
			continue;
		}
		if (FVector::DistSquared(Target->GetActorLocation(), GetActorLocation()) > Radius * Radius)
		{
			continue;
		}

		const ASBPlayerState* MyPS = GetPlayerState<ASBPlayerState>();
		const ASBPlayerState* TheirPS = Target->GetPlayerState<ASBPlayerState>();
		if (MyPS && TheirPS && USBRulesLibrary::IsFriendlyFire(MyPS->GetTeam(), TheirPS->GetTeam()))
		{
			continue;
		}

		Target->LastDamagePowerId = FName(TEXT("SpellShockwave"));
		Target->TakeDamage(SpellDamage, FDamageEvent(), GetController(), this);
	}
}

void ASBCharacter::PerformSpellReinforce()
{
	const float Radius = 500.f;
	const float Repair = FMath::Max(40.f, StructureDamage * 1.5f);
	DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 14, FColor::Emerald, false, 0.35f, 0, 2.f);

	for (TActorIterator<ASBDestructibleStructure> It(GetWorld()); It; ++It)
	{
		ASBDestructibleStructure* Structure = *It;
		if (!Structure)
		{
			continue;
		}
		if (FVector::DistSquared(Structure->GetActorLocation(), GetActorLocation()) > Radius * Radius)
		{
			continue;
		}
		Structure->Repair(Repair);
	}

	// Brief self-ward: small heal if no structures nearby.
	Health = FMath::Min(MaxHealth, Health + 12.f);
	OnRep_Health();
}

void ASBCharacter::PerformMeleeSwing()
{
	MulticastMeleeSwing();

	FVector CamLoc;
	FRotator CamRot;
	if (AController* C = GetController())
	{
		C->GetPlayerViewPoint(CamLoc, CamRot);
	}
	else
	{
		CamLoc = GetActorLocation() + FVector(0.f, 0.f, 60.f);
		CamRot = GetActorRotation();
	}

	const FVector Forward = CamRot.Vector();
	const FVector Start = CamLoc;
	const FVector End = CamLoc + Forward * AttackRange;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SBMelee), false, this);
	Params.AddIgnoredActor(this);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(MeleeSweepRadius),
		Params);

	DrawDebugSphere(GetWorld(), End, MeleeSweepRadius, 10, FColor::Silver, false, 0.2f, 0, 1.5f);

	if (!bHit)
	{
		UE_LOG(LogShadowbaneCombat, Verbose, TEXT("%s melee swing miss"), *GetName());
		return;
	}

	TSet<AActor*> Damaged;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Actor = Hit.GetActor();
		if (!Actor || Damaged.Contains(Actor))
		{
			continue;
		}
		Damaged.Add(Actor);

		if (ASBDestructibleStructure* Structure = Cast<ASBDestructibleStructure>(Actor))
		{
			Structure->ApplyStructureDamage(StructureDamage, GetController(),
				Archetype ? Archetype->ArchetypeId : NAME_None,
				FName(TEXT("BladeSwing_Structure")));
			continue;
		}

		if (ASBCharacter* Target = Cast<ASBCharacter>(Actor))
		{
			const ASBPlayerState* MyPS = GetPlayerState<ASBPlayerState>();
			const ASBPlayerState* TheirPS = Target->GetPlayerState<ASBPlayerState>();
			if (MyPS && TheirPS && USBRulesLibrary::IsFriendlyFire(MyPS->GetTeam(), TheirPS->GetTeam()))
			{
				continue;
			}

			Target->LastDamagePowerId = FName(TEXT("BladeSwing"));
			Target->TakeDamage(AttackDamage, FDamageEvent(), GetController(), this);
			UE_LOG(LogShadowbaneCombat, Log, TEXT("%s BladeSwing hit %s for %.0f"),
				*GetName(), *Target->GetName(), AttackDamage);
		}
	}
}

void ASBCharacter::PerformHitscanFire()
{
	FVector CamLoc;
	FRotator CamRot;
	if (AController* C = GetController())
	{
		C->GetPlayerViewPoint(CamLoc, CamRot);
	}
	else
	{
		CamLoc = GetActorLocation() + FVector(0.f, 0.f, 60.f);
		CamRot = GetActorRotation();
	}

	const FVector End = CamLoc + CamRot.Vector() * AttackRange;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SBFire), false, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Camera, Params);

	const FVector TraceEnd = bHit ? Hit.ImpactPoint : End;
	DrawDebugLine(GetWorld(), CamLoc, TraceEnd, FColor::Orange, false, 0.15f, 0, 1.5f);

	if (!bHit || !Hit.GetActor())
	{
		return;
	}

	if (ASBDestructibleStructure* Structure = Cast<ASBDestructibleStructure>(Hit.GetActor()))
	{
		Structure->ApplyStructureDamage(StructureDamage, GetController(),
			Archetype ? Archetype->ArchetypeId : NAME_None,
			FName(TEXT("BasicFire_Structure")));
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

		Target->LastDamagePowerId = FName(TEXT("BasicFire"));
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
				const float Before = Other->Health;
				Other->Health = FMath::Min(Other->MaxHealth, Other->Health + HealPerSecond * DeltaSeconds);
				const float Applied = Other->Health - Before;
				if (Applied > 0.f)
				{
					float& Acc = PendingHealTelemetry.FindOrAdd(Other);
					Acc += Applied;
					if (Acc >= HealTelemetryFlushAmount)
					{
						if (ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>())
						{
							if (USBMatchTelemetry* Telemetry = GM->GetTelemetry())
							{
								FSBCombatMetric Metric;
								Metric.AttackerName = MyPS->GetPlayerName();
								Metric.AttackerArchetype = MyPS->GetSelectedArchetypeId();
								Metric.AttackerTeam = MyPS->GetTeam();
								Metric.VictimName = TheirPS->GetPlayerName();
								Metric.VictimArchetype = TheirPS->GetSelectedArchetypeId();
								Metric.VictimTeam = TheirPS->GetTeam();
								Metric.PowerId = FName(TEXT("HealAura"));
								Metric.Amount = Acc;
								Metric.VictimHealthAfter = Other->Health;
								Telemetry->RecordCombatHeal(Metric);
							}
						}
						Acc = 0.f;
					}
				}
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
	if (!HasAuthority() || bDead || Health <= 0.f || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Applied);

	if (Applied > 0.f)
	{
		CancelRecall(TEXT("damage"));
		CancelSpellCast(TEXT("damage"));
		MulticastShowDamage(Applied, GetActorLocation() + FVector(0.f, 0.f, 70.f), false);

		// Soft knock — strong launches shove bots off the greybox pads.
		if (APawn* InstigatorPawn = EventInstigator ? EventInstigator->GetPawn() : nullptr)
		{
			const FVector Away = (GetActorLocation() - InstigatorPawn->GetActorLocation()).GetSafeNormal2D();
			LaunchCharacter(Away * 90.f + FVector(0.f, 0.f, 40.f), true, true);
		}
	}

	UE_LOG(LogShadowbaneCombat, Verbose, TEXT("%s took %.1f damage (hp %.0f/%.0f)"),
		*GetName(), Applied, Health, MaxHealth);

	if (Applied > 0.f)
	{
		if (ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>())
		{
			if (USBMatchTelemetry* Telemetry = GM->GetTelemetry())
			{
				const ASBPlayerState* VictimPS = GetPlayerState<ASBPlayerState>();
				const ASBPlayerState* KillerPS = EventInstigator
					? EventInstigator->GetPlayerState<ASBPlayerState>()
					: nullptr;

				FSBCombatMetric Metric;
				Metric.AttackerName = KillerPS ? KillerPS->GetPlayerName() : TEXT("none");
				Metric.AttackerArchetype = KillerPS ? KillerPS->GetSelectedArchetypeId() : NAME_None;
				Metric.AttackerTeam = KillerPS ? KillerPS->GetTeam() : ESBTeam::Unassigned;
				Metric.VictimName = VictimPS ? VictimPS->GetPlayerName() : GetName();
				Metric.VictimArchetype = VictimPS ? VictimPS->GetSelectedArchetypeId()
					: (Archetype ? Archetype->ArchetypeId : NAME_None);
				Metric.VictimTeam = VictimPS ? VictimPS->GetTeam() : ESBTeam::Unassigned;
				Metric.PowerId = LastDamagePowerId.IsNone() ? FName(TEXT("Unknown")) : LastDamagePowerId;
				Metric.Amount = Applied;
				Metric.VictimHealthAfter = Health;
				Metric.bLethal = Health <= 0.f;
				Telemetry->RecordCombatDamage(Metric);
			}
		}
	}

	if (Health <= 0.f)
	{
		Die(EventInstigator);
	}

	return Applied;
}

void ASBCharacter::TickVitals(float DeltaSeconds)
{
	if (bDead || Health <= 0.f)
	{
		return;
	}

	if (!bCasting)
	{
		Mana = FMath::Min(MaxMana, Mana + ManaRegenPerSecond * DeltaSeconds);
	}
	Mana = FMath::Clamp(Mana, 0.f, MaxMana);

	Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * DeltaSeconds);
	Stamina = FMath::Clamp(Stamina, 0.f, MaxStamina);
}

void ASBCharacter::Die(AController* KillerController)
{
	if (!HasAuthority() || bDead)
	{
		return;
	}

	bDead = true;
	Health = 0.f;
	CancelRecall(TEXT("death"));
	CancelSpellCast(TEXT("death"));

	ASBPlayerState* VictimPS = GetPlayerState<ASBPlayerState>();
	ASBPlayerState* KillerPS = KillerController ? KillerController->GetPlayerState<ASBPlayerState>() : nullptr;

	UE_LOG(LogShadowbane, Log, TEXT("%s died (archetype=%s)"),
		*GetName(),
		Archetype ? *Archetype->ArchetypeId.ToString() : TEXT("none"));

	if (ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>())
	{
		GM->NotifyPlayerKilled(VictimPS, KillerPS, LastDamagePowerId);
	}

	MulticastDeathPose();

	// Leave corpse on the field; controller unpossesses so respawn can create a new pawn.
	AController* Ctrl = GetController();
	if (Ctrl)
	{
		Ctrl->UnPossess();
	}

	SetLifeSpan(CorpseLifeSeconds);
}

void ASBCharacter::MulticastDeathPose_Implementation()
{
	ApplyDeathPose();
}

void ASBCharacter::ApplyDeathPose()
{
	bDead = true;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->DisableMovement();
		Move->StopMovementImmediately();
	}
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Lie on the ground (yaw preserved, pitch rolls body onto its side/back).
	const FRotator Flat(0.f, GetActorRotation().Yaw, 90.f);
	SetActorRotation(Flat);

	if (USkeletalMeshComponent* Hero = GetMesh())
	{
		Hero->bPauseAnims = true;
		Hero->SetRelativeRotation(HeroMeshBaseRelativeRot + FRotator(-80.f, 0.f, 15.f));
	}
}

void ASBCharacter::ClearDeathPose()
{
	bDead = false;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (USkeletalMeshComponent* Hero = GetMesh())
	{
		Hero->bPauseAnims = false;
		Hero->SetRelativeRotation(HeroMeshBaseRelativeRot);
	}
}

void ASBCharacter::OnRep_Dead()
{
	if (bDead)
	{
		ApplyDeathPose();
	}
	else
	{
		ClearDeathPose();
	}
}

void ASBCharacter::OnRep_Health()
{
	// Hook for damage feedback / HUD pulse.
}

void ASBCharacter::OnRep_Archetype()
{
	UpdatePlaceholderVisuals();
	UpdateWeaponVisibility();
}

void ASBCharacter::OnRep_RaceName()
{
	UpdatePlaceholderVisuals();
}

void ASBCharacter::OnRep_UsesMelee()
{
	UpdateWeaponVisibility();
}

void ASBCharacter::OnRep_Recalling()
{
	// MulticastRecallEvent handles local feedback; replication drives peer FX via Tick pulse.
}

void ASBCharacter::ServerStartRecall_Implementation()
{
	StartRecall();
}

void ASBCharacter::StartRecall()
{
	if (!HasAuthority() || Health <= 0.f || bRecalling)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastRecallCompleteTime < RecallCooldown)
	{
		return;
	}

	bRecalling = true;
	RecallTimeRemaining = RecallChannelDuration;
	MulticastRecallEvent(true, false);
	UE_LOG(LogShadowbane, Log, TEXT("%s started recall (%.1fs)"), *GetName(), RecallChannelDuration);
}

void ASBCharacter::CancelRecall(const TCHAR* Reason)
{
	if (!HasAuthority() || !bRecalling)
	{
		return;
	}

	bRecalling = false;
	RecallTimeRemaining = 0.f;
	MulticastRecallEvent(false, false);
	UE_LOG(LogShadowbane, Verbose, TEXT("%s recall cancelled (%s)"), *GetName(), Reason ? Reason : TEXT("?"));
}

void ASBCharacter::CompleteRecall()
{
	if (!HasAuthority())
	{
		return;
	}

	bRecalling = false;
	RecallTimeRemaining = 0.f;
	LastRecallCompleteTime = GetWorld()->GetTimeSeconds();

	ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>();
	FVector Loc = GetActorLocation();
	FRotator Rot = GetActorRotation();
	if (GM && PS)
	{
		GM->GetSpawnTransformFor(PS, Loc, Rot);
	}

	MulticastRecallEvent(false, true);

	SetActorLocationAndRotation(Loc, Rot, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = GetController())
	{
		C->SetControlRotation(Rot);
	}

	Health = MaxHealth;
	OnRep_Health();

	UE_LOG(LogShadowbane, Log, TEXT("%s recalled to %s"), *GetName(), *Loc.ToCompactString());
}

void ASBCharacter::TickRecall(float DeltaSeconds)
{
	if (!bRecalling)
	{
		return;
	}

	if (GetVelocity().Size2D() > RecallMoveCancelSpeed)
	{
		CancelRecall(TEXT("velocity"));
		return;
	}

	RecallTimeRemaining -= DeltaSeconds;
	if (RecallTimeRemaining <= 0.f)
	{
		CompleteRecall();
	}
}

void ASBCharacter::MulticastRecallEvent_Implementation(bool bStarted, bool bCompleted)
{
	if (bCompleted)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), 90.f, 16, FColor::Cyan, false, 0.6f, 0, 4.f);
		if (IsLocallyControlled())
		{
			FSBClientDebug::PushMessage(TEXT("Recalled to base"), 3.f);
		}
		return;
	}

	if (bStarted)
	{
		if (IsLocallyControlled())
		{
			FSBClientDebug::PushMessage(TEXT("Recalling… stand still"), 2.5f);
		}
	}
	else if (IsLocallyControlled())
	{
		FSBClientDebug::PushMessage(TEXT("Recall interrupted"), 2.f);
	}
}

void ASBCharacter::UpdateWeaponVisibility()
{
	const bool bShowMelee = bUsesMelee && !bBowEquipped;
	const bool bHasImportedAxe = TpWeaponMesh && TpWeaponMesh->GetStaticMesh()
		&& !TpWeaponMesh->GetStaticMesh()->GetPathName().Contains(TEXT("BasicShapes"));

	if (WeaponMesh && AxeBladeMesh)
	{
		WeaponMesh->SetOwnerNoSee(true);
		AxeBladeMesh->SetOwnerNoSee(true);
		WeaponMesh->SetHiddenInGame(true);
		AxeBladeMesh->SetHiddenInGame(true);
	}

	if (TpWeaponMesh)
	{
		TpWeaponMesh->SetOwnerNoSee(false);
		TpWeaponMesh->SetHiddenInGame(!bShowMelee);
		TpWeaponMesh->SetVisibility(bShowMelee, true);
		TpWeaponMesh->SetCastShadow(true);
	}

	// Hide cube blade / procedural when a real axe mesh is loaded.
	if (TpAxeBladeMesh)
	{
		const bool bShowBlade = bShowMelee && !bHasImportedAxe;
		TpAxeBladeMesh->SetHiddenInGame(!bShowBlade);
		TpAxeBladeMesh->SetVisibility(bShowBlade, true);
	}
	if (TpBattleAxe)
	{
		TpBattleAxe->SetHiddenInGame(true);
		TpBattleAxe->SetVisibility(false, true);
	}

	if (bShowMelee)
	{
		EnsureWeaponInHand();
	}
}

void ASBCharacter::EnsureWeaponInHand()
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero || !TpWeaponPivot)
	{
		return;
	}

	static const FName HandSockets[] = {
		FName(TEXT("weapon_r")),
		FName(TEXT("hand_r")),
		FName(TEXT("Hand_R")),
		FName(TEXT("ik_hand_gun")),
		FName(TEXT("hand_r_socket"))
	};

	bool bAttached = false;
	for (const FName& Socket : HandSockets)
	{
		if (Hero->DoesSocketExist(Socket))
		{
			if (TpWeaponPivot->GetAttachParent() != Hero || TpWeaponPivot->GetAttachSocketName() != Socket)
			{
				TpWeaponPivot->AttachToComponent(Hero, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
			}
			// Grip pose: haft along palm, blade out.
			TpWeaponPivot->SetRelativeLocation(FVector(-2.f, 8.f, 3.f));
			TpWeaponPivot->SetRelativeRotation(FRotator(0.f, 90.f, 10.f));
			bAttached = true;
			break;
		}
	}

	if (!bAttached)
	{
		TpWeaponPivot->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		TpWeaponPivot->SetRelativeLocation(FVector(25.f, 40.f, 35.f));
		TpWeaponPivot->SetRelativeRotation(FRotator(-20.f, 0.f, 15.f));
	}

	if (TpWeaponMesh)
	{
		UStaticMesh* WeaponStatic = TpWeaponMesh->GetStaticMesh();
		const FString Path = WeaponStatic ? WeaponStatic->GetPathName() : FString();
		const bool bImported = WeaponStatic && !Path.Contains(TEXT("BasicShapes"));
		if (bImported && Path.Contains(TEXT("SM_BattleAxe")))
		{
			// Generated battle axe: haft along +Z (~105cm), blade on +X.
			TpWeaponMesh->SetRelativeLocation(FVector(0.f, 0.f, -8.f));
			TpWeaponMesh->SetRelativeRotation(FRotator(0.f, 0.f, -10.f));
			TpWeaponMesh->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.9f));
		}
		else if (bImported)
		{
			// Kenney handaxe: grip near origin — scale up from ~1uu source.
			TpWeaponMesh->SetRelativeLocation(FVector(2.f, 0.f, 4.f));
			TpWeaponMesh->SetRelativeRotation(FRotator(-90.f, 0.f, 90.f));
			TpWeaponMesh->SetRelativeScale3D(FVector(18.f, 18.f, 18.f));
		}
		else
		{
			TpWeaponMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
			TpWeaponMesh->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
			TpWeaponMesh->SetRelativeScale3D(FVector(0.14f, 0.14f, 1.15f));
		}
		TpWeaponMesh->SetCastShadow(true);
	}
	if (TpAxeBladeMesh)
	{
		TpAxeBladeMesh->SetRelativeLocation(FVector(0.f, 8.f, 48.f));
		TpAxeBladeMesh->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		TpAxeBladeMesh->SetRelativeScale3D(FVector(0.95f, 0.12f, 0.55f));
		TpAxeBladeMesh->SetCastShadow(true);
	}
	if (TpBattleAxe)
	{
		TpBattleAxe->SetHiddenInGame(true);
	}
}

void ASBCharacter::PlayMeleeAttackAnimation()
{
	EnsureMeleeSwingAnimAssets();
	bMeleeMontagePlaying = false;

	USkeletalMeshComponent* Hero = GetMesh();
	UAnimInstance* Anim = Hero ? Hero->GetAnimInstance() : nullptr;
	if (Anim && bUsingHeroMesh)
	{
		if (CachedMeleeMontage)
		{
			const float Len = Anim->Montage_Play(CachedMeleeMontage, 1.f);
			if (Len > 0.f)
			{
				bMeleeMontagePlaying = true;
				MeleeSwingAnimDuration = Len;
				MeleeSwingAnimRemaining = Len;
				UE_LOG(LogShadowbaneCombat, Verbose, TEXT("%s melee montage %.2fs"), *GetName(), Len);
				return;
			}
		}

		if (CachedMeleeSwingSequence)
		{
			UAnimMontage* Dyn = Anim->PlaySlotAnimationAsDynamicMontage(
				CachedMeleeSwingSequence,
				FName(TEXT("DefaultSlot")),
				0.05f,
				0.12f,
				1.f,
				1);
			if (Dyn)
			{
				bMeleeMontagePlaying = true;
				const float Len = Dyn->GetPlayLength();
				if (Len > KINDA_SMALL_NUMBER)
				{
					MeleeSwingAnimDuration = Len;
					MeleeSwingAnimRemaining = Len;
				}
				UE_LOG(LogShadowbaneCombat, Verbose, TEXT("%s melee slot sequence %.2fs"), *GetName(), Len);
				return;
			}
		}
	}

	// No montage asset — procedural skeletal arm overlay + light torso lean.
	ApplyMeleeBodyPose(0.f);
}

void ASBCharacter::EnsureMeleeSwingAnimAssets()
{
	if (!CachedMeleeMontage)
	{
		CachedMeleeMontage = LoadObject<UAnimMontage>(nullptr, SBMeleeSwingAnim::MeleeMontagePath);
	}
	if (!CachedMeleeSwingSequence)
	{
		CachedMeleeSwingSequence = LoadObject<UAnimSequence>(nullptr, SBMeleeSwingAnim::MeleeSequencePath);
	}
}

void ASBCharacter::BindHeroBoneSwingOverlay()
{
	UnbindHeroBoneSwingOverlay();
	if (USkeletalMeshComponent* Hero = GetMesh())
	{
		HeroBonesFinalizedHandle = Hero->RegisterOnBoneTransformsFinalizedDelegate(
			FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &ASBCharacter::OnHeroBonesFinalized));
	}
}

void ASBCharacter::UnbindHeroBoneSwingOverlay()
{
	if (HeroBonesFinalizedHandle.IsValid())
	{
		if (USkeletalMeshComponent* Hero = GetMesh())
		{
			Hero->UnregisterOnBoneTransformsFinalizedDelegate(HeroBonesFinalizedHandle);
		}
		HeroBonesFinalizedHandle.Reset();
	}
}

void ASBCharacter::OnHeroBonesFinalized()
{
	if (bMeleeMontagePlaying || MeleeSwingAnimRemaining <= 0.f || !bUsingHeroMesh)
	{
		return;
	}

	const float Alpha = 1.f - (MeleeSwingAnimRemaining / FMath::Max(MeleeSwingAnimDuration, KINDA_SMALL_NUMBER));
	ApplyProceduralMeleeArmBones(Alpha);
}

void ASBCharacter::ApplyProceduralMeleeArmBones(float Alpha01)
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero || !Hero->GetSkeletalMeshAsset())
	{
		return;
	}

	TArray<SBMeleeSwingAnim::FBoneDelta, TInlineAllocator<12>> Deltas;
	SBMeleeSwingAnim::EvalSkeletalBoneDeltas(Alpha01, Deltas);
	if (Deltas.Num() == 0)
	{
		return;
	}

	TArray<FTransform>& CS = Hero->GetEditableComponentSpaceTransforms();
	const int32 NumBones = CS.Num();
	if (NumBones <= 0)
	{
		return;
	}

	// Apply each delta in component space, then rotate all descendants by the same
	// world delta around the bone origin so the limb stays connected.
	for (const SBMeleeSwingAnim::FBoneDelta& Delta : Deltas)
	{
		const int32 BoneIdx = Hero->GetBoneIndex(Delta.Bone);
		if (BoneIdx == INDEX_NONE || !CS.IsValidIndex(BoneIdx))
		{
			continue;
		}

		const FQuat AddQ = Delta.Euler.Quaternion();
		const FVector Pivot = CS[BoneIdx].GetLocation();
		const FQuat OldQ = CS[BoneIdx].GetRotation();
		CS[BoneIdx].SetRotation(AddQ * OldQ);
		CS[BoneIdx].NormalizeRotation();

		for (int32 ChildIdx = BoneIdx + 1; ChildIdx < NumBones; ++ChildIdx)
		{
			if (Hero->GetParentBone(Hero->GetBoneName(ChildIdx)) == NAME_None)
			{
				continue;
			}
			// Only affect bones that list this bone as an ancestor.
			bool bDescendant = false;
			FName Walk = Hero->GetParentBone(Hero->GetBoneName(ChildIdx));
			while (Walk != NAME_None)
			{
				if (Walk == Delta.Bone)
				{
					bDescendant = true;
					break;
				}
				Walk = Hero->GetParentBone(Walk);
			}
			if (!bDescendant || !CS.IsValidIndex(ChildIdx))
			{
				continue;
			}

			const FVector Rel = CS[ChildIdx].GetLocation() - Pivot;
			CS[ChildIdx].SetLocation(Pivot + AddQ.RotateVector(Rel));
			CS[ChildIdx].SetRotation(AddQ * CS[ChildIdx].GetRotation());
			CS[ChildIdx].NormalizeRotation();
		}
	}

	Hero->MarkRenderDynamicDataDirty();
}

void ASBCharacter::ApplyMeleeBodyPose(float Alpha01)
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero || !bUsingHeroMesh || bMeleeMontagePlaying)
	{
		return;
	}

	// Light torso lean — arm bones do the readable chop via montage or procedural overlay.
	const float A = FMath::Clamp(Alpha01, 0.f, 1.f);
	float LeanYaw = 0.f;
	float LeanPitch = 0.f;
	if (A > 0.f && A < 1.f)
	{
		if (A < 0.28f)
		{
			const float U = SBMeleeSwingAnim::Smooth01(A / 0.28f);
			LeanYaw = FMath::Lerp(0.f, -8.f, U);
			LeanPitch = FMath::Lerp(0.f, -3.f, U);
		}
		else if (A < 0.52f)
		{
			const float U = SBMeleeSwingAnim::Smooth01((A - 0.28f) / 0.24f);
			LeanYaw = FMath::Lerp(-8.f, 14.f, U * U);
			LeanPitch = FMath::Lerp(-3.f, 5.f, U);
		}
		else
		{
			const float U = SBMeleeSwingAnim::Smooth01((A - 0.52f) / 0.48f);
			LeanYaw = FMath::Lerp(14.f, 0.f, U);
			LeanPitch = FMath::Lerp(5.f, 0.f, U);
		}
	}

	Hero->SetRelativeRotation(HeroMeshBaseRelativeRot + FRotator(LeanPitch, LeanYaw, 0.f));
}

void ASBCharacter::UpdatePlaceholderVisuals()
{
	SetupHeroMeshForRace();

	const ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	FLinearColor BodyColor = USBPlaceholderArt::TeamColor(PS ? PS->GetTeam() : ESBTeam::Unassigned);

	const FString RaceLower = RaceName.ToLower();
	const bool bNeedRaceExtras = RaceLower.Contains(TEXT("minotaur")) || RaceLower.Contains(TEXT("minot"))
		|| RaceLower.Contains(TEXT("dwarf")) || RaceLower.Contains(TEXT("nightshade"))
		|| RaceLower.Contains(TEXT("elf")) || RaceLower.Contains(TEXT("accipitridae"))
		|| RaceLower.Contains(TEXT("badawian")) || RaceLower.Contains(TEXT("shedim"));

	SBRaceSilhouette::FParts Parts;
	Parts.Body = bUsingHeroMesh ? nullptr : BodyMesh;
	Parts.Head = bUsingHeroMesh ? nullptr : HeadMesh;
	Parts.FeatureL = FeatureL;
	Parts.FeatureR = FeatureR;
	Parts.FeatureExtra = FeatureExtra;
	Parts.Capsule = GetCapsuleComponent();

	SBRaceSilhouette::Apply(RaceName.IsEmpty() ? TEXT("Human") : RaceName, Parts, BodyColor);

	if (bUsingHeroMesh)
	{
		SetPlaceholderBodyVisible(false);
		ApplyHeroRaceMaterials();
		if (USkeletalMeshComponent* Hero = GetMesh())
		{
			const FName HeadSocket = Hero->DoesSocketExist(FName(TEXT("head"))) ? FName(TEXT("head"))
				: (Hero->DoesSocketExist(FName(TEXT("Head"))) ? FName(TEXT("Head")) : NAME_None);
			if (HeadSocket != NAME_None)
			{
				if (FeatureL) { FeatureL->AttachToComponent(Hero, FAttachmentTransformRules::KeepRelativeTransform, HeadSocket); }
				if (FeatureR) { FeatureR->AttachToComponent(Hero, FAttachmentTransformRules::KeepRelativeTransform, HeadSocket); }
				if (FeatureExtra) { FeatureExtra->AttachToComponent(Hero, FAttachmentTransformRules::KeepRelativeTransform, HeadSocket); }

				// Head-local ears/horns (SBRaceSilhouette capsule offsets are wrong on a head socket).
				if (RaceLower.Contains(TEXT("elf")) && !RaceLower.Contains(TEXT("aelf")))
				{
					if (FeatureL)
					{
						FeatureL->SetRelativeLocation(FVector(6.f, -12.f, 4.f));
						FeatureL->SetRelativeRotation(FRotator(10.f, -20.f, -55.f));
						FeatureL->SetRelativeScale3D(FVector(0.08f, 0.22f, 0.4f));
						FeatureL->SetHiddenInGame(false);
					}
					if (FeatureR)
					{
						FeatureR->SetRelativeLocation(FVector(6.f, 12.f, 4.f));
						FeatureR->SetRelativeRotation(FRotator(10.f, 20.f, 55.f));
						FeatureR->SetRelativeScale3D(FVector(0.08f, 0.22f, 0.4f));
						FeatureR->SetHiddenInGame(false);
					}
				}
				else if (RaceLower.Contains(TEXT("aelf")))
				{
					if (FeatureL)
					{
						FeatureL->SetRelativeLocation(FVector(5.f, -10.f, 3.f));
						FeatureL->SetRelativeRotation(FRotator(8.f, -15.f, -45.f));
						FeatureL->SetRelativeScale3D(FVector(0.07f, 0.18f, 0.32f));
						FeatureL->SetHiddenInGame(false);
					}
					if (FeatureR)
					{
						FeatureR->SetRelativeLocation(FVector(5.f, 10.f, 3.f));
						FeatureR->SetRelativeRotation(FRotator(8.f, 15.f, 45.f));
						FeatureR->SetRelativeScale3D(FVector(0.07f, 0.18f, 0.32f));
						FeatureR->SetHiddenInGame(false);
					}
				}
				else if (RaceLower.Contains(TEXT("minotaur")) || RaceLower.Contains(TEXT("minot")))
				{
					if (FeatureL)
					{
						FeatureL->SetRelativeLocation(FVector(4.f, -10.f, 14.f));
						FeatureL->SetRelativeRotation(FRotator(35.f, 0.f, -40.f));
						FeatureL->SetRelativeScale3D(FVector(0.14f, 0.14f, 0.55f));
						FeatureL->SetHiddenInGame(false);
					}
					if (FeatureR)
					{
						FeatureR->SetRelativeLocation(FVector(4.f, 10.f, 14.f));
						FeatureR->SetRelativeRotation(FRotator(35.f, 0.f, 40.f));
						FeatureR->SetRelativeScale3D(FVector(0.14f, 0.14f, 0.55f));
						FeatureR->SetHiddenInGame(false);
					}
				}
				else if (RaceLower.Contains(TEXT("nightshade")))
				{
					if (FeatureExtra)
					{
						FeatureExtra->SetRelativeLocation(FVector(-2.f, 0.f, 8.f));
						FeatureExtra->SetRelativeRotation(FRotator(180.f, 0.f, 0.f));
						FeatureExtra->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.4f));
						FeatureExtra->SetHiddenInGame(false);
					}
				}
				else if (RaceLower.Contains(TEXT("dwarf")))
				{
					if (FeatureExtra)
					{
						FeatureExtra->SetRelativeLocation(FVector(8.f, 0.f, -6.f));
						FeatureExtra->SetRelativeRotation(FRotator::ZeroRotator);
						FeatureExtra->SetRelativeScale3D(FVector(0.28f, 0.4f, 0.3f));
						FeatureExtra->SetHiddenInGame(false);
					}
				}
			}
		}
		if (!bNeedRaceExtras)
		{
			if (FeatureL) { FeatureL->SetHiddenInGame(true); }
			if (FeatureR) { FeatureR->SetHiddenInGame(true); }
			if (FeatureExtra) { FeatureExtra->SetHiddenInGame(true); }
		}
	}
	else
	{
		USBPlaceholderArt::ApplySolidColor(BodyMesh, BodyColor);
		USBPlaceholderArt::ApplySolidColor(HeadMesh, BodyColor * FLinearColor(1.05f, 0.95f, 0.9f));
	}

	USBPlaceholderArt::ApplySolidColor(FeatureL, BodyColor * FLinearColor(0.9f, 0.85f, 0.75f));
	USBPlaceholderArt::ApplySolidColor(FeatureR, BodyColor * FLinearColor(0.9f, 0.85f, 0.75f));
	USBPlaceholderArt::ApplySolidColor(FeatureExtra, BodyColor * FLinearColor(0.8f, 0.75f, 0.7f));

	const FSBPlaceholderIcon Icon = USBPlaceholderArt::MakeIcon(Archetype);
	USBPlaceholderArt::ApplySolidColor(RuneDisc, Icon.Tint);

	if (RuneDisc)
	{
		const float TopZ = GetCapsuleComponent() ? (GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 34.f) : 130.f;
		RuneDisc->SetRelativeLocation(FVector(0.f, 0.f, TopZ));
	}

	UpdateWeaponVisibility();
}

void ASBCharacter::ApplyMeleeIdlePose()
{
	bMeleeMontagePlaying = false;
	if (FpWeaponPivot)
	{
		FpWeaponPivot->SetRelativeRotation(SBMeleeSwingAnim::EvalFpPivot(0.f));
	}
	if (TpWeaponPivot)
	{
		// Hand-socket weapon: idle = grip rest (swing anim offsets from here).
		TpWeaponPivot->SetRelativeRotation(FRotator(0.f, 90.f, 10.f));
	}
	ApplyMeleeBodyPose(0.f);
	bSwingTrailValid = false;
}

void ASBCharacter::TickMeleeSwingVisual(float DeltaSeconds)
{
	if (MeleeSwingAnimRemaining <= 0.f)
	{
		return;
	}

	const float Alpha = 1.f - (MeleeSwingAnimRemaining / FMath::Max(MeleeSwingAnimDuration, KINDA_SMALL_NUMBER));
	if (FpWeaponPivot)
	{
		FpWeaponPivot->SetRelativeRotation(SBMeleeSwingAnim::EvalFpPivot(Alpha));
	}
	if (TpWeaponPivot)
	{
		// Full chop in the hand — do not damp yaw or the swing reads as static.
		const FRotator Chop = SBMeleeSwingAnim::EvalTpPivot(Alpha);
		TpWeaponPivot->SetRelativeRotation(FRotator(Chop.Pitch, 90.f + Chop.Yaw, Chop.Roll + 10.f));
	}
	ApplyMeleeBodyPose(Alpha);

	MeleeSwingAnimRemaining = FMath::Max(0.f, MeleeSwingAnimRemaining - DeltaSeconds);
	if (MeleeSwingAnimRemaining <= 0.f)
	{
		ApplyMeleeIdlePose();
	}
}

void ASBCharacter::SetPlaceholderBodyVisible(bool bVisible)
{
	if (BodyMesh) { BodyMesh->SetHiddenInGame(!bVisible); }
	if (HeadMesh) { HeadMesh->SetHiddenInGame(!bVisible); }
}

void ASBCharacter::SetupHeroMeshForRace()
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero)
	{
		return;
	}

	const FString Race = RaceName.ToLower();
	const bool bPreferQuinn = Race.Contains(TEXT("elf")); // Elf + High Elf

	USkeletalMesh* MeshAsset = nullptr;
	UClass* AnimClass = nullptr;
	if (bPreferQuinn)
	{
		MeshAsset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn.SKM_Quinn"));
		AnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C"));
	}
	if (!MeshAsset)
	{
		MeshAsset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
		AnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));
	}

	if (MeshAsset)
	{
		Hero->SetSkeletalMesh(MeshAsset);
		bUsingHeroMesh = true;
		SetPlaceholderBodyVisible(false);
	}
	if (AnimClass)
	{
		Hero->SetAnimInstanceClass(AnimClass);
	}

	Hero->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	HeroMeshBaseRelativeRot = FRotator(0.f, -90.f, 0.f);
	Hero->SetRelativeRotation(HeroMeshBaseRelativeRot);
	Hero->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Scale tweaks by race silhouette.

	// Race scale / silhouette extras on top of the humanoid.
	FVector MeshScale(1.f, 1.f, 1.f);
	if (Race.Contains(TEXT("dwarf")))
	{
		MeshScale = FVector(1.05f, 1.05f, 0.78f);
	}
	else if (Race.Contains(TEXT("minotaur")) || Race.Contains(TEXT("minot")))
	{
		MeshScale = FVector(1.2f, 1.15f, 1.12f);
	}
	else if (Race.Contains(TEXT("shedim")))
	{
		MeshScale = FVector(1.15f, 1.1f, 1.15f);
	}
	else if (Race.Contains(TEXT("high elf")))
	{
		MeshScale = FVector(0.98f, 0.98f, 1.02f);
	}
	else if (Race.Contains(TEXT("elf")))
	{
		MeshScale = FVector(0.95f, 0.95f, 1.06f);
	}
	Hero->SetRelativeScale3D(MeshScale);

	ApplyHeroRaceMaterials();
	EnsureWeaponInHand();
	ApplyMeleeIdlePose();
	EnsureMeleeSwingAnimAssets();
	BindHeroBoneSwingOverlay();
}

void ASBCharacter::ApplyHeroRaceMaterials()
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero || !bUsingHeroMesh)
	{
		return;
	}

	// Chrome MI_Manny stays metallic if we only MID the existing parent — replace slots entirely.
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BaseMat)
	{
		BaseMat = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	}
	if (!BaseMat)
	{
		return;
	}

	const FString Race = RaceName.ToLower();
	FLinearColor Skin(0.78f, 0.58f, 0.45f); // Human warm
	if (Race.Contains(TEXT("high elf")))
	{
		Skin = FLinearColor(0.86f, 0.74f, 0.42f); // pale gold (High Elf)
	}
	else if (Race.Contains(TEXT("elf")))
	{
		Skin = FLinearColor(0.9f, 0.82f, 0.72f); // fair elven
	}
	else if (Race.Contains(TEXT("nightshade")))
	{
		Skin = FLinearColor(0.52f, 0.55f, 0.58f); // corpse-grey
	}
	else if (Race.Contains(TEXT("badawian")))
	{
		Skin = FLinearColor(0.72f, 0.38f, 0.28f); // desert heat
	}
	else if (Race.Contains(TEXT("shedim")))
	{
		Skin = FLinearColor(0.45f, 0.4f, 0.55f); // ashen violet
	}
	else if (Race.Contains(TEXT("accipitridae")))
	{
		Skin = FLinearColor(0.75f, 0.7f, 0.55f); // sandy feathered
	}
	else if (Race.Contains(TEXT("dwarf")))
	{
		Skin = FLinearColor(0.7f, 0.5f, 0.38f); // ruddy
	}
	else if (Race.Contains(TEXT("minotaur")) || Race.Contains(TEXT("minot")))
	{
		Skin = FLinearColor(0.42f, 0.28f, 0.18f); // hide / fur brown
	}

	// Light team wash so attackers/defenders stay readable without chrome.
	if (const ASBPlayerState* PS = GetPlayerState<ASBPlayerState>())
	{
		const FLinearColor Team = USBPlaceholderArt::TeamColor(PS->GetTeam());
		Skin = FMath::Lerp(Skin, Team, 0.12f);
	}

	Hero->EmptyOverrideMaterials();
	Hero->SetOverlayMaterial(nullptr);

	const int32 NumMats = FMath::Max(Hero->GetNumMaterials(), 4);
	for (int32 Idx = 0; Idx < NumMats; ++Idx)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (!MID)
		{
			continue;
		}
		MID->SetVectorParameterValue(TEXT("Color"), Skin);
		MID->SetVectorParameterValue(TEXT("BaseColor"), Skin);
		MID->SetVectorParameterValue(TEXT("Tint"), Skin);
		MID->SetScalarParameterValue(TEXT("Metallic"), 0.f);
		MID->SetScalarParameterValue(TEXT("Roughness"), 0.85f);
		Hero->SetMaterial(Idx, MID);
	}
}

void ASBCharacter::TickCastInvokeVisual(float DeltaSeconds)
{
	if (!HandL || !HandR)
	{
		return;
	}

	if (CastFlashRemaining <= 0.f)
	{
		HandL->SetHiddenInGame(true);
		HandR->SetHiddenInGame(true);
		return;
	}

	const float Alpha = 1.f - (CastFlashRemaining / FMath::Max(CastInvokeDuration, KINDA_SMALL_NUMBER));
	const float Reach = 0.55f + 0.45f * FMath::Sin(Alpha * PI);
	const float Forward = 35.f + 75.f * Reach;
	const float Up = 55.f + 45.f * Reach;
	const FVector HandScale(0.28f + 0.12f * Reach);

	HandL->SetHiddenInGame(false);
	HandR->SetHiddenInGame(false);
	USBPlaceholderArt::ApplySolidColor(HandL, FLinearColor(0.95f, 0.85f, 0.55f));
	USBPlaceholderArt::ApplySolidColor(HandR, FLinearColor(0.95f, 0.85f, 0.55f));
	HandL->SetRelativeLocation(FVector(Forward, -38.f - 28.f * Reach, Up));
	HandR->SetRelativeLocation(FVector(Forward, 38.f + 28.f * Reach, Up));
	HandL->SetRelativeRotation(FRotator(-25.f * Reach, 0.f, -22.f));
	HandR->SetRelativeRotation(FRotator(-25.f * Reach, 0.f, 22.f));
	HandL->SetRelativeScale3D(HandScale);
	HandR->SetRelativeScale3D(HandScale);
}

void ASBCharacter::OnFireReleased()
{
	// Bow draw-release lands in a follow-up; melee ignores release.
}

void ASBCharacter::OnToggleBowPressed()
{
	if (bUsesMelee)
	{
		ServerToggleBow();
	}
}

void ASBCharacter::ServerFireBow_Implementation(float Charge01)
{
	PerformBowShot(Charge01);
}

void ASBCharacter::ServerToggleBow_Implementation()
{
	if (!bUsesMelee)
	{
		return;
	}
	bBowEquipped = !bBowEquipped;
	OnRep_BowEquipped();
}

void ASBCharacter::ServerCastChargedFireball_Implementation(float Charge01)
{
	PerformSpellBoltCharged(Charge01);
}

void ASBCharacter::MulticastBowShot_Implementation(float Charge01)
{
	(void)Charge01;
	if (IsLocallyControlled())
	{
		FSBClientDebug::PushMessage(TEXT("Arrow loosed"), 1.2f);
	}
}

void ASBCharacter::BeginFireballAim()
{
	bAimingFireball = true;
	AimCharge01 = 0.f;
	SetFirstPersonAim(true);
}

void ASBCharacter::ReleaseFireballAim()
{
	if (!bAimingFireball)
	{
		return;
	}
	const float Charge = AimCharge01;
	bAimingFireball = false;
	SetFirstPersonAim(false);
	ServerCastChargedFireball(Charge);
	AimCharge01 = 0.f;
}

void ASBCharacter::PerformBowShot(float Charge01)
{
	const float Scale = FMath::Lerp(0.55f, 1.35f, FMath::Clamp(Charge01, 0.f, 1.f));
	const float SavedDamage = AttackDamage;
	AttackDamage *= Scale;
	PerformHitscanFire();
	AttackDamage = SavedDamage;
	MulticastBowShot(Charge01);
}

void ASBCharacter::PerformSpellBoltCharged(float Charge01)
{
	// Charged bolt still uses the stand-still cast channel.
	(void)Charge01;
	BeginSpellCast(0);
}

void ASBCharacter::SetFirstPersonAim(bool bAim)
{
	bFirstPersonAim = bAim;
	if (SpringArm)
	{
		SpringArm->TargetArmLength = bAim ? AimSpringArmLength : DefaultSpringArmLength;
		SpringArm->SocketOffset = bAim ? FVector(0.f, 0.f, 55.f) : FVector::ZeroVector;
	}
	if (USkeletalMeshComponent* Hero = GetMesh())
	{
		Hero->SetOwnerNoSee(bAim);
	}
}

void ASBCharacter::TickAimCharge(float DeltaSeconds)
{
	if (bDrawingBow)
	{
		AimCharge01 = FMath::Clamp(AimCharge01 + DeltaSeconds / FMath::Max(BowDrawSeconds, 0.1f), 0.f, 1.f);
	}
	else if (bAimingFireball)
	{
		AimCharge01 = FMath::Clamp(AimCharge01 + DeltaSeconds / FMath::Max(FireballChargeSeconds, 0.1f), 0.f, 1.f);
	}
	UpdateFpAimVisuals();
}

void ASBCharacter::UpdateFpAimVisuals()
{
	// Placeholder — FP bow/fireball poses fill in with the full aim pass.
}

void ASBCharacter::CancelAimModes(const TCHAR* Reason)
{
	(void)Reason;
	bDrawingBow = false;
	bAimingFireball = false;
	AimCharge01 = 0.f;
	SetFirstPersonAim(false);
}

void ASBCharacter::OnRep_BowEquipped()
{
	UpdateWeaponVisibility();
	if (IsLocallyControlled())
	{
		FSBClientDebug::PushMessage(bBowEquipped ? TEXT("Bow ready") : TEXT("Axe ready"), 1.5f);
	}
}
