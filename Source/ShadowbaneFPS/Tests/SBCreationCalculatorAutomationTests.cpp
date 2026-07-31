// Copyright shadowbanefps.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Characters/SBShadowbaneCreationDb.h"
#include "Characters/SBCharacterCalculator.h"
#include "Characters/SBPilotRoster.h"
#include "Characters/SBCharacterArchetype.h"
#include "Core/SBMatchShopCatalog.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBCreationDb_LoadTest,
	"ShadowbaneFPS.Creation.DbLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBCreationDb_LoadTest::RunTest(const FString& Parameters)
{
	USBShadowbaneCreationDb& Db = USBShadowbaneCreationDb::Get();
	TestTrue(TEXT("Creation DB loads"), Db.EnsureLoaded());
	TestTrue(TEXT("Has races"), Db.GetRaces().Num() >= 10);
	TestTrue(TEXT("Has base classes"), Db.GetBaseClasses().Num() == 4);
	TestTrue(TEXT("Has prestige"), Db.GetPrestiges().Num() >= 20);
	TestTrue(TEXT("Has CSV disciplines"), Db.GetDisciplines().Num() >= 20);
	TestTrue(TEXT("Has CSV ability rows"), Db.GetAbilityCsvRowCount() >= 100);
	TestTrue(TEXT("Has client powers"), Db.GetPowerCount() > 0);
	TestTrue(TEXT("Has client skills"), Db.GetSkillCount() > 0);
	TestNotNull(TEXT("Human race"), Db.FindRace(TEXT("Human")));
	TestNotNull(TEXT("Warrior prestige"), Db.FindPrestige(TEXT("Warrior")));
	TestNotNull(TEXT("Berserker discipline (CSV)"), Db.FindDiscipline(TEXT("Berserker")));
	TestNotNull(TEXT("FindPower Archery-ish / Cleave"), Db.FindPowerByName(TEXT("Cleave")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBClientStrings_LoadTest,
	"ShadowbaneFPS.Creation.ClientStringsLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBClientStrings_LoadTest::RunTest(const FString& Parameters)
{
	USBShadowbaneCreationDb& Db = USBShadowbaneCreationDb::Get();
	TestTrue(TEXT("Creation DB loads"), Db.EnsureLoaded());
	TestTrue(TEXT("Powers non-zero"), Db.GetPowerCount() >= 500);
	TestTrue(TEXT("Skills non-zero"), Db.GetSkillCount() >= 50);
	TestNotNull(TEXT("Skill Archery"), Db.FindSkillByName(TEXT("Archery")));

	USBMatchShopCatalog& Shop = USBMatchShopCatalog::Get();
	TestTrue(TEXT("Shop catalog loads"), Shop.EnsureLoaded());
	TestTrue(TEXT("Items non-zero"), Shop.GetItemCount() >= 1000);
	TestTrue(TEXT("Affixes non-zero"), Shop.GetAffixCount() >= 100);

	TArray<FString> Preview;
	Shop.GetPreviewItemNames(8, Preview);
	TestTrue(TEXT("Preview names"), Preview.Num() == 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBCreationCalc_HumanWarriorTest,
	"ShadowbaneFPS.Creation.HumanWarriorVitals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBCreationCalc_HumanWarriorTest::RunTest(const FString& Parameters)
{
	USBShadowbaneCreationDb& Db = USBShadowbaneCreationDb::Get();
	TestTrue(TEXT("DB loaded"), Db.EnsureLoaded());

	TArray<FString> EligibleDisc;
	Db.GetEligibleDisciplines(TEXT("Human"), TEXT("Fighter"), TEXT("Warrior"), EligibleDisc);
	TestTrue(TEXT("Human Warrior has eligible disciplines"), EligibleDisc.Num() > 0);
	TestTrue(TEXT("Berserker eligible for Human Warrior"),
		EligibleDisc.ContainsByPredicate([](const FString& N) { return N.Equals(TEXT("Berserker"), ESearchCase::IgnoreCase); }));

	FSBCharacterBuildState Build;
	Build.Race = TEXT("Human");
	Build.BaseClass = TEXT("Fighter");
	Build.Prestige = TEXT("Warrior");
	USBCharacterCalculator::Recalculate(Build);
	TestTrue(TEXT("Valid build"), Build.bValid);
	TestTrue(TEXT("HP positive"), Build.Vitals.MaxHealth > 50.f);
	TestTrue(TEXT("Mana positive"), Build.Vitals.MaxMana > 20.f);
	TestTrue(TEXT("Race cost 0 for Human"), Build.RaceCost == 0);
	TestFalse(TEXT("Discipline auto-picked"), Build.Discipline.IsEmpty());
	TestTrue(TEXT("Discipline abilities listed"), Build.DisciplineAbilities.Num() > 0);

	USBCharacterCalculator::CycleDiscipline(Build, +1);
	TestFalse(TEXT("Discipline still set after cycle"), Build.Discipline.IsEmpty());

	TArray<TObjectPtr<USBCharacterArchetype>> Roster;
	USBPilotRoster::BuildDefaultRoster(GetTransientPackage(), Roster);
	USBCharacterArchetype* Match = USBCharacterCalculator::FindBestRosterMatch(Build, Roster);
	TestNotNull(TEXT("Matches a roster entry"), Match);
	return true;
}

#endif
