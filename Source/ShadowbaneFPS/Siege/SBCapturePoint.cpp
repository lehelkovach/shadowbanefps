// Copyright shadowbanefps.

#include "SBCapturePoint.h"
#include "Core/SBSiegeGameMode.h"
#include "Core/SBPlayerState.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

ASBCapturePoint::ASBCapturePoint()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	SetRootComponent(Zone);
	Zone->SetBoxExtent(FVector(400.f, 400.f, 300.f));
	Zone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Zone->SetGenerateOverlapEvents(true);
}

void ASBCapturePoint::BeginPlay()
{
	Super::BeginPlay();
	// Only the server evaluates capture logic.
	SetActorTickEnabled(HasAuthority());
}

void ASBCapturePoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASBCapturePoint, bCaptured);
	DOREPLIFETIME(ASBCapturePoint, CaptureProgress);
}

void ASBCapturePoint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || bCaptured)
	{
		return;
	}

	int32 Attackers = 0;
	int32 Defenders = 0;
	CountOccupants(Attackers, Defenders);

	// Contested (defenders present) halts progress; uncontested attackers advance
	// it. Empty or defender-only zones slowly decay progress. See design doc §7.
	if (Attackers > 0 && Defenders == 0)
	{
		CaptureProgress = FMath::Min(CaptureSeconds, CaptureProgress + DeltaSeconds);
	}
	else if (Attackers == 0)
	{
		CaptureProgress = FMath::Max(0.f, CaptureProgress - DeltaSeconds * 0.5f);
	}

	if (CaptureProgress >= CaptureSeconds)
	{
		bCaptured = true;
		OnRep_Captured();
		if (ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>())
		{
			GM->AdvanceConquestStage(StageOnCapture);
		}
	}
}

void ASBCapturePoint::CountOccupants(int32& OutAttackers, int32& OutDefenders) const
{
	OutAttackers = 0;
	OutDefenders = 0;

	TArray<AActor*> Overlapping;
	Zone->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		const APawn* Pawn = Cast<APawn>(Actor);
		const ASBPlayerState* PS = Pawn ? Pawn->GetPlayerState<ASBPlayerState>() : nullptr;
		if (!PS || !PS->IsAlive())
		{
			continue;
		}
		if (PS->GetTeam() == ESBTeam::Attackers) { ++OutAttackers; }
		else if (PS->GetTeam() == ESBTeam::Defenders) { ++OutDefenders; }
	}
}

void ASBCapturePoint::OnRep_Captured()
{
	// TODO: play capture VFX/SFX, flip zone banner colours on clients.
}
