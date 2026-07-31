// Copyright shadowbanefps.
//
// Script-driven pilot bot for populate-and-spectate playtests.
// See docs/BOTS_AND_ADMIN.md and docs/BOT_SCRIPTING.md.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Core/SBTypes.h"
#include "AI/SBBotScript.h"
#include "SBBotController.generated.h"

class ASBCharacter;
class ASBPlayerState;
class USBCharacterArchetype;

/**
 * AI controller driven by Config/BotScripts/<Id>.sbbot shorthand rules.
 * Archetype id selects the script (e.g. Warrior_Blade.sbbot); missing files
 * fall back to Default.sbbot / built-in rules.
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

	UFUNCTION(BlueprintPure, Category = "Siege|Bots")
	FName GetBotScriptId() const { return BotScriptId; }

protected:
	UPROPERTY()
	ESBTeam BotTeam = ESBTeam::Unassigned;

	UPROPERTY()
	TObjectPtr<USBCharacterArchetype> PreferredArchetype = nullptr;

	/** Script file stem under Config/BotScripts/ (usually archetype id). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Siege|Bots|Script")
	FName BotScriptId = FName(TEXT("Default"));

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siege|Bots|Script")
	FSBBotScript ActiveScript;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Siege|Bots")
	float MoveAcceptanceRadius = 120.f;

	float RetargetCooldown = 0.f;
	float FireCooldown = 0.f;
	bool bScriptLoaded = false;

	TWeakObjectPtr<AActor> CurrentTarget;
	FSBBotDecision CurrentDecision;

	void EnsureScriptLoaded();
	void GatherWorldFacts(FSBBotWorldFacts& OutFacts) const;
	void Think();
	void SteerToward(const FVector& WorldTarget, float DeltaSeconds);
	void SteerAwayFrom(const FVector& WorldThreat, float DeltaSeconds);
	FVector ClampToPlayableBounds(const FVector& Desired) const;
	void SoftClampPawnToBounds();
	void TryFire();
	ASBCharacter* GetSBCharacter() const;

	/** Playable XY box matching Broken Citadel greybox pads (+ margin). */
	FBox PlayableBounds = FBox(FVector(-6200.f, -2600.f, -200.f), FVector(4200.f, 2600.f, 800.f));
};
