// Copyright shadowbanefps.

#include "SBSiegeHUD.h"
#include "Core/SBSiegeGameState.h"
#include "Core/SBPlayerState.h"
#include "Core/SBPlayerController.h"
#include "Characters/SBCharacter.h"
#include "Characters/SBCharacterArchetype.h"
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

	float Y = 24.f;

	const ASBSiegeGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASBSiegeGameState>() : nullptr;
	const ASBPlayerController* PC = Cast<ASBPlayerController>(GetOwningPlayerController());
	const ASBPlayerState* PS = PC ? PC->GetPlayerState<ASBPlayerState>() : nullptr;
	const ASBCharacter* Character = PC ? Cast<ASBCharacter>(PC->GetPawn()) : nullptr;

	if (GS)
	{
		const int32 Remaining = FMath::CeilToInt(GS->GetRemainingRegulationSeconds());
		const int32 Minutes = Remaining / 60;
		const int32 Seconds = Remaining % 60;

		DrawLine(Y, FString::Printf(TEXT("TIME %02d:%02d  |  Phase: %s  |  Stage: %s"),
			Minutes,
			Seconds,
			*UEnum::GetDisplayValueAsText(GS->GetPhase()).ToString(),
			*UEnum::GetDisplayValueAsText(GS->GetConquestStage()).ToString()));

		DrawLine(Y, FString::Printf(TEXT("Final Objective: %d%%"),
			FMath::RoundToInt(GS->GetFinalObjectiveProgress() * 100.f)),
			FLinearColor(1.f, 0.85f, 0.3f));

		if (GS->GetResult() != ESBMatchResult::Undecided)
		{
			DrawLine(Y, FString::Printf(TEXT("RESULT: %s"),
				*UEnum::GetDisplayValueAsText(GS->GetResult()).ToString()),
				FLinearColor::Yellow);
		}
	}

	if (PS)
	{
		DrawLine(Y, FString::Printf(TEXT("Team: %s"),
			*UEnum::GetDisplayValueAsText(PS->GetTeam()).ToString()),
			PS->GetTeam() == ESBTeam::Attackers
				? FLinearColor(1.f, 0.4f, 0.3f)
				: FLinearColor(0.4f, 0.6f, 1.f));

		if (const USBCharacterArchetype* Arch = PS->GetSelectedArchetype())
		{
			DrawLine(Y, FString::Printf(TEXT("Character: %s"), *Arch->DisplayName.ToString()));
		}
	}

	if (Character)
	{
		DrawLine(Y, FString::Printf(TEXT("HP: %.0f / %.0f"), Character->GetHealth(), Character->GetMaxHealth()),
			FLinearColor(0.4f, 1.f, 0.4f));
	}
	else if (PC)
	{
		DrawLine(Y, FString::Printf(TEXT("DEAD  |  Respawn in %.1fs  |  Press R when ready"),
			PC->GetRespawnTimeRemaining()),
			FLinearColor(1.f, 0.5f, 0.5f));
		DrawLine(Y, TEXT("Keys 1-0 = switch pre-built character (while dead)"), FLinearColor(0.8f, 0.8f, 0.8f));

		if (GS)
		{
			const TArray<TObjectPtr<USBCharacterArchetype>>& Roster = GS->GetLocalRoster();
			for (int32 i = 0; i < Roster.Num() && i < 10; ++i)
			{
				const USBCharacterArchetype* Arch = Roster[i].Get();
				if (!Arch)
				{
					continue;
				}
				const bool bSelected = PS && PS->GetSelectedArchetypeId() == Arch->ArchetypeId;
				DrawLine(Y,
					FString::Printf(TEXT("%d: %s%s"),
						(i + 1) % 10,
						*Arch->DisplayName.ToString(),
						bSelected ? TEXT("  <selected>") : TEXT("")),
					bSelected ? FLinearColor::Yellow : FLinearColor(0.75f, 0.75f, 0.75f));
			}
		}
	}

	DrawLine(Y, TEXT("LMB Fire  |  F Ping  |  E Interact  |  WASD Move  |  Space Jump"),
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
