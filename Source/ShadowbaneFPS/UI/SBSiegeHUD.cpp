// Copyright shadowbanefps.

#include "SBSiegeHUD.h"
#include "Core/SBSiegeGameState.h"
#include "Core/SBPlayerState.h"
#include "Core/SBPlayerController.h"
#include "Core/SBRulesLibrary.h"
#include "Core/SBLog.h"
#include "Core/SBClientDebug.h"
#include "Core/SBMatchShopCatalog.h"
#include "Characters/SBCharacter.h"
#include "Characters/SBCharacterArchetype.h"
#include "Characters/SBShadowbaneCreationDb.h"
#include "Art/SBPlaceholderArt.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
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
		FSBClientDebug::PushMessage(TEXT("HUD online"), 4.f);
	}

	const float Delta = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	FSBClientDebug::TickMessages(Delta);
	DrawDamageFloaters(Delta);
	DrawWorldNametags();

	float Y = 24.f;
	DrawConnectionPanel(Y);

	const ASBSiegeGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASBSiegeGameState>() : nullptr;
	const ASBPlayerController* PC = Cast<ASBPlayerController>(GetOwningPlayerController());
	const ASBPlayerState* PS = PC ? PC->GetPlayerState<ASBPlayerState>() : nullptr;
	const ASBCharacter* Character = PC ? Cast<ASBCharacter>(PC->GetPawn()) : nullptr;
	const bool bFFA = GS && GS->IsFreeForAll();

	if (GS)
	{
		if (bFFA)
		{
			DrawLine(Y, TEXT("MODE  FFA DEATHMATCH  |  Open lobby — join anytime"),
				FLinearColor(1.f, 0.75f, 0.35f));
		}
		else
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
		}

		if (GS->GetResult() != ESBMatchResult::Undecided)
		{
			DrawLine(Y, FString::Printf(TEXT("RESULT: %s"),
				*UEnum::GetDisplayValueAsText(GS->GetResult()).ToString()),
				FLinearColor::Yellow);
		}
	}

	if (PS)
	{
		DrawLine(Y, FString::Printf(TEXT("You: %s  |  K/D %d/%d"),
			*PS->GetPlayerName(), PS->GetKillCount(), PS->GetDeathCount()),
			FLinearColor(0.95f, 0.95f, 0.7f));

		if (!bFFA)
		{
			const FLinearColor TeamTint = USBPlaceholderArt::TeamColor(PS->GetTeam());
			DrawLine(Y, FString::Printf(TEXT("Team: %s"),
				*UEnum::GetDisplayValueAsText(PS->GetTeam()).ToString()),
				TeamTint);
		}

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
		const float HpPct = Character->GetMaxHealth() > 0.f ? Character->GetHealth() / Character->GetMaxHealth() : 0.f;
		const float ManaPct = Character->GetMaxMana() > 0.f ? Character->GetMana() / Character->GetMaxMana() : 0.f;
		const float StamPct = Character->GetMaxStamina() > 0.f ? Character->GetStamina() / Character->GetMaxStamina() : 0.f;

		DrawLine(Y, FString::Printf(TEXT("HP  %3.0f / 100"),
			Character->GetMaxHealth() > 0.f ? (Character->GetHealth() / Character->GetMaxHealth()) * 100.f : 0.f),
			FLinearColor(0.4f, 1.f, 0.4f));
		DrawVitalBar(24.f, Y, 240.f, 14.f, HpPct, FLinearColor(0.2f, 0.85f, 0.3f, 0.95f));
		Y += 20.f;

		DrawLine(Y, FString::Printf(TEXT("MP  %3.0f / 100"), ManaPct * 100.f),
			FLinearColor(0.35f, 0.55f, 1.f));
		DrawVitalBar(24.f, Y, 240.f, 12.f, ManaPct, FLinearColor(0.25f, 0.45f, 0.95f, 0.95f));
		Y += 18.f;

		DrawLine(Y, FString::Printf(TEXT("ST  %3.0f / 100"), StamPct * 100.f),
			FLinearColor(0.95f, 0.85f, 0.25f));
		DrawVitalBar(24.f, Y, 240.f, 12.f, StamPct, FLinearColor(0.9f, 0.75f, 0.15f, 0.95f));
		Y += 22.f;
	}
	else if (PC && USBRulesLibrary::ShouldShowDeathOverlay(PS ? PS->IsAlive() : false, false))
	{
		DrawLine(Y, FString::Printf(TEXT("DEAD  |  Respawn in %.1fs  |  Press R"),
			PC->GetRespawnTimeRemaining()),
			FLinearColor(1.f, 0.5f, 0.5f));
		DrawLine(Y, TEXT("1-0 roster  |  C shadowbanefps builder"), FLinearColor(0.8f, 0.8f, 0.8f));
		if (PS)
		{
			const FString Summary = PS->GetCreationSummary();
			if (!Summary.IsEmpty())
			{
				DrawLine(Y, FString::Printf(TEXT("Build overlay: %s"), *Summary), FLinearColor(0.7f, 0.9f, 1.f));
			}
		}
		DrawCreationBuilder(Y);
		DrawRosterChips(Y);
	}

	DrawAbilityRunes();

	DrawLine(Y, TEXT("LMB Attack  |  B SpawnBots  |  F Ping  |  E Interact  |  WASD"),
		FLinearColor(0.55f, 0.55f, 0.55f));
}

void ASBSiegeHUD::AddDamageFloater(UWorld* World, const FVector& WorldLocation, float Amount, bool bHeal)
{
	if (!World || Amount <= 0.f)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		ASBSiegeHUD* HUD = PC ? Cast<ASBSiegeHUD>(PC->GetHUD()) : nullptr;
		if (!HUD)
		{
			continue;
		}

		FSBDamageFloater& Floater = HUD->DamageFloaters.AddDefaulted_GetRef();
		Floater.WorldPos = WorldLocation + FVector(0.f, 0.f, 90.f);
		Floater.Amount = Amount;
		Floater.bHeal = bHeal;
		Floater.Color = bHeal ? FLinearColor(0.35f, 1.f, 0.55f) : FLinearColor(1.f, 0.3f, 0.2f);
	}
}

void ASBSiegeHUD::DrawDamageFloaters(float DeltaSeconds)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!Canvas || !PC)
	{
		return;
	}

	for (int32 Index = DamageFloaters.Num() - 1; Index >= 0; --Index)
	{
		FSBDamageFloater& Floater = DamageFloaters[Index];
		Floater.Age += DeltaSeconds;
		if (Floater.Age >= Floater.Lifetime)
		{
			DamageFloaters.RemoveAtSwap(Index);
			continue;
		}

		FVector2D ScreenPos;
		if (!PC->ProjectWorldLocationToScreen(Floater.WorldPos, ScreenPos, true))
		{
			continue;
		}

		const float Progress = Floater.Age / Floater.Lifetime;
		const float Alpha = 1.f - Progress;
		const float Rise = 54.f * Progress;
		const FString Text = FString::Printf(TEXT("%s%d"), Floater.bHeal ? TEXT("+") : TEXT("-"),
			FMath::RoundToInt(Floater.Amount));
		FCanvasTextItem Item(
			ScreenPos + FVector2D(-14.f, -Rise),
			FText::FromString(Text),
			GEngine->GetLargeFont(),
			FLinearColor(Floater.Color.R, Floater.Color.G, Floater.Color.B, Alpha));
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, Alpha));
		Item.Scale = FVector2D(0.8f, 0.8f);
		Canvas->DrawItem(Item);
	}
}

void ASBSiegeHUD::DrawConnectionPanel(float& Y)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const ENetMode NetMode = World->GetNetMode();
	FString ModeLabel = TEXT("Standalone");
	FLinearColor ModeColor(0.55f, 0.9f, 0.55f);
	switch (NetMode)
	{
	case NM_Client:
		ModeLabel = TEXT("Client");
		ModeColor = FLinearColor(0.45f, 0.75f, 1.f);
		break;
	case NM_ListenServer:
		ModeLabel = TEXT("Listen");
		ModeColor = FLinearColor(0.95f, 0.8f, 0.35f);
		break;
	case NM_DedicatedServer:
		ModeLabel = TEXT("Dedicated");
		ModeColor = FLinearColor(1.f, 0.45f, 0.45f);
		break;
	default:
		break;
	}

	const FURL& URL = World->URL;
	FString Endpoint = TEXT("local");
	if (!URL.Host.IsEmpty())
	{
		Endpoint = FString::Printf(TEXT("%s:%d"), *URL.Host, URL.Port > 0 ? URL.Port : 7777);
	}
	else if (NetMode == NM_Client)
	{
		Endpoint = TEXT("(resolving host...)");
	}

	FString Status = TEXT("Local session");
	if (NetMode == NM_Client)
	{
		Status = World->GetGameState() ? TEXT("Connected") : TEXT("Connecting...");
	}
	else if (NetMode == NM_ListenServer)
	{
		Status = TEXT("Hosting (listen)");
	}

	DrawLine(Y, FString::Printf(TEXT("NET %s  ->  %s"), *ModeLabel, *Endpoint), ModeColor);
	DrawLine(Y, FString::Printf(TEXT("STATUS %s"), *Status),
		Status.Contains(TEXT("Connecting")) ? FLinearColor(1.f, 0.85f, 0.3f) : FLinearColor(0.75f, 0.95f, 0.75f));

	// Always show a few recent client messages (connect / phase / errors).
	TArray<FString> Msgs;
	FSBClientDebug::GetActiveMessages(Msgs);
	const int32 MaxAlways = FSBClientDebug::IsEnabled() ? 8 : 3;
	for (int32 i = 0; i < Msgs.Num() && i < MaxAlways; ++i)
	{
		DrawLine(Y, FString::Printf(TEXT("> %s"), *Msgs[i]), FLinearColor(0.85f, 0.9f, 1.f));
	}

	if (!FSBClientDebug::IsEnabled())
	{
		return;
	}

	DrawLine(Y, TEXT("--- SBDebug ---"), FLinearColor(0.5f, 0.55f, 0.65f));
	DrawLine(Y, FString::Printf(TEXT("Map %s"), *World->GetMapName()), FLinearColor(0.7f, 0.7f, 0.75f));

	if (const APlayerController* PC = GetOwningPlayerController())
	{
		float PingMs = 0.f;
		if (const APlayerState* PS = PC->PlayerState)
		{
			PingMs = PS->GetPingInMilliseconds();
			DrawLine(Y, FString::Printf(TEXT("Player %s  ping=%.0fms"), *PS->GetPlayerName(), PingMs),
				FLinearColor(0.7f, 0.85f, 1.f));
		}
		if (FSBClientDebug::IsVerbose())
		{
			if (UNetConnection* Conn = PC->GetNetConnection())
			{
				DrawLine(Y, FString::Printf(TEXT("Conn %s"), *Conn->LowLevelDescribe()),
					FLinearColor(0.65f, 0.7f, 0.8f));
			}
			DrawLine(Y, FString::Printf(TEXT("URL %s"), *URL.ToString()), FLinearColor(0.55f, 0.6f, 0.7f));
		}
	}
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
	DrawVitalBar(X, Y, W, H, Pct, FLinearColor(0.25f, 0.85f, 0.35f, 0.95f));
}

void ASBSiegeHUD::DrawVitalBar(float X, float Y, float W, float H, float Pct, const FLinearColor& Fill)
{
	const float Clamped = FMath::Clamp(Pct, 0.f, 1.f);
	FCanvasTileItem Back(FVector2D(X, Y), FVector2D(W, H), FLinearColor(0.1f, 0.1f, 0.1f, 0.8f));
	Canvas->DrawItem(Back);
	FCanvasTileItem FillTile(FVector2D(X, Y), FVector2D(W * Clamped, H), Fill);
	Canvas->DrawItem(FillTile);
}

void ASBSiegeHUD::DrawCreationBuilder(float& Y)
{
	ASBPlayerController* PC = Cast<ASBPlayerController>(GetOwningPlayerController());
	if (!PC || !PC->IsCreationBuilderOpen() || !Canvas)
	{
		return;
	}

	const FSBCharacterBuildState Build = PC->GetCreationBuild();
	DrawLine(Y, TEXT("--- shadowbanefps Creation Builder ---"), FLinearColor(1.f, 0.85f, 0.4f));
	const FString NameHint = PC->IsHeroNameEditing()
		? TEXT("  ◂ typing (Enter locks)")
		: TEXT("  (N to type, or SBName)");
	DrawLine(Y, FString::Printf(TEXT("Hero name: %s%s"),
		*PC->GetCreationHeroName(), *NameHint),
		PC->IsHeroNameEditing() ? FLinearColor(1.f, 1.f, 0.4f) : FLinearColor(1.f, 0.95f, 0.55f));
	DrawLine(Y, Build.StatusLine, FLinearColor(0.95f, 0.95f, 0.8f));
	DrawLine(Y, FString::Printf(
		TEXT("FPS HP %.0f  Mana %.0f  Stam %.0f  Speed %.0f  Dmg %.0f  %s"),
		Build.Vitals.MaxHealth, Build.Vitals.MaxMana, Build.Vitals.MaxStamina,
		Build.Vitals.MoveSpeed, Build.Vitals.AttackDamage,
		Build.Vitals.bPreferMelee ? TEXT("MELEE") : TEXT("RANGED")),
		FLinearColor(0.6f, 0.9f, 1.f));
	if (Build.InnateAbilities.Num() > 0)
	{
		DrawLine(Y, FString::Printf(TEXT("Innate: %s"), *FString::Join(Build.InnateAbilities, TEXT(", "))),
			FLinearColor(0.75f, 0.75f, 0.9f));
	}
	DrawLine(Y, FString::Printf(TEXT("Discipline: %s"),
		Build.Discipline.IsEmpty() ? TEXT("(none eligible)") : *Build.Discipline),
		FLinearColor(0.95f, 0.8f, 0.55f));
	const int32 MaxAbilityLines = 5;
	for (int32 i = 0; i < Build.DisciplineAbilities.Num() && i < MaxAbilityLines; ++i)
	{
		const FSBCreationAbilityLine& A = Build.DisciplineAbilities[i];
		FString Summary = A.Summary;
		if (Summary.Len() > 56)
		{
			Summary = Summary.Left(53) + TEXT("...");
		}
		DrawLine(Y, FString::Printf(TEXT("  · %s — %s"), *A.Name, *Summary),
			FLinearColor(0.7f, 0.85f, 0.7f));
	}
	if (Build.DisciplineAbilities.Num() > MaxAbilityLines)
	{
		DrawLine(Y, FString::Printf(TEXT("  · +%d more"), Build.DisciplineAbilities.Num() - MaxAbilityLines),
			FLinearColor(0.55f, 0.65f, 0.55f));
	}

	USBShadowbaneCreationDb& Db = USBShadowbaneCreationDb::Get();
	Db.EnsureLoaded();
	DrawLine(Y, FString::Printf(TEXT("Powers DB loaded: %d  |  Skills: %d"),
		Db.GetPowerCount(), Db.GetSkillCount()),
		FLinearColor(0.55f, 0.7f, 0.85f));

	const ASBSiegeGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASBSiegeGameState>() : nullptr;
	USBMatchShopCatalog& Shop = GS ? GS->GetMatchShopCatalog() : USBMatchShopCatalog::Get();
	Shop.EnsureLoaded();
	TArray<FString> Preview;
	Shop.GetPreviewItemNames(6, Preview);
	DrawLine(Y, FString::Printf(TEXT("Shop catalog: %d items (%d affixes)  e.g. %s"),
		Shop.GetItemCount(), Shop.GetAffixCount(),
		Preview.Num() > 0 ? *FString::Join(Preview, TEXT(", ")) : TEXT("(empty)")),
		FLinearColor(0.6f, 0.65f, 0.8f));

	DrawLine(Y, TEXT("N name   [ ] race   , . path   - = prestige   ; ' discipline   Enter confirm"),
		FLinearColor(0.7f, 0.7f, 0.7f));
	DrawLine(Y, TEXT("Or console: SBName YourName"),
		FLinearColor(0.65f, 0.7f, 0.65f));
	Y += 6.f;
}

void ASBSiegeHUD::DrawWorldNametags()
{
	APlayerController* PC = GetOwningPlayerController();
	UWorld* World = GetWorld();
	if (!Canvas || !PC || !World || !GEngine)
	{
		return;
	}

	const APawn* LocalPawn = PC->GetPawn();
	const ASBSiegeGameState* GS = World->GetGameState<ASBSiegeGameState>();
	const bool bFFA = GS && GS->IsFreeForAll();
	const float MaxDistSq = FMath::Square(4500.f);

	for (TActorIterator<ASBCharacter> It(World); It; ++It)
	{
		ASBCharacter* Other = *It;
		if (!Other || Other->GetHealth() <= 0.f)
		{
			continue;
		}

		const ASBPlayerState* TheirPS = Other->GetPlayerState<ASBPlayerState>();
		if (!TheirPS)
		{
			continue;
		}

		const FVector WorldPos = Other->GetActorLocation() + FVector(0.f, 0.f, 110.f);
		if (LocalPawn && FVector::DistSquared(LocalPawn->GetActorLocation(), WorldPos) > MaxDistSq)
		{
			continue;
		}

		FVector2D ScreenPos;
		if (!PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos, true))
		{
			continue;
		}

		const bool bSelf = (Other == LocalPawn);
		FString Label = TheirPS->GetPlayerName();
		if (Label.IsEmpty())
		{
			Label = TEXT("Hero");
		}
		Label = FString::Printf(TEXT("%s  %d/%d"), *Label, TheirPS->GetKillCount(), TheirPS->GetDeathCount());

		FLinearColor Color = bSelf
			? FLinearColor(0.95f, 0.95f, 0.55f)
			: (bFFA ? FLinearColor(1.f, 0.45f, 0.35f) : USBPlaceholderArt::TeamColor(TheirPS->GetTeam()));

		UFont* Font = GEngine->GetLargeFont();
		float TextW = 0.f;
		float TextH = 0.f;
		Canvas->StrLen(Font, Label, TextW, TextH);
		const float Scale = 0.85f;
		const FVector2D DrawAt(ScreenPos.X - TextW * 0.5f * Scale, ScreenPos.Y - TextH * Scale);

		FCanvasTextItem Item(DrawAt, FText::FromString(Label), Font, Color);
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.9f));
		Item.Scale = FVector2D(Scale, Scale);
		Canvas->DrawItem(Item);
	}
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

	const TCHAR* Glyphs[4] = { TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4") };
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
		FVector2D((Canvas->ClipX - 380.f) * 0.5f, Y + Size + 4.f),
		FText::FromString(TEXT("E crew ram  |  1-4 cast (stand still)  |  LMB attack  |  B recall  |  R respawn")),
		GEngine->GetSmallFont(),
		FLinearColor(0.6f, 0.6f, 0.6f));
	Canvas->DrawItem(Hint);
}
