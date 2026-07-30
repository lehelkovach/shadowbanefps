// Copyright shadowbanefps.

#include "SBPlayerController.h"
#include "SBSiegeGameMode.h"
#include "SBPlayerState.h"
#include "Characters/SBCharacterArchetype.h"
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
	ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>();
	ASBPlayerState* PS = GetPlayerState<ASBPlayerState>();
	if (!GM || !PS)
	{
		return;
	}

	USBCharacterArchetype* Archetype = GM->GetRosterArchetype(RosterIndex);
	if (!Archetype)
	{
		return;
	}

	GM->RequestSelectArchetype(PS, Archetype);
}

void ASBPlayerController::ServerRequestRespawn_Implementation()
{
	if (!bCanRespawn)
	{
		return;
	}

	if (ASBSiegeGameMode* GM = GetWorld()->GetAuthGameMode<ASBSiegeGameMode>())
	{
		if (GM->SpawnPlayerFromController(this))
		{
			bCanRespawn = false;
			RespawnTimeRemaining = 0.f;
		}
	}
}

void ASBPlayerController::SelectArchetypeSlot0() { ServerSelectArchetypeByIndex(0); }
void ASBPlayerController::SelectArchetypeSlot1() { ServerSelectArchetypeByIndex(1); }
void ASBPlayerController::SelectArchetypeSlot2() { ServerSelectArchetypeByIndex(2); }
void ASBPlayerController::SelectArchetypeSlot3() { ServerSelectArchetypeByIndex(3); }
void ASBPlayerController::SelectArchetypeSlot4() { ServerSelectArchetypeByIndex(4); }
void ASBPlayerController::SelectArchetypeSlot5() { ServerSelectArchetypeByIndex(5); }
void ASBPlayerController::SelectArchetypeSlot6() { ServerSelectArchetypeByIndex(6); }
void ASBPlayerController::SelectArchetypeSlot7() { ServerSelectArchetypeByIndex(7); }
void ASBPlayerController::SelectArchetypeSlot8() { ServerSelectArchetypeByIndex(8); }
void ASBPlayerController::SelectArchetypeSlot9() { ServerSelectArchetypeByIndex(9); }
void ASBPlayerController::RequestRespawnPressed() { ServerRequestRespawn(); }
