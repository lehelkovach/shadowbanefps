// Copyright shadowbanefps.

#include "SBBotController.h"
#include "SBBotScript.h"
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
	bScriptLoaded = false;

	if (InArchetype && !InArchetype->ArchetypeId.IsNone())
	{
		BotScriptId = InArchetype->ArchetypeId;
	}
	else if (BotScriptId.IsNone())
	{
		BotScriptId = FName(TEXT("Default"));
	}

	if (ASBPlayerState* PS = GetPlayerState<ASBPlayerState>())
	{
		PS->SetPlayerName(BotName);
		PS->SetTeam(InTeam);
		if (InArchetype)
		{
			PS->SetSelectedArchetype(InArchetype);
		}
	}

	EnsureScriptLoaded();

	UE_LOG(LogShadowbaneServer, Log, TEXT("Bot configured name=%s team=%s arch=%s script=%s (%d rules)"),
		*BotName,
		*UEnum::GetValueAsString(InTeam),
		InArchetype ? *InArchetype->ArchetypeId.ToString() : TEXT("none"),
		*ActiveScript.ScriptId.ToString(),
		ActiveScript.Rules.Num());
}

void ASBBotController::EnsureScriptLoaded()
{
	if (bScriptLoaded)
	{
		return;
	}
	USBBotScriptLibrary::LoadScriptById(BotScriptId, ActiveScript);
	bScriptLoaded = true;
}

void ASBBotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	EnsureScriptLoaded();
	RetargetCooldown = 0.f;
	FireCooldown = 0.f;
	CurrentTarget = nullptr;
	CurrentDecision = FSBBotDecision();
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

	EnsureScriptLoaded();

	RetargetCooldown -= DeltaSeconds;
	FireCooldown -= DeltaSeconds;

	if (RetargetCooldown <= 0.f || !CurrentTarget.IsValid())
	{
		Think();
		RetargetCooldown = FMath::Max(0.2f, ActiveScript.RetargetSeconds);
	}

	if (CurrentDecision.bHold)
	{
		return;
	}

	if (CurrentDecision.bRetreat)
	{
		if (AActor* Threat = CurrentTarget.Get())
		{
			SteerAwayFrom(Threat->GetActorLocation(), DeltaSeconds);
		}
		return;
	}

	if (AActor* Target = CurrentTarget.Get())
	{
		SteerToward(Target->GetActorLocation(), DeltaSeconds);

		const float DistSq = FVector::DistSquared(Character->GetActorLocation(), Target->GetActorLocation());
		const float Engage = ActiveScript.EngageRange > 0.f ? ActiveScript.EngageRange : 2800.f;
		if (CurrentDecision.bWantFire && DistSq <= FMath::Square(Engage) && FireCooldown <= 0.f)
		{
			TryFire();
			FireCooldown = FMath::Max(0.15f, ActiveScript.FireInterval);
		}
	}
}

void ASBBotController::GatherWorldFacts(FSBBotWorldFacts& OutFacts) const
{
	OutFacts = FSBBotWorldFacts();
	ASBCharacter* Self = GetSBCharacter();
	if (!Self)
	{
		return;
	}

	OutFacts.SelfLocation = Self->GetActorLocation();
	const FVector Origin = OutFacts.SelfLocation;

	float BestEnemyDist = TNumericLimits<float>::Max();
	float BestAllyDist = TNumericLimits<float>::Max();

	for (TActorIterator<ASBCharacter> It(GetWorld()); It; ++It)
	{
		ASBCharacter* Other = *It;
		if (!Other || Other == Self || Other->GetHealth() <= 0.f)
		{
			continue;
		}
		const ASBPlayerState* TheirPS = Other->GetPlayerState<ASBPlayerState>();
		if (!TheirPS || TheirPS->GetTeam() == ESBTeam::Unassigned)
		{
			continue;
		}

		const float Dist = FVector::Dist(Origin, Other->GetActorLocation());
		if (TheirPS->GetTeam() != BotTeam)
		{
			if (Dist < BestEnemyDist)
			{
				BestEnemyDist = Dist;
				OutFacts.NearestEnemy = Other;
				OutFacts.EnemyDistance = Dist;
			}
		}
		else
		{
			const float MaxHp = FMath::Max(1.f, Other->GetMaxHealth());
			if (Other->GetHealth() / MaxHp < 0.65f && Dist < BestAllyDist)
			{
				BestAllyDist = Dist;
				OutFacts.HurtAlly = Other;
				OutFacts.AllyDistance = Dist;
			}
		}
	}

	for (TActorIterator<ASBCapturePoint> It(GetWorld()); It; ++It)
	{
		if (*It && !(*It)->IsCaptured())
		{
			OutFacts.OpenCapture = *It;
			break;
		}
	}

	for (TActorIterator<ASBConquestObjective> It(GetWorld()); It; ++It)
	{
		if (*It)
		{
			OutFacts.Objective = *It;
			break;
		}
	}

	for (TActorIterator<ASBDestructibleStructure> It(GetWorld()); It; ++It)
	{
		if (*It && (*It)->GetState() != ESBStructureState::Destroyed)
		{
			OutFacts.IntactStructure = *It;
			break;
		}
	}
}

void ASBBotController::Think()
{
	CurrentTarget = nullptr;
	CurrentDecision = FSBBotDecision();

	FSBBotWorldFacts Facts;
	GatherWorldFacts(Facts);

	if (!USBBotScriptLibrary::EvaluateRules(ActiveScript, Facts, CurrentDecision))
	{
		return;
	}

	CurrentTarget = CurrentDecision.MoveTarget;
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
	if (Delta.SizeSquared() < FMath::Square(MoveAcceptanceRadius))
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

void ASBBotController::SteerAwayFrom(const FVector& WorldThreat, float DeltaSeconds)
{
	ASBCharacter* Character = GetSBCharacter();
	if (!Character)
	{
		return;
	}

	const FVector Loc = Character->GetActorLocation();
	FVector Delta = Loc - WorldThreat;
	Delta.Z = 0.f;
	if (Delta.SizeSquared() < 1.f)
	{
		Delta = Character->GetActorForwardVector() * -1.f;
		Delta.Z = 0.f;
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
