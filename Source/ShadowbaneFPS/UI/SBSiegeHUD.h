// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SBSiegeHUD.generated.h"

/** Debug / pilot HUD: timer, phase, conquest, health, roster while dead (§11). */
UCLASS()
class SHADOWBANEFPS_API ASBSiegeHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawLine(float& Y, const FString& Text, const FLinearColor& Color = FLinearColor::White);
};
