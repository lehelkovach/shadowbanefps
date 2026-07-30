// Copyright shadowbanefps.
//
// Automation tests for pure match rules (design doc §5, §7, §9).
// Run: see docs/TESTING.md or scripts/RunAutomationTests.ps1

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/SBRulesLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBRules_StructureStateTest,
	"ShadowbaneFPS.Rules.StructureState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBRules_StructureStateTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Full health is Intact"),
		USBRulesLibrary::ComputeStructureState(1.f, 0.5f),
		ESBStructureState::Intact);

	TestEqual(TEXT("Just below threshold is Damaged"),
		USBRulesLibrary::ComputeStructureState(0.49f, 0.5f),
		ESBStructureState::Damaged);

	TestEqual(TEXT("Exactly threshold stays Intact"),
		USBRulesLibrary::ComputeStructureState(0.5f, 0.5f),
		ESBStructureState::Intact);

	TestEqual(TEXT("Zero health is Destroyed"),
		USBRulesLibrary::ComputeStructureState(0.f, 0.5f),
		ESBStructureState::Destroyed);

	TestEqual(TEXT("Negative clamps to Destroyed"),
		USBRulesLibrary::ComputeStructureState(-1.f, 0.5f),
		ESBStructureState::Destroyed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBRules_ConquestAdvanceTest,
	"ShadowbaneFPS.Rules.ConquestAdvance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBRules_ConquestAdvanceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Same stage allowed"),
		USBRulesLibrary::CanAdvanceConquestStage(ESBConquestStage::Courtyard, ESBConquestStage::Courtyard));

	TestTrue(TEXT("Forward Outer->Courtyard"),
		USBRulesLibrary::CanAdvanceConquestStage(ESBConquestStage::OuterSiege, ESBConquestStage::Courtyard));

	TestTrue(TEXT("Forward Courtyard->InnerKeep"),
		USBRulesLibrary::CanAdvanceConquestStage(ESBConquestStage::Courtyard, ESBConquestStage::InnerKeep));

	TestFalse(TEXT("Backward InnerKeep->Courtyard blocked"),
		USBRulesLibrary::CanAdvanceConquestStage(ESBConquestStage::InnerKeep, ESBConquestStage::Courtyard));

	TestFalse(TEXT("Backward Courtyard->Outer blocked"),
		USBRulesLibrary::CanAdvanceConquestStage(ESBConquestStage::Courtyard, ESBConquestStage::OuterSiege));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBRules_DuplicateLimitTest,
	"ShadowbaneFPS.Rules.DuplicateLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBRules_DuplicateLimitTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Unlimited when limit=0"), USBRulesLibrary::CanUseArchetypeSlot(99, 0));
	TestTrue(TEXT("Unlimited when limit<0"), USBRulesLibrary::CanUseArchetypeSlot(99, -1));
	TestTrue(TEXT("Room under limit"), USBRulesLibrary::CanUseArchetypeSlot(1, 2));
	TestFalse(TEXT("At limit blocked"), USBRulesLibrary::CanUseArchetypeSlot(2, 2));
	TestFalse(TEXT("Over limit blocked"), USBRulesLibrary::CanUseArchetypeSlot(3, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBRules_TeamBalanceTest,
	"ShadowbaneFPS.Rules.TeamBalance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBRules_TeamBalanceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Empty lobby prefers Attackers"),
		USBRulesLibrary::PickBalancedTeam(0, 0),
		ESBTeam::Attackers);

	TestEqual(TEXT("Fewer attackers gets attacker"),
		USBRulesLibrary::PickBalancedTeam(1, 2),
		ESBTeam::Attackers);

	TestEqual(TEXT("Fewer defenders gets defender"),
		USBRulesLibrary::PickBalancedTeam(3, 2),
		ESBTeam::Defenders);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBRules_ObjectiveAndOvertimeTest,
	"ShadowbaneFPS.Rules.ObjectiveAndOvertime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBRules_ObjectiveAndOvertimeTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("OuterSiege locks final objective"),
		USBRulesLibrary::IsFinalObjectiveUnlocked(ESBConquestStage::OuterSiege));
	TestTrue(TEXT("Courtyard unlocks final objective"),
		USBRulesLibrary::IsFinalObjectiveUnlocked(ESBConquestStage::Courtyard));
	TestTrue(TEXT("InnerKeep unlocks final objective"),
		USBRulesLibrary::IsFinalObjectiveUnlocked(ESBConquestStage::InnerKeep));

	TestFalse(TEXT("No overtime at 0 progress"), USBRulesLibrary::ShouldEnterOvertime(0.f));
	TestFalse(TEXT("No overtime at complete"), USBRulesLibrary::ShouldEnterOvertime(1.f));
	TestTrue(TEXT("Overtime when contested mid-progress"), USBRulesLibrary::ShouldEnterOvertime(0.4f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
