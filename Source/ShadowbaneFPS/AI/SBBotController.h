// Copyright shadowbanefps.
//
// Simple pilot bot for populate-and-spectate playtests (no real players yet).
// See docs/BOTS_AND_ADMIN.md and game-design.md §12.1.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Core/SBTypes.h"
#include "SBBotController.generated.h"

class ASBCharacter;
class ASBPlayerState;
class USBCharacterArchetype;

/**
 * Lightweight AI controller: pick a Shadowbane archetype, move toward siege goals,
 * and fire at hostiles. Not a full behavior tree — enough to fill a 5v5 for admin spectate.
 */
UCLASS()
class SHADOWBANEFPS_API ASBBotController : public AAIController
{
	GENERATED_BODY()

public:
	ASBBotController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

	void ConfigureBot(ESBTeam InTeam, USBCharacterArchetype* InArchetype, const FString& BotName);

	UFUNCTION(BlueprintPure, Category = "Siege|Bots")
	ESBTeam GetBotTeam() const { return BotTeam; }

protected:
	UPROPERTY()
	ESBTeam BotTeam = ESBTeam::Unassigned;

	UPROPERTY()
	TObjectPtr<USBCharacterArchetype> PreferredArchetype = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Siege|Bots")
	float RetargetSeconds = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Siege|Bots")
	float FireInterval = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category = "Siege|Bots")
	float EngageRange = 2800.f;

	float RetargetCooldown = 0.f;
	float FireCooldown = 0.f;

	TWeakObjectPtr<AActor> CurrentTarget;

	void PickTarget();
	void SteerToward(const FVector& WorldTarget, float DeltaSeconds);
	void TryFire();
	ASBCharacter* GetSBCharacter() const;
};
