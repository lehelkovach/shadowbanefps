// Copyright shadowbanefps.

#include "SBConquestObjective.h"
#include "Core/SBSiegeGameMode.h"
#include "Core/SBSiegeGameState.h"
#include "Core/SBPlayerState.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

ASBConquestObjective::ASBConquestObjective()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	InteractionZone = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionZone"));
	SetRootComponent(InteractionZone);
	InteractionZone->SetBoxExtent(FVector(300.f, 300.f, 250.f));
	InteractionZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionZone->SetGenerateOverlapEvents(true);
}

void ASBConquestObjective::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());
}

void ASBConquestObjective::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASBConquestObjective, Progress);
	DOREPLIFETIME(ASBConquestObjective, bContested);
}

void ASBConquestObjective::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	ASBSiegeGameState* GS = GetWorld()->GetGameState<ASBSiegeGameState>();

	// Gate on reaching the inner keep (design doc §5, §7).
	if (bRequireInnerKeepStage && GS && GS->GetConquestStage() != ESBConquestStage::InnerKeep)
	{
		return;
	}

	int32 Attackers = 0;
	int32 Defenders = 0;
	CountOccupants(Attackers, Defenders);

	bContested = (Attackers > 0 && Defenders > 0);

	if (Attackers > 0 && Defenders == 0)
	{
		Progress = FMath::Min(CompleteSeconds, Progress + DeltaSeconds);
	}
	else if (!bContested)
	{
		// No attackers holding: decay (0 rate == persist in segments).
		Progress = FMath::Max(0.f, Progress - DecayRatePerSecond * DeltaSeconds);
	}
	// While contested, progress holds.

	PublishProgress();

	if (Progress >= CompleteSeconds)
	{
		if (ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>())
		{
			GM->NotifyFinalObjectiveCompleted();
		}
	}
}

void ASBConquestObjective::CountOccupants(int32& OutAttackers, int32& OutDefenders) const
{
	OutAttackers = 0;
	OutDefenders = 0;

	TArray<AActor*> Overlapping;
	InteractionZone->GetOverlappingActors(Overlapping, APawn::StaticClass());
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

void ASBConquestObjective::PublishProgress() const
{
	if (ASBSiegeGameState* GS = GetWorld()->GetGameState<ASBSiegeGameState>())
	{
		GS->ServerSetFinalObjectiveProgress(CompleteSeconds > 0.f ? Progress / CompleteSeconds : 0.f);
	}
}
