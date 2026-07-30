// Copyright shadowbanefps.

#include "SBBotController.h"
#include "Characters/SBCharacter.h"
#include "Characters/SBCharacterArchetype.h"
#include "Core/SBPlayerState.h"
#include "Core/SBLog.h"
#include "Siege/SBCapturePoint.h"
#include "Siege/SBConquestObjective.h"
#include "Siege/SBDestructibleStructure.h"
#include "EngineUtils.h"

ASBBotController::ASBBotController()
{
	// GameMode assigns ASBPlayerState explicitly so team/archetype exist before spawn.
	bWantsPlayerState = false;
	PrimaryActorTick.bCanEverTick = true;
}

void ASBBotController::ConfigureBot(ESBTeam InTeam, USBCharacterArchetype* InArchetype, const FString& BotName)
{
	BotTeam = InTeam;
	PreferredArchetype = InArchetype;

	if (ASBPlayerState* PS = GetPlayerState<ASBPlayerState>())
	{
		PS->SetPlayerName(BotName);
		PS->SetTeam(InTeam);
		if (InArchetype)
		{
			PS->SetSelectedArchetype(InArchetype);
		}
	}

	UE_LOG(LogShadowbaneServer, Log, TEXT("Bot configured name=%s team=%s arch=%s"),
		*BotName,
		*UEnum::GetValueAsString(InTeam),
		InArchetype ? *InArchetype->ArchetypeId.ToString() : TEXT("none"));
}

void ASBBotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RetargetCooldown = 0.f;
	FireCooldown = 0.f;
	CurrentTarget = nullptr;
}

void ASBBotController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	ASBCharacter* Character = GetSBCharacter();
	if (!Character || Character->GetHealth() <= 0.f)
	{
		return;
	}

	RetargetCooldown -= DeltaSeconds;
	FireCooldown -= DeltaSeconds;

	if (RetargetCooldown <= 0.f || !CurrentTarget.IsValid())
	{
		PickTarget();
		RetargetCooldown = RetargetSeconds;
	}

	if (AActor* Target = CurrentTarget.Get())
	{
		SteerToward(Target->GetActorLocation(), DeltaSeconds);

		const float DistSq = FVector::DistSquared(Character->GetActorLocation(), Target->GetActorLocation());
		if (DistSq <= FMath::Square(EngageRange) && FireCooldown <= 0.f)
		{
			TryFire();
			FireCooldown = FireInterval;
		}
	}
}

void ASBBotController::PickTarget()
{
	CurrentTarget = nullptr;
	ASBCharacter* Self = GetSBCharacter();
	if (!Self)
	{
		return;
	}

	const FVector Origin = Self->GetActorLocation();

	// 1) Nearest living enemy pawn
	float BestEnemyDist = TNumericLimits<float>::Max();
	ASBCharacter* BestEnemy = nullptr;
	for (TActorIterator<ASBCharacter> It(GetWorld()); It; ++It)
	{
		ASBCharacter* Other = *It;
		if (!Other || Other == Self || Other->GetHealth() <= 0.f)
		{
			continue;
		}
		const ASBPlayerState* TheirPS = Other->GetPlayerState<ASBPlayerState>();
		if (!TheirPS || TheirPS->GetTeam() == BotTeam || TheirPS->GetTeam() == ESBTeam::Unassigned)
		{
			continue;
		}
		const float Dist = FVector::DistSquared(Origin, Other->GetActorLocation());
		if (Dist < BestEnemyDist)
		{
			BestEnemyDist = Dist;
			BestEnemy = Other;
		}
	}
	if (BestEnemy && BestEnemyDist < FMath::Square(EngageRange * 1.35f))
	{
		CurrentTarget = BestEnemy;
		return;
	}

	// 2) Attackers: capture / objective / structures. Defenders: hold objective or repair-ish structure.
	if (BotTeam == ESBTeam::Attackers)
	{
		for (TActorIterator<ASBCapturePoint> It(GetWorld()); It; ++It)
		{
			if (*It && !(*It)->IsCaptured())
			{
				CurrentTarget = *It;
				return;
			}
		}
		for (TActorIterator<ASBConquestObjective> It(GetWorld()); It; ++It)
		{
			CurrentTarget = *It;
			return;
		}
		for (TActorIterator<ASBDestructibleStructure> It(GetWorld()); It; ++It)
		{
			if (*It && (*It)->GetState() != ESBStructureState::Destroyed)
			{
				CurrentTarget = *It;
				return;
			}
		}
	}
	else
	{
		for (TActorIterator<ASBConquestObjective> It(GetWorld()); It; ++It)
		{
			CurrentTarget = *It;
			return;
		}
		for (TActorIterator<ASBCapturePoint> It(GetWorld()); It; ++It)
		{
			if (*It && !(*It)->IsCaptured())
			{
				CurrentTarget = *It;
				return;
			}
		}
	}

	if (BestEnemy)
	{
		CurrentTarget = BestEnemy;
	}
}

void ASBBotController::SteerToward(const FVector& WorldTarget, float DeltaSeconds)
{
	ASBCharacter* Character = GetSBCharacter();
	if (!Character)
	{
		return;
	}

	const FVector Loc = Character->GetActorLocation();
	FVector Delta = WorldTarget - Loc;
	Delta.Z = 0.f;
	if (Delta.SizeSquared() < 100.f)
	{
		return;
	}

	const FVector Dir = Delta.GetSafeNormal();
	Character->AddMovementInput(Dir, 1.f);

	const FRotator Desired = Dir.Rotation();
	const FRotator NewRot = FMath::RInterpTo(Character->GetActorRotation(), Desired, DeltaSeconds, 8.f);
	Character->SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
	SetControlRotation(FRotator(0.f, NewRot.Yaw, 0.f));
}

void ASBBotController::TryFire()
{
	if (ASBCharacter* Character = GetSBCharacter())
	{
		Character->BotFire();
	}
}

ASBCharacter* ASBBotController::GetSBCharacter() const
{
	return Cast<ASBCharacter>(GetPawn());
}
