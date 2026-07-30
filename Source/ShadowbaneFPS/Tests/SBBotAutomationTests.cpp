// Copyright shadowbanefps.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/SBRulesLibrary.h"
#include "AI/SBBotController.h"
#include "AI/SBBotScript.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBBots_ScriptParseTest,
	"ShadowbaneFPS.Bots.ScriptParse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBBots_ScriptParseTest::RunTest(const FString& Parameters)
{
	const FString Text = TEXT(
		"id: TestHealer\n"
		"role: healer\n"
		"retarget: 1.0\n"
		"engage: 1800\n"
		"fire: 0.5\n"
		"# peel first\n"
		"when ally_hurt in_range 3500 -> pursue\n"
		"when enemy in_range 1800 -> pursue fire\n"
		"when capture open -> pursue\n"
		"when else -> hold\n");

	FSBBotScript Script;
	FString Error;
	TestTrue(TEXT("Parse succeeds"), USBBotScriptLibrary::ParseScriptText(Text, Script, Error));
	TestEqual(TEXT("Script id"), Script.ScriptId, FName(TEXT("TestHealer")));
	TestEqual(TEXT("Role"), Script.Role, FName(TEXT("healer")));
	TestEqual(TEXT("Retarget"), Script.RetargetSeconds, 1.0f);
	TestEqual(TEXT("Engage"), Script.EngageRange, 1800.f);
	TestEqual(TEXT("Fire"), Script.FireInterval, 0.5f);
	TestEqual(TEXT("Rule count"), Script.Rules.Num(), 4);
	TestEqual(TEXT("First target ally"), Script.Rules[0].Target, ESBBotTargetKind::AllyHurt);
	TestEqual(TEXT("First range"), Script.Rules[0].InRange, 3500.f);
	TestEqual(TEXT("PursueFire"), Script.Rules[1].Action, ESBBotAction::PursueFire);
	TestEqual(TEXT("Else hold"), Script.Rules[3].Action, ESBBotAction::Hold);

	FSBBotWorldFacts Facts;
	Facts.EnemyDistance = 1200.f;
	// No actors — enemy rule should not match without NearestEnemy.
	FSBBotDecision Decision;
	TestTrue(TEXT("Else matches when nothing else does"),
		USBBotScriptLibrary::EvaluateRules(Script, Facts, Decision));
	TestEqual(TEXT("Matched else"), Decision.MatchedTarget, ESBBotTargetKind::Else);
	TestTrue(TEXT("Hold"), Decision.bHold);

	FSBBotScript Missing;
	USBBotScriptLibrary::LoadScriptById(FName(TEXT("__MissingBotScript__")), Missing);
	TestTrue(TEXT("Missing script falls back with rules"), Missing.Rules.Num() > 0);

	FSBBotScript DefaultBuiltIn = USBBotScriptLibrary::MakeDefaultScript(FName(TEXT("Default")));
	TestTrue(TEXT("Built-in has enemy rule"), DefaultBuiltIn.Rules.Num() >= 4);
	TestEqual(TEXT("Built-in first is enemy"), DefaultBuiltIn.Rules[0].Target, ESBBotTargetKind::Enemy);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
