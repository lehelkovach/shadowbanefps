// Copyright shadowbanefps.

#include "SBSiegeGameMode.h"
#include "SBSiegeGameState.h"
#include "SBPlayerState.h"
#include "SBPlayerController.h"
#include "SBSpawnPoint.h"
#include "UI/SBSiegeHUD.h"
#include "Characters/SBCharacter.h"
#include "Characters/SBCharacterArchetype.h"
#include "Characters/SBPilotRoster.h"
#include "Maps/SBBrokenCitadelBuilder.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ASBSiegeGameMode::ASBSiegeGameMode()
{
	GameStateClass = ASBSiegeGameState::StaticClass();
	PlayerStateClass = ASBPlayerState::StaticClass();
	PlayerControllerClass = ASBPlayerController::StaticClass();
	DefaultPawnClass = ASBCharacter::StaticClass();
	HUDClass = ASBSiegeHUD::StaticClass();
	bUseSeamlessTravel = true;
}

void ASBSiegeGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	EnsureRoster();
}

void ASBSiegeGameMode::BeginPlay()
{
	Super::BeginPlay();

	SiegeState = GetGameState<ASBSiegeGameState>();
	EnsureRoster();
	EnsureCitadel();
	StartMatch();
}

void ASBSiegeGameMode::EnsureRoster()
{
	if (Roster.Num() == 0)
	{
		USBPilotRoster::BuildDefaultRoster(this, Roster);
	}
}

void ASBSiegeGameMode::EnsureCitadel()
{
	if (!bAutoBuildBrokenCitadel || !HasAuthority())
	{
		return;
	}

	for (TActorIterator<ASBBrokenCitadelBuilder> It(GetWorld()); It; ++It)
	{
		CitadelBuilder = *It;
		return;
	}

	FActorSpawnParameters Params;
	Params.Name = TEXT("BrokenCitadelBuilder");
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CitadelBuilder = GetWorld()->SpawnActor<ASBBrokenCitadelBuilder>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

void ASBSiegeGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

void ASBSiegeGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Players can join before BeginPlay in PIE — make sure map + roster exist.
	EnsureRoster();
	EnsureCitadel();
	if (!SiegeState)
	{
		SiegeState = GetGameState<ASBSiegeGameState>();
	}

	ASBPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ASBPlayerState>() : nullptr;
	if (!PS)
	{
		return;
	}

	PS->SetTeam(PickTeamForNewPlayer());
	PS->SetAlive(false);

	if (USBCharacterArchetype* DefaultArch = FindDefaultArchetypeForTeam(PS->GetTeam()))
	{
		PS->SetSelectedArchetype(DefaultArch);
	}

	// Immediate first spawn for the pilot (lobby selection comes later).
	SpawnPlayerFromController(NewPlayer);
}

void ASBSiegeGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

void ASBSiegeGameMode::StartMatch()
{
	if (!SiegeState)
	{
		return;
	}

	RegulationDeadline = GetWorld()->GetTimeSeconds() + RegulationSeconds;
	SiegeState->ServerSetRegulationDeadline(SiegeState->GetServerWorldTimeSeconds() + RegulationSeconds);
	SiegeState->ServerSetPhase(ESBMatchPhase::Recon);
	SiegeState->ServerSetConquestStage(ESBConquestStage::OuterSiege);

	GetWorldTimerManager().SetTimer(
		MatchTimerHandle, this, &ASBSiegeGameMode::HandleRegulationExpired, RegulationSeconds, false);

	GetWorldTimerManager().SetTimer(
		PhaseTickHandle, this, &ASBSiegeGameMode::TickPhase, 1.0f, true);
}

void ASBSiegeGameMode::TickPhase()
{
	UpdatePhaseForElapsed();
	UpdateInnerKeepStageFromPressure();
}

void ASBSiegeGameMode::UpdatePhaseForElapsed()
{
	if (!SiegeState || SiegeState->GetPhase() == ESBMatchPhase::Finished
		|| SiegeState->GetPhase() == ESBMatchPhase::Overtime)
	{
		return;
	}

	const float Elapsed = RegulationSeconds - SiegeState->GetRemainingRegulationSeconds();

	ESBMatchPhase Desired = ESBMatchPhase::Recon;
	if (Elapsed >= InnerAssaultPhaseElapsedSeconds)
	{
		Desired = ESBMatchPhase::InnerAssault;
	}
	else if (Elapsed >= BreachPhaseElapsedSeconds)
	{
		Desired = ESBMatchPhase::Breach;
	}

	if (Desired != SiegeState->GetPhase())
	{
		SiegeState->ServerSetPhase(Desired);
	}
}

void ASBSiegeGameMode::UpdateInnerKeepStageFromPressure()
{
	if (!SiegeState || SiegeState->GetConquestStage() != ESBConquestStage::Courtyard)
	{
		return;
	}

	// Once courtyard is held, push to InnerKeep when attackers press into the keep.
	for (TActorIterator<ASBCharacter> It(GetWorld()); It; ++It)
	{
		ASBCharacter* Character = *It;
		const ASBPlayerState* PS = Character ? Character->GetPlayerState<ASBPlayerState>() : nullptr;
		if (!PS || PS->GetTeam() != ESBTeam::Attackers || !PS->IsAlive())
		{
			continue;
		}

		if (Character->GetActorLocation().X >= 1800.f)
		{
			AdvanceConquestStage(ESBConquestStage::InnerKeep);
			return;
		}
	}
}

void ASBSiegeGameMode::AdvanceConquestStage(ESBConquestStage NewStage)
{
	if (!HasAuthority() || !SiegeState)
	{
		return;
	}

	if (static_cast<uint8>(NewStage) < static_cast<uint8>(SiegeState->GetConquestStage()))
	{
		return; // Never move the front line backwards in the first pilot.
	}

	SiegeState->ServerSetConquestStage(NewStage);

	if (NewStage == ESBConquestStage::Courtyard && SiegeState->GetPhase() == ESBMatchPhase::Recon)
	{
		SiegeState->ServerSetPhase(ESBMatchPhase::Breach);
	}
	else if (NewStage == ESBConquestStage::InnerKeep && SiegeState->GetPhase() != ESBMatchPhase::Overtime)
	{
		SiegeState->ServerSetPhase(ESBMatchPhase::InnerAssault);
	}
}

void ASBSiegeGameMode::NotifyFinalObjectiveCompleted()
{
	if (!HasAuthority())
	{
		return;
	}

	if (SiegeState)
	{
		SiegeState->ServerSetFinalObjectiveProgress(1.f);
	}
	EndMatch(ESBMatchResult::AttackersWin);
}

void ASBSiegeGameMode::HandleRegulationExpired()
{
	if (!HasAuthority() || !SiegeState)
	{
		return;
	}

	const bool bObjectiveContested = SiegeState->GetFinalObjectiveProgress() > 0.f
		&& SiegeState->GetFinalObjectiveProgress() < 1.f;

	if (bObjectiveContested && !bInOvertime)
	{
		bInOvertime = true;
		SiegeState->ServerSetPhase(ESBMatchPhase::Overtime);
		return;
	}

	EndMatch(ESBMatchResult::DefendersWin);
}

void ASBSiegeGameMode::EndMatch(ESBMatchResult Result)
{
	if (!HasAuthority() || !SiegeState || SiegeState->GetPhase() == ESBMatchPhase::Finished)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(MatchTimerHandle);
	GetWorldTimerManager().ClearTimer(PhaseTickHandle);

	SiegeState->ServerSetResult(Result);
	SiegeState->ServerSetPhase(ESBMatchPhase::Finished);

	// Freeze pawns lightly by disabling input on all controllers.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			PC->SetIgnoreMoveInput(true);
			PC->SetIgnoreLookInput(true);
		}
	}
}

USBCharacterArchetype* ASBSiegeGameMode::GetRosterArchetype(int32 Index) const
{
	return Roster.IsValidIndex(Index) ? Roster[Index].Get() : nullptr;
}

bool ASBSiegeGameMode::RequestSelectArchetype(ASBPlayerState* PlayerState, USBCharacterArchetype* Archetype)
{
	if (!HasAuthority() || !PlayerState || !Archetype)
	{
		return false;
	}

	if (PlayerState->IsAlive())
	{
		return false;
	}

	const ESBTeam Team = PlayerState->GetTeam();

	if ((Team == ESBTeam::Attackers && !Archetype->bAttackerEligible)
		|| (Team == ESBTeam::Defenders && !Archetype->bDefenderEligible))
	{
		return false;
	}

	if (!CanTeamUseArchetype(Team, Archetype, PlayerState))
	{
		return false;
	}

	PlayerState->SetSelectedArchetype(Archetype);
	return true;
}

void ASBSiegeGameMode::NotifyPlayerKilled(ASBPlayerState* Victim, ASBPlayerState* /*Killer*/)
{
	if (!HasAuthority() || !Victim)
	{
		return;
	}

	Victim->SetAlive(false);

	if (APlayerController* PC = Cast<APlayerController>(Victim->GetOwningController()))
	{
		ScheduleRespawn(PC);
	}
}

void ASBSiegeGameMode::ScheduleRespawn(APlayerController* PC)
{
	ASBPlayerController* SBPC = Cast<ASBPlayerController>(PC);
	if (!SBPC)
	{
		return;
	}

	SBPC->ServerBeginRespawnCountdown(RespawnDelaySeconds);

	// Auto-respawn after the delay so solo PIE testing stays smooth.
	FTimerHandle AutoRespawnHandle;
	TWeakObjectPtr<ASBPlayerController> WeakPC(SBPC);
	GetWorldTimerManager().SetTimer(AutoRespawnHandle, FTimerDelegate::CreateLambda([this, WeakPC]()
	{
		if (ASBPlayerController* AlivePC = WeakPC.Get())
		{
			SpawnPlayerFromController(AlivePC);
			AlivePC->ServerBeginRespawnCountdown(0.f);
		}
	}), RespawnDelaySeconds, false);
}

bool ASBSiegeGameMode::SpawnPlayerFromController(APlayerController* PC)
{
	if (!HasAuthority() || !PC)
	{
		return false;
	}

	ASBPlayerState* PS = PC->GetPlayerState<ASBPlayerState>();
	if (!PS || PS->GetTeam() == ESBTeam::Unassigned)
	{
		return false;
	}

	USBCharacterArchetype* Archetype = PS->GetSelectedArchetype();
	if (!Archetype)
	{
		Archetype = FindDefaultArchetypeForTeam(PS->GetTeam());
		if (!Archetype)
		{
			return false;
		}
		PS->SetSelectedArchetype(Archetype);
	}

	ASBSpawnPoint* Spot = FindSpawnPoint(PS->GetTeam(), Archetype);
	const FVector Location = Spot ? Spot->GetActorLocation() : FVector(-4500.f, 0.f, 120.f);
	const FRotator Rotation = Spot ? Spot->GetActorRotation() : FRotator::ZeroRotator;

	if (APawn* Existing = PC->GetPawn())
	{
		PC->UnPossess();
		Existing->Destroy();
	}

	FActorSpawnParameters Params;
	Params.Owner = PC;
	Params.Instigator = nullptr;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UClass* PawnClass = DefaultPawnClass;
	if (!Archetype->PawnClass.IsNull())
	{
		if (UClass* Loaded = Archetype->PawnClass.LoadSynchronous())
		{
			PawnClass = Loaded;
		}
	}

	ASBCharacter* Character = GetWorld()->SpawnActor<ASBCharacter>(PawnClass, Location, Rotation, Params);
	if (!Character)
	{
		return false;
	}

	Character->ApplyArchetype(Archetype);
	PC->Possess(Character);
	PS->SetAlive(true);

	if (ASBPlayerController* SBPC = Cast<ASBPlayerController>(PC))
	{
		SBPC->ServerBeginRespawnCountdown(0.f);
	}

	return true;
}

bool ASBSiegeGameMode::CanTeamUseArchetype(ESBTeam Team, const USBCharacterArchetype* Archetype, const ASBPlayerState* Ignoring) const
{
	if (!Archetype || Archetype->PerTeamDuplicateLimit <= 0 || !GameState)
	{
		return true;
	}

	int32 Count = 0;
	for (APlayerState* Base : GameState->PlayerArray)
	{
		const ASBPlayerState* PS = Cast<ASBPlayerState>(Base);
		if (!PS || PS == Ignoring || PS->GetTeam() != Team)
		{
			continue;
		}
		if (PS->GetSelectedArchetypeId() == Archetype->ArchetypeId)
		{
			++Count;
		}
	}
	return Count < Archetype->PerTeamDuplicateLimit;
}

ESBTeam ASBSiegeGameMode::PickTeamForNewPlayer() const
{
	int32 Attackers = 0;
	int32 Defenders = 0;
	if (GameState)
	{
		for (APlayerState* Base : GameState->PlayerArray)
		{
			if (const ASBPlayerState* PS = Cast<ASBPlayerState>(Base))
			{
				if (PS->GetTeam() == ESBTeam::Attackers) { ++Attackers; }
				else if (PS->GetTeam() == ESBTeam::Defenders) { ++Defenders; }
			}
		}
	}
	return (Attackers <= Defenders) ? ESBTeam::Attackers : ESBTeam::Defenders;
}

USBCharacterArchetype* ASBSiegeGameMode::FindDefaultArchetypeForTeam(ESBTeam Team) const
{
	for (USBCharacterArchetype* Arch : Roster)
	{
		if (!Arch)
		{
			continue;
		}
		if (Team == ESBTeam::Attackers && Arch->bAttackerEligible)
		{
			return Arch;
		}
		if (Team == ESBTeam::Defenders && Arch->bDefenderEligible)
		{
			return Arch;
		}
	}
	return Roster.Num() > 0 ? Roster[0].Get() : nullptr;
}

ASBSpawnPoint* ASBSiegeGameMode::FindSpawnPoint(ESBTeam Team, const USBCharacterArchetype* Archetype) const
{
	const ESBConquestStage Stage = SiegeState ? SiegeState->GetConquestStage() : ESBConquestStage::OuterSiege;

	TArray<ASBSpawnPoint*> Candidates;
	for (TActorIterator<ASBSpawnPoint> It(GetWorld()); It; ++It)
	{
		ASBSpawnPoint* Spot = *It;
		if (Spot && Spot->IsAvailableFor(Team, Stage))
		{
			Candidates.Add(Spot);
		}
	}

	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	// Prefer siege-tagged pads for high-siege archetypes.
	if (Archetype && Archetype->RoleProfile.Siege >= 3)
	{
		for (ASBSpawnPoint* Spot : Candidates)
		{
			if (Spot->bSiegeDeployment)
			{
				return Spot;
			}
		}
	}

	const int32 Index = FMath::RandRange(0, Candidates.Num() - 1);
	return Candidates[Index];
}
