// Copyright shadowbanefps.

#include "SBBrokenCitadelBuilder.h"
#include "Core/SBSpawnPoint.h"
#include "Siege/SBDestructibleStructure.h"
#include "Siege/SBCapturePoint.h"
#include "Siege/SBConquestObjective.h"
#include "Siege/SBSiegeWeapon.h"
#include "Art/SBWorldMarker.h"
#include "Art/SBPlaceholderArt.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ASBBrokenCitadelBuilder::ASBBrokenCitadelBuilder()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		CubeMesh = Cube.Object;
	}
}

void ASBBrokenCitadelBuilder::BeginPlay()
{
	Super::BeginPlay();

	if (!bBuildOnBeginPlay)
	{
		return;
	}

	BuildVisualGeometry();

	if (HasAuthority())
	{
		BuildGameplayActors();
	}
}

UStaticMeshComponent* ASBBrokenCitadelBuilder::AddBox(const FVector& Location, const FVector& Scale, const FLinearColor& Color, const FName& Name)
{
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, Name);
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetStaticMesh(CubeMesh);
	Mesh->SetWorldLocation(Location);
	Mesh->SetWorldScale3D(Scale);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->RegisterComponent();
	USBPlaceholderArt::ApplySolidColor(Mesh, Color);
	return Mesh;
}

void ASBBrokenCitadelBuilder::BuildVisualGeometry()
{
	if (bBuiltVisuals || !CubeMesh)
	{
		return;
	}
	bBuiltVisuals = true;

	// Entry has no placed lights — without these, lit BasicShapes materials render black
	// while the canvas HUD still draws (exactly the "black screen + HUD" symptom).
	{
		UDirectionalLightComponent* Sun = NewObject<UDirectionalLightComponent>(this, TEXT("GreyboxSun"));
		Sun->SetupAttachment(GetRootComponent());
		Sun->SetRelativeRotation(FRotator(-50.f, 30.f, 0.f));
		Sun->Intensity = 10.f;
		Sun->LightColor = FColor(255, 244, 214);
		Sun->CastShadows = false;
		Sun->RegisterComponent();

		USkyLightComponent* Sky = NewObject<USkyLightComponent>(this, TEXT("GreyboxSky"));
		Sky->SetupAttachment(GetRootComponent());
		Sky->SourceType = ESkyLightSourceType::SLS_CapturedScene;
		Sky->SetIntensity(2.f);
		Sky->bLowerHemisphereIsBlack = false;
		Sky->RegisterComponent();
		Sky->RecaptureSky();
	}

	const FLinearColor Ground(0.22f, 0.24f, 0.2f);
	const FLinearColor Wall(0.35f, 0.34f, 0.32f);
	const FLinearColor Keep(0.28f, 0.3f, 0.36f);
	const FLinearColor Path(0.4f, 0.32f, 0.22f);
	const FLinearColor Ruin(0.42f, 0.3f, 0.24f);
	const FLinearColor Beach(0.32f, 0.3f, 0.24f);

	// Continuous playable floor (stops lemming falls into the void).
	AddBox(FVector(-1000.f, 0.f, -80.f), FVector(110.f, 70.f, 1.2f), Beach, "Ground_Continuous");
	AddBox(FVector(-3500.f, 0.f, -50.f), FVector(55.f, 55.f, 1.f), Beach, "Ground_SiegeField");
	AddBox(FVector(0.f, 0.f, -50.f), FVector(40.f, 40.f, 1.f), Ground, "Ground_Courtyard");
	AddBox(FVector(2500.f, 0.f, -50.f), FVector(30.f, 30.f, 1.f), Keep, "Ground_Keep");

	// Invisible perimeter walls (keep bots/players on the pad).
	const FLinearColor Barrier(0.15f, 0.15f, 0.18f);
	AddBox(FVector(-1000.f, 3200.f, 200.f), FVector(110.f, 1.5f, 6.f), Barrier, "Bound_North");
	AddBox(FVector(-1000.f, -3200.f, 200.f), FVector(110.f, 1.5f, 6.f), Barrier, "Bound_South");
	AddBox(FVector(-6500.f, 0.f, 200.f), FVector(1.5f, 70.f, 6.f), Barrier, "Bound_West");
	AddBox(FVector(4500.f, 0.f, 200.f), FVector(1.5f, 70.f, 6.f), Barrier, "Bound_East");

	// Sparse beach rubble (cover, not walls)
	AddBox(FVector(-2800.f, 600.f, 40.f), FVector(2.5f, 1.8f, 0.8f), Ruin, "Rubble_Beach_N1");
	AddBox(FVector(-2600.f, -700.f, 35.f), FVector(2.f, 2.2f, 0.7f), Ruin, "Rubble_Beach_S1");
	AddBox(FVector(-2000.f, 200.f, 45.f), FVector(1.6f, 3.f, 0.9f), Ruin, "Rubble_Beach_Mid");
	AddBox(FVector(-1800.f, -500.f, 30.f), FVector(2.8f, 1.4f, 0.6f), Ruin, "Rubble_Beach_S2");

	// Broken outer curtain — gaps left open so infantry can enter without the gate.
	AddBox(FVector(-1000.f, 1100.f, 160.f), FVector(2.f, 5.f, 3.2f), Wall, "Curtain_GateN_Stub");
	AddBox(FVector(-1000.f, -1100.f, 160.f), FVector(2.f, 5.f, 3.2f), Wall, "Curtain_GateS_Stub");
	// Pre-collapsed north gap (walk-in)
	AddBox(FVector(-1000.f, 1600.f, 60.f), FVector(3.f, 4.f, 1.2f), Ruin, "Ruin_NorthGap");
	// South service already open
	AddBox(FVector(-1100.f, -1500.f, 70.f), FVector(6.f, 2.2f, 1.4f), FLinearColor(0.18f, 0.16f, 0.14f), "ServiceBreach");

	// Low courtyard cover only (no fortress maze)
	AddBox(FVector(-200.f, 450.f, 90.f), FVector(1.8f, 1.8f, 1.8f), Wall, "Cover_N1");
	AddBox(FVector(-200.f, -450.f, 90.f), FVector(1.8f, 1.8f, 1.8f), Wall, "Cover_S1");
	AddBox(FVector(250.f, 0.f, 90.f), FVector(2.5f, 1.2f, 1.8f), Wall, "Cover_Mid");

	// Inner keep — where defense actually concentrates
	AddBox(FVector(2200.f, 900.f, 220.f), FVector(12.f, 2.f, 4.4f), Keep, "Keep_Wall_N");
	AddBox(FVector(2200.f, -900.f, 220.f), FVector(12.f, 2.f, 4.4f), Keep, "Keep_Wall_S");
	AddBox(FVector(2800.f, 0.f, 220.f), FVector(2.f, 18.f, 4.4f), Keep, "Keep_Wall_Back");

	// Route strips
	AddBox(FVector(-2200.f, 0.f, 2.f), FVector(22.f, 4.f, 0.1f), Path, "Route_MainGate");
	AddBox(FVector(-2200.f, 1500.f, 2.f), FVector(22.f, 2.5f, 0.1f), Path, "Route_NorthGap");
	AddBox(FVector(-2200.f, -1500.f, 2.f), FVector(22.f, 2.5f, 0.1f), Path, "Route_Service");

	// Local (non-replicated) dummy world markers — readable icons/runes without Content assets.
	auto SpawnLocalMarker = [this](const FVector& Loc, const TCHAR* Label, const FLinearColor& Color, float Scale)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ASBWorldMarker* Marker = GetWorld()->SpawnActor<ASBWorldMarker>(Loc, FRotator::ZeroRotator, Params))
		{
			Marker->SetReplicates(false);
			Marker->Configure(Label, Color, Scale);
		}
	};

	SpawnLocalMarker(FVector(-1000.f, 0.f, 360.f), TEXT("MAIN GATE"), FLinearColor(0.75f, 0.45f, 0.2f), 1.2f);
	SpawnLocalMarker(FVector(-2200.f, 1500.f, 140.f), TEXT("NORTH GAP"), FLinearColor(0.55f, 0.7f, 1.f), 1.f);
	SpawnLocalMarker(FVector(-2200.f, -1500.f, 140.f), TEXT("SERVICE"), FLinearColor(0.7f, 0.5f, 0.9f), 1.f);
	SpawnLocalMarker(FVector(-2400.f, 0.f, 160.f), TEXT("SIEGE RAM"), FLinearColor(0.9f, 0.55f, 0.2f), 1.1f);
	SpawnLocalMarker(FVector(0.f, 0.f, 180.f), TEXT("COURTYARD"), FLinearColor(0.95f, 0.8f, 0.25f), 1.4f);
	SpawnLocalMarker(FVector(2400.f, 0.f, 200.f), TEXT("KEEP RUNE"), FLinearColor(0.3f, 0.95f, 0.55f), 1.6f);
}

void ASBBrokenCitadelBuilder::SpawnSpawnPoint(const FVector& Location, float Yaw, ESBTeam Team, ESBConquestStage MinStage, ESBConquestStage MaxStage, bool bSiege, const FName& Name)
{
	FActorSpawnParameters Params;
	Params.Name = Name;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASBSpawnPoint* Spot = GetWorld()->SpawnActor<ASBSpawnPoint>(Location, FRotator(0.f, Yaw, 0.f), Params);
	if (!Spot)
	{
		return;
	}

	Spot->Team = Team;
	Spot->MinStage = MinStage;
	Spot->MaxStage = MaxStage;
	Spot->bSiegeDeployment = bSiege;
}

void ASBBrokenCitadelBuilder::BuildGameplayActors()
{
	if (bBuiltGameplay)
	{
		return;
	}
	bBuiltGameplay = true;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Main gate — soft HP so ram + infantry can crack it mid-match (not a forever wall).
	{
		ASBDestructibleStructure* Gate = GetWorld()->SpawnActor<ASBDestructibleStructure>(
			FVector(-1000.f, 0.f, 160.f), FRotator::ZeroRotator, Params);
		if (Gate)
		{
			Gate->Tags.Add(FName(TEXT("MainGate")));
			Gate->ConfigureStructure(520.f, FVector(1.4f, 6.5f, 3.2f));
		}
	}

	// Secondary breach — already weak north rubble plug
	{
		ASBDestructibleStructure* Breach = GetWorld()->SpawnActor<ASBDestructibleStructure>(
			FVector(-1000.f, 1500.f, 100.f), FRotator::ZeroRotator, Params);
		if (Breach)
		{
			Breach->Tags.Add(FName(TEXT("SecondaryBreach")));
			Breach->ConfigureStructure(280.f, FVector(1.2f, 4.f, 2.2f));
		}
	}

	// Attacker battering ram on the open field (crew with Interact / E).
	{
		ASBSiegeWeapon* Ram = GetWorld()->SpawnActor<ASBSiegeWeapon>(
			FVector(-2400.f, 0.f, 60.f), FRotator(0.f, 0.f, 0.f), Params);
		if (Ram)
		{
			Ram->Tags.Add(FName(TEXT("BatteringRam")));
			Ram->TargetStructureTag = FName(TEXT("MainGate"));
			Ram->ConfigureWeapon(650.f, 95.f, 1.25f);
		}
	}

	// Courtyard capture
	{
		ASBCapturePoint* Courtyard = GetWorld()->SpawnActor<ASBCapturePoint>(
			FVector(0.f, 0.f, 50.f), FRotator::ZeroRotator, Params);
		if (Courtyard)
		{
			Courtyard->Tags.Add(FName(TEXT("Courtyard")));
		}
	}

	// Final objective in the keep
	{
		ASBConquestObjective* Objective = GetWorld()->SpawnActor<ASBConquestObjective>(
			FVector(2400.f, 0.f, 50.f), FRotator::ZeroRotator, Params);
		if (Objective)
		{
			Objective->Tags.Add(FName(TEXT("FinalObjective")));
		}
	}

	// Attacker staging spawns (OuterSiege + Courtyard until forward unlock)
	SpawnSpawnPoint(FVector(-4800.f, -400.f, 100.f), 0.f, ESBTeam::Attackers, ESBConquestStage::OuterSiege, ESBConquestStage::OuterSiege, true, "Spawn_Atk_Staging_A");
	SpawnSpawnPoint(FVector(-4800.f, 0.f, 100.f), 0.f, ESBTeam::Attackers, ESBConquestStage::OuterSiege, ESBConquestStage::OuterSiege, false, "Spawn_Atk_Staging_B");
	SpawnSpawnPoint(FVector(-4800.f, 400.f, 100.f), 0.f, ESBTeam::Attackers, ESBConquestStage::OuterSiege, ESBConquestStage::OuterSiege, false, "Spawn_Atk_Staging_C");

	// Attacker forward spawns after courtyard capture
	SpawnSpawnPoint(FVector(-400.f, -300.f, 100.f), 0.f, ESBTeam::Attackers, ESBConquestStage::Courtyard, ESBConquestStage::InnerKeep, false, "Spawn_Atk_Courtyard_A");
	SpawnSpawnPoint(FVector(-400.f, 300.f, 100.f), 0.f, ESBTeam::Attackers, ESBConquestStage::Courtyard, ESBConquestStage::InnerKeep, false, "Spawn_Atk_Courtyard_B");

	// Defender outer wall spawns (fall back as stages advance)
	SpawnSpawnPoint(FVector(-600.f, -500.f, 100.f), 180.f, ESBTeam::Defenders, ESBConquestStage::OuterSiege, ESBConquestStage::OuterSiege, false, "Spawn_Def_Outer_A");
	SpawnSpawnPoint(FVector(-600.f, 500.f, 100.f), 180.f, ESBTeam::Defenders, ESBConquestStage::OuterSiege, ESBConquestStage::OuterSiege, false, "Spawn_Def_Outer_B");
	SpawnSpawnPoint(FVector(-600.f, 0.f, 100.f), 180.f, ESBTeam::Defenders, ESBConquestStage::OuterSiege, ESBConquestStage::OuterSiege, true, "Spawn_Def_Outer_Siege");

	// Defender courtyard fallback
	SpawnSpawnPoint(FVector(600.f, -400.f, 100.f), 180.f, ESBTeam::Defenders, ESBConquestStage::Courtyard, ESBConquestStage::Courtyard, false, "Spawn_Def_Court_A");
	SpawnSpawnPoint(FVector(600.f, 400.f, 100.f), 180.f, ESBTeam::Defenders, ESBConquestStage::Courtyard, ESBConquestStage::Courtyard, false, "Spawn_Def_Court_B");

	// Defender inner keep
	SpawnSpawnPoint(FVector(2100.f, -300.f, 100.f), 180.f, ESBTeam::Defenders, ESBConquestStage::InnerKeep, ESBConquestStage::InnerKeep, false, "Spawn_Def_Keep_A");
	SpawnSpawnPoint(FVector(2100.f, 300.f, 100.f), 180.f, ESBTeam::Defenders, ESBConquestStage::InnerKeep, ESBConquestStage::InnerKeep, true, "Spawn_Def_Keep_B");
}
