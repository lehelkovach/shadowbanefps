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
#include "Art/SBHeroSkinPaths.h"
#include "Art/SBBattleAxeMesh.h"
#include "HAL/IConsoleManager.h"
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

namespace
{
	TAutoConsoleVariable<int32> CVarHeroUseCountess(
		TEXT("sb.Hero.UseCountess"),
		1,
		TEXT("1 = try Paragon Countess soft paths for mapped races when assets exist; 0 = always mannequin."),
		ECVF_Default);

	TAutoConsoleVariable<FString> CVarHeroCountessRaces(
		TEXT("sb.Hero.CountessRaces"),
		TEXT("nightshades,countess"),
		TEXT("Comma-separated race substrings that load Countess (case-insensitive)."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarHeroUseShadowKight(
		TEXT("sb.Hero.UseShadowKight"),
		0,
		TEXT("0 = Manny/Quinn + AM_MM_GreystoneSwing_* (fallback AxeSwing). 1 = ShadowKight LibSwing for mapped races."),
		ECVF_Default);

	TAutoConsoleVariable<FString> CVarHeroShadowKightRaces(
		TEXT("sb.Hero.ShadowKightRaces"),
		TEXT("human,shadowkight,shadowknight"),
		TEXT("Comma-separated race substrings that load ShadowKight (case-insensitive). Default: Human dogfood."),
		ECVF_Default);

	// Live sword grip tuning. Tick applies Relative* only (no re-Attach).
	// PIE: sb.Sword.LocX 10 | sb.Sword.RotY -90 | sb.Sword.ChopScale 0.2 etc.
	//
	// Attach: TpSwordMesh DIRECTLY on hero hand_r / weapon_r (NOT TpWeaponPivot).
	// SwordLODS: blade along +Y (~143cm), grip/pommel at origin (Y≈0).
	// Manny hand_r: +X toward fingers — Yaw -90 maps mesh +Y → hand +X (tip out of palm).
	// LocX ~8 seats hilt from wrist bone into palm center (not knuckles / not floating).
	// Greystone body montage already carries the hand — keep ChopScale 0 so relative
	// grip stays planted; torso lean still uses sb.Melee.ProceduralChop.
	TAutoConsoleVariable<float> CVarSwordLocX(
		TEXT("sb.Sword.LocX"), 8.f,
		TEXT("Sword mesh relative Loc X (cm) on hand. Wrist→palm along fingers."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwordLocY(
		TEXT("sb.Sword.LocY"), 1.f,
		TEXT("Sword mesh relative Loc Y (cm) on hand. Lateral (thumb/pinky)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwordLocZ(
		TEXT("sb.Sword.LocZ"), -2.f,
		TEXT("Sword mesh relative Loc Z (cm) on hand. Into palm (away from knuckles)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwordRotP(
		TEXT("sb.Sword.RotP"), 0.f,
		TEXT("Sword mesh relative Pitch (degrees)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwordRotY(
		TEXT("sb.Sword.RotY"), -90.f,
		TEXT("Sword mesh relative Yaw (degrees). -90 = SwordLODS +Y tip along hand +X."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwordRotR(
		TEXT("sb.Sword.RotR"), 0.f,
		TEXT("Sword mesh relative Roll (degrees). Try ±90 if blade edge faces wrong."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwordScale(
		TEXT("sb.Sword.Scale"), 0.65f,
		TEXT("Sword mesh uniform scale. SwordLODS ~143cm; 0.65 ≈ one-hand readable vs Greystone."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwordChopScale(
		TEXT("sb.Sword.ChopScale"), 0.f,
		TEXT("Scales EvalTpSwordChop added to grip Rot (0=Greystone owns arc; 0.2–1=extra blade polish)."), ECVF_Default);

	/** Procedural sword/torso chop layered on body montage (readable LMB dogfood). */
	TAutoConsoleVariable<int32> CVarMeleeProceduralChop(
		TEXT("sb.Melee.ProceduralChop"),
		1,
		TEXT("1 = procedural pivot + torso lean with body montage (default). 0 = montage-only isolate. Sword relative chop uses sb.Sword.ChopScale."),
		ECVF_Default);

	/** Manny Greystone A→B→C: seconds after montage ends to press (or banked LMB) for next letter; miss → A. */
	TAutoConsoleVariable<float> CVarMeleeComboWindowSec(
		TEXT("sb.Melee.ComboWindowSec"),
		0.5f,
		TEXT("Manny Greystone: after A/B ends, LMB within this many seconds plays B/C. LMB during anim banks one continue. After C or miss → A. RMB cancels swing."),
		ECVF_Default);

	bool IsMeleeProceduralChopEnabled()
	{
		return CVarMeleeProceduralChop.GetValueOnGameThread() != 0;
	}

	float GetMeleeComboWindowSec()
	{
		return FMath::Max(0.05f, CVarMeleeComboWindowSec.GetValueOnGameThread());
	}

	bool IsMannyPreferredSwingName(const FString& N)
	{
		return N.Contains(TEXT("GreystoneSwing"))
			|| N.Contains(TEXT("SteelSwing"))
			|| N.Contains(TEXT("SerathSwing"))
			|| N.Contains(TEXT("AxeSwing"));
	}

	bool RaceMapsToTokens(const FString& RaceLower, const FString& CommaSeparatedTokens)
	{
		TArray<FString> Tokens;
		CommaSeparatedTokens.ParseIntoArray(Tokens, TEXT(","), true);
		for (FString& Token : Tokens)
		{
			Token.TrimStartAndEndInline();
			if (!Token.IsEmpty() && RaceLower.Contains(Token.ToLower()))
			{
				return true;
			}
		}
		return false;
	}

	bool RaceMapsToCountess(const FString& RaceLower)
	{
		if (CVarHeroUseCountess.GetValueOnGameThread() == 0)
		{
			return false;
		}
		return RaceMapsToTokens(RaceLower, CVarHeroCountessRaces.GetValueOnGameThread());
	}

	bool RaceMapsToShadowKight(const FString& RaceLower)
	{
		if (CVarHeroUseShadowKight.GetValueOnGameThread() == 0)
		{
			return false;
		}
		return RaceMapsToTokens(RaceLower, CVarHeroShadowKightRaces.GetValueOnGameThread());
	}
}

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

	TpSwordMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TpSwordMesh"));
	// Temporary parent until EnsureWeaponInHand reparents directly to hand_r.
	TpSwordMesh->SetupAttachment(TpWeaponPivot);
	TpSwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TpSwordMesh->SetGenerateOverlapEvents(false);
	TpSwordMesh->SetSimulatePhysics(false);
	TpSwordMesh->SetEnableGravity(false);
	TpSwordMesh->SetCastShadow(true);
	TpSwordMesh->SetHiddenInGame(true);
	// Palm grip defaults — CVars re-applied each tick (Relative only).
	// SwordLODS +Y blade → hand +X (Yaw -90); LocX seats hilt in palm from wrist.
	TpSwordMesh->SetRelativeLocation(FVector(8.f, 1.f, -2.f));
	TpSwordMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	TpSwordMesh->SetRelativeScale3D(FVector(0.65f));

	TpBattleAxe = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TpBattleAxe"));
	TpBattleAxe->SetupAttachment(TpWeaponPivot);
	TpBattleAxe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TpBattleAxe->SetCastShadow(true);
	TpBattleAxe->SetHiddenInGame(true);

	// Prefer free sword (skeletal); axe static meshes remain as fallback.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FreeSwordMesh(
		TEXT("/Game/Art/Weapons/SwordLODS.SwordLODS"));
	if (FreeSwordMesh.Succeeded())
	{
		TpSwordMesh->SetSkeletalMesh(FreeSwordMesh.Object);
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> FreeSwordMat(
			TEXT("/Game/Art/Weapons/Sword.Sword"));
		if (FreeSwordMat.Succeeded())
		{
			TpSwordMesh->SetMaterial(0, FreeSwordMat.Object);
		}
	}

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
		Hero->bNoSkeletonUpdate = false;

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
	// Procedural CS bone overlay disabled — caused EditableBoneVisibilityStates assert / crash.

	// AnimBP / mesh init can stomp materials on the first frames — re-tint shortly after.
	if (UWorld* World = GetWorld())
	{
		FTimerHandle SkinTimer;
		World->GetTimerManager().SetTimer(SkinTimer, FTimerDelegate::CreateUObject(this, &ASBCharacter::ApplyHeroRaceMaterials), 0.15f, false);
	}
}

void ASBCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindHeroBoneSwingOverlay(); // no-op unless a prior build left a handle
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
		if (MeleeCancelAction)
		{
			EIC->BindAction(MeleeCancelAction, ETriggerEvent::Started, this, &ASBCharacter::OnMeleeCancelPressed);
		}
	}

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ASBCharacter::OnFirePressed);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &ASBCharacter::OnFireReleased);
	PlayerInputComponent->BindAction(TEXT("MeleeCancel"), IE_Pressed, this, &ASBCharacter::OnMeleeCancelPressed);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ASBCharacter::OnMeleeCancelPressed);
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

	// Grip first (base Loc/Rot from CVars), then swing visual layers chop on sword + pivots.
	if (TpSwordMesh && !TpSwordMesh->bHiddenInGame && TpSwordMesh->GetSkeletalMeshAsset())
	{
		ApplySwordGripFromCVars();
	}

	if (WeaponMesh || TpWeaponMesh || TpSwordMesh)
	{
		TickMeleeSwingVisual(DeltaSeconds);
	}

	if (TpSwordMesh && !TpSwordMesh->bHiddenInGame && TpSwordMesh->GetSkeletalMeshAsset())
	{
		UpdateSwordHoldMontage();
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

	MeleeCancelAction = NewObject<UInputAction>(this, TEXT("IA_SB_MeleeCancel"));
	MeleeCancelAction->ValueType = EInputActionValueType::Boolean;

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
	MoveMappingContext->MapKey(MeleeCancelAction, EKeys::RightMouseButton);
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
	// Local predict: never wait on Server→Multicast for the sword to move.
	// Authority still validates stamina/interval in PerformAttack.
	if (IsLocallyControlled() && !bDead && bUsesMelee && !bBowEquipped
		&& Stamina >= MeleeStaminaCost)
	{
		// Bank one LMB during active Greystone swing — defer ServerFire until anim ends.
		if (IsMannyGreystoneComboPath() && IsMeleeSwingMontageActive())
		{
			bMeleeLmbBanked = true;
			UE_LOG(LogTemp, Warning,
				TEXT("%s LMB banked during Greystone swing (combo=%d) — continue on end"),
				*GetName(), MeleeComboIndex);
			return;
		}
		PlayMeleeAttackAnimation();
	}
	ServerFire();
}

void ASBCharacter::OnMeleeCancelPressed()
{
	if (!IsLocallyControlled() || bDead || !bUsesMelee || bBowEquipped)
	{
		return;
	}
	if (!IsMeleeSwingMontageActive() && MeleeSwingAnimRemaining <= 0.f)
	{
		return;
	}
	CancelMeleeSwing(TEXT("RMB"));
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
			UE_LOG(LogShadowbaneCombat, Log, TEXT("%s LMB melee BLOCKED — stamina %.1f < cost %.1f"),
				*GetName(), Stamina, MeleeStaminaCost);
			if (IsLocallyControlled())
			{
				FSBClientDebug::PushMessage(TEXT("Melee blocked — low stamina"), 1.5f);
			}
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
	// PlayMeleeAttackAnimation always StartMeleeSwingVisual (procedural chop) first.
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
		if (MyPS && TheirPS && USBRulesLibrary::IsFriendlyFire(MyPS->GetTeam(), TheirPS->GetTeam(), USBRulesLibrary::IsWorldFreeForAll(this)))
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
		const bool bFFA = USBRulesLibrary::IsWorldFreeForAll(this);
		if (MyPS && TheirPS
			&& !USBRulesLibrary::AreAllies(MyPS->GetTeam(), TheirPS->GetTeam(), bFFA, Ally == this))
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
		if (MyPS && TheirPS && USBRulesLibrary::IsFriendlyFire(MyPS->GetTeam(), TheirPS->GetTeam(), USBRulesLibrary::IsWorldFreeForAll(this)))
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
			if (MyPS && TheirPS && USBRulesLibrary::IsFriendlyFire(MyPS->GetTeam(), TheirPS->GetTeam(), USBRulesLibrary::IsWorldFreeForAll(this)))
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
		if (MyPS && TheirPS && USBRulesLibrary::IsFriendlyFire(MyPS->GetTeam(), TheirPS->GetTeam(), USBRulesLibrary::IsWorldFreeForAll(this)))
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
			const bool bFFA = USBRulesLibrary::IsWorldFreeForAll(this);
			if (!MyPS || !TheirPS
				|| !USBRulesLibrary::AreAllies(MyPS->GetTeam(), TheirPS->GetTeam(), bFFA, false))
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
	const bool bHasSword = TpSwordMesh && TpSwordMesh->GetSkeletalMeshAsset() != nullptr;
	const bool bHasImportedAxe = !bHasSword && TpWeaponMesh && TpWeaponMesh->GetStaticMesh()
		&& !TpWeaponMesh->GetStaticMesh()->GetPathName().Contains(TEXT("BasicShapes"));

	if (WeaponMesh && AxeBladeMesh)
	{
		WeaponMesh->SetOwnerNoSee(true);
		AxeBladeMesh->SetOwnerNoSee(true);
		WeaponMesh->SetHiddenInGame(true);
		AxeBladeMesh->SetHiddenInGame(true);
	}

	if (TpSwordMesh)
	{
		const bool bShowSword = bShowMelee && bHasSword;
		TpSwordMesh->SetOwnerNoSee(false);
		TpSwordMesh->SetHiddenInGame(!bShowSword);
		TpSwordMesh->SetVisibility(bShowSword, true);
		TpSwordMesh->SetCastShadow(true);
	}

	if (TpWeaponMesh)
	{
		const bool bShowAxe = bShowMelee && !bHasSword;
		TpWeaponMesh->SetOwnerNoSee(false);
		TpWeaponMesh->SetHiddenInGame(!bShowAxe);
		TpWeaponMesh->SetVisibility(bShowAxe, true);
		TpWeaponMesh->SetCastShadow(true);
	}

	// Hide cube blade / procedural when a real axe mesh is loaded (or sword preferred).
	if (TpAxeBladeMesh)
	{
		const bool bShowBlade = bShowMelee && !bHasSword && !bHasImportedAxe;
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
	else
	{
		StopSwordHoldMontage();
	}
}

void ASBCharacter::ApplySwordGripFromCVars()
{
	if (!TpSwordMesh || !TpSwordMesh->GetSkeletalMeshAsset())
	{
		return;
	}
	const float LocX = CVarSwordLocX.GetValueOnGameThread();
	const float LocY = CVarSwordLocY.GetValueOnGameThread();
	const float LocZ = CVarSwordLocZ.GetValueOnGameThread();
	const float RotP = CVarSwordRotP.GetValueOnGameThread();
	const float RotY = CVarSwordRotY.GetValueOnGameThread();
	const float RotR = CVarSwordRotR.GetValueOnGameThread();
	const float Scale = CVarSwordScale.GetValueOnGameThread();
	const float ChopScale = CVarSwordChopScale.GetValueOnGameThread();

	// Grip Loc/base Rot always from CVars — never let chop translate the hilt out of palm.
	FRotator GripRot(RotP, RotY, RotR);
	// Optional additive blade polish. Default ChopScale 0: Greystone/Manny body montage
	// owns the front-of-body arc; full EvalTpSwordChop previously warped tip through torso.
	if (ChopScale > KINDA_SMALL_NUMBER
		&& MeleeSwingAnimRemaining > 0.f
		&& (IsMeleeProceduralChopEnabled() || bMeleeEmergencyChop))
	{
		const float Alpha = 1.f - (MeleeSwingAnimRemaining / FMath::Max(MeleeSwingAnimDuration, KINDA_SMALL_NUMBER));
		const FRotator Chop = SBMeleeSwingAnim::EvalTpSwordChop(Alpha);
		GripRot = FRotator(
			GripRot.Pitch + Chop.Pitch * ChopScale,
			GripRot.Yaw + Chop.Yaw * ChopScale,
			GripRot.Roll + Chop.Roll * ChopScale);
	}

	TpSwordMesh->SetRelativeLocation(FVector(LocX, LocY, LocZ));
	TpSwordMesh->SetRelativeRotation(GripRot);
	TpSwordMesh->SetRelativeScale3D(FVector(Scale));
}

void ASBCharacter::EnsureWeaponInHand()
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero)
	{
		return;
	}

	const bool bHasSword = TpSwordMesh && TpSwordMesh->GetSkeletalMeshAsset() != nullptr;

	// --- Sword: TpSwordMesh DIRECTLY on hero hand (identity parent chain) ---
	if (bHasSword)
	{
		// Prefer weapon_r when present (often palm-authored); else hand_r bone.
		static const FName SwordHandSockets[] = {
			FName(TEXT("weapon_r")),
			FName(TEXT("Weapon_R")),
			FName(TEXT("weapon_socket_r")),
			FName(TEXT("hand_r")),
			FName(TEXT("Hand_R")),
			FName(TEXT("RightHand")),
			FName(TEXT("ik_hand_gun")),
			FName(TEXT("hand_r_socket"))
		};

		TpSwordMesh->SetSimulatePhysics(false);
		TpSwordMesh->SetEnableGravity(false);
		TpSwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TpSwordMesh->SetGenerateOverlapEvents(false);
		TpSwordMesh->SetCastShadow(true);

		FName FoundSocket = NAME_None;
		for (const FName& Socket : SwordHandSockets)
		{
			if (Hero->DoesSocketExist(Socket))
			{
				FoundSocket = Socket;
				break;
			}
		}

		if (FoundSocket.IsNone())
		{
			if (!bLoggedSwordHandSocketMissing)
			{
				UE_LOG(LogShadowbaneCombat, Warning,
					TEXT("%s sword hand socket missing (hand_r etc.) — attaching to mesh root"),
					*GetName());
				bLoggedSwordHandSocketMissing = true;
			}
			if (TpSwordMesh->GetAttachParent() != Hero || !TpSwordMesh->GetAttachSocketName().IsNone())
			{
				TpSwordMesh->AttachToComponent(Hero, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}
		}
		else if (TpSwordMesh->GetAttachParent() != Hero || TpSwordMesh->GetAttachSocketName() != FoundSocket)
		{
			// NOT IncludingScale — hero non-uniform race scale was blowing mesh up/off.
			// NOT via TpWeaponPivot — axe Rot(0,90,10)+Loc compounded into multi-foot world gap.
			TpSwordMesh->AttachToComponent(
				Hero, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FoundSocket);
		}

		ApplySwordGripFromCVars();
		UpdateSwordHoldMontage();
		return;
	}

	// --- Axe path: TpWeaponPivot on weapon_r / hand_r ---
	if (!TpWeaponPivot)
	{
		return;
	}

	static const FName AxeHandSockets[] = {
		FName(TEXT("weapon_r")),
		FName(TEXT("Weapon_R")),
		FName(TEXT("hand_r")),
		FName(TEXT("Hand_R")),
		FName(TEXT("RightHand")),
		FName(TEXT("ik_hand_gun")),
		FName(TEXT("hand_r_socket")),
		FName(TEXT("weapon_socket_r"))
	};

	bool bAttached = false;
	for (const FName& Socket : AxeHandSockets)
	{
		if (Hero->DoesSocketExist(Socket))
		{
			if (TpWeaponPivot->GetAttachParent() != Hero || TpWeaponPivot->GetAttachSocketName() != Socket)
			{
				TpWeaponPivot->AttachToComponent(
					Hero, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
			}
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
	else
	{
		// Axe grip pivot — swing anim offsets from (0,90,10).
		TpWeaponPivot->SetRelativeLocation(FVector(-2.f, 8.f, 3.f));
		if (MeleeSwingAnimRemaining <= 0.f)
		{
			TpWeaponPivot->SetRelativeRotation(FRotator(0.f, 90.f, 10.f));
		}
	}

	if (TpWeaponMesh)
	{
		UStaticMesh* WeaponStatic = TpWeaponMesh->GetStaticMesh();
		const FString Path = WeaponStatic ? WeaponStatic->GetPathName() : FString();
		const bool bImported = WeaponStatic && !Path.Contains(TEXT("BasicShapes"));
		if (bImported && Path.Contains(TEXT("SM_BattleAxe")))
		{
			TpWeaponMesh->SetRelativeLocation(FVector(0.f, 0.f, -8.f));
			TpWeaponMesh->SetRelativeRotation(FRotator(0.f, 0.f, -10.f));
			TpWeaponMesh->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.9f));
		}
		else if (bImported)
		{
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

void ASBCharacter::EnsureShadowKightMeleeAnimBP()
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero || !bUsingShadowKightHeroMesh)
	{
		return;
	}

	Hero->bPauseAnims = false;
	Hero->bNoSkeletonUpdate = false;
	if (Hero->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
	{
		Hero->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}

	// Prefer ABP_SK_Melee (DefaultSlot after loco, no Control Rig). Fallback
	// Anim_ShadowKight via TryLoadShadowKightAnimClass candidate list.
	// Never leave the mesh in single-node PlayAnimation mode — freezes loco.
	UClass* Desired = SBHeroSkinPaths::TryLoadShadowKightAnimClass();
	if (!Desired)
	{
		UE_LOG(LogShadowbaneCombat, Log,
			TEXT("%s EnsureShadowKightMeleeAnimBP — NO AnimBP (ABP_SK_Melee / Anim_ShadowKight missing)"),
			*GetName());
		return;
	}

	const UClass* Current = Hero->GetAnimClass();
	const bool bNeedSwap = (Current != Desired) || (Hero->GetAnimInstance() == nullptr);
	if (bNeedSwap)
	{
		Hero->SetAnimInstanceClass(Desired);
		UE_LOG(LogShadowbaneCombat, Log,
			TEXT("%s EnsureShadowKightMeleeAnimBP -> %s"),
			*GetName(), *GetNameSafe(Desired));
	}
}

void ASBCharacter::StartMeleeSwingVisual()
{
	bSwingTrailValid = false;
	EnsureWeaponInHand();
	UpdateWeaponVisibility();

	if (TpSwordMesh && TpSwordMesh->GetSkeletalMeshAsset())
	{
		// Force sword readable even if a prior hide raced replication.
		if (bUsesMelee && !bBowEquipped)
		{
			TpSwordMesh->SetHiddenInGame(false);
			TpSwordMesh->SetVisibility(true, true);
			TpSwordMesh->SetOwnerNoSee(false);
		}
		ApplySwordGripFromCVars();
	}

	const bool bChop = IsMeleeProceduralChopEnabled();
	if (!bChop)
	{
		UE_LOG(LogShadowbaneCombat, Log,
			TEXT("%s MELEE VISUAL START chop=off (sb.Melee.ProceduralChop=0) sk=%d sword=%d"),
			*GetName(),
			bUsingShadowKightHeroMesh ? 1 : 0,
			(TpSwordMesh && TpSwordMesh->GetSkeletalMeshAsset() && !TpSwordMesh->bHiddenInGame) ? 1 : 0);
		if (IsLocallyControlled())
		{
			FSBClientDebug::PushMessage(TEXT("MELEE SWING"), 0.8f);
		}
		return;
	}

	// Guaranteed visible feedback: procedural sword/axe chop on every melee LMB.
	MeleeSwingAnimDuration = SBMeleeSwingAnim::DurationSeconds;
	MeleeSwingAnimRemaining = MeleeSwingAnimDuration;
	// Nudge off alpha=0 (EvalTp* idle) so the first applied pose already chops.
	MeleeSwingAnimRemaining = FMath::Max(0.01f, MeleeSwingAnimRemaining - 0.02f);
	if (TpSwordMesh && TpSwordMesh->GetSkeletalMeshAsset())
	{
		ApplySwordGripFromCVars();
	}
	if (TpWeaponPivot)
	{
		const float Alpha = 1.f - (MeleeSwingAnimRemaining / FMath::Max(MeleeSwingAnimDuration, KINDA_SMALL_NUMBER));
		const FRotator Chop = SBMeleeSwingAnim::EvalTpPivot(Alpha);
		TpWeaponPivot->SetRelativeRotation(FRotator(Chop.Pitch, 90.f + Chop.Yaw, Chop.Roll + 10.f));
	}
	if (FpWeaponPivot)
	{
		const float Alpha = 1.f - (MeleeSwingAnimRemaining / FMath::Max(MeleeSwingAnimDuration, KINDA_SMALL_NUMBER));
		FpWeaponPivot->SetRelativeRotation(SBMeleeSwingAnim::EvalFpPivot(Alpha));
	}

	UE_LOG(LogShadowbaneCombat, Log,
		TEXT("%s MELEE VISUAL START procedural chop remaining=%.2fs sk=%d sword=%d melee=%d bow=%d"),
		*GetName(),
		MeleeSwingAnimRemaining,
		bUsingShadowKightHeroMesh ? 1 : 0,
		(TpSwordMesh && TpSwordMesh->GetSkeletalMeshAsset() && !TpSwordMesh->bHiddenInGame) ? 1 : 0,
		bUsesMelee ? 1 : 0,
		bBowEquipped ? 1 : 0);
	if (IsLocallyControlled())
	{
		FSBClientDebug::PushMessage(TEXT("MELEE SWING"), 0.8f);
	}
}

void ASBCharacter::PlayMeleeAttackAnimation()
{
	// Debounce duplicates (Enhanced Input + Multicast) ONLY after a successful play stamps
	// LastMeleeVisualTime. Never stamp on entry — that rejected Multicast with dt=0 and left
	// observers seeing only debounce Warnings while body montage never ran.
	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	const bool bChop = IsMeleeProceduralChopEnabled() || bMeleeEmergencyChop;

	USkeletalMeshComponent* HeroEarly = GetMesh();
	UAnimInstance* AnimInstEarly = HeroEarly ? HeroEarly->GetAnimInstance() : nullptr;
	const bool bMontageStillPlaying = bMeleeMontagePlaying
		|| (AnimInstEarly && CachedMeleeMontage && AnimInstEarly->Montage_IsPlaying(CachedMeleeMontage))
		|| (AnimInstEarly && CachedMeleeMontage == nullptr && AnimInstEarly->IsAnyMontagePlaying()
			&& !bMeleeHoldMontagePlaying);

	// Greystone: never interrupt mid-montage. Bank one LMB from a real extra press;
	// Multicast echo of the same swing (dt since start tiny) must not bank.
	if (IsMannyGreystoneComboPath() && bMontageStillPlaying)
	{
		if (World && (Now - LastMeleeVisualTime) < 0.25f)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("%s Greystone Multicast echo while playing — skip (no bank)"),
				*GetName());
			return;
		}
		bMeleeLmbBanked = true;
		UE_LOG(LogTemp, Warning,
			TEXT("%s Greystone swing playing — LMB banked (combo=%d), no interrupt"),
			*GetName(), MeleeComboIndex);
		return;
	}

	int32 PendingComboIndex = 0;
	if (IsMannyGreystoneComboPath())
	{
		PendingComboIndex = ResolveMannyGreystoneComboIndex(Now);
	}

	EnsureMeleeSwingAnimAssets();
	if (IsMannyGreystoneComboPath())
	{
		SelectMannyGreystoneComboAssets(PendingComboIndex);
	}

	// SK: full-swing debounce. Manny: short anti-dupe only (combo gated by montage + window).
	const float MeleeVisualDebounceSeconds = bUsingShadowKightHeroMesh ? 1.65f : 0.08f;
	if (World && (Now - LastMeleeVisualTime) < MeleeVisualDebounceSeconds)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s PlayMeleeAttackAnimation debounced (dt=%.3f < %.2f) — duplicate after success"),
			*GetName(), Now - LastMeleeVisualTime, MeleeVisualDebounceSeconds);
		return;
	}

	auto MarkMeleeVisualSuccess = [this, World, PendingComboIndex](float PlayedSeconds)
	{
		if (World)
		{
			LastMeleeVisualTime = World->GetTimeSeconds();
		}
		if (IsMannyGreystoneComboPath())
		{
			MeleeComboIndex = PendingComboIndex;
			MeleeComboAcceptUntil = -1000.f; // closed until montage ends
			const float Len = FMath::Max(PlayedSeconds, 0.1f);
			MeleeComboExpectedEndTime = LastMeleeVisualTime + Len;
			bMeleeLmbBanked = false;
			UE_LOG(LogTemp, Warning,
				TEXT("%s Greystone combo start step=%d (%s) len=%.2fs"),
				*GetName(), MeleeComboIndex,
				MeleeComboIndex == 0 ? TEXT("A") : (MeleeComboIndex == 1 ? TEXT("B") : TEXT("C")),
				Len);
		}
	};

	auto MontageKind = [](const UAnimMontage* M) -> const TCHAR*
	{
		if (!M)
		{
			return TEXT("null");
		}
		const FString N = M->GetName();
		if (N.Contains(TEXT("LibSwing")))
		{
			return TEXT("LibSwing");
		}
		if (N.Contains(TEXT("SwordSwing")))
		{
			return TEXT("SwordSwing");
		}
		if (N.Contains(TEXT("GreystoneSwing")))
		{
			return TEXT("GreystoneSwing");
		}
		if (N.Contains(TEXT("SteelSwing")))
		{
			return TEXT("SteelSwing");
		}
		if (N.Contains(TEXT("SerathSwing")))
		{
			return TEXT("SerathSwing");
		}
		if (N.Contains(TEXT("AxeSwing")))
		{
			return TEXT("AxeSwing");
		}
		return TEXT("other");
	};

	auto SequenceKind = [](const UAnimSequence* S) -> const TCHAR*
	{
		if (!S)
		{
			return TEXT("null");
		}
		const FString N = S->GetName();
		if (N.Contains(TEXT("LibSwing")))
		{
			return TEXT("LibSwing");
		}
		if (N.Contains(TEXT("SwordSwing")))
		{
			return TEXT("SwordSwing");
		}
		if (N.Contains(TEXT("GreystoneSwing")))
		{
			return TEXT("GreystoneSwing");
		}
		if (N.Contains(TEXT("SteelSwing")))
		{
			return TEXT("SteelSwing");
		}
		if (N.Contains(TEXT("SerathSwing")))
		{
			return TEXT("SerathSwing");
		}
		if (N.Contains(TEXT("AxeSwing")))
		{
			return TEXT("AxeSwing");
		}
		return TEXT("other");
	};

	USkeletalMeshComponent* Hero = GetMesh();

	// ShadowKight / non-Manny: don't restart until current swing finishes.
	if (!IsMannyGreystoneComboPath() && bMontageStillPlaying)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s ISOLATE: chop=%s montage=%s playing — skip Montage_Play restart"),
			*GetName(),
			bChop ? TEXT("on") : TEXT("off"),
			MontageKind(CachedMeleeMontage));
		return;
	}
	// Procedural-chop window still active (non-isolate): skip restarting the short chop.
	// Manny Greystone: allow next letter once montage ended even if chop leftover is tiny.
	if (bChop && MeleeSwingAnimRemaining > 0.05f && !IsMannyGreystoneComboPath())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s ISOLATE: chop=on remaining=%.2fs — skip restart"),
			*GetName(), MeleeSwingAnimRemaining);
		return;
	}

	// Body montage is priority; procedural sword chop is optional polish (sb.Melee.ProceduralChop).
	StartMeleeSwingVisual();
	bMeleeMontagePlaying = false;
	StopSwordHoldMontage();

	if (!Hero)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s PlayMeleeAttackAnimation — no hero mesh; chop=%s"),
			*GetName(), bChop ? TEXT("on") : TEXT("off"));
		if (bChop)
		{
			ApplyMeleeBodyPose(0.05f);
		}
		MarkMeleeVisualSuccess(SBMeleeSwingAnim::DurationSeconds);
		return;
	}

	if (bUsingShadowKightHeroMesh)
	{
		EnsureShadowKightMeleeAnimBP();
	}

	// Re-fetch after EnsureShadowKightMeleeAnimBP may have swapped the class.
	UAnimInstance* AnimInst = Hero->GetAnimInstance();
	UE_LOG(LogTemp, Warning,
		TEXT("%s melee play begin AnimBP=%s AnimInst=%s sk=%d chop=%d montage=%s(%s) seq=%s(%s)"),
		*GetName(),
		*GetNameSafe(Hero->GetAnimClass()),
		*GetNameSafe(AnimInst),
		bUsingShadowKightHeroMesh ? 1 : 0,
		bChop ? 1 : 0,
		*GetNameSafe(CachedMeleeMontage),
		MontageKind(CachedMeleeMontage),
		*GetNameSafe(CachedMeleeSwingSequence),
		SequenceKind(CachedMeleeSwingSequence));
	if (AnimInst)
	{
		if (CachedMeleeHoldMontage && AnimInst->Montage_IsPlaying(CachedMeleeHoldMontage))
		{
			AnimInst->Montage_Stop(0.05f, CachedMeleeHoldMontage);
		}
	}
	bMeleeHoldMontagePlaying = false;

	if (!AnimInst)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s body montage failed — no AnimInstance; chop=%s"),
			*GetName(), bChop ? TEXT("on/RUNNING") : TEXT("off"));
		if (bChop)
		{
			ApplyMeleeBodyPose(0.05f);
		}
		MarkMeleeVisualSuccess(SBMeleeSwingAnim::DurationSeconds);
		return;
	}

	auto BeginMontageSwingTiming = [this, bChop](float PlayedSeconds)
	{
		bMeleeMontagePlaying = true;
		// Only drive procedural remaining when chop is on. Do NOT stretch Alpha over
		// Montage_Play length (Alpha stayed ~0 → sword looked dead).
		if (bChop && MeleeSwingAnimRemaining <= 0.f)
		{
			MeleeSwingAnimDuration = SBMeleeSwingAnim::DurationSeconds;
			MeleeSwingAnimRemaining = MeleeSwingAnimDuration;
		}
		(void)PlayedSeconds;
	};

	// --- ShadowKight: Montage_Play(AM_SK_LibSwing_01 preferred, else AM_SK_SwordSwing_01) ---
	if (bUsingShadowKightHeroMesh)
	{
		if (!CachedMeleeMontage)
		{
			UE_LOG(LogTemp, Error,
				TEXT("%s ShadowKight melee montage missing (tried LibSwing then SwordSwing)"),
				*GetName());
		}
		if (!CachedMeleeSwingSequence)
		{
			UE_LOG(LogTemp, Error,
				TEXT("%s ShadowKight melee sequence missing (tried LibSwing then SwordSwing)"),
				*GetName());
		}

		if (CachedMeleeMontage)
		{
			if (USkeleton* MontSkel = CachedMeleeMontage->GetSkeleton())
			{
				USkeleton* MeshSkel = Hero->GetSkeletalMeshAsset()
					? Hero->GetSkeletalMeshAsset()->GetSkeleton()
					: nullptr;
				UE_LOG(LogTemp, Warning,
					TEXT("%s montage=%s kind=%s skel=%s mesh skel=%s match=%d"),
					*GetName(),
					*GetNameSafe(CachedMeleeMontage),
					MontageKind(CachedMeleeMontage),
					*GetNameSafe(MontSkel),
					*GetNameSafe(MeshSkel),
					(MontSkel == MeshSkel) ? 1 : 0);
			}
			const float Played = AnimInst->Montage_Play(CachedMeleeMontage, 1.f);
			if (Played > 0.f)
			{
				BeginMontageSwingTiming(Played);
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &ASBCharacter::OnMeleeSwingMontageEnded);
				AnimInst->Montage_SetEndDelegate(EndDelegate, CachedMeleeMontage);
				UE_LOG(LogTemp, Warning,
					TEXT("%s ISOLATE: chop=%s montage=%s playing (%.2fs) AnimBP=%s"),
					*GetName(),
					bChop ? TEXT("on") : TEXT("off"),
					MontageKind(CachedMeleeMontage),
					Played,
					*GetNameSafe(Hero->GetAnimClass()));
				MarkMeleeVisualSuccess(Played);
				return;
			}

			UE_LOG(LogTemp, Error,
				TEXT("%s body montage failed — Montage_Play returned 0 for %s kind=%s AnimBP=%s"),
				*GetName(), *GetNameSafe(CachedMeleeMontage), MontageKind(CachedMeleeMontage),
				*GetNameSafe(Hero->GetAnimClass()));
		}

		if (CachedMeleeSwingSequence)
		{
			static const FName DefaultSlot(TEXT("DefaultSlot"));
			UAnimMontage* Dyn = AnimInst->PlaySlotAnimationAsDynamicMontage(
				CachedMeleeSwingSequence,
				DefaultSlot,
				0.05f,
				0.12f,
				1.f,
				1);
			if (Dyn)
			{
				const float PlayLen = FMath::Max(Dyn->GetPlayLength(), SBMeleeSwingAnim::DurationSeconds);
				BeginMontageSwingTiming(PlayLen);
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &ASBCharacter::OnMeleeSwingMontageEnded);
				AnimInst->Montage_SetEndDelegate(EndDelegate, Dyn);
				UE_LOG(LogTemp, Warning,
					TEXT("%s ISOLATE: chop=%s montage=%s playing via dynamic slot (%.2fs) AnimBP=%s"),
					*GetName(),
					bChop ? TEXT("on") : TEXT("off"),
					SequenceKind(CachedMeleeSwingSequence),
					PlayLen, *GetNameSafe(Hero->GetAnimClass()));
				MarkMeleeVisualSuccess(PlayLen);
				return;
			}

			UE_LOG(LogTemp, Error,
				TEXT("%s dynamic slot FAILED %s kind=%s AnimBP=%s — chop=%s"),
				*GetName(), *GetNameSafe(CachedMeleeSwingSequence), SequenceKind(CachedMeleeSwingSequence),
				*GetNameSafe(Hero->GetAnimClass()), bChop ? TEXT("on only") : TEXT("off (no body)"));
		}

		UE_LOG(LogTemp, Error,
			TEXT("%s body montage failed — ShadowKight swing assets/slot; chop=%s AnimBP=%s"),
			*GetName(), bChop ? TEXT("on still running") : TEXT("off"),
			*GetNameSafe(Hero->GetAnimClass()));
		if (bChop)
		{
			ApplyMeleeBodyPose(0.05f);
		}
		MarkMeleeVisualSuccess(SBMeleeSwingAnim::DurationSeconds);
		return;
	}

	// --- Mannequin / non-SK: prefer Greystone/Steel DefaultSlot montages, else AxeSwing ---
	bMeleeEmergencyChop = false;

	// Montage_Play can return >0 while DefaultSlot/ControlRig leaves the output pose
	// unchanged (19-58 capture: AxeSwing OK, zero visible motion, ProceduralChop=0).
	// Always layer sword + torso lean on MM so LMB is readable even if the slot is dead.
	auto LayerMannyVisibleChop = [this](const TCHAR* Reason)
	{
		bMeleeEmergencyChop = true;
		MeleeSwingAnimDuration = SBMeleeSwingAnim::DurationSeconds;
		MeleeSwingAnimRemaining = FMath::Max(0.01f, MeleeSwingAnimDuration - 0.02f);
		ApplyMeleeBodyPose(0.05f);
		if (TpSwordMesh && TpSwordMesh->GetSkeletalMeshAsset())
		{
			ApplySwordGripFromCVars();
		}
		UE_LOG(LogTemp, Warning,
			TEXT("%s MM visible chop layered (%s) remaining=%.2fs"),
			*GetName(), Reason, MeleeSwingAnimRemaining);
	};

	if (!CachedMeleeMontage)
	{
		UE_LOG(LogTemp, Error,
			TEXT("%s MM melee montage missing (expected AM_MM_GreystoneSwing_A or AM_MM_AxeSwing_01)"),
			*GetName());
	}
	if (!CachedMeleeSwingSequence)
	{
		UE_LOG(LogTemp, Error,
			TEXT("%s MM melee sequence missing (expected AS_MM_GreystoneSwing_A or AS_MM_AxeSwing_01)"),
			*GetName());
	}

	if (CachedMeleeMontage)
	{
		if (USkeleton* MontSkel = CachedMeleeMontage->GetSkeleton())
		{
			USkeleton* MeshSkel = Hero->GetSkeletalMeshAsset()
				? Hero->GetSkeletalMeshAsset()->GetSkeleton()
				: nullptr;
			UE_LOG(LogTemp, Warning,
				TEXT("%s montage=%s kind=%s skel=%s mesh skel=%s match=%d"),
				*GetName(),
				*GetNameSafe(CachedMeleeMontage),
				MontageKind(CachedMeleeMontage),
				*GetNameSafe(MontSkel),
				*GetNameSafe(MeshSkel),
				(MontSkel == MeshSkel) ? 1 : 0);
		}
		const float Played = AnimInst->Montage_Play(CachedMeleeMontage, 1.f);
		if (Played > 0.f)
		{
			BeginMontageSwingTiming(Played);
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ASBCharacter::OnMeleeSwingMontageEnded);
			AnimInst->Montage_SetEndDelegate(EndDelegate, CachedMeleeMontage);
			UE_LOG(LogTemp, Warning,
				TEXT("%s Montage_Play OK %s kind=%s (%.2fs) AnimBP=%s chop=%d"),
				*GetName(),
				*GetNameSafe(CachedMeleeMontage),
				MontageKind(CachedMeleeMontage),
				Played,
				*GetNameSafe(Hero->GetAnimClass()),
				IsMeleeProceduralChopEnabled() ? 1 : 0);
			if (!IsMeleeProceduralChopEnabled())
			{
				LayerMannyVisibleChop(TEXT("after Montage_Play; ProceduralChop=0"));
			}
			MarkMeleeVisualSuccess(Played);
			return;
		}

		UE_LOG(LogTemp, Error,
			TEXT("%s body montage failed — Montage_Play returned 0 for %s kind=%s AnimBP=%s (DefaultSlot missing?)"),
			*GetName(), *GetNameSafe(CachedMeleeMontage), MontageKind(CachedMeleeMontage),
			*GetNameSafe(Hero->GetAnimClass()));
	}
	if (CachedMeleeSwingSequence)
	{
		static const FName DefaultSlot(TEXT("DefaultSlot"));
		if (UAnimMontage* Dyn = AnimInst->PlaySlotAnimationAsDynamicMontage(
				CachedMeleeSwingSequence, DefaultSlot, 0.05f, 0.12f, 1.f, 1))
		{
			const float PlayLen = FMath::Max(Dyn->GetPlayLength(), SBMeleeSwingAnim::DurationSeconds);
			BeginMontageSwingTiming(PlayLen);
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &ASBCharacter::OnMeleeSwingMontageEnded);
			AnimInst->Montage_SetEndDelegate(EndDelegate, Dyn);
			UE_LOG(LogTemp, Warning,
				TEXT("%s Montage_Play OK via dynamic slot %s kind=%s (%.2fs) AnimBP=%s"),
				*GetName(),
				*GetNameSafe(CachedMeleeSwingSequence),
				SequenceKind(CachedMeleeSwingSequence),
				PlayLen, *GetNameSafe(Hero->GetAnimClass()));
			if (!IsMeleeProceduralChopEnabled())
			{
				LayerMannyVisibleChop(TEXT("after dynamic slot; ProceduralChop=0"));
			}
			MarkMeleeVisualSuccess(PlayLen);
			return;
		}

		UE_LOG(LogTemp, Error,
			TEXT("%s dynamic MM slot FAILED %s kind=%s AnimBP=%s — chop=%s"),
			*GetName(), *GetNameSafe(CachedMeleeSwingSequence), SequenceKind(CachedMeleeSwingSequence),
			*GetNameSafe(Hero->GetAnimClass()), bChop ? TEXT("on only") : TEXT("off (no body)"));
	}

	// Last resort: DefaultSlot dead / assets missing — force procedural chop so LMB is visible.
	UE_LOG(LogTemp, Error,
		TEXT("%s body montage unavailable (sk=0 montage=%s) — emergency procedural chop AnimBP=%s"),
		*GetName(), MontageKind(CachedMeleeMontage),
		*GetNameSafe(Hero->GetAnimClass()));
	LayerMannyVisibleChop(TEXT("no montage/slot"));
	MarkMeleeVisualSuccess(SBMeleeSwingAnim::DurationSeconds);
}

void ASBCharacter::OnMeleeSwingMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Ignore hold montage end callbacks; accept LibSwing/SwordSwing or dynamic slot montages.
	if (CachedMeleeHoldMontage && Montage == CachedMeleeHoldMontage)
	{
		return;
	}
	bMeleeMontagePlaying = false;
	bMeleeEmergencyChop = false;
	if (!bInterrupted && IsMannyGreystoneComboPath())
	{
		OpenMannyMeleeComboWindow();
	}
	if (MeleeSwingAnimRemaining <= 0.f)
	{
		UpdateSwordHoldMontage();
	}
}

bool ASBCharacter::IsMannyGreystoneComboPath() const
{
	return !bUsingShadowKightHeroMesh && !bUsingParagonHeroMesh;
}

bool ASBCharacter::IsMeleeSwingMontageActive() const
{
	if (bMeleeMontagePlaying)
	{
		return true;
	}
	const USkeletalMeshComponent* Hero = GetMesh();
	const UAnimInstance* AnimInst = Hero ? Hero->GetAnimInstance() : nullptr;
	if (!AnimInst)
	{
		return false;
	}
	if (CachedMeleeMontage && AnimInst->Montage_IsPlaying(CachedMeleeMontage))
	{
		return true;
	}
	return CachedMeleeMontage == nullptr && AnimInst->IsAnyMontagePlaying() && !bMeleeHoldMontagePlaying;
}

void ASBCharacter::ResetMannyMeleeComboState()
{
	MeleeComboIndex = 0;
	MeleeComboAcceptUntil = -1000.f;
	MeleeComboExpectedEndTime = -1000.f;
	bMeleeLmbBanked = false;
}

int32 ASBCharacter::ResolveMannyGreystoneComboIndex(float Now) const
{
	// After C, next is always A (no wrap chain).
	if (MeleeComboIndex >= 2)
	{
		return 0;
	}

	bool bInWindow = false;
	if (MeleeComboAcceptUntil > -999.f && Now <= MeleeComboAcceptUntil)
	{
		bInWindow = true;
	}
	else if (MeleeComboAcceptUntil <= -999.f && MeleeComboExpectedEndTime > -999.f)
	{
		const float Window = GetMeleeComboWindowSec();
		if (Now >= MeleeComboExpectedEndTime && Now <= MeleeComboExpectedEndTime + Window)
		{
			bInWindow = true;
		}
	}

	if (bInWindow && MeleeComboIndex >= 0 && MeleeComboIndex < 2)
	{
		return MeleeComboIndex + 1;
	}
	return 0;
}

void ASBCharacter::SelectMannyGreystoneComboAssets(int32 ComboIndex)
{
	ComboIndex = FMath::Clamp(ComboIndex, 0, SBMeleeSwingAnim::GreystoneComboCount - 1);
	CachedMeleeMontage = LoadObject<UAnimMontage>(
		nullptr, SBMeleeSwingAnim::GreystoneComboMontagePaths[ComboIndex]);
	CachedMeleeSwingSequence = LoadObject<UAnimSequence>(
		nullptr, SBMeleeSwingAnim::GreystoneComboSequencePaths[ComboIndex]);

	if (!CachedMeleeMontage)
	{
		for (const TCHAR* Path : SBMeleeSwingAnim::PreferredMeleeMontagePaths)
		{
			if (UAnimMontage* M = LoadObject<UAnimMontage>(nullptr, Path))
			{
				CachedMeleeMontage = M;
				UE_LOG(LogTemp, Warning,
					TEXT("%s Greystone combo %d montage miss — fallback %s"),
					*GetName(), ComboIndex, *GetNameSafe(M));
				break;
			}
		}
	}
	if (!CachedMeleeSwingSequence)
	{
		for (const TCHAR* Path : SBMeleeSwingAnim::PreferredMeleeSequencePaths)
		{
			if (UAnimSequence* S = LoadObject<UAnimSequence>(nullptr, Path))
			{
				CachedMeleeSwingSequence = S;
				break;
			}
		}
	}
}

void ASBCharacter::OpenMannyMeleeComboWindow()
{
	if (!IsMannyGreystoneComboPath())
	{
		return;
	}

	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	const float Window = GetMeleeComboWindowSec();
	MeleeComboAcceptUntil = Now + Window;

	const bool bHadBank = bMeleeLmbBanked;
	bMeleeLmbBanked = false;

	UE_LOG(LogTemp, Warning,
		TEXT("%s Greystone combo window open %.2fs (step=%d bank=%d)"),
		*GetName(), Window, MeleeComboIndex, bHadBank ? 1 : 0);

	// Banked LMB during A/B → immediately continue to B/C (as if pressed in window).
	if (bHadBank && MeleeComboIndex < 2 && IsLocallyControlled() && !bDead
		&& bUsesMelee && !bBowEquipped && Stamina >= MeleeStaminaCost)
	{
		PlayMeleeAttackAnimation();
		ServerFire();
	}
}

void ASBCharacter::CancelMeleeSwing(const TCHAR* Reason)
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (UAnimInstance* AnimInst = Hero ? Hero->GetAnimInstance() : nullptr)
	{
		if (CachedMeleeMontage && AnimInst->Montage_IsPlaying(CachedMeleeMontage))
		{
			AnimInst->Montage_Stop(0.05f, CachedMeleeMontage);
		}
		else if (AnimInst->IsAnyMontagePlaying() && !bMeleeHoldMontagePlaying)
		{
			AnimInst->StopAllMontages(0.05f);
		}
	}

	bMeleeMontagePlaying = false;
	bMeleeEmergencyChop = false;
	MeleeSwingAnimRemaining = 0.f;
	if (IsMannyGreystoneComboPath())
	{
		ResetMannyMeleeComboState();
	}
	ApplyMeleeIdlePose();
	UpdateSwordHoldMontage();
	UE_LOG(LogTemp, Warning, TEXT("%s melee swing cancelled (%s)"), *GetName(), Reason ? Reason : TEXT("?"));
	if (IsLocallyControlled())
	{
		FSBClientDebug::PushMessage(TEXT("MELEE CANCEL"), 0.6f);
	}
}

void ASBCharacter::EnsureMeleeSwingAnimAssets()
{
	// Paragon Countess (and similar): incompatible skeleton — clear cache.
	if (bUsingParagonHeroMesh && !bUsingShadowKightHeroMesh)
	{
		CachedMeleeMontage = nullptr;
		CachedMeleeSwingSequence = nullptr;
		CachedMeleeHoldMontage = nullptr;
		CachedMeleeHoldSequence = nullptr;
		return;
	}

	if (bUsingShadowKightHeroMesh)
	{
		// Hard-coded LibSwing FIRST — never stick on a prior SwordSwing cache.
		static const TCHAR* const LibSwingMontagePath =
			TEXT("/Game/ShadowKight/Animations/Combat/AM_SK_LibSwing_01.AM_SK_LibSwing_01");
		static const TCHAR* const LibSwingSequencePath =
			TEXT("/Game/ShadowKight/Animations/Combat/AS_SK_LibSwing_01.AS_SK_LibSwing_01");
		static const TCHAR* const SwordSwingMontagePath =
			TEXT("/Game/ShadowKight/Animations/Combat/AM_SK_SwordSwing_01.AM_SK_SwordSwing_01");
		static const TCHAR* const SwordSwingSequencePath =
			TEXT("/Game/ShadowKight/Animations/Combat/AS_SK_SwordSwing_01.AS_SK_SwordSwing_01");

		const bool bHaveLibMontage = CachedMeleeMontage
			&& CachedMeleeMontage->GetName().Contains(TEXT("LibSwing"));
		if (!bHaveLibMontage)
		{
			UAnimMontage* LibMontage = LoadObject<UAnimMontage>(nullptr, LibSwingMontagePath);
			if (LibMontage)
			{
				CachedMeleeMontage = LibMontage;
				UE_LOG(LogTemp, Warning,
					TEXT("%s load AM_SK_LibSwing_01 OK path=%s ptr=%s"),
					*GetName(), LibSwingMontagePath, *GetNameSafe(CachedMeleeMontage));
			}
			else
			{
				UE_LOG(LogTemp, Error,
					TEXT("%s MISSING AM_SK_LibSwing_01 — LoadObject failed path=%s (run scripts/Create-ShadowKightLibSwing.ps1)"),
					*GetName(), LibSwingMontagePath);
				if (!CachedMeleeMontage)
				{
					CachedMeleeMontage = LoadObject<UAnimMontage>(nullptr, SwordSwingMontagePath);
					UE_LOG(LogTemp, Warning,
						TEXT("%s load AM_SK_SwordSwing_01 fallback %s → %s"),
						*GetName(), SwordSwingMontagePath,
						CachedMeleeMontage ? TEXT("OK") : TEXT("FAIL"));
				}
			}
		}

		const bool bHaveLibSeq = CachedMeleeSwingSequence
			&& CachedMeleeSwingSequence->GetName().Contains(TEXT("LibSwing"));
		if (!bHaveLibSeq)
		{
			UAnimSequence* LibSeq = LoadObject<UAnimSequence>(nullptr, LibSwingSequencePath);
			if (LibSeq)
			{
				CachedMeleeSwingSequence = LibSeq;
				UE_LOG(LogTemp, Warning,
					TEXT("%s load AS_SK_LibSwing_01 OK path=%s ptr=%s"),
					*GetName(), LibSwingSequencePath, *GetNameSafe(CachedMeleeSwingSequence));
			}
			else
			{
				UE_LOG(LogTemp, Error,
					TEXT("%s MISSING AS_SK_LibSwing_01 — LoadObject failed path=%s (run scripts/Create-ShadowKightLibSwing.ps1)"),
					*GetName(), LibSwingSequencePath);
				if (!CachedMeleeSwingSequence)
				{
					CachedMeleeSwingSequence = LoadObject<UAnimSequence>(nullptr, SwordSwingSequencePath);
					UE_LOG(LogTemp, Warning,
						TEXT("%s load AS_SK_SwordSwing_01 fallback %s → %s"),
						*GetName(), SwordSwingSequencePath,
						CachedMeleeSwingSequence ? TEXT("OK") : TEXT("FAIL"));
				}
			}
		}
		EnsureMeleeHoldAnimAssets();
		return;
	}

	// Hard-coded preferred Manny swings FIRST (Greystone A/B/C → Steel A → AxeSwing).
	// Never stick on a prior SK LibSwing/SwordSwing cache.
	const bool bHaveMannyMontage = CachedMeleeMontage
		&& IsMannyPreferredSwingName(CachedMeleeMontage->GetName());
	if (!bHaveMannyMontage)
	{
		CachedMeleeMontage = nullptr;
		for (const TCHAR* Path : SBMeleeSwingAnim::PreferredMeleeMontagePaths)
		{
			if (UAnimMontage* M = LoadObject<UAnimMontage>(nullptr, Path))
			{
				CachedMeleeMontage = M;
				UE_LOG(LogTemp, Warning,
					TEXT("%s load MM melee montage OK name=%s path=%s"),
					*GetName(), *GetNameSafe(M), Path);
				break;
			}
			UE_LOG(LogTemp, Verbose,
				TEXT("%s MM melee montage miss path=%s"), *GetName(), Path);
		}
		if (!CachedMeleeMontage)
		{
			UE_LOG(LogTemp, Error,
				TEXT("%s MISSING all MM melee montages (Greystone/Steel/AxeSwing) — run scripts/Import-MannyMeleeFbx.ps1 or Create-MeleeSwingMontage.ps1"),
				*GetName());
		}
	}

	const bool bHaveMannySeq = CachedMeleeSwingSequence
		&& IsMannyPreferredSwingName(CachedMeleeSwingSequence->GetName());
	if (!bHaveMannySeq)
	{
		CachedMeleeSwingSequence = nullptr;
		for (const TCHAR* Path : SBMeleeSwingAnim::PreferredMeleeSequencePaths)
		{
			if (UAnimSequence* S = LoadObject<UAnimSequence>(nullptr, Path))
			{
				CachedMeleeSwingSequence = S;
				UE_LOG(LogTemp, Warning,
					TEXT("%s load MM melee sequence OK name=%s path=%s"),
					*GetName(), *GetNameSafe(S), Path);
				break;
			}
		}
		if (!CachedMeleeSwingSequence)
		{
			UE_LOG(LogTemp, Error,
				TEXT("%s MISSING all MM melee sequences (Greystone/Steel/AxeSwing) — run scripts/Import-MannyMeleeFbx.ps1"),
				*GetName());
		}
	}
}

void ASBCharacter::EnsureMeleeHoldAnimAssets()
{
	if (!bUsingShadowKightHeroMesh)
	{
		CachedMeleeHoldMontage = nullptr;
		CachedMeleeHoldSequence = nullptr;
		return;
	}
	if (!CachedMeleeHoldSequence)
	{
		CachedMeleeHoldSequence = LoadObject<UAnimSequence>(nullptr, SBMeleeSwingAnim::ShadowKightMeleeHoldSequencePath);
	}
	if (!CachedMeleeHoldMontage)
	{
		CachedMeleeHoldMontage = LoadObject<UAnimMontage>(nullptr, SBMeleeSwingAnim::ShadowKightMeleeHoldMontagePath);
		if (!CachedMeleeHoldMontage && !CachedMeleeHoldSequence)
		{
			UE_LOG(LogShadowbaneCombat, Warning,
				TEXT("%s AS/AM_SK_SwordHold_01 missing — run Create-ShadowKightHoldMontage.ps1"),
				*GetName());
		}
	}
}

void ASBCharacter::StopSwordHoldMontage()
{
	if (USkeletalMeshComponent* Hero = GetMesh())
	{
		// Safety: never leave the hero mesh in single-node mode (kills loco).
		if (Hero->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
		{
			EnsureShadowKightMeleeAnimBP();
		}
		if (UAnimInstance* AnimInst = Hero->GetAnimInstance())
		{
			if (CachedMeleeHoldMontage && AnimInst->Montage_IsPlaying(CachedMeleeHoldMontage))
			{
				AnimInst->Montage_Stop(0.f, CachedMeleeHoldMontage);
			}
		}
	}
	bMeleeHoldMontagePlaying = false;
}

void ASBCharacter::UpdateSwordHoldMontage()
{
	const bool bShowSword = bUsesMelee && !bBowEquipped
		&& TpSwordMesh && !TpSwordMesh->bHiddenInGame && TpSwordMesh->GetSkeletalMeshAsset();
	// Optional arm hold only — never during swing montage OR procedural chop window
	// (restarting hold every tick was fighting DefaultSlot / reading as "no swing").
	const bool bWantHold = bUsingShadowKightHeroMesh && bShowSword
		&& !bMeleeMontagePlaying && MeleeSwingAnimRemaining <= 0.f;

	if (!bWantHold)
	{
		StopSwordHoldMontage();
		return;
	}

	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero)
	{
		return;
	}

	EnsureMeleeHoldAnimAssets();

	UAnimInstance* AnimInst = Hero->GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}

	// Swing montage still finishing after procedural chop timer — do not steal the slot.
	if (CachedMeleeMontage && AnimInst->Montage_IsPlaying(CachedMeleeMontage))
	{
		bMeleeMontagePlaying = true;
		return;
	}

	// Weak hold: Montage on DefaultSlot only. If AnimBP lacks the slot, skip —
	// idle/walk/run wins over a frozen hold. Never PlayAnimation for hold.
	if (!CachedMeleeHoldMontage)
	{
		return;
	}

	if (AnimInst->Montage_IsPlaying(CachedMeleeHoldMontage))
	{
		bMeleeHoldMontagePlaying = true;
		return;
	}

	const float Played = AnimInst->Montage_Play(CachedMeleeHoldMontage, 1.f);
	if (Played <= 0.f)
	{
		bMeleeHoldMontagePlaying = false;
		if (!bLoggedMeleeHoldRestartSkip)
		{
			UE_LOG(LogShadowbaneCombat, Log,
				TEXT("%s hold Montage_Play skipped (no DefaultSlot) — locomotion only"),
				*GetName());
			bLoggedMeleeHoldRestartSkip = true;
		}
		return;
	}

	static const FName DefaultSection(TEXT("Default"));
	AnimInst->Montage_SetNextSection(DefaultSection, DefaultSection, CachedMeleeHoldMontage);
	bMeleeHoldMontagePlaying = true;
}

void ASBCharacter::BindHeroBoneSwingOverlay()
{
	// Disabled: mutating CS bone buffers after finalize (and disabling CS double-
	// buffering) caused:
	//   EditableBoneVisibilityStates.Num() == GetNumComponentSpaceTransforms()
	// assert in SkeletalMeshComponent.cpp — crash on start/PIE with mesh+sword flash.
	// Melee uses weapon-pivot chop only until a safe AnimInstance/montages path returns.
	UnbindHeroBoneSwingOverlay();
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
	// Intentionally empty — do not write component-space bone transforms here.
}

void ASBCharacter::ApplyProceduralMeleeArmBones(float Alpha01)
{
	// Intentionally empty — unsafe CS bone mutation removed for stability.
	(void)Alpha01;
}

void ASBCharacter::ApplyMeleeBodyPose(float Alpha01)
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero || !bUsingHeroMesh)
	{
		return;
	}

	// ShadowKight: never yaw/pitch the whole mesh — reads as sideways wobble while
	// AM_SK_LibSwing_01 / SwordSwing + procedural chop drive the arm/blade.
	if (bUsingShadowKightHeroMesh)
	{
		Hero->SetRelativeRotation(HeroMeshBaseRelativeRot);
		return;
	}

	// Bold torso lean — readable even when montage/Control Rig fails to show arms.
	const float A = FMath::Clamp(Alpha01, 0.f, 1.f);
	float LeanYaw = 0.f;
	float LeanPitch = 0.f;
	if (A > 0.f && A < 1.f)
	{
		if (A < 0.28f)
		{
			const float U = SBMeleeSwingAnim::Smooth01(A / 0.28f);
			LeanYaw = FMath::Lerp(0.f, -22.f, U);
			LeanPitch = FMath::Lerp(0.f, -10.f, U);
		}
		else if (A < 0.52f)
		{
			const float U = SBMeleeSwingAnim::Smooth01((A - 0.28f) / 0.24f);
			LeanYaw = FMath::Lerp(-22.f, 32.f, U * U);
			LeanPitch = FMath::Lerp(-10.f, 12.f, U);
		}
		else
		{
			const float U = SBMeleeSwingAnim::Smooth01((A - 0.52f) / 0.48f);
			LeanYaw = FMath::Lerp(32.f, 0.f, U);
			LeanPitch = FMath::Lerp(12.f, 0.f, U);
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
	if (bUsingShadowKightHeroMesh)
	{
		EnsureShadowKightMeleeAnimBP();
	}

	if (FpWeaponPivot)
	{
		FpWeaponPivot->SetRelativeRotation(SBMeleeSwingAnim::EvalFpPivot(0.f));
	}
	if (TpWeaponPivot)
	{
		// Hand-socket weapon: idle = grip rest (swing anim offsets from here).
		TpWeaponPivot->SetRelativeRotation(FRotator(0.f, 90.f, 10.f));
	}
	if (TpSwordMesh && !TpSwordMesh->bHiddenInGame && TpSwordMesh->GetSkeletalMeshAsset())
	{
		ApplySwordGripFromCVars();
	}
	ApplyMeleeBodyPose(0.f);
	bSwingTrailValid = false;
	UpdateSwordHoldMontage();
}

void ASBCharacter::TickMeleeSwingVisual(float DeltaSeconds)
{
	if (MeleeSwingAnimRemaining <= 0.f)
	{
		return;
	}

	if (!IsMeleeProceduralChopEnabled() && !bMeleeEmergencyChop)
	{
		// Isolate: body montage owns the swing — clear leftover chop timer.
		MeleeSwingAnimRemaining = 0.f;
		ApplyMeleeIdlePose();
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
	// Sword is on hand_r (not TpWeaponPivot) — re-apply EvalTpSwordChop while swinging.
	if (TpSwordMesh && TpSwordMesh->GetSkeletalMeshAsset())
	{
		if (bUsesMelee && !bBowEquipped && TpSwordMesh->bHiddenInGame)
		{
			TpSwordMesh->SetHiddenInGame(false);
			TpSwordMesh->SetVisibility(true, true);
		}
		ApplySwordGripFromCVars();
	}
	// Torso lean for mannequin fallback only — SK skips inside ApplyMeleeBodyPose.
	ApplyMeleeBodyPose(Alpha);

	MeleeSwingAnimRemaining = FMath::Max(0.f, MeleeSwingAnimRemaining - DeltaSeconds);
	if (MeleeSwingAnimRemaining <= 0.f)
	{
		bMeleeEmergencyChop = false;
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
	bUsingParagonHeroMesh = false;
	bUsingShadowKightHeroMesh = false;

	USkeletalMesh* MeshAsset = nullptr;
	UClass* AnimClass = nullptr;

	// Optional Paragon Countess (Fab) — soft paths; no-op until Captain migrates the pack.
	if (RaceMapsToCountess(Race))
	{
		MeshAsset = SBHeroSkinPaths::TryLoadCountessMesh();
		if (MeshAsset)
		{
			AnimClass = SBHeroSkinPaths::TryLoadCountessAnimClass();
			bUsingParagonHeroMesh = true;
			UE_LOG(LogShadowbaneCombat, Log,
				TEXT("%s using Paragon Countess hero mesh for race '%s'"), *GetName(), *RaceName);
		}
	}

	// ShadowKight pack (Content/ShadowKight) — dogfood on Human when CVar on + assets present.
	if (!MeshAsset && RaceMapsToShadowKight(Race))
	{
		MeshAsset = SBHeroSkinPaths::TryLoadShadowKightMesh();
		if (MeshAsset)
		{
			AnimClass = SBHeroSkinPaths::TryLoadShadowKightAnimClass();
			bUsingParagonHeroMesh = true; // non-UE5-mannequin: skip MM montage + race tint
			bUsingShadowKightHeroMesh = true;
			UE_LOG(LogShadowbaneCombat, Log,
				TEXT("%s using ShadowKight hero mesh for race '%s' AnimBP=%s"),
				*GetName(), *RaceName, *GetNameSafe(AnimClass));
		}
	}

	const bool bPreferQuinn = !bUsingParagonHeroMesh && Race.Contains(TEXT("elf")); // Elf + High Elf
	if (!MeshAsset && bPreferQuinn)
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
	Hero->bNoSkeletonUpdate = false;
	Hero->bOwnerNoSee = false;

	// Scale tweaks by race silhouette.

	// Race scale / silhouette extras on top of the humanoid.
	FVector MeshScale(1.f, 1.f, 1.f);
	if (bUsingParagonHeroMesh)
	{
		MeshScale = FVector(1.f, 1.f, 1.f); // Countess authored scale
	}
	else if (Race.Contains(TEXT("dwarf")))
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
	StopSwordHoldMontage();
	CachedMeleeMontage = nullptr;
	CachedMeleeSwingSequence = nullptr;
	CachedMeleeHoldMontage = nullptr;
	CachedMeleeHoldSequence = nullptr;
	EnsureWeaponInHand();
	EnsureMeleeSwingAnimAssets();
	ApplyMeleeIdlePose();
}

void ASBCharacter::ApplyHeroRaceMaterials()
{
	USkeletalMeshComponent* Hero = GetMesh();
	if (!Hero || !bUsingHeroMesh)
	{
		return;
	}

	// Keep authored Paragon / Countess materials (no flat race tint).
	if (bUsingParagonHeroMesh)
	{
		Hero->EmptyOverrideMaterials();
		Hero->SetOverlayMaterial(nullptr);
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
