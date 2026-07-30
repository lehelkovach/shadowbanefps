// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SBSiegeHUD.generated.h"

/** Pilot HUD with dummy icon chips / rune glyphs (§11). */
UCLASS()
class SHADOWBANEFPS_API ASBSiegeHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawLine(float& Y, const FString& Text, const FLinearColor& Color = FLinearColor::White);
	void DrawHealthBar(float X, float Y, float W, float H, float Pct);
	void DrawRosterChips(float& Y);
	void DrawAbilityRunes();
};
