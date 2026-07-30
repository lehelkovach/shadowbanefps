// Copyright shadowbanefps.
//
// Lightweight match event logger for pilot telemetry (design doc §12).
// Writes to LogShadowbaneTelemetry and CSV under Saved/Telemetry/.
// Combat rows support build-vs-build balance analysis (nerf/buff).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SBTypes.h"
#include "SBMatchTelemetry.generated.h"

UENUM(BlueprintType)
enum class ESBTelemetryEvent : uint8
{
	MatchStarted			UMETA(DisplayName = "MatchStarted"),
	PhaseChanged			UMETA(DisplayName = "PhaseChanged"),
	ConquestStageChanged	UMETA(DisplayName = "ConquestStageChanged"),
	FirstContact			UMETA(DisplayName = "FirstContact"),
	StructureDamaged		UMETA(DisplayName = "StructureDamaged"),
	StructureDestroyed		UMETA(DisplayName = "StructureDestroyed"),
	CourtyardCaptured		UMETA(DisplayName = "CourtyardCaptured"),
	FinalObjectiveAttempt	UMETA(DisplayName = "FinalObjectiveAttempt"),
	FinalObjectiveCompleted	UMETA(DisplayName = "FinalObjectiveCompleted"),
	PlayerSpawned			UMETA(DisplayName = "PlayerSpawned"),
	PlayerKilled			UMETA(DisplayName = "PlayerKilled"),
	ArchetypeSwitched		UMETA(DisplayName = "ArchetypeSwitched"),
	CombatDamage			UMETA(DisplayName = "CombatDamage"),
	CombatHeal				UMETA(DisplayName = "CombatHeal"),
	ShopPurchase			UMETA(DisplayName = "ShopPurchase"),
	OvertimeStarted			UMETA(DisplayName = "OvertimeStarted"),
	MatchEnded				UMETA(DisplayName = "MatchEnded"),
	BalanceSummary			UMETA(DisplayName = "BalanceSummary")
};

/** One combat hit / heal / kill attribution row for balance CSVs. */
USTRUCT(BlueprintType)
struct FSBCombatMetric
{
	GENERATED_BODY()

	UPROPERTY()
	FString AttackerName;

	UPROPERTY()
	FName AttackerArchetype = NAME_None;

	UPROPERTY()
	ESBTeam AttackerTeam = ESBTeam::Unassigned;

	UPROPERTY()
	FString VictimName;

	UPROPERTY()
	FName VictimArchetype = NAME_None;

	UPROPERTY()
	ESBTeam VictimTeam = ESBTeam::Unassigned;

	/** Ability / weapon / item id (e.g. BasicFire, FlameBolt, Shop_ResistCharm). */
	UPROPERTY()
	FName PowerId = NAME_None;

	UPROPERTY()
	float Amount = 0.f;

	UPROPERTY()
	float VictimHealthAfter = 0.f;

	UPROPERTY()
	bool bLethal = false;

	UPROPERTY()
	FString Extra;
};

UCLASS()
class SHADOWBANEFPS_API USBMatchTelemetry : public UObject
{
	GENERATED_BODY()

public:
	/** Creates a telemetry sink owned by Outer (typically the GameMode). */
	static USBMatchTelemetry* Create(UObject* Outer);

	void StartMatchSession();
	void EndMatchSession(ESBMatchResult Result);

	void Record(ESBTelemetryEvent Event, const FString& Details = FString());
	void RecordPhase(ESBMatchPhase Phase);
	void RecordConquestStage(ESBConquestStage Stage);
	void RecordPlayerSpawn(const FString& PlayerName, ESBTeam Team, FName ArchetypeId);
	void RecordPlayerKill(
		const FString& Victim,
		const FString& Killer,
		FName VictimArchetypeId,
		FName KillerArchetypeId = NAME_None,
		FName PowerId = NAME_None);
	void RecordArchetypeSwitch(const FString& PlayerName, FName FromId, FName ToId);
	void RecordStructure(ESBTelemetryEvent Event, const FString& StructureTag, float HealthPercent);

	/** Per-hit / per-heal combat attribution for build-vs-build balance. */
	void RecordCombatDamage(const FSBCombatMetric& Metric);
	void RecordCombatHeal(const FSBCombatMetric& Metric);
	void RecordShopPurchase(const FString& PlayerName, FName ArchetypeId, FName ItemId, int32 CostGold);

	UFUNCTION(BlueprintPure, Category = "Siege|Telemetry")
	int32 GetEventCount() const { return Events.Num(); }

	UFUNCTION(BlueprintPure, Category = "Siege|Telemetry")
	int32 GetCombatRowCount() const { return CombatRows.Num(); }

	UFUNCTION(BlueprintPure, Category = "Siege|Telemetry")
	bool HasRecorded(ESBTelemetryEvent Event) const;

	/** Sum of damage attributed from AttackerArch onto VictimArch this match. */
	UFUNCTION(BlueprintPure, Category = "Siege|Telemetry")
	float GetDamageBetween(FName AttackerArchetype, FName VictimArchetype) const;

	UFUNCTION(BlueprintPure, Category = "Siege|Telemetry")
	int32 GetKillsBetween(FName AttackerArchetype, FName VictimArchetype) const;

protected:
	UPROPERTY()
	FString SessionId;

	UPROPERTY()
	double SessionStartSeconds = 0.0;

	UPROPERTY()
	bool bCsvEnabled = true;

	/** When false, skip per-hit CombatDamage rows (kills / summary still recorded). */
	UPROPERTY()
	bool bLogPerHitCombat = true;

	struct FSBTelemetryRow
	{
		double Elapsed = 0.0;
		ESBTelemetryEvent Event = ESBTelemetryEvent::MatchStarted;
		FString Details;
	};

	struct FSBCombatCsvRow
	{
		double Elapsed = 0.0;
		FString Kind; // Damage | Heal | Kill
		FSBCombatMetric Metric;
	};

	TArray<FSBTelemetryRow> Events;
	TArray<FSBCombatCsvRow> CombatRows;

	/** Aggregates: "AttackerArch>VictimArch" -> totals */
	TMap<FString, float> DamageByMatchup;
	TMap<FString, int32> KillsByMatchup;
	TMap<FName, float> DamageByPower;
	TMap<FName, int32> KillsByPower;

	FString CsvPath;
	FString CombatCsvPath;

	void EnsureCsv();
	void AppendCsv(const FSBTelemetryRow& Row);
	void AppendCombatCsv(const FSBCombatCsvRow& Row);
	void AccumulateCombat(const FString& Kind, const FSBCombatMetric& Metric);
	void WriteBalanceSummary();
	static FString MatchupKey(FName Attacker, FName Victim);
	double GetElapsed() const;
};
