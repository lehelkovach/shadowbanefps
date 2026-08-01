// Copyright shadowbanefps.

#include "SBPlayerController.h"
#include "SBSiegeGameMode.h"
#include "SBSiegeGameState.h"
#include "SBPlayerState.h"
#include "Characters/SBCharacter.h"
#include "Characters/SBCharacterArchetype.h"
#include "Characters/SBCharacterCalculator.h"
#include "SBLog.h"
#include "SBClientDebug.h"
#include "SBRulesLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerInput.h"

ASBPlayerController::ASBPlayerController()
{
	bReplicates = true;
}

void ASBPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASBPlayerController, RespawnTimeRemaining);
	DOREPLIFETIME(ASBPlayerController, bCanRespawn);
	DOREPLIFETIME(ASBPlayerController, bAdminSpectator);
}

void ASBPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && !bAdminSpectator)
	{
		bShowMouseCursor = false;
		FInputModeGameOnly Mode;
		SetInputMode(Mode);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const ENetMode Mode = World->GetNetMode();
	const FURL& URL = World->URL;
	if (Mode == NM_Client)
	{
		const FString Host = URL.Host.IsEmpty()
			? TEXT("(host pending)")
			: FString::Printf(TEXT("%s:%d"), *URL.Host, URL.Port > 0 ? URL.Port : 7777);
		FSBClientDebug::PushMessage(FString::Printf(TEXT("Connecting to %s"), *Host), 8.f);
		UE_LOG(LogShadowbaneClient, Log, TEXT("Client BeginPlay -> connecting to %s"), *Host);
		UE_LOG(LogShadowbaneNet, Log, TEXT("Client session URL=%s"), *URL.ToString());
	}
	else if (Mode == NM_ListenServer)
	{
		FSBClientDebug::PushMessage(TEXT("Listen server ready (local host)"), 6.f);
		UE_LOG(LogShadowbaneClient, Log, TEXT("Listen BeginPlay — local host session"));
	}
	else if (Mode == NM_Standalone)
	{
		FSBClientDebug::PushMessage(TEXT("Local PIE / standalone session"), 5.f);
		UE_LOG(LogShadowbaneClient, Log, TEXT("Standalone BeginPlay — local session"));
	}

	if (FSBClientDebug::IsEnabled())
	{
		FSBClientDebug::PushMessage(
			FSBClientDebug::IsVerbose() ? TEXT("SBVerbose HUD enabled") : TEXT("SBDebug HUD enabled"),
			5.f);
	}
}

void ASBPlayerController::EnterAdminSpectate()
{
	bAdminSpectator = true;
	bWantAdminSpectate = true;

	if (APawn* Existing = GetPawn())
	{
		UnPossess();
		Existing->Destroy();
	}

	ChangeState(NAME_Spectating);
	ClientGotoState(NAME_Spectating);

	UE_LOG(LogShadowbaneClient, Log, TEXT("Admin spectate enabled — free cam; use AddBots N in console if needed"));
	UE_LOG(LogShadowbaneServer, Log, TEXT("Admin spectate for %s"),
		GetPlayerState<APlayerState>() ? *GetPlayerState<APlayerState>()->GetPlayerName() : TEXT("?"));
}

void ASBPlayerController::AddBots(int32 Count)
{
	if (ASBSiegeGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASBSiegeGameMode>() : nullptr)
	{
		const int32 Spawned = GM->SpawnBots(FMath::Clamp(Count, 0, 20));
		UE_LOG(LogShadowbaneServer, Log, TEXT("AddBots console: requested=%d spawned=%d"), Count, Spawned);
	}
	else
	{
		UE_LOG(LogShadowbaneServer, Warning, TEXT("AddBots: no auth GameMode (listen/dedicated only)"));
	}
}

void ASBPlayerController::AdminSpectate()
{
	EnterAdminSpectate();
}

void ASBPlayerController::SBDebugMsg(const FString& Message)
{
	FSBClientDebug::PushMessage(Message.IsEmpty() ? TEXT("(empty)") : Message, 8.f);
	UE_LOG(LogShadowbaneClient, Log, TEXT("SBDebugMsg: %s"), *Message);
}

void ASBPlayerController::SpawnTestBots(int32 Count)
{
	AddBots(Count);
	FSBClientDebug::PushMessage(FString::Printf(TEXT("SpawnTestBots %d"), Count), 5.f);
}

void ASBPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Alive: 1-4 cast. Dead: 1-0 pick archetype. R = respawn. B = recall. W = move.
	InputComponent->BindAction(TEXT("SelectArchetype1"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot0);
	InputComponent->BindAction(TEXT("SelectArchetype2"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot1);
	InputComponent->BindAction(TEXT("SelectArchetype3"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot2);
	InputComponent->BindAction(TEXT("SelectArchetype4"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot3);
	InputComponent->BindAction(TEXT("SelectArchetype5"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot4);
	InputComponent->BindAction(TEXT("SelectArchetype6"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot5);
	InputComponent->BindAction(TEXT("SelectArchetype7"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot6);
	InputComponent->BindAction(TEXT("SelectArchetype8"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot7);
	InputComponent->BindAction(TEXT("SelectArchetype9"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot8);
	InputComponent->BindAction(TEXT("SelectArchetype0"), IE_Pressed, this, &ASBPlayerController::SelectArchetypeSlot9);
	InputComponent->BindAction(TEXT("Respawn"), IE_Pressed, this, &ASBPlayerController::RequestRespawnPressed);
	InputComponent->BindAction(TEXT("Recall"), IE_Pressed, this, &ASBPlayerController::RecallPressed);
	InputComponent->BindAction(TEXT("SpawnTestBots"), IE_Pressed, this, &ASBPlayerController::SpawnTestBotsPressed);
	InputComponent->BindAction(TEXT("CreationBuilder"), IE_Pressed, this, &ASBPlayerController::ToggleCreationBuilder);
	InputComponent->BindAction(TEXT("BuilderRaceNext"), IE_Pressed, this, &ASBPlayerController::BuilderCycleRaceNext);
	InputComponent->BindAction(TEXT("BuilderRacePrev"), IE_Pressed, this, &ASBPlayerController::BuilderCycleRacePrev);
	InputComponent->BindAction(TEXT("BuilderBaseNext"), IE_Pressed, this, &ASBPlayerController::BuilderCycleBaseNext);
	InputComponent->BindAction(TEXT("BuilderBasePrev"), IE_Pressed, this, &ASBPlayerController::BuilderCycleBasePrev);
	InputComponent->BindAction(TEXT("BuilderPrestigeNext"), IE_Pressed, this, &ASBPlayerController::BuilderCyclePrestigeNext);
	InputComponent->BindAction(TEXT("BuilderPrestigePrev"), IE_Pressed, this, &ASBPlayerController::BuilderCyclePrestigePrev);
	InputComponent->BindAction(TEXT("BuilderDisciplineNext"), IE_Pressed, this, &ASBPlayerController::BuilderCycleDisciplineNext);
	InputComponent->BindAction(TEXT("BuilderDisciplinePrev"), IE_Pressed, this, &ASBPlayerController::BuilderCycleDisciplinePrev);
	InputComponent->BindAction(TEXT("BuilderConfirm"), IE_Pressed, this, &ASBPlayerController::BuilderConfirm);
}

FSBCharacterBuildState ASBPlayerController::GetCreationBuild() const
{
	FSBCharacterBuildState Out = CreationBuild;
	Out.HeroName = CreationHeroName;
	return Out;
}

FString ASBPlayerController::SanitizeHeroName(const FString& InName)
{
	FString Clean;
	Clean.Reserve(InName.Len());
	for (const TCHAR Ch : InName)
	{
		if (FChar::IsAlnum(Ch) || Ch == TEXT('_') || Ch == TEXT('-') || Ch == TEXT(' '))
		{
			Clean.AppendChar(Ch);
		}
	}
	Clean = Clean.TrimStartAndEnd();
	if (Clean.IsEmpty())
	{
		Clean = TEXT("Hero");
	}
	if (Clean.Len() > 24)
	{
		Clean = Clean.Left(24);
	}
	return Clean;
}

bool ASBPlayerController::InputKey(const FInputKeyParams& Params)
{
	if (bCreationBuilderOpen && Params.Event == IE_Pressed && IsLocalController())
	{
		const FKey Key = Params.Key;

		if (!bHeroNameEditing && Key == EKeys::N)
		{
			BuilderToggleNameEdit();
			return true;
		}

		if (bHeroNameEditing)
		{
			if (Key == EKeys::Escape || Key == EKeys::Enter)
			{
				bHeroNameEditing = false;
				SetCreationHeroName(CreationHeroName);
				ServerSetHeroName(CreationHeroName);
				FSBClientDebug::PushMessage(FString::Printf(TEXT("Name locked: %s"), *CreationHeroName), 3.f);
				return true;
			}
			if (Key == EKeys::BackSpace)
			{
				if (CreationHeroName.Len() > 0)
				{
					CreationHeroName.LeftChopInline(1);
				}
				return true;
			}

			const FString KeyName = Key.GetDisplayName().ToString();
			if (KeyName.Len() == 1)
			{
				const TCHAR Ch = KeyName[0];
				if (FChar::IsAlnum(Ch) || Ch == TEXT('_') || Ch == TEXT('-'))
				{
					if (CreationHeroName.Len() < 24)
					{
						CreationHeroName.AppendChar(Ch);
					}
					return true;
				}
			}
			if (Key == EKeys::SpaceBar && CreationHeroName.Len() < 24)
			{
				CreationHeroName.AppendChar(TEXT(' '));
				return true;
			}
			// Swallow other keys while editing so builder cycle binds don't fire.
			return true;
		}
	}

	return Super::InputKey(Params);
}

void ASBPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (HasAuthority() && RespawnTimeRemaining > 0.f)
	{
		RespawnTimeRemaining = FMath::Max(0.f, RespawnTimeRemaining - DeltaTime);
		if (RespawnTimeRemaining <= 0.f)
		{
			bCanRespawn = true;
		}
	}
}

void ASBPlayerController::ServerBeginRespawnCountdown(float DelaySeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	RespawnTimeRemaining = FMath::Max(0.f, DelaySeconds);
	// Enabled only when a real countdown finishes; 0 means "clear / just spawned".
	bCanRespawn = false;
}

void ASBPlayerController::ServerSelectArchetypeByIndex_Implementation(int32 RosterIndex)
{
	UE_LOG(LogShadowbaneNet, Log, TEXT("ServerSelectArchetypeByIndex index=%d"), RosterIndex);

	ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>();
	ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	if (!GM || !PS)
	{
		return;
	}

	if (!USBRulesLibrary::CanSelectArchetypeWhileDead(PS->IsAlive()))
	{
		UE_LOG(LogShadowbaneServer, Verbose, TEXT("Archetype RPC rejected (alive): %s"), *PS->GetPlayerName());
		return;
	}

	USBCharacterArchetype* Archetype = GM->GetRosterArchetype(RosterIndex);
	if (!Archetype)
	{
		UE_LOG(LogShadowbaneServer, Warning, TEXT("Archetype RPC bad index=%d"), RosterIndex);
		return;
	}

	GM->RequestSelectArchetype(PS, Archetype);
}

void ASBPlayerController::ServerRequestRespawn_Implementation()
{
	UE_LOG(LogShadowbaneNet, Log, TEXT("ServerRequestRespawn can=%d remaining=%.2f"),
		bCanRespawn ? 1 : 0, RespawnTimeRemaining);

	if (!USBRulesLibrary::CanRequestRespawn(bCanRespawn, RespawnTimeRemaining))
	{
		return;
	}

	if (ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>())
	{
		if (GM->SpawnPlayerFromController(this))
		{
			bCanRespawn = false;
			RespawnTimeRemaining = 0.f;
			UE_LOG(LogShadowbaneServer, Log, TEXT("Respawn granted for %s"),
				GetPlayerState<ASBPlayerState>() ? *GetPlayerState<ASBPlayerState>()->GetPlayerName() : TEXT("?"));
		}
	}
}

void ASBPlayerController::SelectArchetypeSlot0() { TryCastOrSelectArchetype(0); }
void ASBPlayerController::SelectArchetypeSlot1() { TryCastOrSelectArchetype(1); }
void ASBPlayerController::SelectArchetypeSlot2() { TryCastOrSelectArchetype(2); }
void ASBPlayerController::SelectArchetypeSlot3() { TryCastOrSelectArchetype(3); }
void ASBPlayerController::SelectArchetypeSlot4() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 4")); ServerSelectArchetypeByIndex(4); }
void ASBPlayerController::SelectArchetypeSlot5() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 5")); ServerSelectArchetypeByIndex(5); }
void ASBPlayerController::SelectArchetypeSlot6() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 6")); ServerSelectArchetypeByIndex(6); }
void ASBPlayerController::SelectArchetypeSlot7() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 7")); ServerSelectArchetypeByIndex(7); }
void ASBPlayerController::SelectArchetypeSlot8() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 8")); ServerSelectArchetypeByIndex(8); }
void ASBPlayerController::SelectArchetypeSlot9() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 9")); ServerSelectArchetypeByIndex(9); }

void ASBPlayerController::TryCastOrSelectArchetype(int32 SlotIndex)
{
	const ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	if (PS && PS->IsAlive() && SlotIndex >= 0 && SlotIndex <= 3)
	{
		RequestCastSpell(SlotIndex);
		return;
	}
	UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot %d"), SlotIndex);
	ServerSelectArchetypeByIndex(SlotIndex);
}

void ASBPlayerController::RequestCastSpell(int32 SlotIndex)
{
	const ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	if (!PS || !PS->IsAlive())
	{
		return;
	}

	if (ASBCharacter* SBChar = Cast<ASBCharacter>(GetPawn()))
	{
		UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client cast spell slot %d"), SlotIndex);
		SBChar->ServerCastSpell(SlotIndex);
	}
}

void ASBPlayerController::RecallPressed()
{
	const ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	if (!PS || !PS->IsAlive())
	{
		return;
	}

	if (ASBCharacter* SBChar = Cast<ASBCharacter>(GetPawn()))
	{
		UE_LOG(LogShadowbaneClient, Log, TEXT("Client recall pressed"));
		SBChar->ServerStartRecall();
	}
}

void ASBPlayerController::RequestRespawnPressed()
{
	UE_LOG(LogShadowbaneClient, Log, TEXT("Client respawn pressed (can=%d remaining=%.2f)"),
		bCanRespawn ? 1 : 0, RespawnTimeRemaining);
	ServerRequestRespawn();
}

void ASBPlayerController::SpawnTestBotsPressed()
{
	SpawnTestBots(6);
}

void ASBPlayerController::EnsureCreationBuildInitialized()
{
	if (CreationBuild.Race.IsEmpty())
	{
		CreationBuild.Race = TEXT("Human");
		CreationBuild.BaseClass = TEXT("Fighter");
		CreationBuild.Prestige = TEXT("Warrior");
	}
	CreationBuild.HeroName = CreationHeroName;
	USBCharacterCalculator::Recalculate(CreationBuild);
}

void ASBPlayerController::ToggleCreationBuilder()
{
	const ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	if (PS && PS->IsAlive())
	{
		FSBClientDebug::PushMessage(TEXT("Creation builder: available while dead"), 3.f);
		return;
	}

	bCreationBuilderOpen = !bCreationBuilderOpen;
	if (bCreationBuilderOpen)
	{
		EnsureCreationBuildInitialized();
		if (CreationHeroName.IsEmpty() && PS)
		{
			CreationHeroName = PS->GetPlayerName();
		}
		if (CreationHeroName.IsEmpty())
		{
			CreationHeroName = TEXT("Hero");
		}
		bHeroNameEditing = false;
		FSBClientDebug::PushMessage(TEXT("Builder ON: N name  [ ] race  , . path  - = prestige  Enter apply"), 5.f);
	}
	else
	{
		bHeroNameEditing = false;
		FSBClientDebug::PushMessage(TEXT("Creation builder OFF"), 2.f);
	}
}

void ASBPlayerController::BuilderToggleNameEdit()
{
	if (!bCreationBuilderOpen)
	{
		return;
	}
	bHeroNameEditing = !bHeroNameEditing;
	if (bHeroNameEditing)
	{
		FSBClientDebug::PushMessage(TEXT("Type hero name (Backspace / Enter to lock)"), 4.f);
	}
	else
	{
		SetCreationHeroName(CreationHeroName);
		ServerSetHeroName(CreationHeroName);
		FSBClientDebug::PushMessage(FString::Printf(TEXT("Name locked: %s"), *CreationHeroName), 3.f);
	}
}

void ASBPlayerController::SetCreationHeroName(const FString& InName)
{
	CreationHeroName = SanitizeHeroName(InName);
	CreationBuild.HeroName = CreationHeroName;
}

void ASBPlayerController::SBName(const FString& NewName)
{
	SetCreationHeroName(NewName);
	ServerSetHeroName(CreationHeroName);
	FSBClientDebug::PushMessage(FString::Printf(TEXT("Hero name -> %s"), *CreationHeroName), 4.f);
}

void ASBPlayerController::ServerSetHeroName_Implementation(const FString& HeroName)
{
	ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	if (!PS)
	{
		return;
	}
	const FString Clean = SanitizeHeroName(HeroName);
	PS->SetPlayerName(Clean);
	UE_LOG(LogShadowbaneServer, Log, TEXT("Hero name set: %s"), *Clean);
}

void ASBPlayerController::BuilderCycleRaceNext()
{
	if (!bCreationBuilderOpen) { return; }
	EnsureCreationBuildInitialized();
	USBCharacterCalculator::CycleRace(CreationBuild, +1);
}

void ASBPlayerController::BuilderCycleRacePrev()
{
	if (!bCreationBuilderOpen) { return; }
	EnsureCreationBuildInitialized();
	USBCharacterCalculator::CycleRace(CreationBuild, -1);
}

void ASBPlayerController::BuilderCycleBaseNext()
{
	if (!bCreationBuilderOpen) { return; }
	EnsureCreationBuildInitialized();
	USBCharacterCalculator::CycleBaseClass(CreationBuild, +1);
}

void ASBPlayerController::BuilderCycleBasePrev()
{
	if (!bCreationBuilderOpen) { return; }
	EnsureCreationBuildInitialized();
	USBCharacterCalculator::CycleBaseClass(CreationBuild, -1);
}

void ASBPlayerController::BuilderCyclePrestigeNext()
{
	if (!bCreationBuilderOpen) { return; }
	EnsureCreationBuildInitialized();
	USBCharacterCalculator::CyclePrestige(CreationBuild, +1);
}

void ASBPlayerController::BuilderCyclePrestigePrev()
{
	if (!bCreationBuilderOpen) { return; }
	EnsureCreationBuildInitialized();
	USBCharacterCalculator::CyclePrestige(CreationBuild, -1);
}

void ASBPlayerController::BuilderCycleDisciplineNext()
{
	if (!bCreationBuilderOpen) { return; }
	EnsureCreationBuildInitialized();
	USBCharacterCalculator::CycleDiscipline(CreationBuild, +1);
}

void ASBPlayerController::BuilderCycleDisciplinePrev()
{
	if (!bCreationBuilderOpen) { return; }
	EnsureCreationBuildInitialized();
	USBCharacterCalculator::CycleDiscipline(CreationBuild, -1);
}

void ASBPlayerController::BuilderConfirm()
{
	if (!bCreationBuilderOpen) { return; }
	if (bHeroNameEditing)
	{
		bHeroNameEditing = false;
		SetCreationHeroName(CreationHeroName);
		ServerSetHeroName(CreationHeroName);
		FSBClientDebug::PushMessage(FString::Printf(TEXT("Name locked: %s"), *CreationHeroName), 3.f);
		return;
	}
	EnsureCreationBuildInitialized();
	if (!CreationBuild.bValid)
	{
		FSBClientDebug::PushMessage(TEXT("Invalid shadowbanefps build"), 3.f);
		return;
	}
	if (CreationHeroName.IsEmpty())
	{
		if (const ASBPlayerState* PS = GetPlayerState<ASBPlayerState>())
		{
			CreationHeroName = PS->GetPlayerName();
		}
	}
	CreationHeroName = SanitizeHeroName(CreationHeroName);
	ServerConfirmCreationBuild(
		CreationBuild.Race,
		CreationBuild.BaseClass,
		CreationBuild.Prestige,
		CreationBuild.Discipline,
		CreationHeroName);
	FSBClientDebug::PushMessage(FString::Printf(TEXT("Confirmed %s as %s"), *CreationHeroName, *CreationBuild.StatusLine), 4.f);
}

void ASBPlayerController::ServerConfirmCreationBuild_Implementation(
	const FString& Race,
	const FString& BaseClass,
	const FString& Prestige,
	const FString& Discipline,
	const FString& HeroName)
{
	ASBSiegeGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASBSiegeGameMode>() : nullptr;
	ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	ASBSiegeGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASBSiegeGameState>() : nullptr;
	if (!GM || !PS || !GS)
	{
		return;
	}
	if (!USBRulesLibrary::CanSelectArchetypeWhileDead(PS->IsAlive()))
	{
		return;
	}

	const FString CleanName = SanitizeHeroName(HeroName);
	PS->SetPlayerName(CleanName);

	FSBCharacterBuildState Build;
	Build.HeroName = CleanName;
	Build.Race = Race;
	Build.BaseClass = BaseClass;
	Build.Prestige = Prestige;
	Build.Discipline = Discipline;
	USBCharacterCalculator::Recalculate(Build);
	if (!Build.bValid)
	{
		return;
	}

	USBCharacterArchetype* Match = USBCharacterCalculator::FindBestRosterMatch(Build, GS->GetLocalRoster());
	if (!Match)
	{
		Match = GM->GetRosterArchetype(0);
	}
	if (!Match)
	{
		return;
	}

	PS->SetCreationVitalsOverlay(Build.Vitals, Build.Race, Build.BaseClass, Build.Prestige, Build.Discipline);
	GM->RequestSelectArchetype(PS, Match);
	UE_LOG(LogShadowbaneServer, Log, TEXT("Creation build confirmed: %s as %s -> roster %s"),
		*CleanName, *Build.StatusLine, *Match->ArchetypeId.ToString());
}

