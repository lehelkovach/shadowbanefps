// Copyright shadowbanefps.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Characters/SBPilotRoster.h"
#include "Characters/SBCharacterArchetype.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBRoster_DefaultRosterShapeTest,
	"ShadowbaneFPS.Roster.DefaultShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBRoster_DefaultRosterShapeTest::RunTest(const FString& Parameters)
{
	TArray<TObjectPtr<USBCharacterArchetype>> Roster;
	USBPilotRoster::BuildDefaultRoster(GetTransientPackage(), Roster);

	TestTrue(TEXT("Pilot roster has at least 8 archetypes"), Roster.Num() >= 8);
	TestTrue(TEXT("Pilot roster has at most 12 archetypes"), Roster.Num() <= 12);
	TestEqual(TEXT("Current curated count is 10"), Roster.Num(), 10);

	TSet<FName> Ids;
	int32 AttackerOnly = 0;
	int32 DefenderOnly = 0;
	int32 Both = 0;

	for (USBCharacterArchetype* Arch : Roster)
	{
		TestNotNull(TEXT("Archetype entry non-null"), Arch);
		if (!Arch)
		{
			continue;
		}

		TestFalse(TEXT("ArchetypeId set"), Arch->ArchetypeId.IsNone());
		TestFalse(TEXT("DisplayName set"), Arch->DisplayName.IsEmpty());
		TestTrue(TEXT("MaxHealth positive"), Arch->MaxHealth > 0.f);
		TestTrue(TEXT("MoveSpeed positive"), Arch->MoveSpeed > 0.f);
		TestTrue(TEXT("AttackDamage non-negative"), Arch->AttackDamage >= 0.f);

		const bool bAlready = Ids.Contains(Arch->ArchetypeId);
		TestFalse(FString::Printf(TEXT("Unique id %s"), *Arch->ArchetypeId.ToString()), bAlready);
		Ids.Add(Arch->ArchetypeId);

		if (Arch->bAttackerEligible && Arch->bDefenderEligible)
		{
			++Both;
		}
		else if (Arch->bAttackerEligible)
		{
			++AttackerOnly;
		}
		else if (Arch->bDefenderEligible)
		{
			++DefenderOnly;
		}
		else
		{
			AddError(FString::Printf(TEXT("Archetype %s eligible for neither side"), *Arch->ArchetypeId.ToString()));
		}
	}

	TestTrue(TEXT("At least one attacker-capable archetype"), Both + AttackerOnly > 0);
	TestTrue(TEXT("At least one defender-capable archetype"), Both + DefenderOnly > 0);
	TestTrue(TEXT("Has an attacker-only siege option"), AttackerOnly >= 1);
	TestTrue(TEXT("Has a defender-only warden option"), DefenderOnly >= 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBRoster_RoleCoverageTest,
	"ShadowbaneFPS.Roster.RoleCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBRoster_RoleCoverageTest::RunTest(const FString& Parameters)
{
	TArray<TObjectPtr<USBCharacterArchetype>> Roster;
	USBPilotRoster::BuildDefaultRoster(GetTransientPackage(), Roster);

	bool bHasHealer = false;
	bool bHasSiege = false;
	bool bHasDetection = false;
	bool bHasControl = false;

	for (const USBCharacterArchetype* Arch : Roster)
	{
		if (!Arch)
		{
			continue;
		}
		bHasHealer |= Arch->RoleProfile.Healing >= 2;
		bHasSiege |= Arch->RoleProfile.Siege >= 2;
		bHasDetection |= Arch->RoleProfile.Detection >= 2;
		bHasControl |= Arch->RoleProfile.Control >= 2;
	}

	TestTrue(TEXT("Roster includes a dedicated healer profile"), bHasHealer);
	TestTrue(TEXT("Roster includes a siege profile"), bHasSiege);
	TestTrue(TEXT("Roster includes a detection/scout profile"), bHasDetection);
	TestTrue(TEXT("Roster includes a control profile"), bHasControl);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
