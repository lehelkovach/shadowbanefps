// Copyright shadowbanefps.

#include "SBSiegeGameMode.h"
#include "SBSiegeGameState.h"
#include "SBPlayerState.h"
#include "SBPlayerController.h"
#include "SBSpawnPoint.h"
#include "SBLog.h"
#include "SBRulesLibrary.h"
#include "SBMatchTelemetry.h"
#include "UI/SBSiegeHUD.h"
#include "Characters/SBCharacter.h"
#include "Characters/SBCharacterArchetype.h"
#include "Characters/SBPilotRoster.h"
#include "Characters/SBCharacterCalculator.h"
#include "AI/SBBotController.h"
#include "Maps/SBBrokenCitadelBuilder.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
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
	UE_LOG(LogShadowbaneServer, Log, TEXT("InitGame map=%s options=%s"), *MapName, *Options);
	UE_LOG(LogShadowbane, Log, TEXT("InitGame map=%s options=%s"), *MapName, *Options);
	ParseLaunchOptions(Options);
	EnsureRoster();
	if (!Telemetry)
	{
		Telemetry = USBMatchTelemetry::Create(this);
	}
}

void ASBSiegeGameMode::ParseLaunchOptions(const FString& Options)
{
	if (UGameplayStatics::HasOption(Options, TEXT("Bots")))
	{
		AutoSpawnBots = FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("Bots")));
	}
	if (UGameplayStatics::HasOption(Options, TEXT("AdminSpectate"))
		|| UGameplayStatics::HasOption(Options, TEXT("Spectator")))
	{
		bForceAdminSpectate = true;
	}

	UE_LOG(LogShadowbaneServer, Log, TEXT("Launch options: AutoSpawnBots=%d AdminSpectate=%d"),
		AutoSpawnBots, bForceAdminSpectate ? 1 : 0);
}

void ASBSiegeGameMode::BeginPlay()
{
	Super::BeginPlay();

	SiegeState = GetGameState<ASBSiegeGameState>();
	EnsureRoster();
	EnsureCitadel();
	StartMatch();
	MaybeSpawnConfiguredBots();
}

void ASBSiegeGameMode::EnsureRoster()
{
	if (Roster.Num() == 0)
	{
		USBPilotRoster::BuildDefaultRoster(this, Roster);
		UE_LOG(LogShadowbane, Log, TEXT("Built default pilot roster (%d archetypes)"), Roster.Num());
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
	UE_LOG(LogShadowbane, Log, TEXT("Spawned BrokenCitadelBuilder for runtime greybox"));
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

	ASBPlayerController* SBPC = Cast<ASBPlayerController>(NewPlayer);
	ASBPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ASBPlayerState>() : nullptr;
	if (!PS)
	{
		return;
	}

	if (bForceAdminSpectate || (SBPC && SBPC->WantsAdminSpectate()))
	{
		if (SBPC)
		{
			SBPC->EnterAdminSpectate();
		}
		UE_LOG(LogShadowbaneServer, Log, TEXT("Admin spectator joined: %s"), *PS->GetPlayerName());
		UE_LOG(LogShadowbaneClient, Log, TEXT("Joined as admin spectator — watch bots populate the siege"));
		return;
	}

	const ESBTeam Team = PickTeamForNewPlayer();
	PS->SetTeam(Team);
	PS->SetAlive(false);

	if (USBCharacterArchetype* DefaultArch = FindDefaultArchetypeForTeam(Team))
	{
		PS->SetSelectedArchetype(DefaultArch);
	}

	UE_LOG(LogShadowbaneServer, Log, TEXT("Player starting: %s -> %s archetype=%s"),
		*PS->GetPlayerName(),
		*UEnum::GetValueAsString(Team),
		*PS->GetSelectedArchetypeId().ToString());
	UE_LOG(LogShadowbane, Log, TEXT("Player starting: %s -> %s archetype=%s"),
		*PS->GetPlayerName(),
		*UEnum::GetValueAsString(Team),
		*PS->GetSelectedArchetypeId().ToString());

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

	if (!Telemetry)
	{
		Telemetry = USBMatchTelemetry::Create(this);
	}
	Telemetry->StartMatchSession();

	RegulationDeadline = GetWorld()->GetTimeSeconds() + RegulationSeconds;
	SiegeState->ServerSetRegulationDeadline(SiegeState->GetServerWorldTimeSeconds() + RegulationSeconds);
	SiegeState->ServerSetPhase(ESBMatchPhase::Recon);
	SiegeState->ServerSetConquestStage(ESBConquestStage::OuterSiege);
	LastLoggedPhase = ESBMatchPhase::Recon;
	Telemetry->RecordPhase(ESBMatchPhase::Recon);

	UE_LOG(LogShadowbaneServer, Log, TEXT("Match started: regulation=%.0fs roster=%d"),
		RegulationSeconds, Roster.Num());
	UE_LOG(LogShadowbane, Log, TEXT("Match started: regulation=%.0fs roster=%d"),
		RegulationSeconds, Roster.Num());

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
		if (Desired != LastLoggedPhase)
		{
			LastLoggedPhase = Desired;
			UE_LOG(LogShadowbaneServer, Log, TEXT("Phase -> %s (elapsed=%.1fs)"),
				*UEnum::GetValueAsString(Desired), Elapsed);
			UE_LOG(LogShadowbane, Log, TEXT("Phase -> %s (elapsed=%.1fs)"),
				*UEnum::GetValueAsString(Desired), Elapsed);
			if (Telemetry)
			{
				Telemetry->RecordPhase(Desired);
			}
		}
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

	const ESBConquestStage Current = SiegeState->GetConquestStage();
	if (!USBRulesLibrary::CanAdvanceConquestStage(Current, NewStage) || NewStage == Current)
	{
		return;
	}

	SiegeState->ServerSetConquestStage(NewStage);
	UE_LOG(LogShadowbaneServer, Log, TEXT("Conquest stage %s -> %s"),
		*UEnum::GetValueAsString(Current),
		*UEnum::GetValueAsString(NewStage));
	UE_LOG(LogShadowbane, Log, TEXT("Conquest stage %s -> %s"),
		*UEnum::GetValueAsString(Current),
		*UEnum::GetValueAsString(NewStage));
	if (Telemetry)
	{
		Telemetry->RecordConquestStage(NewStage);
	}

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

	UE_LOG(LogShadowbaneServer, Log, TEXT("Final objective completed — Attackers win"));
	UE_LOG(LogShadowbane, Log, TEXT("Final objective completed — Attackers win"));
	if (Telemetry)
	{
		Telemetry->Record(ESBTelemetryEvent::FinalObjectiveCompleted);
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

	const float Progress = SiegeState->GetFinalObjectiveProgress();
	if (USBRulesLibrary::ShouldEnterOvertime(Progress) && !bInOvertime)
	{
		bInOvertime = true;
		SiegeState->ServerSetPhase(ESBMatchPhase::Overtime);
		UE_LOG(LogShadowbaneServer, Log, TEXT("Regulation expired with contested objective (%.0f%%) — OVERTIME"),
			Progress * 100.f);
		UE_LOG(LogShadowbane, Log, TEXT("Regulation expired with contested objective (%.0f%%) — OVERTIME"),
			Progress * 100.f);
		if (Telemetry)
		{
			Telemetry->Record(ESBTelemetryEvent::OvertimeStarted,
				FString::Printf(TEXT("progress=%.2f"), Progress));
		}
		return;
	}

	UE_LOG(LogShadowbaneServer, Log, TEXT("Regulation expired — Defenders win"));
	UE_LOG(LogShadowbane, Log, TEXT("Regulation expired — Defenders win"));
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

	UE_LOG(LogShadowbaneServer, Log, TEXT("Match finished: %s"), *UEnum::GetValueAsString(Result));
	UE_LOG(LogShadowbane, Log, TEXT("Match finished: %s"), *UEnum::GetValueAsString(Result));
	if (Telemetry)
	{
		Telemetry->EndMatchSession(Result);
	}

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
		UE_LOG(LogShadowbane, Verbose, TEXT("Archetype select rejected (alive): %s"), *PlayerState->GetPlayerName());
		return false;
	}

	const ESBTeam Team = PlayerState->GetTeam();

	if ((Team == ESBTeam::Attackers && !Archetype->bAttackerEligible)
		|| (Team == ESBTeam::Defenders && !Archetype->bDefenderEligible))
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Archetype %s not eligible for %s"),
			*Archetype->ArchetypeId.ToString(),
			*UEnum::GetValueAsString(Team));
		return false;
	}

	if (!CanTeamUseArchetype(Team, Archetype, PlayerState))
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Archetype %s duplicate limit reached for %s"),
			*Archetype->ArchetypeId.ToString(),
			*UEnum::GetValueAsString(Team));
		return false;
	}

	const FName FromId = PlayerState->GetSelectedArchetypeId();
	PlayerState->SetSelectedArchetype(Archetype);

	UE_LOG(LogShadowbaneServer, Log, TEXT("Archetype switch: %s %s -> %s"),
		*PlayerState->GetPlayerName(),
		*FromId.ToString(),
		*Archetype->ArchetypeId.ToString());
	UE_LOG(LogShadowbane, Log, TEXT("Archetype switch: %s %s -> %s"),
		*PlayerState->GetPlayerName(),
		*FromId.ToString(),
		*Archetype->ArchetypeId.ToString());

	if (Telemetry && FromId != Archetype->ArchetypeId)
	{
		Telemetry->RecordArchetypeSwitch(PlayerState->GetPlayerName(), FromId, Archetype->ArchetypeId);
	}

	return true;
}

void ASBSiegeGameMode::NotifyPlayerKilled(ASBPlayerState* Victim, ASBPlayerState* Killer, FName KillingPowerId)
{
	if (!HasAuthority() || !Victim)
	{
		return;
	}

	Victim->SetAlive(false);

	UE_LOG(LogShadowbaneServer, Log, TEXT("Player killed: victim=%s(%s) killer=%s(%s) power=%s"),
		*Victim->GetPlayerName(),
		*Victim->GetSelectedArchetypeId().ToString(),
		Killer ? *Killer->GetPlayerName() : TEXT("none"),
		Killer ? *Killer->GetSelectedArchetypeId().ToString() : TEXT("none"),
		*KillingPowerId.ToString());
	UE_LOG(LogShadowbane, Log, TEXT("Player killed: victim=%s killer=%s archetype=%s"),
		*Victim->GetPlayerName(),
		Killer ? *Killer->GetPlayerName() : TEXT("none"),
		*Victim->GetSelectedArchetypeId().ToString());

	if (Telemetry)
	{
		Telemetry->RecordPlayerKill(
			Victim->GetPlayerName(),
			Killer ? Killer->GetPlayerName() : TEXT("none"),
			Victim->GetSelectedArchetypeId(),
			Killer ? Killer->GetSelectedArchetypeId() : NAME_None,
			KillingPowerId);
	}

	if (APlayerController* PC = Cast<APlayerController>(Victim->GetOwningController()))
	{
		ScheduleRespawn(PC);
	}
	else if (ASBBotController* Bot = Cast<ASBBotController>(Victim->GetOwningController()))
	{
		ScheduleBotRespawn(Bot);
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

void ASBSiegeGameMode::ScheduleBotRespawn(ASBBotController* Bot)
{
	if (!Bot)
	{
		return;
	}

	TWeakObjectPtr<ASBBotController> WeakBot(Bot);
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([this, WeakBot]()
	{
		if (ASBBotController* AliveBot = WeakBot.Get())
		{
			if (ASBPlayerState* PS = AliveBot->GetPlayerState<ASBPlayerState>())
			{
				PS->SetAlive(true);
			}
			SpawnCharacterForController(AliveBot);
		}
	}), RespawnDelaySeconds, false);
}

bool ASBSiegeGameMode::SpawnPlayerFromController(APlayerController* PC)
{
	return SpawnCharacterForController(PC);
}

bool ASBSiegeGameMode::GetSpawnTransformFor(const ASBPlayerState* PS, FVector& OutLocation, FRotator& OutRotation) const
{
	if (!PS || PS->GetTeam() == ESBTeam::Unassigned)
	{
		return false;
	}

	const USBCharacterArchetype* Archetype = PS->GetSelectedArchetype();
	ASBSpawnPoint* Spot = FindSpawnPoint(PS->GetTeam(), Archetype);
	OutLocation = Spot ? Spot->GetActorLocation() : FVector(-4500.f, 0.f, 120.f);
	OutRotation = Spot ? Spot->GetActorRotation() : FRotator::ZeroRotator;
	return true;
}

bool ASBSiegeGameMode::SpawnCharacterForController(AController* Controller)
{
	if (!HasAuthority() || !Controller)
	{
		return false;
	}

	ASBPlayerState* PS = Controller->GetPlayerState<ASBPlayerState>();
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

	if (APawn* Existing = Controller->GetPawn())
	{
		Controller->UnPossess();
		Existing->Destroy();
	}

	FActorSpawnParameters Params;
	Params.Owner = Controller;
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
	FSBCreationFpsVitals OverlayVitals;
	if (PS->ConsumeCreationVitalsOverlay(OverlayVitals))
	{
		Character->ApplyCreationVitals(OverlayVitals, PS->GetCreationRace());
	}
	Controller->Possess(Character);
	PS->SetAlive(true);

	if (ASBPlayerController* SBPC = Cast<ASBPlayerController>(Controller))
	{
		SBPC->ServerBeginRespawnCountdown(0.f);
	}

	UE_LOG(LogShadowbaneServer, Log, TEXT("Spawned %s as %s at %s (pad=%s)"),
		*PS->GetPlayerName(),
		*Archetype->ArchetypeId.ToString(),
		*Location.ToCompactString(),
		Spot ? *Spot->GetName() : TEXT("fallback"));
	UE_LOG(LogShadowbane, Log, TEXT("Spawned %s as %s at %s (pad=%s)"),
		*PS->GetPlayerName(),
		*Archetype->ArchetypeId.ToString(),
		*Location.ToCompactString(),
		Spot ? *Spot->GetName() : TEXT("fallback"));

	if (Telemetry)
	{
		Telemetry->RecordPlayerSpawn(PS->GetPlayerName(), PS->GetTeam(), Archetype->ArchetypeId);
	}

	return true;
}

void ASBSiegeGameMode::MaybeSpawnConfiguredBots()
{
	if (AutoSpawnBots > 0)
	{
		SpawnBots(AutoSpawnBots);
	}
}

int32 ASBSiegeGameMode::SpawnBots(int32 TotalBots)
{
	if (!HasAuthority())
	{
		return 0;
	}

	EnsureRoster();
	EnsureCitadel();

	int32 Atk = 0;
	int32 Def = 0;
	USBRulesLibrary::SplitBotsAcrossTeams(TotalBots, MaxBotsPerTeam, Atk, Def);

	// 5v5 with human as attacker hero: leave one attacker slot open (Bots=9 → 4 atk + 5 def).
	if (TotalBots == 9 && MaxBotsPerTeam >= 5)
	{
		Atk = 4;
		Def = 5;
	}

	int32 Spawned = 0;
	for (int32 i = 0; i < Atk; ++i)
	{
		if (SpawnOneBot(ESBTeam::Attackers))
		{
			++Spawned;
		}
	}
	for (int32 i = 0; i < Def; ++i)
	{
		if (SpawnOneBot(ESBTeam::Defenders))
		{
			++Spawned;
		}
	}

	UE_LOG(LogShadowbaneServer, Log, TEXT("SpawnBots requested=%d spawned=%d (atk=%d def=%d)"),
		TotalBots, Spawned, Atk, Def);
	return Spawned;
}

ASBBotController* ASBSiegeGameMode::SpawnOneBot(ESBTeam Team)
{
	if (!HasAuthority() || Team == ESBTeam::Unassigned)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASBBotController* Bot = GetWorld()->SpawnActor<ASBBotController>(
		ASBBotController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!Bot)
	{
		return nullptr;
	}

	// Ensure a PlayerState exists for team / archetype / telemetry.
	if (!Bot->PlayerState)
	{
		ASBPlayerState* PS = GetWorld()->SpawnActor<ASBPlayerState>(
			PlayerStateClass ? PlayerStateClass.Get() : ASBPlayerState::StaticClass());
		if (PS)
		{
			Bot->PlayerState = PS;
			PS->SetOwner(Bot);
			if (GameState)
			{
				GameState->AddPlayerState(PS);
			}
		}
	}

	USBCharacterArchetype* Arch = PickBotArchetype(Team);
	const int32 Index = ActiveBots.Num() + 1;
	const FString Name = FString::Printf(TEXT("Bot_%s_%d"),
		Team == ESBTeam::Attackers ? TEXT("Atk") : TEXT("Def"), Index);
	Bot->ConfigureBot(Team, Arch, Name);

	if (!SpawnCharacterForController(Bot))
	{
		UE_LOG(LogShadowbaneServer, Warning, TEXT("Failed to spawn pawn for %s"), *Name);
		Bot->Destroy();
		return nullptr;
	}

	ActiveBots.Add(Bot);
	return Bot;
}

USBCharacterArchetype* ASBSiegeGameMode::PickBotArchetype(ESBTeam Team) const
{
	TArray<USBCharacterArchetype*> Candidates;
	for (USBCharacterArchetype* Arch : Roster)
	{
		if (!Arch)
		{
			continue;
		}
		if ((Team == ESBTeam::Attackers && Arch->bAttackerEligible)
			|| (Team == ESBTeam::Defenders && Arch->bDefenderEligible))
		{
			if (CanTeamUseArchetype(Team, Arch, nullptr))
			{
				Candidates.Add(Arch);
			}
		}
	}
	if (Candidates.Num() == 0)
	{
		return FindDefaultArchetypeForTeam(Team);
	}
	return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
}

bool ASBSiegeGameMode::CanTeamUseArchetype(ESBTeam Team, const USBCharacterArchetype* Archetype, const ASBPlayerState* Ignoring) const
{
	if (!Archetype || !GameState)
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
	return USBRulesLibrary::CanUseArchetypeSlot(Count, Archetype->PerTeamDuplicateLimit);
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
	return USBRulesLibrary::PickBalancedTeam(Attackers, Defenders);
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
