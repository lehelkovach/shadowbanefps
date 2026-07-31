// Copyright shadowbanefps.

#include "SBMatchShopCatalog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Core/SBLog.h"

USBMatchShopCatalog& USBMatchShopCatalog::Get()
{
	static USBMatchShopCatalog* Instance = nullptr;
	if (!Instance)
	{
		Instance = NewObject<USBMatchShopCatalog>();
		Instance->AddToRoot();
		Instance->EnsureLoaded();
	}
	return *Instance;
}

bool USBMatchShopCatalog::EnsureLoaded()
{
	if (bLoaded)
	{
		return true;
	}

	const bool bItems = LoadItemsJson();
	LoadAffixesJson();

	bLoaded = bItems;
	UE_LOG(LogShadowbane, Log,
		TEXT("Match shop catalog loaded: %d items, %d affixes"),
		Items.Num(), Affixes.Num());
	return bLoaded;
}

bool USBMatchShopCatalog::LoadItemsJson()
{
	Items.Reset();
	const FString Path = FPaths::ProjectConfigDir() / TEXT("Shadowbane") / TEXT("items-catalog.json");
	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *Path))
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Missing items catalog: %s"), *Path);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> Arr;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
	if (!FJsonSerializer::Deserialize(Reader, Arr) || Arr.Num() == 0)
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Bad items-catalog.json: %s"), *Path);
		return false;
	}

	Items.Reserve(Arr.Num());
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O) continue;

		FSBShopItemDef Item;
		if (O->HasTypedField<EJson::Number>(TEXT("id")))
		{
			Item.Id = static_cast<int32>(O->GetNumberField(TEXT("id")));
		}
		Item.Name = O->GetStringField(TEXT("name"));
		if (O->HasField(TEXT("gender")))
		{
			Item.Gender = O->GetStringField(TEXT("gender"));
		}
		if (O->HasField(TEXT("nameFormat")))
		{
			Item.NameFormat = O->GetStringField(TEXT("nameFormat"));
		}
		if (!Item.Name.IsEmpty())
		{
			Items.Add(MoveTemp(Item));
		}
	}
	return Items.Num() > 0;
}

bool USBMatchShopCatalog::LoadAffixesJson()
{
	Affixes.Reset();
	const FString Path = FPaths::ProjectConfigDir() / TEXT("Shadowbane") / TEXT("effect-affixes.json");
	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *Path))
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Missing effect-affixes.json: %s"), *Path);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> Arr;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
	if (!FJsonSerializer::Deserialize(Reader, Arr) || Arr.Num() == 0)
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Bad effect-affixes.json: %s"), *Path);
		return false;
	}

	Affixes.Reserve(Arr.Num());
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O) continue;
		FSBEffectAffixDef A;
		A.Id = O->GetStringField(TEXT("id"));
		A.Kind = O->GetStringField(TEXT("kind"));
		A.Name = O->GetStringField(TEXT("name"));
		if (!A.Name.IsEmpty())
		{
			Affixes.Add(MoveTemp(A));
		}
	}
	return Affixes.Num() > 0;
}

void USBMatchShopCatalog::GetPreviewItemNames(int32 MaxCount, TArray<FString>& OutNames) const
{
	OutNames.Reset();
	const int32 N = FMath::Clamp(MaxCount, 0, Items.Num());
	OutNames.Reserve(N);
	for (int32 i = 0; i < N; ++i)
	{
		OutNames.Add(Items[i].Name);
	}
}

const FSBShopItemDef* USBMatchShopCatalog::FindItemById(int32 Id) const
{
	return Items.FindByPredicate([&](const FSBShopItemDef& I) { return I.Id == Id; });
}

const FSBShopItemDef* USBMatchShopCatalog::FindItemByName(const FString& Name) const
{
	return Items.FindByPredicate(
		[&](const FSBShopItemDef& I) { return I.Name.Equals(Name, ESearchCase::IgnoreCase); });
}
