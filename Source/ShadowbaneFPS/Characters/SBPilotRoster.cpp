// Copyright shadowbanefps.

#include "SBPilotRoster.h"
#include "SBCharacterArchetype.h"

USBCharacterArchetype* USBPilotRoster::Make(
	UObject* Outer,
	FName Id,
	const TCHAR* DisplayName,
	const TCHAR* Race,
	const TCHAR* ClassName,
	const TCHAR* Promotion,
	const TCHAR* Discipline,
	const TCHAR* Signature,
	uint8 Damage,
	uint8 Healing,
	uint8 Control,
	uint8 Mobility,
	uint8 Detection,
	uint8 Siege,
	float MaxHealth,
	float MoveSpeed,
	float AttackDamage,
	float AttackRange,
	float AttackInterval,
	float StructureDamage,
	float HealPerSecond,
	float RepairPerSecond,
	bool bAttackerEligible,
	bool bDefenderEligible,
	int32 DuplicateLimit)
{
	USBCharacterArchetype* A = NewObject<USBCharacterArchetype>(Outer, Id);
	A->ArchetypeId = Id;
	A->DisplayName = FText::FromString(DisplayName);
	A->Race = FText::FromString(Race);
	A->Class = FText::FromString(ClassName);
	A->Promotion = FText::FromString(Promotion);
	A->Discipline = FText::FromString(Discipline);
	A->ObservableSignature = FText::FromString(Signature);
	A->RoleProfile.Damage = Damage;
	A->RoleProfile.Healing = Healing;
	A->RoleProfile.Control = Control;
	A->RoleProfile.Mobility = Mobility;
	A->RoleProfile.Detection = Detection;
	A->RoleProfile.Siege = Siege;
	A->MaxHealth = MaxHealth;
	A->MoveSpeed = MoveSpeed;
	A->AttackDamage = AttackDamage;
	A->AttackRange = AttackRange;
	A->AttackInterval = AttackInterval;
	A->StructureDamage = StructureDamage;
	A->HealPerSecond = HealPerSecond;
	A->RepairPerSecond = RepairPerSecond;
	A->bAttackerEligible = bAttackerEligible;
	A->bDefenderEligible = bDefenderEligible;
	A->PerTeamDuplicateLimit = DuplicateLimit;
	return A;
}

void USBPilotRoster::BuildDefaultRoster(UObject* Outer, TArray<TObjectPtr<USBCharacterArchetype>>& OutRoster)
{
	OutRoster.Reset();

	// 10 curated pre-builts — Shadowbane-flavored, immediately playable (§3 / §12).
	OutRoster.Add(Make(Outer, "Warrior_Blade", TEXT("Ironbrand Warrior"),
		TEXT("Human"), TEXT("Warrior"), TEXT("Warlord"), TEXT("Blade Weaving"),
		TEXT("heavy melee pressure"),
		3, 0, 1, 1, 0, 1, 140.f, 520.f, 28.f, 250.f, 0.45f, 12.f, 0.f, 0.f, true, true, 2));

	OutRoster.Add(Make(Outer, "Ranger_Scout", TEXT("Greyfen Ranger"),
		TEXT("Elf"), TEXT("Ranger"), TEXT("Huntress"), TEXT("Way of the Bow"),
		TEXT("long-range bow / high mobility"),
		3, 0, 0, 3, 2, 0, 95.f, 650.f, 20.f, 4000.f, 0.28f, 8.f, 0.f, 0.f, true, true, 2));

	OutRoster.Add(Make(Outer, "Assassin_Shadow", TEXT("Nightcoil Assassin"),
		TEXT("Aelfborn"), TEXT("Assassin"), TEXT("Nightstalker"), TEXT("Shadowmantle"),
		TEXT("stealth / close burst"),
		3, 0, 1, 3, 1, 0, 85.f, 680.f, 34.f, 300.f, 0.55f, 6.f, 0.f, 0.f, true, true, 1));

	OutRoster.Add(Make(Outer, "Channeler_Flame", TEXT("Ashwake Channeler"),
		TEXT("Human"), TEXT("Channeler"), TEXT("Furia"), TEXT("Flame"),
		TEXT("fire magic / siege burn"),
		3, 0, 1, 1, 0, 2, 90.f, 560.f, 22.f, 2800.f, 0.40f, 40.f, 0.f, 0.f, true, true, 1));

	OutRoster.Add(Make(Outer, "Healer_Prelate", TEXT("Dawnward Prelate"),
		TEXT("Human"), TEXT("Healer"), TEXT("Prelate"), TEXT("Blessed Mantle"),
		TEXT("sustained healing aura"),
		0, 3, 1, 1, 0, 0, 100.f, 540.f, 12.f, 1800.f, 0.50f, 5.f, 18.f, 0.f, true, true, 2));

	OutRoster.Add(Make(Outer, "Wizard_Frost", TEXT("Rimebind Wizard"),
		TEXT("Elf"), TEXT("Wizard"), TEXT("Warlock"), TEXT("Frost"),
		TEXT("crowd control / chill zones"),
		2, 0, 3, 1, 0, 1, 90.f, 550.f, 16.f, 2600.f, 0.45f, 15.f, 0.f, 0.f, true, true, 1));

	OutRoster.Add(Make(Outer, "Scout_Thief", TEXT("Underlane Scout"),
		TEXT("Shade"), TEXT("Thief"), TEXT("Saboteur"), TEXT("Silent Step"),
		TEXT("scouting / detection / sabotage"),
		1, 0, 1, 3, 3, 1, 90.f, 670.f, 14.f, 2000.f, 0.35f, 20.f, 0.f, 0.f, true, true, 2));

	OutRoster.Add(Make(Outer, "Templar_Bulwark", TEXT("Bastion Templar"),
		TEXT("Dwarf"), TEXT("Templar"), TEXT("Paladin"), TEXT("Bulwark"),
		TEXT("frontline hold / light heal"),
		1, 2, 1, 0, 0, 1, 170.f, 480.f, 18.f, 350.f, 0.50f, 18.f, 8.f, 0.f, true, true, 2));

	OutRoster.Add(Make(Outer, "Siege_Engineer", TEXT("Breachwright Engineer"),
		TEXT("Dwarf"), TEXT("Warrior"), TEXT("Huntmaster"), TEXT("Siegecraft"),
		TEXT("siege device / structure damage"),
		1, 0, 0, 1, 1, 3, 110.f, 500.f, 15.f, 2200.f, 0.40f, 55.f, 0.f, 15.f, true, false, 1));

	OutRoster.Add(Make(Outer, "Defender_Warden", TEXT("Wallwarden"),
		TEXT("Human"), TEXT("Warrior"), TEXT("Warlord"), TEXT("Fortress"),
		TEXT("repair / emplacement defense"),
		1, 0, 1, 0, 2, 2, 130.f, 500.f, 16.f, 2200.f, 0.40f, 10.f, 0.f, 35.f, false, true, 2));
}
