// Copyright shadowbanefps.
//
// Client-side unit tests (HUD clock, death overlay, respawn UX).
// Run: .\scripts\RunClientTests.ps1

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/SBRulesLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBClient_HudClockTest,
	"ShadowbaneFPS.Client.HudClock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBClient_HudClockTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Exact minute"), USBRulesLibrary::FormatMatchClock(120.f), FString(TEXT("02:00")));
	TestEqual(TEXT("Ceil fractional"), USBRulesLibrary::FormatMatchClock(59.1f), FString(TEXT("01:00")));
	TestEqual(TEXT("Under one minute"), USBRulesLibrary::FormatMatchClock(9.f), FString(TEXT("00:09")));
	TestEqual(TEXT("Clamps negative"), USBRulesLibrary::FormatMatchClock(-3.f), FString(TEXT("00:00")));
	TestEqual(TEXT("Twenty minute pilot"), USBRulesLibrary::FormatMatchClock(1200.f), FString(TEXT("20:00")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBClient_DeathOverlayTest,
	"ShadowbaneFPS.Client.DeathOverlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBClient_DeathOverlayTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Dead without pawn"), USBRulesLibrary::ShouldShowDeathOverlay(false, false));
	TestTrue(TEXT("Alive but no pawn (transition)"), USBRulesLibrary::ShouldShowDeathOverlay(true, false));
	TestFalse(TEXT("Alive with pawn"), USBRulesLibrary::ShouldShowDeathOverlay(true, true));
	TestTrue(TEXT("Dead with lingering pawn flag"), USBRulesLibrary::ShouldShowDeathOverlay(false, true));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBClient_RespawnAndSwitchUxTest,
	"ShadowbaneFPS.Client.RespawnAndSwitchUx",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBClient_RespawnAndSwitchUxTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Can switch while dead"), USBRulesLibrary::CanSelectArchetypeWhileDead(false));
	TestFalse(TEXT("Cannot switch while alive"), USBRulesLibrary::CanSelectArchetypeWhileDead(true));

	TestTrue(TEXT("Respawn when flag set and timer done"),
		USBRulesLibrary::CanRequestRespawn(true, 0.f));
	TestFalse(TEXT("Blocked while countdown running"),
		USBRulesLibrary::CanRequestRespawn(false, 2.5f));
	TestFalse(TEXT("Blocked if flag false even at zero"),
		USBRulesLibrary::CanRequestRespawn(false, 0.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBClient_ObjectiveHudProgressTest,
	"ShadowbaneFPS.Client.ObjectiveHudProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBClient_ObjectiveHudProgressTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("0%"), USBRulesLibrary::NormalizeProgress(0.f, 25.f), 0.f);
	TestEqual(TEXT("40%"), USBRulesLibrary::NormalizeProgress(10.f, 25.f), 0.4f);
	TestEqual(TEXT("100%"), USBRulesLibrary::NormalizeProgress(25.f, 25.f), 1.f);
	TestEqual(TEXT("Clamp over"), USBRulesLibrary::NormalizeProgress(40.f, 25.f), 1.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
