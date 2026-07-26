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
