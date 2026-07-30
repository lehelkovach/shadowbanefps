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
	CombatRows.Reset();
	DamageByMatchup.Reset();
	KillsByMatchup.Reset();
	DamageByPower.Reset();
	KillsByPower.Reset();

	EnsureCsv();
	Record(ESBTelemetryEvent::MatchStarted, FString::Printf(TEXT("session=%s"), *SessionId));
	UE_LOG(LogShadowbaneTelemetry, Log, TEXT("Telemetry session started: %s"), *SessionId);
}

void USBMatchTelemetry::EndMatchSession(ESBMatchResult Result)
{
	WriteBalanceSummary();

	Record(ESBTelemetryEvent::MatchEnded,
		FString::Printf(TEXT("result=%s events=%d combatRows=%d"),
			*UEnum::GetValueAsString(Result),
			Events.Num(),
			CombatRows.Num()));

	UE_LOG(LogShadowbaneTelemetry, Log, TEXT("Telemetry session ended: %s result=%s events=%s combat=%s"),
		*SessionId,
		*UEnum::GetValueAsString(Result),
		*CsvPath,
		*CombatCsvPath);
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

void USBMatchTelemetry::RecordPlayerKill(
	const FString& Victim,
	const FString& Killer,
	FName VictimArchetypeId,
	FName KillerArchetypeId,
	FName PowerId)
{
	FSBCombatMetric Metric;
	Metric.AttackerName = Killer;
	Metric.AttackerArchetype = KillerArchetypeId;
	Metric.VictimName = Victim;
	Metric.VictimArchetype = VictimArchetypeId;
	Metric.PowerId = PowerId.IsNone() ? FName(TEXT("Unknown")) : PowerId;
	Metric.bLethal = true;

	AccumulateCombat(TEXT("Kill"), Metric);

	FSBCombatCsvRow CombatRow;
	CombatRow.Elapsed = GetElapsed();
	CombatRow.Kind = TEXT("Kill");
	CombatRow.Metric = Metric;
	CombatRows.Add(CombatRow);
	AppendCombatCsv(CombatRow);

	Record(ESBTelemetryEvent::PlayerKilled,
		FString::Printf(TEXT("victim=%s victimArch=%s killer=%s killerArch=%s power=%s"),
			*Victim,
			*VictimArchetypeId.ToString(),
			*Killer,
			*KillerArchetypeId.ToString(),
			*Metric.PowerId.ToString()));
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

void USBMatchTelemetry::RecordCombatDamage(const FSBCombatMetric& Metric)
{
	AccumulateCombat(TEXT("Damage"), Metric);

	if (bLogPerHitCombat)
	{
		FSBCombatCsvRow CombatRow;
		CombatRow.Elapsed = GetElapsed();
		CombatRow.Kind = TEXT("Damage");
		CombatRow.Metric = Metric;
		CombatRows.Add(CombatRow);
		AppendCombatCsv(CombatRow);
	}

	Record(ESBTelemetryEvent::CombatDamage,
		FString::Printf(
			TEXT("atk=%s atkArch=%s vic=%s vicArch=%s power=%s dmg=%.1f hpAfter=%.1f lethal=%d"),
			*Metric.AttackerName,
			*Metric.AttackerArchetype.ToString(),
			*Metric.VictimName,
			*Metric.VictimArchetype.ToString(),
			*Metric.PowerId.ToString(),
			Metric.Amount,
			Metric.VictimHealthAfter,
			Metric.bLethal ? 1 : 0));
}

void USBMatchTelemetry::RecordCombatHeal(const FSBCombatMetric& Metric)
{
	AccumulateCombat(TEXT("Heal"), Metric);

	if (bLogPerHitCombat)
	{
		FSBCombatCsvRow CombatRow;
		CombatRow.Elapsed = GetElapsed();
		CombatRow.Kind = TEXT("Heal");
		CombatRow.Metric = Metric;
		CombatRows.Add(CombatRow);
		AppendCombatCsv(CombatRow);
	}

	Record(ESBTelemetryEvent::CombatHeal,
		FString::Printf(
			TEXT("src=%s srcArch=%s tgt=%s tgtArch=%s power=%s heal=%.1f hpAfter=%.1f"),
			*Metric.AttackerName,
			*Metric.AttackerArchetype.ToString(),
			*Metric.VictimName,
			*Metric.VictimArchetype.ToString(),
			*Metric.PowerId.ToString(),
			Metric.Amount,
			Metric.VictimHealthAfter));
}

void USBMatchTelemetry::RecordShopPurchase(const FString& PlayerName, FName ArchetypeId, FName ItemId, int32 CostGold)
{
	Record(ESBTelemetryEvent::ShopPurchase,
		FString::Printf(TEXT("player=%s archetype=%s item=%s cost=%d"),
			*PlayerName,
			*ArchetypeId.ToString(),
			*ItemId.ToString(),
			CostGold));
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

float USBMatchTelemetry::GetDamageBetween(FName AttackerArchetype, FName VictimArchetype) const
{
	if (const float* Found = DamageByMatchup.Find(MatchupKey(AttackerArchetype, VictimArchetype)))
	{
		return *Found;
	}
	return 0.f;
}

int32 USBMatchTelemetry::GetKillsBetween(FName AttackerArchetype, FName VictimArchetype) const
{
	if (const int32* Found = KillsByMatchup.Find(MatchupKey(AttackerArchetype, VictimArchetype)))
	{
		return *Found;
	}
	return 0;
}

FString USBMatchTelemetry::MatchupKey(FName Attacker, FName Victim)
{
	return FString::Printf(TEXT("%s>%s"), *Attacker.ToString(), *Victim.ToString());
}

void USBMatchTelemetry::AccumulateCombat(const FString& Kind, const FSBCombatMetric& Metric)
{
	const FString Key = MatchupKey(Metric.AttackerArchetype, Metric.VictimArchetype);

	if (Kind == TEXT("Damage") || Kind == TEXT("Heal"))
	{
		DamageByMatchup.FindOrAdd(Key) += Metric.Amount;
		if (!Metric.PowerId.IsNone())
		{
			DamageByPower.FindOrAdd(Metric.PowerId) += Metric.Amount;
		}
	}
	else if (Kind == TEXT("Kill"))
	{
		KillsByMatchup.FindOrAdd(Key) += 1;
		if (!Metric.PowerId.IsNone())
		{
			KillsByPower.FindOrAdd(Metric.PowerId) += 1;
		}
	}
}

void USBMatchTelemetry::WriteBalanceSummary()
{
	for (const TPair<FString, float>& Pair : DamageByMatchup)
	{
		const FString Line = FString::Printf(TEXT("matchup=%s damage=%.1f kills=%d"),
			*Pair.Key,
			Pair.Value,
			KillsByMatchup.FindRef(Pair.Key));
		Record(ESBTelemetryEvent::BalanceSummary, Line);
	}

	for (const TPair<FName, float>& Pair : DamageByPower)
	{
		Record(ESBTelemetryEvent::BalanceSummary,
			FString::Printf(TEXT("power=%s damage=%.1f kills=%d"),
				*Pair.Key.ToString(),
				Pair.Value,
				KillsByPower.FindRef(Pair.Key)));
	}
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
	const FString SessionShort = SessionId.Left(8);

	CsvPath = FPaths::Combine(Dir, FString::Printf(TEXT("match_%s_%s.csv"), *Stamp, *SessionShort));
	CombatCsvPath = FPaths::Combine(Dir, FString::Printf(TEXT("combat_%s_%s.csv"), *Stamp, *SessionShort));

	FFileHelper::SaveStringToFile(TEXT("elapsed_s,event,details\n"), *CsvPath);
	FFileHelper::SaveStringToFile(
		TEXT("elapsed_s,kind,attacker,attacker_arch,attacker_team,victim,victim_arch,victim_team,power,amount,victim_hp_after,lethal,extra\n"),
		*CombatCsvPath);
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

void USBMatchTelemetry::AppendCombatCsv(const FSBCombatCsvRow& Row)
{
	if (!bCsvEnabled || CombatCsvPath.IsEmpty())
	{
		return;
	}

	const FSBCombatMetric& M = Row.Metric;
	const FString Line = FString::Printf(
		TEXT("%.3f,%s,%s,%s,%s,%s,%s,%s,%s,%.2f,%.2f,%d,\"%s\"\n"),
		Row.Elapsed,
		*Row.Kind,
		*M.AttackerName,
		*M.AttackerArchetype.ToString(),
		*UEnum::GetValueAsString(M.AttackerTeam),
		*M.VictimName,
		*M.VictimArchetype.ToString(),
		*UEnum::GetValueAsString(M.VictimTeam),
		*M.PowerId.ToString(),
		M.Amount,
		M.VictimHealthAfter,
		M.bLethal ? 1 : 0,
		*M.Extra.Replace(TEXT("\""), TEXT("'")));

	FFileHelper::SaveStringToFile(
		Line,
		*CombatCsvPath,
		FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(),
		FILEWRITE_Append);
}

double USBMatchTelemetry::GetElapsed() const
{
	return FPlatformTime::Seconds() - SessionStartSeconds;
}
