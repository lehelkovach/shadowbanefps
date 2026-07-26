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

#endif // WITH_DEV_AUTOMATION_TESTS
