// Copyright shadowbanefps.

#include "SBCharacterCalculator.h"
#include "SBCharacterArchetype.h"
#include "SBShadowbaneCreationDb.h"

namespace SBCalc
{
	static int32 ClampStat(int32 V, int32 MinV, int32 MaxV)
	{
		return FMath::Clamp(V, MinV, MaxV);
	}

	static void SpendPoints(FSBCreationStats& S, int32& Points, int32 StrW, int32 DexW, int32 ConW, int32 IntW, int32 SpiW)
	{
		const int32 Weights[5] = { StrW, DexW, ConW, IntW, SpiW };
		int32* Stats[5] = { &S.Strength, &S.Dexterity, &S.Constitution, &S.Intelligence, &S.Spirit };
		while (Points > 0)
		{
			int32 Best = -1;
			int32 BestW = -1;
			for (int32 i = 0; i < 5; ++i)
			{
				if (Weights[i] > BestW && *Stats[i] < 100)
				{
					BestW = Weights[i];
					Best = i;
				}
			}
			if (Best < 0)
			{
				break;
			}
			(*Stats[Best])++;
			--Points;
		}
	}

	static bool IsMeleePrestige(const FString& Prestige)
	{
		static const TCHAR* Melee[] = {
			TEXT("Warrior"), TEXT("Barbarian"), TEXT("Templar"), TEXT("Judicator"),
			TEXT("Guardian"), TEXT("Assassin"), TEXT("Nightstalker"), TEXT("Huntress")
		};
		for (const TCHAR* M : Melee)
		{
			if (Prestige.Equals(M, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}
}

void USBCharacterCalculator::Recalculate(FSBCharacterBuildState& B)
{
	USBShadowbaneCreationDb& Db = USBShadowbaneCreationDb::Get();
	Db.EnsureLoaded();

	const FSBCreationRaceDef* Race = Db.FindRace(B.Race);
	const FSBCreationBaseClassDef* Base = Db.FindBaseClass(B.BaseClass);
	const FSBCreationPrestigeDef* Prestige = Db.FindPrestige(B.Prestige);

	B.bValid = false;
	B.InnateAbilities.Reset();
	B.DisciplineAbilities.Reset();
	B.StatusLine.Empty();

	if (!Race || !Base)
	{
		B.StatusLine = TEXT("Invalid race or base class");
		return;
	}

	TArray<FString> EligibleBases;
	Db.GetEligibleBaseClasses(B.Race, EligibleBases);
	if (!EligibleBases.ContainsByPredicate([&](const FString& N) { return N.Equals(B.BaseClass, ESearchCase::IgnoreCase); }))
	{
		if (EligibleBases.Num() > 0)
		{
			B.BaseClass = EligibleBases[0];
			Base = Db.FindBaseClass(B.BaseClass);
		}
	}

	TArray<FString> EligiblePrestige;
	Db.GetEligiblePrestiges(B.Race, B.BaseClass, EligiblePrestige);
	if (EligiblePrestige.Num() == 0)
	{
		B.StatusLine = TEXT("No prestige available for this race/path");
		B.Discipline.Empty();
		return;
	}
	if (!EligiblePrestige.ContainsByPredicate([&](const FString& N) { return N.Equals(B.Prestige, ESearchCase::IgnoreCase); }))
	{
		B.Prestige = EligiblePrestige[0];
		Prestige = Db.FindPrestige(B.Prestige);
	}

	TArray<FString> EligibleDisc;
	Db.GetEligibleDisciplines(B.Race, B.BaseClass, B.Prestige, EligibleDisc);
	if (EligibleDisc.Num() == 0)
	{
		B.Discipline.Empty();
	}
	else if (!EligibleDisc.ContainsByPredicate([&](const FString& N) { return N.Equals(B.Discipline, ESearchCase::IgnoreCase); }))
	{
		B.Discipline = EligibleDisc[0];
	}

	// Morloch / shadowbanefps: 55 points, race cost, start at 40 after remove-five-from-all (35+5).
	B.RaceCost = Race->Cost;
	int32 Points = 55 - Race->Cost;
	B.Stats.Strength = 40 + Race->GrantedBaseStr + Base->GrantedBaseStr;
	B.Stats.Dexterity = 40 + Race->GrantedBaseDex + Base->GrantedBaseDex;
	B.Stats.Constitution = 40 + Race->GrantedBaseCon + Base->GrantedBaseCon;
	B.Stats.Intelligence = 40 + Race->GrantedBaseInt + Base->GrantedBaseInt;
	B.Stats.Spirit = 40 + Race->GrantedBaseSpi + Base->GrantedBaseSpi;

	// Role bias by prestige / base path
	int32 StrW = 1, DexW = 1, ConW = 1, IntW = 1, SpiW = 1;
	if (B.BaseClass.Equals(TEXT("Fighter"), ESearchCase::IgnoreCase))
	{
		StrW = 3; ConW = 3; DexW = 2;
	}
	else if (B.BaseClass.Equals(TEXT("Rogue"), ESearchCase::IgnoreCase))
	{
		DexW = 4; StrW = 2; IntW = 2;
	}
	else if (B.BaseClass.Equals(TEXT("Mage"), ESearchCase::IgnoreCase))
	{
		IntW = 4; SpiW = 3;
	}
	else if (B.BaseClass.Equals(TEXT("Healer"), ESearchCase::IgnoreCase))
	{
		SpiW = 4; ConW = 2; IntW = 2;
	}

	if (B.Prestige.Equals(TEXT("Warden"), ESearchCase::IgnoreCase) || B.Prestige.Equals(TEXT("Huntress"), ESearchCase::IgnoreCase)
		|| B.Prestige.Equals(TEXT("Scout"), ESearchCase::IgnoreCase))
	{
		DexW = 5; StrW = 1;
	}
	if (B.Prestige.Equals(TEXT("Wizard"), ESearchCase::IgnoreCase) || B.Prestige.Equals(TEXT("Spellweaver"), ESearchCase::IgnoreCase)
		|| B.Prestige.Equals(TEXT("Warlock"), ESearchCase::IgnoreCase) || B.Prestige.Equals(TEXT("Ravager"), ESearchCase::IgnoreCase))
	{
		IntW = 5; SpiW = 3;
	}
	if (B.Prestige.Equals(TEXT("Smite Cleric"), ESearchCase::IgnoreCase) || B.Prestige.Equals(TEXT("Priest"), ESearchCase::IgnoreCase))
	{
		SpiW = 5; ConW = 2;
	}

	SBCalc::SpendPoints(B.Stats, Points, StrW, DexW, ConW, IntW, SpiW);
	B.PointsRemaining = Points;

	B.Stats.Strength = SBCalc::ClampStat(B.Stats.Strength, 10, 100 + FMath::Max(0, Race->GrantedMaxStr));
	B.Stats.Dexterity = SBCalc::ClampStat(B.Stats.Dexterity, 10, 100 + FMath::Max(0, Race->GrantedMaxDex));
	B.Stats.Constitution = SBCalc::ClampStat(B.Stats.Constitution, 10, 100 + FMath::Max(0, Race->GrantedMaxCon));
	B.Stats.Intelligence = SBCalc::ClampStat(B.Stats.Intelligence, 10, 100 + FMath::Max(0, Race->GrantedMaxInt));
	B.Stats.Spirit = SBCalc::ClampStat(B.Stats.Spirit, 10, 100 + FMath::Max(0, Race->GrantedMaxSpi));

	// FPS compression of Con/Spi/Int/Dex/Str (+ race pool bonuses from shadowbane-db).
	B.Vitals.MaxHealth = 50.f + B.Stats.Constitution * 1.4f + Race->HealthBonus;
	B.Vitals.MaxMana = 30.f + B.Stats.Spirit * 1.1f + B.Stats.Intelligence * 0.45f + Race->ManaBonus;
	B.Vitals.MaxStamina = 40.f + B.Stats.Constitution * 0.7f + B.Stats.Dexterity * 0.25f + Race->StaminaBonus;
	B.Vitals.ManaRegen = 5.f + B.Stats.Spirit * 0.08f;
	B.Vitals.StaminaRegen = 10.f + B.Stats.Constitution * 0.06f;
	B.Vitals.MoveSpeed = 420.f + B.Stats.Dexterity * 3.2f;
	B.Vitals.bPreferMelee = SBCalc::IsMeleePrestige(B.Prestige);
	B.Vitals.AttackDamage = B.Vitals.bPreferMelee
		? (8.f + B.Stats.Strength * 0.28f)
		: (8.f + B.Stats.Dexterity * 0.22f + B.Stats.Intelligence * 0.12f);

	B.InnateAbilities = Race->InnateAbilityNames;

	if (const FSBCreationDisciplineDef* Disc = Db.FindDiscipline(B.Discipline))
	{
		for (const FSBCreationAbilityDef& A : Disc->Abilities)
		{
			FSBCreationAbilityLine Line;
			Line.Name = A.NewAbility;
			Line.Summary = A.MechanicSummary;

			// Prefer shadowbanefps PowerDescription when the CSV ability maps to a client power.
			const FSBClientPowerDef* Power = Db.FindPowerByName(A.OriginalAbility);
			if (!Power)
			{
				Power = Db.FindPowerByName(A.NewAbility);
			}
			if (Power && !Power->Description.IsEmpty())
			{
				Line.Summary = Power->Description;
			}

			B.DisciplineAbilities.Add(MoveTemp(Line));
		}
	}

	B.bValid = Prestige != nullptr;
	B.StatusLine = FString::Printf(
		TEXT("%s %s / %s | disc %s | pts left %d | STR %d DEX %d CON %d INT %d SPI %d"),
		*B.Race, *B.BaseClass, *B.Prestige,
		B.Discipline.IsEmpty() ? TEXT("(none)") : *B.Discipline,
		B.PointsRemaining,
		B.Stats.Strength, B.Stats.Dexterity, B.Stats.Constitution, B.Stats.Intelligence, B.Stats.Spirit);
}

void USBCharacterCalculator::CycleRace(FSBCharacterBuildState& B, int32 Delta)
{
	USBShadowbaneCreationDb& Db = USBShadowbaneCreationDb::Get();
	Db.EnsureLoaded();
	const TArray<FSBCreationRaceDef>& Races = Db.GetRaces();
	if (Races.Num() == 0)
	{
		return;
	}
	int32 Idx = 0;
	for (int32 i = 0; i < Races.Num(); ++i)
	{
		if (Races[i].Name.Equals(B.Race, ESearchCase::IgnoreCase))
		{
			Idx = i;
			break;
		}
	}
	Idx = (Idx + Delta + Races.Num() * 8) % Races.Num();
	B.Race = Races[Idx].Name;
	Recalculate(B);
}

void USBCharacterCalculator::CycleBaseClass(FSBCharacterBuildState& B, int32 Delta)
{
	USBShadowbaneCreationDb& Db = USBShadowbaneCreationDb::Get();
	TArray<FString> Eligible;
	Db.GetEligibleBaseClasses(B.Race, Eligible);
	if (Eligible.Num() == 0)
	{
		return;
	}
	int32 Idx = 0;
	for (int32 i = 0; i < Eligible.Num(); ++i)
	{
		if (Eligible[i].Equals(B.BaseClass, ESearchCase::IgnoreCase))
		{
			Idx = i;
			break;
		}
	}
	Idx = (Idx + Delta + Eligible.Num() * 8) % Eligible.Num();
	B.BaseClass = Eligible[Idx];
	Recalculate(B);
}

void USBCharacterCalculator::CyclePrestige(FSBCharacterBuildState& B, int32 Delta)
{
	USBShadowbaneCreationDb& Db = USBShadowbaneCreationDb::Get();
	TArray<FString> Eligible;
	Db.GetEligiblePrestiges(B.Race, B.BaseClass, Eligible);
	if (Eligible.Num() == 0)
	{
		return;
	}
	int32 Idx = 0;
	for (int32 i = 0; i < Eligible.Num(); ++i)
	{
		if (Eligible[i].Equals(B.Prestige, ESearchCase::IgnoreCase))
		{
			Idx = i;
			break;
		}
	}
	Idx = (Idx + Delta + Eligible.Num() * 8) % Eligible.Num();
	B.Prestige = Eligible[Idx];
	Recalculate(B);
}

void USBCharacterCalculator::CycleDiscipline(FSBCharacterBuildState& B, int32 Delta)
{
	USBShadowbaneCreationDb& Db = USBShadowbaneCreationDb::Get();
	TArray<FString> Eligible;
	Db.GetEligibleDisciplines(B.Race, B.BaseClass, B.Prestige, Eligible);
	if (Eligible.Num() == 0)
	{
		B.Discipline.Empty();
		B.DisciplineAbilities.Reset();
		Recalculate(B);
		return;
	}
	int32 Idx = 0;
	for (int32 i = 0; i < Eligible.Num(); ++i)
	{
		if (Eligible[i].Equals(B.Discipline, ESearchCase::IgnoreCase))
		{
			Idx = i;
			break;
		}
	}
	Idx = (Idx + Delta + Eligible.Num() * 8) % Eligible.Num();
	B.Discipline = Eligible[Idx];
	Recalculate(B);
}

USBCharacterArchetype* USBCharacterCalculator::FindBestRosterMatch(
	const FSBCharacterBuildState& Build,
	const TArray<TObjectPtr<USBCharacterArchetype>>& Roster)
{
	USBCharacterArchetype* Best = nullptr;
	int32 BestScore = -1;

	for (USBCharacterArchetype* Arch : Roster)
	{
		if (!Arch)
		{
			continue;
		}
		int32 Score = 0;
		if (Arch->Race.ToString().Equals(Build.Race, ESearchCase::IgnoreCase))
		{
			Score += 10;
		}
		if (Arch->Class.ToString().Equals(Build.Prestige, ESearchCase::IgnoreCase)
			|| Arch->Promotion.ToString().Equals(Build.Prestige, ESearchCase::IgnoreCase))
		{
			Score += 20;
		}
		if (Arch->BasePath.ToString().Equals(Build.BaseClass, ESearchCase::IgnoreCase))
		{
			Score += 5;
		}
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Arch;
		}
	}
	return Best;
}
