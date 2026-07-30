// Copyright shadowbanefps.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/SBMatchTelemetry.h"
#include "Core/SBTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBTelemetry_SessionLifecycleTest,
	"ShadowbaneFPS.Telemetry.SessionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBTelemetry_SessionLifecycleTest::RunTest(const FString& Parameters)
{
	USBMatchTelemetry* Telemetry = USBMatchTelemetry::Create(GetTransientPackage());
	TestNotNull(TEXT("Telemetry created"), Telemetry);

	Telemetry->StartMatchSession();
	TestTrue(TEXT("MatchStarted recorded"), Telemetry->HasRecorded(ESBTelemetryEvent::MatchStarted));
	TestTrue(TEXT("At least one event after start"), Telemetry->GetEventCount() >= 1);

	Telemetry->RecordPhase(ESBMatchPhase::Breach);
	TestTrue(TEXT("PhaseChanged recorded"), Telemetry->HasRecorded(ESBTelemetryEvent::PhaseChanged));

	Telemetry->RecordConquestStage(ESBConquestStage::Courtyard);
	TestTrue(TEXT("CourtyardCaptured recorded"), Telemetry->HasRecorded(ESBTelemetryEvent::CourtyardCaptured));

	Telemetry->RecordPlayerSpawn(TEXT("TestPlayer"), ESBTeam::Attackers, FName(TEXT("Warrior_Blade")));
	TestTrue(TEXT("PlayerSpawned recorded"), Telemetry->HasRecorded(ESBTelemetryEvent::PlayerSpawned));

	Telemetry->RecordArchetypeSwitch(TEXT("TestPlayer"), FName(TEXT("Warrior_Blade")), FName(TEXT("Ranger_Scout")));
	TestTrue(TEXT("ArchetypeSwitched recorded"), Telemetry->HasRecorded(ESBTelemetryEvent::ArchetypeSwitched));

	const int32 BeforeEnd = Telemetry->GetEventCount();
	Telemetry->EndMatchSession(ESBMatchResult::AttackersWin);
	TestTrue(TEXT("MatchEnded recorded"), Telemetry->HasRecorded(ESBTelemetryEvent::MatchEnded));
	TestTrue(TEXT("Event count grew on end"), Telemetry->GetEventCount() > BeforeEnd);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBTelemetry_CombatBalanceAttributionTest,
	"ShadowbaneFPS.Telemetry.CombatBalanceAttribution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBTelemetry_CombatBalanceAttributionTest::RunTest(const FString& Parameters)
{
	USBMatchTelemetry* Telemetry = USBMatchTelemetry::Create(GetTransientPackage());
	Telemetry->StartMatchSession();

	FSBCombatMetric Hit;
	Hit.AttackerName = TEXT("A");
	Hit.AttackerArchetype = FName(TEXT("Channeler_Flame"));
	Hit.AttackerTeam = ESBTeam::Attackers;
	Hit.VictimName = TEXT("B");
	Hit.VictimArchetype = FName(TEXT("Templar_Bulwark"));
	Hit.VictimTeam = ESBTeam::Defenders;
	Hit.PowerId = FName(TEXT("BasicFire"));
	Hit.Amount = 22.f;
	Hit.VictimHealthAfter = 148.f;
	Telemetry->RecordCombatDamage(Hit);

	Hit.Amount = 40.f;
	Hit.VictimHealthAfter = 108.f;
	Hit.PowerId = FName(TEXT("FlameBolt"));
	Telemetry->RecordCombatDamage(Hit);

	Telemetry->RecordPlayerKill(
		TEXT("B"), TEXT("A"),
		FName(TEXT("Templar_Bulwark")),
		FName(TEXT("Channeler_Flame")),
		FName(TEXT("FlameBolt")));

	TestTrue(TEXT("CombatDamage event recorded"), Telemetry->HasRecorded(ESBTelemetryEvent::CombatDamage));
	TestTrue(TEXT("PlayerKilled recorded"), Telemetry->HasRecorded(ESBTelemetryEvent::PlayerKilled));
	TestTrue(TEXT("Combat rows present"), Telemetry->GetCombatRowCount() >= 3);
	TestEqual(TEXT("Damage sum Channeler->Templar"),
		Telemetry->GetDamageBetween(FName(TEXT("Channeler_Flame")), FName(TEXT("Templar_Bulwark"))),
		62.f);
	TestEqual(TEXT("Kills Channeler->Templar"),
		Telemetry->GetKillsBetween(FName(TEXT("Channeler_Flame")), FName(TEXT("Templar_Bulwark"))),
		1);

	Telemetry->EndMatchSession(ESBMatchResult::AttackersWin);
	TestTrue(TEXT("BalanceSummary written"), Telemetry->HasRecorded(ESBTelemetryEvent::BalanceSummary));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
