// Copyright shadowbanefps.
//
// Integration-style rule chains (match flow, overtime, archetype policy).
// These stay worldless so Cloud Agent can author them and local UE runs them.
// Run: .\scripts\RunIntegrationTests.ps1

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/SBRulesLibrary.h"
#include "Characters/SBPilotRoster.h"
#include "Characters/SBCharacterArchetype.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBIntegration_MatchFlowPathTest,
	"ShadowbaneFPS.Integration.MatchFlowPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBIntegration_MatchFlowPathTest::RunTest(const FString& Parameters)
{
	// Outer siege → courtyard capture → inner keep → objective unlock → complete.
	ESBConquestStage Stage = ESBConquestStage::OuterSiege;
	TestFalse(TEXT("Locked at outer"), USBRulesLibrary::IsFinalObjectiveUnlocked(Stage));

	TestTrue(TEXT("Advance to courtyard"),
		USBRulesLibrary::CanAdvanceConquestStage(Stage, ESBConquestStage::Courtyard));
	Stage = ESBConquestStage::Courtyard;
	TestTrue(TEXT("Unlocked at courtyard"), USBRulesLibrary::IsFinalObjectiveUnlocked(Stage));

	TestTrue(TEXT("Advance to inner keep"),
		USBRulesLibrary::CanAdvanceConquestStage(Stage, ESBConquestStage::InnerKeep));
	Stage = ESBConquestStage::InnerKeep;
	TestTrue(TEXT("Still unlocked at keep"), USBRulesLibrary::IsFinalObjectiveUnlocked(Stage));

	// Simulate capture + objective channel durations.
	float Capture = 0.f;
	for (int32 i = 0; i < 12; ++i)
	{
		Capture = USBRulesLibrary::TickCaptureProgress(Capture, 1.f, 2, 0, 12.f);
	}
	TestTrue(TEXT("Courtyard capture completes in 12s"), USBRulesLibrary::IsCaptureComplete(Capture, 12.f));

	float Objective = 0.f;
	for (int32 i = 0; i < 20; ++i)
	{
		Objective = USBRulesLibrary::TickObjectiveProgress(Objective, 1.f, 1, 0, 20.f, 1.f);
	}
	TestTrue(TEXT("Final objective completes"), USBRulesLibrary::IsObjectiveComplete(Objective, 20.f));
	TestEqual(TEXT("Published progress is 1"), USBRulesLibrary::NormalizeProgress(Objective, 20.f), 1.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBIntegration_OvertimeGateTest,
	"ShadowbaneFPS.Integration.OvertimeGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBIntegration_OvertimeGateTest::RunTest(const FString& Parameters)
{
	// Clock hits zero with mid-progress → overtime; complete/zero → no OT (defenders).
	TestTrue(TEXT("Mid progress triggers OT"), USBRulesLibrary::ShouldEnterOvertime(0.35f));
	TestFalse(TEXT("Zero progress defenders win"), USBRulesLibrary::ShouldEnterOvertime(0.f));
	TestFalse(TEXT("Full progress already decided"), USBRulesLibrary::ShouldEnterOvertime(1.f));

	// Contested channel mid-fight should not wipe progress (OT scenario).
	float Progress = USBRulesLibrary::TickObjectiveProgress(8.f, 5.f, 2, 2, 20.f, 2.f);
	TestEqual(TEXT("Contested holds for OT window"), Progress, 8.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBIntegration_ArchetypeSwitchPolicyTest,
	"ShadowbaneFPS.Integration.ArchetypeSwitchPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBIntegration_ArchetypeSwitchPolicyTest::RunTest(const FString& Parameters)
{
	TArray<TObjectPtr<USBCharacterArchetype>> Roster;
	USBPilotRoster::BuildDefaultRoster(GetTransientPackage(), Roster);
	TestTrue(TEXT("Roster built"), Roster.Num() >= 8);

	// Dead player may switch; alive may not.
	TestTrue(TEXT("Dead can switch"), USBRulesLibrary::CanSelectArchetypeWhileDead(false));
	TestFalse(TEXT("Alive cannot switch"), USBRulesLibrary::CanSelectArchetypeWhileDead(true));

	// Find a limited archetype and assert duplicate gate.
	USBCharacterArchetype* Limited = nullptr;
	for (USBCharacterArchetype* Arch : Roster)
	{
		if (Arch && Arch->PerTeamDuplicateLimit > 0)
		{
			Limited = Arch;
			break;
		}
	}
	TestNotNull(TEXT("Found limited archetype"), Limited);
	if (Limited)
	{
		TestTrue(TEXT("Room under limit"),
			USBRulesLibrary::CanUseArchetypeSlot(Limited->PerTeamDuplicateLimit - 1, Limited->PerTeamDuplicateLimit));
		TestFalse(TEXT("At limit blocked"),
			USBRulesLibrary::CanUseArchetypeSlot(Limited->PerTeamDuplicateLimit, Limited->PerTeamDuplicateLimit));
	}

	// Side eligibility sanity for both teams.
	bool bAtk = false;
	bool bDef = false;
	for (USBCharacterArchetype* Arch : Roster)
	{
		if (!Arch) { continue; }
		bAtk = bAtk || Arch->bAttackerEligible;
		bDef = bDef || Arch->bDefenderEligible;
	}
	TestTrue(TEXT("Attacker-eligible present"), bAtk);
	TestTrue(TEXT("Defender-eligible present"), bDef);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBIntegration_SpawnFrontLineTest,
	"ShadowbaneFPS.Integration.SpawnFrontLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBIntegration_SpawnFrontLineTest::RunTest(const FString& Parameters)
{
	// Front line never retreats: InnerKeep cannot go back to OuterSiege.
	TestFalse(TEXT("No retreat Inner->Outer"),
		USBRulesLibrary::CanAdvanceConquestStage(ESBConquestStage::InnerKeep, ESBConquestStage::OuterSiege));
	TestFalse(TEXT("No retreat Courtyard->Outer"),
		USBRulesLibrary::CanAdvanceConquestStage(ESBConquestStage::Courtyard, ESBConquestStage::OuterSiege));
	TestTrue(TEXT("Stay put ok"),
		USBRulesLibrary::CanAdvanceConquestStage(ESBConquestStage::Courtyard, ESBConquestStage::Courtyard));

	// Structure path that opens a courtyard breach.
	TestEqual(TEXT("Gate starts intact"),
		USBRulesLibrary::ComputeStructureState(1.f, 0.5f), ESBStructureState::Intact);
	TestEqual(TEXT("Gate damaged mid fight"),
		USBRulesLibrary::ComputeStructureState(0.4f, 0.5f), ESBStructureState::Damaged);
	TestEqual(TEXT("Gate destroyed"),
		USBRulesLibrary::ComputeStructureState(0.f, 0.5f), ESBStructureState::Destroyed);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
