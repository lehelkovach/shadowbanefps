// Copyright shadowbanefps.
//
// Lightweight match event logger for pilot telemetry (design doc §12).
// Writes to LogShadowbaneTelemetry and optionally a CSV under Saved/Telemetry/.

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
	OvertimeStarted			UMETA(DisplayName = "OvertimeStarted"),
	MatchEnded				UMETA(DisplayName = "MatchEnded")
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
	void RecordPlayerKill(const FString& Victim, const FString& Killer, FName VictimArchetypeId);
	void RecordArchetypeSwitch(const FString& PlayerName, FName FromId, FName ToId);
	void RecordStructure(ESBTelemetryEvent Event, const FString& StructureTag, float HealthPercent);

	UFUNCTION(BlueprintPure, Category = "Siege|Telemetry")
	int32 GetEventCount() const { return Events.Num(); }

	UFUNCTION(BlueprintPure, Category = "Siege|Telemetry")
	bool HasRecorded(ESBTelemetryEvent Event) const;

protected:
	UPROPERTY()
	FString SessionId;

	UPROPERTY()
	double SessionStartSeconds = 0.0;

	UPROPERTY()
	bool bCsvEnabled = true;

	struct FSBTelemetryRow
	{
		double Elapsed = 0.0;
		ESBTelemetryEvent Event = ESBTelemetryEvent::MatchStarted;
		FString Details;
	};

	TArray<FSBTelemetryRow> Events;

	FString CsvPath;
	void EnsureCsv();
	void AppendCsv(const FSBTelemetryRow& Row);
	double GetElapsed() const;
};
