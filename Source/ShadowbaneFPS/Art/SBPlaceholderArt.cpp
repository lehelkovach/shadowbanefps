// Copyright shadowbanefps.

#include "SBPlaceholderArt.h"
#include "Characters/SBCharacterArchetype.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"

namespace SBPlaceholderArtPrivate
{
	static TMap<uint32, TObjectPtr<UTexture2D>> SolidTextureCache;

	static uint32 ColorKey(const FLinearColor& Color)
	{
		const FColor SRGB = Color.ToFColor(true);
		return (uint32(SRGB.R) << 24) | (uint32(SRGB.G) << 16) | (uint32(SRGB.B) << 8) | uint32(SRGB.A);
	}
}

FLinearColor USBPlaceholderArt::TeamColor(ESBTeam Team)
{
	switch (Team)
	{
	case ESBTeam::Attackers: return FLinearColor(0.86f, 0.28f, 0.16f);
	case ESBTeam::Defenders: return FLinearColor(0.20f, 0.45f, 0.92f);
	default: return FLinearColor(0.65f, 0.65f, 0.65f);
	}
}

FLinearColor USBPlaceholderArt::StructureColor(ESBStructureState State)
{
	switch (State)
	{
	case ESBStructureState::Intact: return FLinearColor(0.55f, 0.52f, 0.48f);
	case ESBStructureState::Damaged: return FLinearColor(0.75f, 0.45f, 0.15f);
	case ESBStructureState::Destroyed: return FLinearColor(0.15f, 0.15f, 0.15f);
	default: return FLinearColor::White;
	}
}

FLinearColor USBPlaceholderArt::ObjectiveColor(float Progress01)
{
	const float T = FMath::Clamp(Progress01, 0.f, 1.f);
	return FLinearColor::LerpUsingHSV(FLinearColor(0.95f, 0.82f, 0.25f), FLinearColor(0.2f, 0.95f, 0.35f), T);
}

FString USBPlaceholderArt::InferCode(const USBCharacterArchetype* Archetype)
{
	if (!Archetype)
	{
		return TEXT("???");
	}

	const FString Id = Archetype->ArchetypeId.ToString();
	if (Id.Contains(TEXT("Warrior"))) return TEXT("WAR");
	if (Id.Contains(TEXT("Ranger"))) return TEXT("RNG");
	if (Id.Contains(TEXT("Assassin"))) return TEXT("ASN");
	if (Id.Contains(TEXT("Channeler"))) return TEXT("FLM");
	if (Id.Contains(TEXT("Healer")) || Id.Contains(TEXT("Prelate"))) return TEXT("HEA");
	if (Id.Contains(TEXT("Wizard"))) return TEXT("FRS");
	if (Id.Contains(TEXT("Scout")) || Id.Contains(TEXT("Thief"))) return TEXT("SCT");
	if (Id.Contains(TEXT("Templar"))) return TEXT("TMP");
	if (Id.Contains(TEXT("Siege")) || Id.Contains(TEXT("Engineer"))) return TEXT("SGE");
	if (Id.Contains(TEXT("Warden"))) return TEXT("WDN");

	return Id.Left(3).ToUpper();
}

FLinearColor USBPlaceholderArt::InferTint(const USBCharacterArchetype* Archetype)
{
	if (!Archetype)
	{
		return FLinearColor::White;
	}

	const FSBRoleProfile& R = Archetype->RoleProfile;
	// Dominant role picks the dummy "rune color".
	if (R.Healing >= R.Damage && R.Healing >= R.Siege && R.Healing >= R.Control && R.Healing >= R.Detection)
	{
		return FLinearColor(0.45f, 0.95f, 0.55f); // heal — green
	}
	if (R.Siege >= R.Damage && R.Siege >= R.Control && R.Siege >= R.Detection)
	{
		return FLinearColor(0.95f, 0.55f, 0.15f); // siege — amber
	}
	if (R.Control >= R.Damage && R.Control >= R.Detection && R.Control >= R.Mobility)
	{
		return FLinearColor(0.45f, 0.75f, 1.f); // control — ice blue
	}
	if (R.Detection >= R.Damage && R.Detection >= R.Mobility)
	{
		return FLinearColor(0.75f, 0.45f, 0.95f); // detection — violet
	}
	if (R.Mobility >= R.Damage)
	{
		return FLinearColor(0.95f, 0.9f, 0.35f); // mobility — gold
	}
	return FLinearColor(0.95f, 0.35f, 0.3f); // damage — crimson
}

FSBPlaceholderIcon USBPlaceholderArt::MakeIcon(const USBCharacterArchetype* Archetype)
{
	FSBPlaceholderIcon Icon;
	Icon.Code = InferCode(Archetype);
	Icon.Tint = InferTint(Archetype);
	if (Archetype)
	{
		Icon.RoleLabel = Archetype->Class;
	}
	return Icon;
}

UMaterialInstanceDynamic* USBPlaceholderArt::ApplySolidColor(UPrimitiveComponent* Mesh, FLinearColor Color, int32 MaterialIndex)
{
	if (!Mesh)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
	if (!MID)
	{
		return nullptr;
	}

	// BasicShapes / common UE materials use different parameter names — set all.
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
	MID->SetVectorParameterValue(TEXT("Tint"), Color);
	MID->SetScalarParameterValue(TEXT("Metallic"), 0.05f);
	MID->SetScalarParameterValue(TEXT("Roughness"), 0.55f);
	return MID;
}

UTexture2D* USBPlaceholderArt::GetSolidTexture(FLinearColor Color)
{
	// Canvas HUD uses solid-color tiles directly. This hook exists so UMG / widget
	// work can later request a texture without inventing a second API. Returning
	// null is fine for the greybox pilot.
	(void)Color;
	(void)SBPlaceholderArtPrivate::SolidTextureCache;
	return nullptr;
}

void USBPlaceholderArt::DrawIconChip(UCanvas* Canvas, float X, float Y, float Size, const FSBPlaceholderIcon& Icon, bool bSelected)
{
	if (!Canvas)
	{
		return;
	}

	const FLinearColor Border = bSelected ? FLinearColor::Yellow : FLinearColor(0.05f, 0.05f, 0.05f, 0.9f);
	const float BorderW = bSelected ? 3.f : 1.5f;

	FCanvasTileItem BorderTile(FVector2D(X - BorderW, Y - BorderW), FVector2D(Size + BorderW * 2.f, Size + BorderW * 2.f), Border);
	BorderTile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BorderTile);

	FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(Size, Size), Icon.Tint);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);

	if (UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr)
	{
		const float TextScale = Size / 28.f;
		float TextW = 0.f, TextH = 0.f;
		Canvas->StrLen(Font, Icon.Code, TextW, TextH);
		TextW *= TextScale;
		TextH *= TextScale;
		FCanvasTextItem Text(
			FVector2D(X + (Size - TextW) * 0.5f, Y + (Size - TextH) * 0.5f),
			FText::FromString(Icon.Code),
			Font,
			FLinearColor::Black);
		Text.Scale = FVector2D(TextScale, TextScale);
		Text.EnableShadow(FLinearColor(1.f, 1.f, 1.f, 0.25f));
		Canvas->DrawItem(Text);
	}
}

void USBPlaceholderArt::DrawRuneGlyph(UCanvas* Canvas, float X, float Y, float Size, FLinearColor Color, const FString& Glyph)
{
	if (!Canvas)
	{
		return;
	}

	// Diamond made from two triangles via rotated square tile approximation:
	// draw a square, then overlay the glyph — good enough for dummy runes.
	FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(Size, Size), Color * FLinearColor(1.f, 1.f, 1.f, 0.85f));
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);

	if (UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr)
	{
		FCanvasTextItem Text(FVector2D(X + Size * 0.28f, Y + Size * 0.22f), FText::FromString(Glyph), Font, FLinearColor::White);
		Text.Scale = FVector2D(Size / 22.f, Size / 22.f);
		Text.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(Text);
	}
}
