// Copyright shadowbanefps.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/SBRulesLibrary.h"
#include "AI/SBBotController.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBBots_TeamSplitTest,
	"ShadowbaneFPS.Bots.TeamSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBBots_TeamSplitTest::RunTest(const FString& Parameters)
{
	int32 Atk = -1;
	int32 Def = -1;

	USBRulesLibrary::SplitBotsAcrossTeams(0, 5, Atk, Def);
	TestEqual(TEXT("Zero bots"), Atk + Def, 0);

	USBRulesLibrary::SplitBotsAcrossTeams(8, 5, Atk, Def);
	TestEqual(TEXT("8 bots total"), Atk + Def, 8);
	TestTrue(TEXT("Balanced within 1"), FMath::Abs(Atk - Def) <= 1);
	TestTrue(TEXT("Respect cap"), Atk <= 5 && Def <= 5);

	USBRulesLibrary::SplitBotsAcrossTeams(20, 5, Atk, Def);
	TestEqual(TEXT("Cap at 5v5"), Atk, 5);
	TestEqual(TEXT("Cap at 5v5 def"), Def, 5);

	USBRulesLibrary::SplitBotsAcrossTeams(1, 5, Atk, Def);
	TestEqual(TEXT("Single bot to attackers"), Atk, 1);
	TestEqual(TEXT("No defenders yet"), Def, 0);

	TestNotNull(TEXT("Bot controller class exists"), ASBBotController::StaticClass());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
