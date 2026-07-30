// Copyright shadowbanefps.

#include "SBMatchTelemetry.h"
#include "SBLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"

USBMatchTelemetry* USBMatchTelemetry::Create(UObject* Outer)
{
	return NewObject<USBMatchTelemetry>(Outer, TEXT("SBMatchTelemetry"));
}

void USBMatchTelemetry::StartMatchSession()
{
	SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
	SessionStartSeconds = FPlatformTime::Seconds();
	Events.Reset();

	EnsureCsv();
	Record(ESBTelemetryEvent::MatchStarted, FString::Printf(TEXT("session=%s"), *SessionId));
	UE_LOG(LogShadowbaneTelemetry, Log, TEXT("Telemetry session started: %s"), *SessionId);
}

void USBMatchTelemetry::EndMatchSession(ESBMatchResult Result)
{
	Record(ESBTelemetryEvent::MatchEnded,
		FString::Printf(TEXT("result=%s events=%d"),
			*UEnum::GetValueAsString(Result),
			Events.Num()));

	UE_LOG(LogShadowbaneTelemetry, Log, TEXT("Telemetry session ended: %s result=%s csv=%s"),
		*SessionId,
		*UEnum::GetValueAsString(Result),
		*CsvPath);
}

void USBMatchTelemetry::Record(ESBTelemetryEvent Event, const FString& Details)
{
	FSBTelemetryRow Row;
	Row.Elapsed = GetElapsed();
	Row.Event = Event;
	Row.Details = Details;
	Events.Add(Row);

	UE_LOG(LogShadowbaneTelemetry, Log, TEXT("[+%.2fs] %s %s"),
		Row.Elapsed,
		*UEnum::GetValueAsString(Event),
		*Details);

	AppendCsv(Row);
}

void USBMatchTelemetry::RecordPhase(ESBMatchPhase Phase)
{
	Record(ESBTelemetryEvent::PhaseChanged, UEnum::GetValueAsString(Phase));
}

void USBMatchTelemetry::RecordConquestStage(ESBConquestStage Stage)
{
	const ESBTelemetryEvent Event = (Stage == ESBConquestStage::Courtyard)
		? ESBTelemetryEvent::CourtyardCaptured
		: ESBTelemetryEvent::ConquestStageChanged;

	Record(Event, UEnum::GetValueAsString(Stage));
}

void USBMatchTelemetry::RecordPlayerSpawn(const FString& PlayerName, ESBTeam Team, FName ArchetypeId)
{
	Record(ESBTelemetryEvent::PlayerSpawned,
		FString::Printf(TEXT("player=%s team=%s archetype=%s"),
			*PlayerName,
			*UEnum::GetValueAsString(Team),
			*ArchetypeId.ToString()));
}

void USBMatchTelemetry::RecordPlayerKill(const FString& Victim, const FString& Killer, FName VictimArchetypeId)
{
	Record(ESBTelemetryEvent::PlayerKilled,
		FString::Printf(TEXT("victim=%s killer=%s victimArchetype=%s"),
			*Victim,
			*Killer,
			*VictimArchetypeId.ToString()));
}

void USBMatchTelemetry::RecordArchetypeSwitch(const FString& PlayerName, FName FromId, FName ToId)
{
	Record(ESBTelemetryEvent::ArchetypeSwitched,
		FString::Printf(TEXT("player=%s from=%s to=%s"),
			*PlayerName,
			*FromId.ToString(),
			*ToId.ToString()));
}

void USBMatchTelemetry::RecordStructure(ESBTelemetryEvent Event, const FString& StructureTag, float HealthPercent)
{
	Record(Event,
		FString::Printf(TEXT("structure=%s health=%.2f"),
			*StructureTag,
			HealthPercent));
}

bool USBMatchTelemetry::HasRecorded(ESBTelemetryEvent Event) const
{
	for (const FSBTelemetryRow& Row : Events)
	{
		if (Row.Event == Event)
		{
			return true;
		}
	}
	return false;
}

void USBMatchTelemetry::EnsureCsv()
{
	if (!bCsvEnabled)
	{
		return;
	}

	const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Telemetry"));
	IFileManager::Get().MakeDirectory(*Dir, true);

	const FString Stamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
	CsvPath = FPaths::Combine(Dir, FString::Printf(TEXT("match_%s_%s.csv"), *Stamp, *SessionId.Left(8)));

	const FString Header = TEXT("elapsed_s,event,details\n");
	FFileHelper::SaveStringToFile(Header, *CsvPath);
}

void USBMatchTelemetry::AppendCsv(const FSBTelemetryRow& Row)
{
	if (!bCsvEnabled || CsvPath.IsEmpty())
	{
		return;
	}

	const FString Line = FString::Printf(TEXT("%.3f,%s,\"%s\"\n"),
		Row.Elapsed,
		*UEnum::GetValueAsString(Row.Event),
		*Row.Details.Replace(TEXT("\""), TEXT("'")));

	FFileHelper::SaveStringToFile(
		Line,
		*CsvPath,
		FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(),
		FILEWRITE_Append);
}

double USBMatchTelemetry::GetElapsed() const
{
	return FPlatformTime::Seconds() - SessionStartSeconds;
}
