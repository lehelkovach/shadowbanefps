// Copyright shadowbanefps.
//
// Shadowbane character creation DB — loaded from Config/Shadowbane/*.json,
// shadowbane_ability_import.csv (shadowbanefps renames), and shadowbanefps client string
// extracts (powers.json / skills.json). See Config/Shadowbane/SOURCE.md.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SBShadowbaneCreationDb.generated.h"

USTRUCT(BlueprintType)
struct FSBCreationRaceDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) int32 Cost = 0;
	UPROPERTY(BlueprintReadOnly) int32 HealthBonus = 0;
	UPROPERTY(BlueprintReadOnly) int32 ManaBonus = 0;
	UPROPERTY(BlueprintReadOnly) int32 StaminaBonus = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseStr = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedMaxStr = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseDex = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedMaxDex = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseCon = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedMaxCon = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseInt = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedMaxInt = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseSpi = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedMaxSpi = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FString> AvailableBaseClasses;
	UPROPERTY(BlueprintReadOnly) TArray<FString> AvailablePrestigeClasses;
	UPROPERTY(BlueprintReadOnly) TArray<FString> InnateAbilityNames;
};

USTRUCT(BlueprintType)
struct FSBCreationBaseClassDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseStr = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseDex = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseCon = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseInt = 0;
	UPROPERTY(BlueprintReadOnly) int32 GrantedBaseSpi = 0;
};

USTRUCT(BlueprintType)
struct FSBCreationPrestigeDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) TArray<FString> AvailableBaseClasses;
	UPROPERTY(BlueprintReadOnly) TArray<FString> AvailableRaces;
	UPROPERTY(BlueprintReadOnly) TArray<FString> AvailableDisciplines;
};

USTRUCT(BlueprintType)
struct FSBCreationTraitDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) int32 Cost = 0;
	UPROPERTY(BlueprintReadOnly) FString Category;
	UPROPERTY(BlueprintReadOnly) FString Description;
};

/** One ability row from shadowbane_ability_import.csv (shadowbanefps renames). */
USTRUCT(BlueprintType)
struct FSBCreationAbilityDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString NewAbility;
	UPROPERTY(BlueprintReadOnly) FString OriginalAbility;
	UPROPERTY(BlueprintReadOnly) FString MechanicSummary;
	UPROPERTY(BlueprintReadOnly) FString RenameStatus;
};

/** Discipline + its CSV abilities (keyed by new_discipline). */
USTRUCT(BlueprintType)
struct FSBCreationDisciplineDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) FString OriginalName;
	UPROPERTY(BlueprintReadOnly) TArray<FSBCreationAbilityDef> Abilities;
	/** From disciplines.json when present — used for race/base/prestige filter. */
	UPROPERTY(BlueprintReadOnly) TArray<FString> AvailableBaseClasses;
	UPROPERTY(BlueprintReadOnly) TArray<FString> AvailableRaces;
	UPROPERTY(BlueprintReadOnly) TArray<FString> AvailablePrestigeClasses;
	UPROPERTY(BlueprintReadOnly) bool bHasJsonEligibility = false;
};

/** Power display string from shadowbanefps DataStringENGLISH (powers.json). */
USTRUCT(BlueprintType)
struct FSBClientPowerDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Id;
	UPROPERTY(BlueprintReadOnly) FString Name;
	/** From PowerDescription when key matches power id/name (often empty). */
	UPROPERTY(BlueprintReadOnly) FString Description;
};

/** Skill display string from shadowbanefps DataStringENGLISH (skills.json). */
USTRUCT(BlueprintType)
struct FSBClientSkillDef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Id;
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) FString Description;
};

/** Singletons load once; safe to call from GameMode / HUD. */
UCLASS()
class SHADOWBANEFPS_API USBShadowbaneCreationDb : public UObject
{
	GENERATED_BODY()

public:
	static USBShadowbaneCreationDb& Get();

	bool IsLoaded() const { return bLoaded; }
	bool EnsureLoaded();

	const TArray<FSBCreationRaceDef>& GetRaces() const { return Races; }
	const TArray<FSBCreationBaseClassDef>& GetBaseClasses() const { return BaseClasses; }
	const TArray<FSBCreationPrestigeDef>& GetPrestiges() const { return Prestiges; }
	const TArray<FSBCreationTraitDef>& GetStartingTraits() const { return StartingTraits; }
	const TArray<FSBCreationDisciplineDef>& GetDisciplines() const { return Disciplines; }
	const TArray<FSBClientPowerDef>& GetPowers() const { return Powers; }
	const TArray<FSBClientSkillDef>& GetSkills() const { return Skills; }
	int32 GetAbilityCsvRowCount() const { return AbilityCsvRowCount; }
	int32 GetPowerCount() const { return Powers.Num(); }
	int32 GetSkillCount() const { return Skills.Num(); }

	const FSBCreationRaceDef* FindRace(const FString& Name) const;
	const FSBCreationBaseClassDef* FindBaseClass(const FString& Name) const;
	const FSBCreationPrestigeDef* FindPrestige(const FString& Name) const;
	const FSBCreationDisciplineDef* FindDiscipline(const FString& Name) const;
	/** Match by power id or display name (ignore case / hyphen-space). */
	const FSBClientPowerDef* FindPowerByName(const FString& Name) const;
	const FSBClientSkillDef* FindSkillByName(const FString& Name) const;

	/** Prestiges legal for race + base path (shadowbanefps / shadowbane-db intersection). */
	void GetEligiblePrestiges(const FString& Race, const FString& BaseClass, TArray<FString>& OutNames) const;

	/** Base classes legal for race. */
	void GetEligibleBaseClasses(const FString& Race, TArray<FString>& OutNames) const;

	/**
	 * Disciplines legal for race + base + prestige:
	 * prestige AvailableDisciplines ∩ CSV disciplines, further filtered by
	 * disciplines.json race/base/prestige lists when present.
	 */
	void GetEligibleDisciplines(const FString& Race, const FString& BaseClass, const FString& Prestige, TArray<FString>& OutNames) const;

private:
	bool bLoaded = false;
	int32 AbilityCsvRowCount = 0;
	TArray<FSBCreationRaceDef> Races;
	TArray<FSBCreationBaseClassDef> BaseClasses;
	TArray<FSBCreationPrestigeDef> Prestiges;
	TArray<FSBCreationTraitDef> StartingTraits;
	TArray<FSBCreationDisciplineDef> Disciplines;
	TArray<FSBClientPowerDef> Powers;
	TArray<FSBClientSkillDef> Skills;

	bool LoadJsonArray(const FString& FileName, TArray<TSharedPtr<FJsonValue>>& OutArray);
	bool LoadAbilityCsv();
	bool LoadPowersJson();
	bool LoadSkillsJson();
	void ParseRaces(const TArray<TSharedPtr<FJsonValue>>& Arr);
	void ParseBaseClasses(const TArray<TSharedPtr<FJsonValue>>& Arr);
	void ParsePrestiges(const TArray<TSharedPtr<FJsonValue>>& Arr);
	void ParseTraits(const TArray<TSharedPtr<FJsonValue>>& Arr);
	void ParseDisciplineEligibility(const TArray<TSharedPtr<FJsonValue>>& Arr);

	static bool NamesMatchLoose(const FString& A, const FString& B);
	static bool ListContainsLoose(const TArray<FString>& List, const FString& Name);
	static bool ListContainsBaseLoose(const TArray<FString>& List, const FString& BaseClass);
};
