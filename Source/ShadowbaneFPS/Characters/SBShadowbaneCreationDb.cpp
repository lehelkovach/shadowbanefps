// Copyright shadowbanefps.

#include "SBShadowbaneCreationDb.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Core/SBLog.h"

USBShadowbaneCreationDb& USBShadowbaneCreationDb::Get()
{
	static USBShadowbaneCreationDb* Instance = nullptr;
	if (!Instance)
	{
		Instance = NewObject<USBShadowbaneCreationDb>();
		Instance->AddToRoot();
		Instance->EnsureLoaded();
	}
	return *Instance;
}

bool USBShadowbaneCreationDb::EnsureLoaded()
{
	if (bLoaded)
	{
		return true;
	}

	TArray<TSharedPtr<FJsonValue>> RaceArr, BaseArr, PrestigeArr, TraitArr, DiscArr;
	const bool bOk =
		LoadJsonArray(TEXT("races.json"), RaceArr) &&
		LoadJsonArray(TEXT("base-classes.json"), BaseArr) &&
		LoadJsonArray(TEXT("prestige-classes.json"), PrestigeArr);

	if (!bOk)
	{
		UE_LOG(LogShadowbane, Error, TEXT("Shadowbane creation DB failed to load from Config/Shadowbane"));
		return false;
	}

	ParseRaces(RaceArr);
	ParseBaseClasses(BaseArr);
	ParsePrestiges(PrestigeArr);
	if (LoadJsonArray(TEXT("starting-traits.json"), TraitArr))
	{
		ParseTraits(TraitArr);
	}

	LoadAbilityCsv();
	if (LoadJsonArray(TEXT("disciplines.json"), DiscArr))
	{
		ParseDisciplineEligibility(DiscArr);
	}

	LoadPowersJson();
	LoadSkillsJson();

	bLoaded = true;
	UE_LOG(LogShadowbane, Log,
		TEXT("Shadowbane creation DB loaded: %d races, %d base, %d prestige, %d traits, %d disciplines (%d ability rows), %d powers, %d skills"),
		Races.Num(), BaseClasses.Num(), Prestiges.Num(), StartingTraits.Num(), Disciplines.Num(), AbilityCsvRowCount,
		Powers.Num(), Skills.Num());
	return true;
}

bool USBShadowbaneCreationDb::LoadJsonArray(const FString& FileName, TArray<TSharedPtr<FJsonValue>>& OutArray)
{
	OutArray.Reset();
	const FString Path = FPaths::ProjectConfigDir() / TEXT("Shadowbane") / FileName;
	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *Path))
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Missing creation data: %s"), *Path);
		return false;
	}

	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
	if (!FJsonSerializer::Deserialize(Reader, OutArray) || OutArray.Num() == 0)
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Bad JSON array: %s"), *Path);
		return false;
	}
	return true;
}

static void ReadStringArray(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, TArray<FString>& Out)
{
	Out.Reset();
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (Obj->TryGetArrayField(Field, Arr) && Arr)
	{
		for (const TSharedPtr<FJsonValue>& V : *Arr)
		{
			Out.Add(V->AsString());
		}
	}
}

/** RFC-ish CSV field parse: handles quoted commas and "" escapes. */
static bool ParseCsvLine(const FString& Line, TArray<FString>& OutFields)
{
	OutFields.Reset();
	FString Cur;
	bool bInQuotes = false;
	for (int32 i = 0; i < Line.Len(); ++i)
	{
		const TCHAR C = Line[i];
		if (bInQuotes)
		{
			if (C == TEXT('"'))
			{
				if (i + 1 < Line.Len() && Line[i + 1] == TEXT('"'))
				{
					Cur.AppendChar(TEXT('"'));
					++i;
				}
				else
				{
					bInQuotes = false;
				}
			}
			else
			{
				Cur.AppendChar(C);
			}
		}
		else if (C == TEXT('"'))
		{
			bInQuotes = true;
		}
		else if (C == TEXT(','))
		{
			OutFields.Add(Cur.TrimStartAndEnd());
			Cur.Reset();
		}
		else
		{
			Cur.AppendChar(C);
		}
	}
	OutFields.Add(Cur.TrimStartAndEnd());
	return OutFields.Num() > 0;
}

bool USBShadowbaneCreationDb::LoadAbilityCsv()
{
	AbilityCsvRowCount = 0;
	Disciplines.Reset();

	const FString Path = FPaths::ProjectConfigDir() / TEXT("Shadowbane") / TEXT("shadowbane_ability_import.csv");
	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *Path))
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Missing ability CSV: %s"), *Path);
		return false;
	}

	TArray<FString> Lines;
	Raw.ParseIntoArrayLines(Lines, true);
	if (Lines.Num() < 2)
	{
		UE_LOG(LogShadowbane, Warning, TEXT("Ability CSV empty: %s"), *Path);
		return false;
	}

	TMap<FString, int32> DiscIndex;
	for (int32 LineIdx = 1; LineIdx < Lines.Num(); ++LineIdx)
	{
		const FString& Line = Lines[LineIdx];
		if (Line.TrimStartAndEnd().IsEmpty())
		{
			continue;
		}

		TArray<FString> Fields;
		if (!ParseCsvLine(Line, Fields) || Fields.Num() < 5)
		{
			continue;
		}

		const FString& OriginalDisc = Fields[0];
		const FString& NewDisc = Fields[1];
		const FString& OriginalAbility = Fields[2];
		const FString& NewAbility = Fields[3];
		const FString& Mechanic = Fields[4];
		const FString RenameStatus = Fields.Num() > 5 ? Fields[5] : FString();

		if (NewDisc.IsEmpty() || NewAbility.IsEmpty())
		{
			continue;
		}

		int32* FoundIdx = DiscIndex.Find(NewDisc);
		int32 Idx;
		if (!FoundIdx)
		{
			FSBCreationDisciplineDef D;
			D.Name = NewDisc;
			D.OriginalName = OriginalDisc;
			Idx = Disciplines.Add(MoveTemp(D));
			DiscIndex.Add(NewDisc, Idx);
		}
		else
		{
			Idx = *FoundIdx;
		}

		FSBCreationAbilityDef A;
		A.NewAbility = NewAbility;
		A.OriginalAbility = OriginalAbility;
		A.MechanicSummary = Mechanic;
		A.RenameStatus = RenameStatus;
		Disciplines[Idx].Abilities.Add(MoveTemp(A));
		++AbilityCsvRowCount;
	}

	return AbilityCsvRowCount > 0;
}

bool USBShadowbaneCreationDb::LoadPowersJson()
{
	Powers.Reset();
	TArray<TSharedPtr<FJsonValue>> Arr;
	if (!LoadJsonArray(TEXT("powers.json"), Arr))
	{
		return false;
	}

	Powers.Reserve(Arr.Num());
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O) continue;
		FSBClientPowerDef P;
		P.Id = O->GetStringField(TEXT("id"));
		P.Name = O->GetStringField(TEXT("name"));
		if (O->HasField(TEXT("description")))
		{
			P.Description = O->GetStringField(TEXT("description"));
		}
		if (!P.Id.IsEmpty() || !P.Name.IsEmpty())
		{
			Powers.Add(MoveTemp(P));
		}
	}
	return Powers.Num() > 0;
}

bool USBShadowbaneCreationDb::LoadSkillsJson()
{
	Skills.Reset();
	TArray<TSharedPtr<FJsonValue>> Arr;
	if (!LoadJsonArray(TEXT("skills.json"), Arr))
	{
		return false;
	}

	Skills.Reserve(Arr.Num());
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O) continue;
		FSBClientSkillDef S;
		S.Id = O->GetStringField(TEXT("id"));
		S.Name = O->GetStringField(TEXT("name"));
		if (O->HasField(TEXT("description")))
		{
			S.Description = O->GetStringField(TEXT("description"));
		}
		if (!S.Id.IsEmpty() || !S.Name.IsEmpty())
		{
			Skills.Add(MoveTemp(S));
		}
	}
	return Skills.Num() > 0;
}

void USBShadowbaneCreationDb::ParseDisciplineEligibility(const TArray<TSharedPtr<FJsonValue>>& Arr)
{
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O) continue;

		const FString Name = O->GetStringField(TEXT("name"));
		FSBCreationDisciplineDef* Disc = Disciplines.FindByPredicate(
			[&](const FSBCreationDisciplineDef& D) { return D.Name.Equals(Name, ESearchCase::IgnoreCase); });
		if (!Disc)
		{
			// JSON-only discipline with no CSV abilities — still track for eligibility lists.
			FSBCreationDisciplineDef D;
			D.Name = Name;
			D.OriginalName = Name;
			const int32 NewIdx = Disciplines.Add(MoveTemp(D));
			Disc = &Disciplines[NewIdx];
		}

		ReadStringArray(O, TEXT("availableBaseClasses"), Disc->AvailableBaseClasses);
		ReadStringArray(O, TEXT("availableRaces"), Disc->AvailableRaces);
		ReadStringArray(O, TEXT("availablePrestigeClasses"), Disc->AvailablePrestigeClasses);
		Disc->bHasJsonEligibility = true;
	}
}

void USBShadowbaneCreationDb::ParseRaces(const TArray<TSharedPtr<FJsonValue>>& Arr)
{
	Races.Reset();
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O) continue;

		FSBCreationRaceDef R;
		R.Name = O->GetStringField(TEXT("name"));
		R.Cost = O->GetIntegerField(TEXT("cost"));
		R.HealthBonus = O->GetIntegerField(TEXT("healthBonus"));
		R.ManaBonus = O->GetIntegerField(TEXT("manaBonus"));
		R.StaminaBonus = O->GetIntegerField(TEXT("staminaBonus"));
		R.GrantedBaseStr = O->GetIntegerField(TEXT("grantedBaseStr"));
		R.GrantedMaxStr = O->GetIntegerField(TEXT("grantedMaxStr"));
		R.GrantedBaseDex = O->GetIntegerField(TEXT("grantedBaseDex"));
		R.GrantedMaxDex = O->GetIntegerField(TEXT("grantedMaxDex"));
		R.GrantedBaseCon = O->GetIntegerField(TEXT("grantedBaseCon"));
		R.GrantedMaxCon = O->GetIntegerField(TEXT("grantedMaxCon"));
		R.GrantedBaseInt = O->GetIntegerField(TEXT("grantedBaseInt"));
		R.GrantedMaxInt = O->GetIntegerField(TEXT("grantedMaxInt"));
		R.GrantedBaseSpi = O->GetIntegerField(TEXT("grantedBaseSpi"));
		R.GrantedMaxSpi = O->GetIntegerField(TEXT("grantedMaxSpi"));
		ReadStringArray(O, TEXT("availableBaseClasses"), R.AvailableBaseClasses);
		ReadStringArray(O, TEXT("availablePrestigeClasses"), R.AvailablePrestigeClasses);

		const TArray<TSharedPtr<FJsonValue>>* Granted = nullptr;
		if (O->TryGetArrayField(TEXT("granted"), Granted) && Granted)
		{
			for (const TSharedPtr<FJsonValue>& G : *Granted)
			{
				const TSharedPtr<FJsonObject> GO = G->AsObject();
				if (!GO) continue;
				const TArray<TSharedPtr<FJsonValue>>* Abilities = nullptr;
				if (GO->TryGetArrayField(TEXT("abilities"), Abilities) && Abilities)
				{
					for (const TSharedPtr<FJsonValue>& A : *Abilities)
					{
						if (const TSharedPtr<FJsonObject> AO = A->AsObject())
						{
							R.InnateAbilityNames.Add(AO->GetStringField(TEXT("name")));
						}
					}
				}
			}
		}
		Races.Add(MoveTemp(R));
	}
}

void USBShadowbaneCreationDb::ParseBaseClasses(const TArray<TSharedPtr<FJsonValue>>& Arr)
{
	BaseClasses.Reset();
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O) continue;
		FSBCreationBaseClassDef B;
		B.Name = O->GetStringField(TEXT("name"));
		B.GrantedBaseStr = O->GetIntegerField(TEXT("grantedBaseStr"));
		B.GrantedBaseDex = O->GetIntegerField(TEXT("grantedBaseDex"));
		B.GrantedBaseCon = O->GetIntegerField(TEXT("grantedBaseCon"));
		B.GrantedBaseInt = O->GetIntegerField(TEXT("grantedBaseInt"));
		B.GrantedBaseSpi = O->GetIntegerField(TEXT("grantedBaseSpi"));
		BaseClasses.Add(MoveTemp(B));
	}
}

void USBShadowbaneCreationDb::ParsePrestiges(const TArray<TSharedPtr<FJsonValue>>& Arr)
{
	Prestiges.Reset();
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O) continue;
		FSBCreationPrestigeDef P;
		P.Name = O->GetStringField(TEXT("name"));
		ReadStringArray(O, TEXT("availableBaseClasses"), P.AvailableBaseClasses);
		ReadStringArray(O, TEXT("availableRaces"), P.AvailableRaces);
		ReadStringArray(O, TEXT("availableDisciplines"), P.AvailableDisciplines);
		Prestiges.Add(MoveTemp(P));
	}
}

void USBShadowbaneCreationDb::ParseTraits(const TArray<TSharedPtr<FJsonValue>>& Arr)
{
	StartingTraits.Reset();
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O) continue;
		FSBCreationTraitDef T;
		T.Name = O->GetStringField(TEXT("name"));
		if (O->HasField(TEXT("cost")))
		{
			T.Cost = O->GetIntegerField(TEXT("cost"));
		}
		else if (O->HasField(TEXT("creationCost")))
		{
			T.Cost = O->GetIntegerField(TEXT("creationCost"));
		}
		if (O->HasField(TEXT("category")))
		{
			T.Category = O->GetStringField(TEXT("category"));
		}
		else if (O->HasField(TEXT("runeCategory")))
		{
			T.Category = O->GetStringField(TEXT("runeCategory"));
		}
		if (O->HasField(TEXT("description")))
		{
			T.Description = O->GetStringField(TEXT("description"));
		}
		StartingTraits.Add(MoveTemp(T));
	}
}

bool USBShadowbaneCreationDb::NamesMatchLoose(const FString& A, const FString& B)
{
	if (A.Equals(B, ESearchCase::IgnoreCase))
	{
		return true;
	}
	const FString NA = A.Replace(TEXT("-"), TEXT(" "));
	const FString NB = B.Replace(TEXT("-"), TEXT(" "));
	return NA.Equals(NB, ESearchCase::IgnoreCase);
}

bool USBShadowbaneCreationDb::ListContainsLoose(const TArray<FString>& List, const FString& Name)
{
	for (const FString& Entry : List)
	{
		if (NamesMatchLoose(Entry, Name))
		{
			return true;
		}
	}
	return false;
}

bool USBShadowbaneCreationDb::ListContainsBaseLoose(const TArray<FString>& List, const FString& BaseClass)
{
	for (const FString& Entry : List)
	{
		// disciplines.json sometimes stores "Fighter, Healer" as one string.
		TArray<FString> Parts;
		Entry.ParseIntoArray(Parts, TEXT(","), true);
		for (FString& Part : Parts)
		{
			Part = Part.TrimStartAndEnd();
			if (NamesMatchLoose(Part, BaseClass))
			{
				return true;
			}
		}
		if (Parts.Num() == 0 && NamesMatchLoose(Entry, BaseClass))
		{
			return true;
		}
	}
	return false;
}

const FSBCreationRaceDef* USBShadowbaneCreationDb::FindRace(const FString& Name) const
{
	return Races.FindByPredicate([&](const FSBCreationRaceDef& R) { return R.Name.Equals(Name, ESearchCase::IgnoreCase); });
}

const FSBCreationBaseClassDef* USBShadowbaneCreationDb::FindBaseClass(const FString& Name) const
{
	return BaseClasses.FindByPredicate([&](const FSBCreationBaseClassDef& B) { return B.Name.Equals(Name, ESearchCase::IgnoreCase); });
}

const FSBCreationPrestigeDef* USBShadowbaneCreationDb::FindPrestige(const FString& Name) const
{
	return Prestiges.FindByPredicate([&](const FSBCreationPrestigeDef& P) { return P.Name.Equals(Name, ESearchCase::IgnoreCase); });
}

const FSBCreationDisciplineDef* USBShadowbaneCreationDb::FindDiscipline(const FString& Name) const
{
	return Disciplines.FindByPredicate([&](const FSBCreationDisciplineDef& D) { return D.Name.Equals(Name, ESearchCase::IgnoreCase); });
}

const FSBClientPowerDef* USBShadowbaneCreationDb::FindPowerByName(const FString& Name) const
{
	if (Name.IsEmpty())
	{
		return nullptr;
	}
	return Powers.FindByPredicate([&](const FSBClientPowerDef& P)
	{
		return NamesMatchLoose(P.Id, Name) || NamesMatchLoose(P.Name, Name);
	});
}

const FSBClientSkillDef* USBShadowbaneCreationDb::FindSkillByName(const FString& Name) const
{
	if (Name.IsEmpty())
	{
		return nullptr;
	}
	return Skills.FindByPredicate([&](const FSBClientSkillDef& S)
	{
		return NamesMatchLoose(S.Id, Name) || NamesMatchLoose(S.Name, Name);
	});
}

void USBShadowbaneCreationDb::GetEligibleBaseClasses(const FString& Race, TArray<FString>& OutNames) const
{
	OutNames.Reset();
	if (const FSBCreationRaceDef* R = FindRace(Race))
	{
		OutNames = R->AvailableBaseClasses;
	}
}

void USBShadowbaneCreationDb::GetEligiblePrestiges(const FString& Race, const FString& BaseClass, TArray<FString>& OutNames) const
{
	OutNames.Reset();
	const FSBCreationRaceDef* RaceDef = FindRace(Race);
	if (!RaceDef)
	{
		return;
	}

	for (const FSBCreationPrestigeDef& P : Prestiges)
	{
		bool bRaceMatch = false;
		for (const FString& RN : P.AvailableRaces)
		{
			if (NamesMatchLoose(RN, Race))
			{
				bRaceMatch = true;
				break;
			}
		}
		if (!bRaceMatch)
		{
			continue;
		}
		if (!RaceDef->AvailablePrestigeClasses.ContainsByPredicate(
			[&](const FString& N) { return N.Equals(P.Name, ESearchCase::IgnoreCase); }))
		{
			continue;
		}
		if (!P.AvailableBaseClasses.ContainsByPredicate(
			[&](const FString& N) { return N.Equals(BaseClass, ESearchCase::IgnoreCase); }))
		{
			continue;
		}
		OutNames.Add(P.Name);
	}
}

void USBShadowbaneCreationDb::GetEligibleDisciplines(
	const FString& Race, const FString& BaseClass, const FString& Prestige, TArray<FString>& OutNames) const
{
	OutNames.Reset();
	const FSBCreationPrestigeDef* PrestigeDef = FindPrestige(Prestige);
	if (!PrestigeDef)
	{
		return;
	}

	for (const FString& DiscName : PrestigeDef->AvailableDisciplines)
	{
		const FSBCreationDisciplineDef* Disc = FindDiscipline(DiscName);
		if (!Disc)
		{
			continue;
		}
		// Prefer disciplines that have CSV abilities for the builder HUD.
		if (Disc->Abilities.Num() == 0)
		{
			continue;
		}

		if (Disc->bHasJsonEligibility)
		{
			if (!ListContainsLoose(Disc->AvailableRaces, Race))
			{
				continue;
			}
			if (!ListContainsBaseLoose(Disc->AvailableBaseClasses, BaseClass))
			{
				continue;
			}
			if (!ListContainsLoose(Disc->AvailablePrestigeClasses, Prestige))
			{
				continue;
			}
		}

		OutNames.Add(Disc->Name);
	}
}
