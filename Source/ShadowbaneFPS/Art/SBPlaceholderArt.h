// Copyright shadowbanefps.
//
// Runtime dummy art kit for the pilot: team colors, archetype icon codes,
// solid-color materials, and HUD chip helpers. Replace later with real icons /
// runes / meshes — see docs/PLACEHOLDER_ART.md.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/SBTypes.h"
#include "SBPlaceholderArt.generated.h"

class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UTexture2D;
class USBCharacterArchetype;
class UCanvas;

USTRUCT(BlueprintType)
struct FSBPlaceholderIcon
{
	GENERATED_BODY()

	/** Short 2–3 letter glyph shown on HUD chips / nameplates. */
	UPROPERTY(BlueprintReadOnly)
	FString Code;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly)
	FText RoleLabel;
};

UCLASS()
class SHADOWBANEFPS_API USBPlaceholderArt : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Siege|Art")
	static FLinearColor TeamColor(ESBTeam Team);

	UFUNCTION(BlueprintPure, Category = "Siege|Art")
	static FLinearColor StructureColor(ESBStructureState State);

	UFUNCTION(BlueprintPure, Category = "Siege|Art")
	static FLinearColor ObjectiveColor(float Progress01);

	/** Derives a readable dummy icon from an archetype's id / role profile. */
	UFUNCTION(BlueprintPure, Category = "Siege|Art")
	static FSBPlaceholderIcon MakeIcon(const USBCharacterArchetype* Archetype);

	/** Tints a mesh with a dynamic material instance (BasicShapes-friendly). */
	UFUNCTION(BlueprintCallable, Category = "Siege|Art")
	static UMaterialInstanceDynamic* ApplySolidColor(UPrimitiveComponent* Mesh, FLinearColor Color, int32 MaterialIndex = 0);

	/** Creates (or returns cached) a solid-color 64x64 texture for HUD icons. */
	UFUNCTION(BlueprintCallable, Category = "Siege|Art")
	static UTexture2D* GetSolidTexture(FLinearColor Color);

	/** Draws a colored chip with a letter code — dummy "icon". */
	static void DrawIconChip(UCanvas* Canvas, float X, float Y, float Size, const FSBPlaceholderIcon& Icon, bool bSelected = false);

	/** Draws a small "rune" diamond (placeholder ability glyph). */
	static void DrawRuneGlyph(UCanvas* Canvas, float X, float Y, float Size, FLinearColor Color, const FString& Glyph);

private:
	static FString InferCode(const USBCharacterArchetype* Archetype);
	static FLinearColor InferTint(const USBCharacterArchetype* Archetype);
};
