// Copyright shadowbanefps.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/SBSpawnPoint.h"
#include "Core/SBTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBSpawn_AvailabilityMatrixTest,
	"ShadowbaneFPS.Spawn.AvailabilityMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBSpawn_AvailabilityMatrixTest::RunTest(const FString& Parameters)
{
	// Pure availability matrix — no world required.
	ASBSpawnPoint* AtkStaging = NewObject<ASBSpawnPoint>();
	AtkStaging->Team = ESBTeam::Attackers;
	AtkStaging->MinStage = ESBConquestStage::OuterSiege;
	AtkStaging->MaxStage = ESBConquestStage::OuterSiege;

	ASBSpawnPoint* AtkForward = NewObject<ASBSpawnPoint>();
	AtkForward->Team = ESBTeam::Attackers;
	AtkForward->MinStage = ESBConquestStage::Courtyard;
	AtkForward->MaxStage = ESBConquestStage::InnerKeep;

	ASBSpawnPoint* DefKeep = NewObject<ASBSpawnPoint>();
	DefKeep->Team = ESBTeam::Defenders;
	DefKeep->MinStage = ESBConquestStage::InnerKeep;
	DefKeep->MaxStage = ESBConquestStage::InnerKeep;

	TestTrue(TEXT("Staging open at start"),
		AtkStaging->IsAvailableFor(ESBTeam::Attackers, ESBConquestStage::OuterSiege));
	TestFalse(TEXT("Staging closed after courtyard"),
		AtkStaging->IsAvailableFor(ESBTeam::Attackers, ESBConquestStage::Courtyard));
	TestFalse(TEXT("Wrong team rejected on staging"),
		AtkStaging->IsAvailableFor(ESBTeam::Defenders, ESBConquestStage::OuterSiege));

	TestFalse(TEXT("Forward closed at start"),
		AtkForward->IsAvailableFor(ESBTeam::Attackers, ESBConquestStage::OuterSiege));
	TestTrue(TEXT("Forward open at courtyard"),
		AtkForward->IsAvailableFor(ESBTeam::Attackers, ESBConquestStage::Courtyard));
	TestTrue(TEXT("Forward still open at inner keep"),
		AtkForward->IsAvailableFor(ESBTeam::Attackers, ESBConquestStage::InnerKeep));

	TestFalse(TEXT("Keep spawn closed before InnerKeep"),
		DefKeep->IsAvailableFor(ESBTeam::Defenders, ESBConquestStage::Courtyard));
	TestTrue(TEXT("Keep spawn open at InnerKeep"),
		DefKeep->IsAvailableFor(ESBTeam::Defenders, ESBConquestStage::InnerKeep));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
