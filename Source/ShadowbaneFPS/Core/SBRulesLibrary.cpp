// Copyright shadowbanefps.

#include "SBRulesLibrary.h"

ESBStructureState USBRulesLibrary::ComputeStructureState(float HealthPercent, float DamagedThreshold)
{
	const float Clamped = FMath::Clamp(HealthPercent, 0.f, 1.f);
	const float Threshold = FMath::Clamp(DamagedThreshold, 0.f, 1.f);

	if (Clamped <= 0.f)
	{
		return ESBStructureState::Destroyed;
	}
	if (Clamped < Threshold)
	{
		return ESBStructureState::Damaged;
	}
	return ESBStructureState::Intact;
}

bool USBRulesLibrary::CanAdvanceConquestStage(ESBConquestStage Current, ESBConquestStage Next)
{
	return static_cast<uint8>(Next) >= static_cast<uint8>(Current);
}

bool USBRulesLibrary::CanUseArchetypeSlot(int32 CurrentTeamCount, int32 PerTeamDuplicateLimit)
{
	if (PerTeamDuplicateLimit <= 0)
	{
		return true;
	}
	return CurrentTeamCount < PerTeamDuplicateLimit;
}

ESBTeam USBRulesLibrary::PickBalancedTeam(int32 AttackerCount, int32 DefenderCount)
{
	return (AttackerCount <= DefenderCount) ? ESBTeam::Attackers : ESBTeam::Defenders;
}

bool USBRulesLibrary::IsFinalObjectiveUnlocked(ESBConquestStage Stage)
{
	return Stage != ESBConquestStage::OuterSiege;
}

bool USBRulesLibrary::ShouldEnterOvertime(float FinalObjectiveProgress01)
{
	return FinalObjectiveProgress01 > 0.f && FinalObjectiveProgress01 < 1.f;
}

float USBRulesLibrary::TickCaptureProgress(
	float CurrentProgressSeconds,
	float DeltaSeconds,
	int32 AttackersInZone,
	int32 DefendersInZone,
	float CaptureSeconds,
	float DecayPerSecond)
{
	const float Cap = FMath::Max(0.f, CaptureSeconds);
	float Progress = FMath::Clamp(CurrentProgressSeconds, 0.f, Cap);
	const float Dt = FMath::Max(0.f, DeltaSeconds);

	if (AttackersInZone > 0 && DefendersInZone == 0)
	{
		Progress = FMath::Min(Cap, Progress + Dt);
	}
	else if (AttackersInZone == 0)
	{
		Progress = FMath::Max(0.f, Progress - Dt * FMath::Max(0.f, DecayPerSecond));
	}
	// Contested: hold.

	return Progress;
}

bool USBRulesLibrary::IsCaptureComplete(float ProgressSeconds, float CaptureSeconds)
{
	return ProgressSeconds >= FMath::Max(0.f, CaptureSeconds) && CaptureSeconds > 0.f;
}

float USBRulesLibrary::TickObjectiveProgress(
	float CurrentProgressSeconds,
	float DeltaSeconds,
	int32 AttackersInZone,
	int32 DefendersInZone,
	float CompleteSeconds,
	float DecayRatePerSecond)
{
	const float Cap = FMath::Max(0.f, CompleteSeconds);
	float Progress = FMath::Clamp(CurrentProgressSeconds, 0.f, Cap);
	const float Dt = FMath::Max(0.f, DeltaSeconds);
	const bool bContested = (AttackersInZone > 0 && DefendersInZone > 0);

	if (AttackersInZone > 0 && DefendersInZone == 0)
	{
		Progress = FMath::Min(Cap, Progress + Dt);
	}
	else if (!bContested)
	{
		Progress = FMath::Max(0.f, Progress - FMath::Max(0.f, DecayRatePerSecond) * Dt);
	}
	// Contested: hold.

	return Progress;
}

bool USBRulesLibrary::IsObjectiveComplete(float ProgressSeconds, float CompleteSeconds)
{
	return ProgressSeconds >= FMath::Max(0.f, CompleteSeconds) && CompleteSeconds > 0.f;
}

float USBRulesLibrary::NormalizeProgress(float ProgressSeconds, float TotalSeconds)
{
	if (TotalSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(ProgressSeconds / TotalSeconds, 0.f, 1.f);
}

FString USBRulesLibrary::FormatMatchClock(float RemainingSeconds)
{
	const int32 Remaining = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
	const int32 Minutes = Remaining / 60;
	const int32 Seconds = Remaining % 60;
	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

bool USBRulesLibrary::CanSelectArchetypeWhileDead(bool bAlive)
{
	return !bAlive;
}

bool USBRulesLibrary::CanRequestRespawn(bool bCanRespawn, float RespawnTimeRemaining)
{
	return bCanRespawn && RespawnTimeRemaining <= 0.f;
}

bool USBRulesLibrary::ShouldShowDeathOverlay(bool bAlive, bool bHasPawn)
{
	return !bAlive || !bHasPawn;
}

bool USBRulesLibrary::IsFriendlyFire(ESBTeam ShooterTeam, ESBTeam TargetTeam)
{
	if (ShooterTeam == ESBTeam::Unassigned || TargetTeam == ESBTeam::Unassigned)
	{
		return false;
	}
	return ShooterTeam == TargetTeam;
}

void USBRulesLibrary::SplitBotsAcrossTeams(int32 TotalBots, int32 MaxPerTeam, int32& OutAttackers, int32& OutDefenders)
{
	OutAttackers = 0;
	OutDefenders = 0;
	const int32 Cap = FMath::Max(0, MaxPerTeam);
	int32 Remaining = FMath::Max(0, TotalBots);

	while (Remaining > 0 && (OutAttackers < Cap || OutDefenders < Cap))
	{
		if (OutAttackers <= OutDefenders && OutAttackers < Cap)
		{
			++OutAttackers;
		}
		else if (OutDefenders < Cap)
		{
			++OutDefenders;
		}
		else if (OutAttackers < Cap)
		{
			++OutAttackers;
		}
		else
		{
			break;
		}
		--Remaining;
	}
}
