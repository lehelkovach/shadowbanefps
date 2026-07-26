// Copyright shadowbanefps.

#include "SBBrokenCitadelBuilder.h"
#include "Core/SBSpawnPoint.h"
#include "Siege/SBDestructibleStructure.h"
#include "Siege/SBCapturePoint.h"
#include "Siege/SBConquestObjective.h"
#include "Art/SBWorldMarker.h"
#include "Art/SBPlaceholderArt.h"
#include "Components/StaticMeshComponent.h"
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

	const FLinearColor Ground(0.22f, 0.24f, 0.2f);
	const FLinearColor Wall(0.35f, 0.34f, 0.32f);
	const FLinearColor Keep(0.28f, 0.3f, 0.36f);
	const FLinearColor Path(0.4f, 0.32f, 0.22f);

	// Ground planes: attacker field -> courtyard -> keep
	AddBox(FVector(-3500.f, 0.f, -50.f), FVector(50.f, 50.f, 1.f), Ground, "Ground_SiegeField");
	AddBox(FVector(0.f, 0.f, -50.f), FVector(40.f, 40.f, 1.f), Ground, "Ground_Courtyard");
	AddBox(FVector(2500.f, 0.f, -50.f), FVector(30.f, 30.f, 1.f), Keep, "Ground_Keep");

	// Outer walls (north / south), with gaps for routes
	AddBox(FVector(-800.f, 1800.f, 200.f), FVector(30.f, 2.f, 4.f), Wall, "Wall_North");
	AddBox(FVector(-800.f, -1800.f, 200.f), FVector(30.f, 2.f, 4.f), Wall, "Wall_South");

	// Main curtain walls left/right of gate
	AddBox(FVector(-1000.f, 700.f, 250.f), FVector(2.f, 10.f, 5.f), Wall, "Curtain_GateN");
	AddBox(FVector(-1000.f, -700.f, 250.f), FVector(2.f, 10.f, 5.f), Wall, "Curtain_GateS");

	// Secondary breach wall (weaker north section) — visual only; gameplay structure spawned separately
	AddBox(FVector(-1000.f, 1400.f, 180.f), FVector(2.f, 4.f, 3.5f), FLinearColor(0.45f, 0.3f, 0.25f), "Wall_SecondaryBreach_Visual");

	// Courtyard colonnade / cover
	AddBox(FVector(-200.f, 400.f, 120.f), FVector(2.f, 2.f, 2.5f), Wall, "Cover_N1");
	AddBox(FVector(-200.f, -400.f, 120.f), FVector(2.f, 2.f, 2.5f), Wall, "Cover_S1");
	AddBox(FVector(200.f, 0.f, 120.f), FVector(3.f, 1.5f, 2.5f), Wall, "Cover_Mid");

	// Inner keep shell
	AddBox(FVector(2200.f, 900.f, 250.f), FVector(12.f, 2.f, 5.f), Keep, "Keep_Wall_N");
	AddBox(FVector(2200.f, -900.f, 250.f), FVector(12.f, 2.f, 5.f), Keep, "Keep_Wall_S");
	AddBox(FVector(2800.f, 0.f, 250.f), FVector(2.f, 18.f, 5.f), Keep, "Keep_Wall_Back");

	// Route markers (flat path strips)
	AddBox(FVector(-2200.f, 0.f, 2.f), FVector(20.f, 3.f, 0.1f), Path, "Route_MainGate");
	AddBox(FVector(-2200.f, 1400.f, 2.f), FVector(20.f, 2.f, 0.1f), Path, "Route_NorthWall");
	AddBox(FVector(-2200.f, -1400.f, 2.f), FVector(20.f, 2.f, 0.1f), Path, "Route_SouthService");

	// South service tunnel mouth
	AddBox(FVector(-1200.f, -1400.f, 80.f), FVector(8.f, 2.5f, 1.6f), FLinearColor(0.18f, 0.16f, 0.14f), "ServiceTunnel");

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

	SpawnLocalMarker(FVector(-1000.f, 0.f, 420.f), TEXT("MAIN GATE"), FLinearColor(0.75f, 0.45f, 0.2f), 1.2f);
	SpawnLocalMarker(FVector(-2200.f, 1400.f, 160.f), TEXT("NORTH WALL"), FLinearColor(0.55f, 0.7f, 1.f), 1.f);
	SpawnLocalMarker(FVector(-2200.f, -1400.f, 160.f), TEXT("SERVICE"), FLinearColor(0.7f, 0.5f, 0.9f), 1.f);
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

	// Main gate — destructible
	{
		ASBDestructibleStructure* Gate = GetWorld()->SpawnActor<ASBDestructibleStructure>(
			FVector(-1000.f, 0.f, 200.f), FRotator::ZeroRotator, Params);
		if (Gate)
		{
			Gate->Tags.Add(FName(TEXT("MainGate")));
			if (USceneComponent* Root = Gate->GetRootComponent())
			{
				// Ensure it has a visible/collidable proxy if none exists yet.
			}
		}
	}

	// Secondary breach wall
	{
		ASBDestructibleStructure* Breach = GetWorld()->SpawnActor<ASBDestructibleStructure>(
			FVector(-1000.f, 1400.f, 180.f), FRotator::ZeroRotator, Params);
		if (Breach)
		{
			Breach->Tags.Add(FName(TEXT("SecondaryBreach")));
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
