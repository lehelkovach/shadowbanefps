// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SBSiegeHUD.generated.h"

/** Pilot HUD with dummy icon chips / rune glyphs (§11) + connection debug strip. */
UCLASS()
class SHADOWBANEFPS_API ASBSiegeHUD : public AHUD
{
	GENERATED_BODY()

public:
	struct FSBDamageFloater
	{
		FVector WorldPos = FVector::ZeroVector;
		float Amount = 0.f;
		float Age = 0.f;
		float Lifetime = 0.85f;
		bool bHeal = false;
		FLinearColor Color = FLinearColor::White;
	};

	virtual void DrawHUD() override;

	/** Adds a combat number to every local HUD in this world. */
	static void AddDamageFloater(UWorld* World, const FVector& WorldLocation, float Amount, bool bHeal);

private:
	void DrawLine(float& Y, const FString& Text, const FLinearColor& Color = FLinearColor::White);
	void DrawHealthBar(float X, float Y, float W, float H, float Pct);
	void DrawVitalBar(float X, float Y, float W, float H, float Pct, const FLinearColor& Fill);
	void DrawRosterChips(float& Y);
	void DrawCreationBuilder(float& Y);
	void DrawAbilityRunes();
	void DrawConnectionPanel(float& Y);
	void DrawDamageFloaters(float DeltaSeconds);
	void DrawWorldNametags();

	/** One-shot client log so HUD readiness shows up without spam. */
	bool bLoggedHudReady = false;
	TArray<FSBDamageFloater> DamageFloaters;
};
