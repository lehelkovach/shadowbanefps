// Copyright shadowbanefps.
//
// Match shop catalog stub — item display names from shadowbanefps ItemENGLISH
// (items-catalog.json) plus effect prefix/suffix affixes. See Config/Shadowbane/SOURCE.md.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SBMatchShopCatalog.generated.h"

USTRUCT(BlueprintType)
struct FSBShopItemDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Id = 0;
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) FString Gender;
	UPROPERTY(BlueprintReadOnly) FString NameFormat;
};

USTRUCT(BlueprintType)
struct FSBEffectAffixDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Id;
	/** "prefix" or "suffix". */
	UPROPERTY(BlueprintReadOnly) FString Kind;
	UPROPERTY(BlueprintReadOnly) FString Name;
};

/** Singleton catalog for future match-shop UI; safe to call from GameState / HUD. */
UCLASS()
class SHADOWBANEFPS_API USBMatchShopCatalog : public UObject
{
	GENERATED_BODY()

public:
	static USBMatchShopCatalog& Get();

	bool IsLoaded() const { return bLoaded; }
	bool EnsureLoaded();

	const TArray<FSBShopItemDef>& GetItems() const { return Items; }
	const TArray<FSBEffectAffixDef>& GetAffixes() const { return Affixes; }
	int32 GetItemCount() const { return Items.Num(); }
	int32 GetAffixCount() const { return Affixes.Num(); }

	/** First N catalog names for HUD / debug preview (full list stays in memory). */
	void GetPreviewItemNames(int32 MaxCount, TArray<FString>& OutNames) const;

	const FSBShopItemDef* FindItemById(int32 Id) const;
	const FSBShopItemDef* FindItemByName(const FString& Name) const;

private:
	bool bLoaded = false;
	TArray<FSBShopItemDef> Items;
	TArray<FSBEffectAffixDef> Affixes;

	bool LoadItemsJson();
	bool LoadAffixesJson();
};
