// Copyright shadowbanefps.

#include "SBSiegeHUD.h"
#include "Core/SBSiegeGameState.h"
#include "Core/SBPlayerState.h"
#include "Core/SBPlayerController.h"
#include "Core/SBRulesLibrary.h"
#include "Core/SBLog.h"
#include "Characters/SBCharacter.h"
#include "Characters/SBCharacterArchetype.h"
#include "Art/SBPlaceholderArt.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "Core/SBTypes.h"

void ASBSiegeHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	if (!bLoggedHudReady)
	{
		bLoggedHudReady = true;
		UE_LOG(LogShadowbaneClient, Log, TEXT("Siege HUD ready (canvas=%dx%d)"),
			FMath::RoundToInt(Canvas->ClipX), FMath::RoundToInt(Canvas->ClipY));
	}

	float Y = 24.f;

	const ASBSiegeGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASBSiegeGameState>() : nullptr;
	const ASBPlayerController* PC = Cast<ASBPlayerController>(GetOwningPlayerController());
	const ASBPlayerState* PS = PC ? PC->GetPlayerState<ASBPlayerState>() : nullptr;
	const ASBCharacter* Character = PC ? Cast<ASBCharacter>(PC->GetPawn()) : nullptr;

	if (GS)
	{
		const FString Clock = USBRulesLibrary::FormatMatchClock(GS->GetRemainingRegulationSeconds());

		DrawLine(Y, FString::Printf(TEXT("TIME %s  |  %s  |  %s"),
			*Clock,
			*UEnum::GetDisplayValueAsText(GS->GetPhase()).ToString(),
			*UEnum::GetDisplayValueAsText(GS->GetConquestStage()).ToString()));

		const float ObjPct = GS->GetFinalObjectiveProgress();
		DrawLine(Y, FString::Printf(TEXT("Final Objective: %d%%"), FMath::RoundToInt(ObjPct * 100.f)),
			USBPlaceholderArt::ObjectiveColor(ObjPct));
		DrawHealthBar(24.f, Y, 280.f, 10.f, ObjPct);
		Y += 18.f;

		if (GS->GetResult() != ESBMatchResult::Undecided)
		{
			DrawLine(Y, FString::Printf(TEXT("RESULT: %s"),
				*UEnum::GetDisplayValueAsText(GS->GetResult()).ToString()),
				FLinearColor::Yellow);
		}
	}

	if (PS)
	{
		const FLinearColor TeamTint = USBPlaceholderArt::TeamColor(PS->GetTeam());
		DrawLine(Y, FString::Printf(TEXT("Team: %s"),
			*UEnum::GetDisplayValueAsText(PS->GetTeam()).ToString()),
			TeamTint);

		if (const USBCharacterArchetype* Arch = PS->GetSelectedArchetype())
		{
			const FSBPlaceholderIcon Icon = USBPlaceholderArt::MakeIcon(Arch);
			USBPlaceholderArt::DrawIconChip(Canvas, 24.f, Y, 28.f, Icon, true);
			DrawLine(Y, FString::Printf(TEXT("     %s  [%s]"),
				*Arch->DisplayName.ToString(),
				*Icon.Code),
				Icon.Tint);
		}
	}

	if (Character)
	{
		const float Pct = Character->GetMaxHealth() > 0.f ? Character->GetHealth() / Character->GetMaxHealth() : 0.f;
		DrawLine(Y, FString::Printf(TEXT("HP: %.0f / %.0f"), Character->GetHealth(), Character->GetMaxHealth()),
			FLinearColor(0.4f, 1.f, 0.4f));
		DrawHealthBar(24.f, Y, 220.f, 12.f, Pct);
		Y += 20.f;
	}
	else if (PC && USBRulesLibrary::ShouldShowDeathOverlay(PS ? PS->IsAlive() : false, false))
	{
		DrawLine(Y, FString::Printf(TEXT("DEAD  |  Respawn in %.1fs  |  Press R"),
			PC->GetRespawnTimeRemaining()),
			FLinearColor(1.f, 0.5f, 0.5f));
		DrawLine(Y, TEXT("1-0 switch pre-built character"), FLinearColor(0.8f, 0.8f, 0.8f));
		DrawRosterChips(Y);
	}

	DrawAbilityRunes();

	DrawLine(Y, TEXT("LMB Fire  |  F Ping  |  E Interact  |  WASD Move"),
		FLinearColor(0.55f, 0.55f, 0.55f));
}

void ASBSiegeHUD::DrawLine(float& Y, const FString& Text, const FLinearColor& Color)
{
	if (!Canvas)
	{
		return;
	}

	const float Scale = Canvas->ClipX / 1920.f;
	FCanvasTextItem Item(FVector2D(24.f * Scale, Y), FText::FromString(Text), GEngine->GetSmallFont(), Color);
	Item.EnableShadow(FLinearColor::Black);
	Item.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(Item);
	Y += 18.f * FMath::Max(1.f, Scale);
}

void ASBSiegeHUD::DrawHealthBar(float X, float Y, float W, float H, float Pct)
{
	const float Clamped = FMath::Clamp(Pct, 0.f, 1.f);
	FCanvasTileItem Back(FVector2D(X, Y), FVector2D(W, H), FLinearColor(0.1f, 0.1f, 0.1f, 0.8f));
	Canvas->DrawItem(Back);
	FCanvasTileItem Fill(FVector2D(X, Y), FVector2D(W * Clamped, H), FLinearColor(0.25f, 0.85f, 0.35f, 0.95f));
	Canvas->DrawItem(Fill);
}

void ASBSiegeHUD::DrawRosterChips(float& Y)
{
	const ASBSiegeGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASBSiegeGameState>() : nullptr;
	const ASBPlayerState* PS = GetOwningPlayerController()
		? GetOwningPlayerController()->GetPlayerState<ASBPlayerState>()
		: nullptr;
	if (!GS || !Canvas)
	{
		return;
	}

	const TArray<TObjectPtr<USBCharacterArchetype>>& Roster = GS->GetLocalRoster();
	const float Chip = 36.f;
	const float Gap = 8.f;
	float X = 24.f;

	for (int32 i = 0; i < Roster.Num() && i < 10; ++i)
	{
		const USBCharacterArchetype* Arch = Roster[i].Get();
		if (!Arch)
		{
			continue;
		}

		const FSBPlaceholderIcon Icon = USBPlaceholderArt::MakeIcon(Arch);
		const bool bSelected = PS && PS->GetSelectedArchetypeId() == Arch->ArchetypeId;
		USBPlaceholderArt::DrawIconChip(Canvas, X, Y, Chip, Icon, bSelected);

		// Hotkey under chip
		FCanvasTextItem Key(
			FVector2D(X + 12.f, Y + Chip + 2.f),
			FText::FromString(FString::Printf(TEXT("%d"), (i + 1) % 10)),
			GEngine->GetSmallFont(),
			bSelected ? FLinearColor::Yellow : FLinearColor(0.75f, 0.75f, 0.75f));
		Canvas->DrawItem(Key);

		X += Chip + Gap;
	}

	Y += Chip + 22.f;
}

void ASBSiegeHUD::DrawAbilityRunes()
{
	if (!Canvas)
	{
		return;
	}

	// Dummy ability rune bar — stands in for real spell icons until art lands.
	const ASBPlayerState* PS = GetOwningPlayerController()
		? GetOwningPlayerController()->GetPlayerState<ASBPlayerState>()
		: nullptr;
	const USBCharacterArchetype* Arch = PS ? PS->GetSelectedArchetype() : nullptr;
	const FSBPlaceholderIcon Icon = USBPlaceholderArt::MakeIcon(Arch);

	const float Size = 42.f;
	const float Gap = 10.f;
	const float TotalW = Size * 4.f + Gap * 3.f;
	float X = (Canvas->ClipX - TotalW) * 0.5f;
	const float Y = Canvas->ClipY - Size - 28.f;

	const TCHAR* Glyphs[4] = { TEXT("I"), TEXT("II"), TEXT("III"), TEXT("R") };
	const FLinearColor Colors[4] = {
		Icon.Tint,
		Icon.Tint * FLinearColor(0.85f, 0.85f, 1.f),
		Icon.Tint * FLinearColor(1.f, 0.85f, 0.7f),
		FLinearColor(0.7f, 0.7f, 0.75f)
	};

	for (int32 i = 0; i < 4; ++i)
	{
		USBPlaceholderArt::DrawRuneGlyph(Canvas, X, Y, Size, Colors[i], Glyphs[i]);
		X += Size + Gap;
	}

	FCanvasTextItem Hint(
		FVector2D((Canvas->ClipX - 220.f) * 0.5f, Y + Size + 4.f),
		FText::FromString(TEXT("placeholder ability runes")),
		GEngine->GetSmallFont(),
		FLinearColor(0.6f, 0.6f, 0.6f));
	Canvas->DrawItem(Hint);
}
