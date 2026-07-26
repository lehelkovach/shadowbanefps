// Copyright shadowbanefps.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Art/SBPlaceholderArt.h"
#include "Characters/SBPilotRoster.h"
#include "Characters/SBCharacterArchetype.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSBArt_PlaceholderIconsTest,
	"ShadowbaneFPS.Art.PlaceholderIcons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSBArt_PlaceholderIconsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Attacker color is reddish"),
		USBPlaceholderArt::TeamColor(ESBTeam::Attackers).R >
		USBPlaceholderArt::TeamColor(ESBTeam::Attackers).B);

	TestTrue(TEXT("Defender color is bluish"),
		USBPlaceholderArt::TeamColor(ESBTeam::Defenders).B >
		USBPlaceholderArt::TeamColor(ESBTeam::Defenders).R);

	TArray<TObjectPtr<USBCharacterArchetype>> Roster;
	USBPilotRoster::BuildDefaultRoster(GetTransientPackage(), Roster);

	TSet<FString> Codes;
	for (const USBCharacterArchetype* Arch : Roster)
	{
		const FSBPlaceholderIcon Icon = USBPlaceholderArt::MakeIcon(Arch);
		TestTrue(TEXT("Icon code length 2-4"), Icon.Code.Len() >= 2 && Icon.Code.Len() <= 4);
		TestTrue(TEXT("Icon tint opaque-ish"), Icon.Tint.A > 0.5f);
		Codes.Add(Icon.Code);
	}

	TestTrue(TEXT("Multiple distinct icon codes"), Codes.Num() >= 6);
	return true;
}

#endif
