// Copyright shadowbanefps.
//
// Server-side unit tests (capture / objective / combat gates).
// Run: .\scripts\RunServerTests.ps1

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/SBRulesLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBServer_CaptureTickTest,
	"ShadowbaneFPS.Server.CaptureTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBServer_CaptureTickTest::RunTest(const FString& Parameters)
{
	const float Cap = 12.f;

	float Progress = USBRulesLibrary::TickCaptureProgress(0.f, 3.f, 1, 0, Cap);
	TestEqual(TEXT("Uncontested attackers advance"), Progress, 3.f);

	Progress = USBRulesLibrary::TickCaptureProgress(Progress, 2.f, 2, 1, Cap);
	TestEqual(TEXT("Contested holds"), Progress, 3.f);

	Progress = USBRulesLibrary::TickCaptureProgress(Progress, 4.f, 0, 0, Cap, 0.5f);
	TestEqual(TEXT("Empty decays at half rate"), Progress, 1.f);

	Progress = USBRulesLibrary::TickCaptureProgress(11.f, 2.f, 1, 0, Cap);
	TestEqual(TEXT("Clamps at capture seconds"), Progress, Cap);
	TestTrue(TEXT("Capture complete"), USBRulesLibrary::IsCaptureComplete(Progress, Cap));
	TestFalse(TEXT("Incomplete before threshold"), USBRulesLibrary::IsCaptureComplete(11.9f, Cap));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBServer_ObjectiveTickTest,
	"ShadowbaneFPS.Server.ObjectiveTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBServer_ObjectiveTickTest::RunTest(const FString& Parameters)
{
	const float Complete = 20.f;

	float Progress = USBRulesLibrary::TickObjectiveProgress(0.f, 5.f, 1, 0, Complete, 1.f);
	TestEqual(TEXT("Attackers channel"), Progress, 5.f);

	Progress = USBRulesLibrary::TickObjectiveProgress(Progress, 3.f, 1, 1, Complete, 1.f);
	TestEqual(TEXT("Contested holds"), Progress, 5.f);

	Progress = USBRulesLibrary::TickObjectiveProgress(Progress, 2.f, 0, 0, Complete, 1.f);
	TestEqual(TEXT("Empty decays"), Progress, 3.f);

	TestEqual(TEXT("Normalize mid"), USBRulesLibrary::NormalizeProgress(10.f, Complete), 0.5f);
	TestEqual(TEXT("Normalize zero total"), USBRulesLibrary::NormalizeProgress(5.f, 0.f), 0.f);
	TestTrue(TEXT("Objective complete"), USBRulesLibrary::IsObjectiveComplete(Complete, Complete));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBServer_FriendlyFireGateTest,
	"ShadowbaneFPS.Server.FriendlyFireGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBServer_FriendlyFireGateTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Same team is friendly fire"),
		USBRulesLibrary::IsFriendlyFire(ESBTeam::Attackers, ESBTeam::Attackers));
	TestFalse(TEXT("Opposite teams allowed"),
		USBRulesLibrary::IsFriendlyFire(ESBTeam::Attackers, ESBTeam::Defenders));
	TestFalse(TEXT("Unassigned shooter not friendly"),
		USBRulesLibrary::IsFriendlyFire(ESBTeam::Unassigned, ESBTeam::Attackers));
	TestFalse(TEXT("Unassigned target not friendly"),
		USBRulesLibrary::IsFriendlyFire(ESBTeam::Defenders, ESBTeam::Unassigned));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBServer_TeamBalanceJoinTest,
	"ShadowbaneFPS.Server.TeamBalanceJoin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBServer_TeamBalanceJoinTest::RunTest(const FString& Parameters)
{
	// Simulate a 5v5 fill order for the first 10 joiners.
	int32 Atk = 0;
	int32 Def = 0;
	for (int32 i = 0; i < 10; ++i)
	{
		const ESBTeam Pick = USBRulesLibrary::PickBalancedTeam(Atk, Def);
		if (Pick == ESBTeam::Attackers) { ++Atk; }
		else { ++Def; }
	}

	TestEqual(TEXT("Five attackers after fill"), Atk, 5);
	TestEqual(TEXT("Five defenders after fill"), Def, 5);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
