// Copyright shadowbanefps.
//
// Shorthand bot-logic scripts (.sbbot). See docs/BOT_SCRIPTING.md.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SBBotScript.generated.h"

class AActor;
class ASBCharacter;

/** What the bot is trying to interact with. */
UENUM(BlueprintType)
enum class ESBBotTargetKind : uint8
{
	None		UMETA(DisplayName = "None"),
	Enemy		UMETA(DisplayName = "Enemy"),
	AllyHurt	UMETA(DisplayName = "AllyHurt"),
	Capture		UMETA(DisplayName = "Capture"),
	Objective	UMETA(DisplayName = "Objective"),
	Structure	UMETA(DisplayName = "Structure"),
	Spawn		UMETA(DisplayName = "Spawn"),
	Else		UMETA(DisplayName = "Else")
};

/** What to do once a rule matches. */
UENUM(BlueprintType)
enum class ESBBotAction : uint8
{
	Pursue		UMETA(DisplayName = "Pursue"),
	Hold		UMETA(DisplayName = "Hold"),
	Fire		UMETA(DisplayName = "Fire"),
	PursueFire	UMETA(DisplayName = "PursueFire"),
	Retreat		UMETA(DisplayName = "Retreat")
};

USTRUCT(BlueprintType)
struct FSBBotRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	ESBBotTargetKind Target = ESBBotTargetKind::Else;

	/** 0 = any distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	float InRange = 0.f;

	/** Optional filter: open capture, intact structure, unlocked objective. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	FName Qualifier = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	ESBBotAction Action = ESBBotAction::Pursue;

	/** Original shorthand line (debug). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BotScript")
	FString SourceLine;
};

USTRUCT(BlueprintType)
struct FSBBotScript
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	FName ScriptId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	FName Role = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	float RetargetSeconds = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	float EngageRange = 2800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	float FireInterval = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BotScript")
	TArray<FSBBotRule> Rules;

	bool IsValid() const { return Rules.Num() > 0 || !ScriptId.IsNone(); }
};

/** Snapshot of interesting actors for rule evaluation (filled by the bot controller). */
USTRUCT(BlueprintType)
struct FSBBotWorldFacts
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> NearestEnemy = nullptr;

	UPROPERTY()
	float EnemyDistance = 1.0e12f;

	UPROPERTY()
	TObjectPtr<AActor> HurtAlly = nullptr;

	UPROPERTY()
	float AllyDistance = 1.0e12f;

	UPROPERTY()
	TObjectPtr<AActor> OpenCapture = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> Objective = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> IntactStructure = nullptr;

	UPROPERTY()
	FVector SelfLocation = FVector::ZeroVector;
};

/** Result of evaluating the first matching rule. */
USTRUCT(BlueprintType)
struct FSBBotDecision
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> MoveTarget = nullptr;

	UPROPERTY()
	bool bWantFire = false;

	UPROPERTY()
	bool bHold = false;

	UPROPERTY()
	bool bRetreat = false;

	UPROPERTY()
	ESBBotTargetKind MatchedTarget = ESBBotTargetKind::None;

	UPROPERTY()
	ESBBotAction MatchedAction = ESBBotAction::Hold;
};

/**
 * Parses .sbbot shorthand and loads scripts from Config/BotScripts/.
 * Markup cheat-sheet is in docs/BOT_SCRIPTING.md.
 */
UCLASS()
class SHADOWBANEFPS_API USBBotScriptLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** Parse a full .sbbot document into OutScript. Returns false on hard errors. */
	UFUNCTION(BlueprintCallable, Category = "Siege|Bots")
	static bool ParseScriptText(const FString& Text, FSBBotScript& OutScript, FString& OutError);

	/** Load Config/BotScripts/<Id>.sbbot (or Default.sbbot). */
	UFUNCTION(BlueprintCallable, Category = "Siege|Bots")
	static bool LoadScriptById(FName ScriptId, FSBBotScript& OutScript);

	/** Built-in fallback if no file exists. */
	UFUNCTION(BlueprintCallable, Category = "Siege|Bots")
	static FSBBotScript MakeDefaultScript(FName ScriptId = FName(TEXT("Default")));

	static bool ParseRuleLine(const FString& Line, FSBBotRule& OutRule, FString& OutError);

	/** First matching rule wins. Returns false if no rule matched. */
	static bool EvaluateRules(const FSBBotScript& Script, const FSBBotWorldFacts& Facts, FSBBotDecision& OutDecision);
};
