// Copyright shadowbanefps.

#include "SBPlayerController.h"
#include "SBSiegeGameMode.h"
#include "SBPlayerState.h"
#include "Characters/SBCharacterArchetype.h"
#include "SBLog.h"
#include "SBRulesLibrary.h"
#include "Net/UnrealNetwork.h"

ASBPlayerController::ASBPlayerController()
{
	bReplicates = true;
}

void ASBPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASBPlayerController, RespawnTimeRemaining);
	DOREPLIFETIME(ASBPlayerController, bCanRespawn);
}

void ASBPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Number keys select roster slots while dead (§9). R requests respawn.
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

void ASBPlayerController::SelectArchetypeSlot0() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 0")); ServerSelectArchetypeByIndex(0); }
void ASBPlayerController::SelectArchetypeSlot1() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 1")); ServerSelectArchetypeByIndex(1); }
void ASBPlayerController::SelectArchetypeSlot2() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 2")); ServerSelectArchetypeByIndex(2); }
void ASBPlayerController::SelectArchetypeSlot3() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 3")); ServerSelectArchetypeByIndex(3); }
void ASBPlayerController::SelectArchetypeSlot4() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 4")); ServerSelectArchetypeByIndex(4); }
void ASBPlayerController::SelectArchetypeSlot5() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 5")); ServerSelectArchetypeByIndex(5); }
void ASBPlayerController::SelectArchetypeSlot6() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 6")); ServerSelectArchetypeByIndex(6); }
void ASBPlayerController::SelectArchetypeSlot7() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 7")); ServerSelectArchetypeByIndex(7); }
void ASBPlayerController::SelectArchetypeSlot8() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 8")); ServerSelectArchetypeByIndex(8); }
void ASBPlayerController::SelectArchetypeSlot9() { UE_LOG(LogShadowbaneClient, Verbose, TEXT("Client select slot 9")); ServerSelectArchetypeByIndex(9); }
void ASBPlayerController::RequestRespawnPressed()
{
	UE_LOG(LogShadowbaneClient, Log, TEXT("Client respawn pressed (can=%d remaining=%.2f)"),
		bCanRespawn ? 1 : 0, RespawnTimeRemaining);
	ServerRequestRespawn();
}
