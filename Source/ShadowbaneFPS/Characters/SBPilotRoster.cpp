// Copyright shadowbanefps.
//
// Curated pilot roster — Morloch Wiki race/class compressed into FPS pre-builts.
// Keep in sync with docs/reference/SHADOWBANE_RACE_CLASS_ROSTER.md and docs/game-design.md §3.

#include "SBPilotRoster.h"
#include "SBCharacterArchetype.h"

USBCharacterArchetype* USBPilotRoster::Make(
	UObject* Outer,
	FName Id,
	const TCHAR* DisplayName,
	const TCHAR* Race,
	const TCHAR* BasePath,
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
	float MaxMana,
	float MaxStamina,
	float ManaRegen,
	float StaminaRegen,
	float MoveSpeed,
	float AttackDamage,
	float AttackRange,
	float AttackInterval,
	float StructureDamage,
	float HealPerSecond,
	float RepairPerSecond,
	bool bAttackerEligible,
	bool bDefenderEligible,
	int32 DuplicateLimit,
	bool bMeleeAttack)
{
	USBCharacterArchetype* A = NewObject<USBCharacterArchetype>(Outer, Id);
	A->ArchetypeId = Id;
	A->DisplayName = FText::FromString(DisplayName);
	A->Race = FText::FromString(Race);
	A->BasePath = FText::FromString(BasePath);
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
	A->MaxMana = MaxMana;
	A->MaxStamina = MaxStamina;
	A->ManaRegenPerSecond = ManaRegen;
	A->StaminaRegenPerSecond = StaminaRegen;
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
	A->bMeleeAttack = bMeleeAttack;
	return A;
}

void USBPilotRoster::BuildDefaultRoster(UObject* Outer, TArray<TObjectPtr<USBCharacterArchetype>>& OutRoster)
{
	OutRoster.Reset();

	// 11 curated pre-builts — Shadowbane Four Paths + race innates (§3 / §12).
	// Args: roles(6), HP, Mana, Stam, ManaRegen, StamRegen, Speed, Dmg, Range, Interval, Struct, Heal, Repair, Atk?, Def?, Dup, Melee?

	OutRoster.Add(Make(Outer, "Warrior_Blade", TEXT("Ironbrand Warrior"),
		TEXT("Human"), TEXT("Fighter"), TEXT("Warrior"), TEXT("Warlord"), TEXT("Blade Weaving"),
		TEXT("heavy melee pressure"),
		3, 0, 1, 1, 0, 1,
		140.f, 90.f, 110.f, 7.f, 16.f, 520.f, 28.f, 250.f, 0.45f, 12.f, 0.f, 0.f, true, true, 2, true));

	OutRoster.Add(Make(Outer, "Ranger_Scout", TEXT("Greyfen Warden"),
		TEXT("Elf"), TEXT("Rogue"), TEXT("Warden"), TEXT("Huntress"), TEXT("Way of the Bow"),
		TEXT("long-range bow / high mobility"),
		3, 0, 0, 3, 2, 0,
		95.f, 115.f, 95.f, 9.f, 13.f, 650.f, 20.f, 4000.f, 0.28f, 8.f, 0.f, 0.f, true, true, 2, false));

	OutRoster.Add(Make(Outer, "Assassin_Shadow", TEXT("Nightcoil Assassin"),
		TEXT("High Elf"), TEXT("Rogue"), TEXT("Assassin"), TEXT("Nightstalker"), TEXT("Shadowmantle"),
		TEXT("stealth / close burst / snare-break"),
		3, 0, 1, 3, 1, 0,
		85.f, 95.f, 115.f, 8.f, 17.f, 680.f, 34.f, 300.f, 0.55f, 6.f, 0.f, 0.f, true, true, 1, true));

	OutRoster.Add(Make(Outer, "Channeler_Flame", TEXT("Ashwake Spellweaver"),
		TEXT("Human"), TEXT("Mage"), TEXT("Spellweaver"), TEXT("Furia"), TEXT("Flame"),
		TEXT("fire magic / siege burn"),
		3, 0, 1, 1, 0, 2,
		90.f, 125.f, 90.f, 11.f, 12.f, 560.f, 22.f, 2800.f, 0.40f, 40.f, 0.f, 0.f, true, true, 1, false));

	OutRoster.Add(Make(Outer, "Healer_Prelate", TEXT("Dawnward Smite Cleric"),
		TEXT("Human"), TEXT("Healer"), TEXT("Smite Cleric"), TEXT("Smite Cleric"), TEXT("Blessed Mantle"),
		TEXT("sustained healing aura"),
		0, 3, 1, 1, 0, 0,
		100.f, 120.f, 100.f, 12.f, 13.f, 540.f, 12.f, 1800.f, 0.50f, 5.f, 18.f, 0.f, true, true, 2, false));

	OutRoster.Add(Make(Outer, "Wizard_Frost", TEXT("Rimebind Wizard"),
		TEXT("Elf"), TEXT("Mage"), TEXT("Wizard"), TEXT("Warlock"), TEXT("Frost"),
		TEXT("crowd control / chill zones"),
		2, 0, 3, 1, 0, 1,
		90.f, 130.f, 85.f, 12.f, 11.f, 550.f, 16.f, 2600.f, 0.45f, 15.f, 0.f, 0.f, true, true, 1, false));

	OutRoster.Add(Make(Outer, "Scout_Thief", TEXT("Underlane Scout"),
		TEXT("Nightshades"), TEXT("Rogue"), TEXT("Thief"), TEXT("Saboteur"), TEXT("Silent Step"),
		TEXT("scouting / detection / sabotage"),
		1, 0, 1, 3, 3, 1,
		90.f, 100.f, 105.f, 8.f, 15.f, 670.f, 14.f, 2000.f, 0.35f, 20.f, 0.f, 0.f, true, true, 2, false));

	OutRoster.Add(Make(Outer, "Templar_Bulwark", TEXT("Bastion Templar"),
		TEXT("Dwarf"), TEXT("Fighter"), TEXT("Templar"), TEXT("Paladin"), TEXT("Bulwark"),
		TEXT("frontline hold / light heal"),
		1, 2, 1, 0, 0, 1,
		170.f, 80.f, 120.f, 7.f, 16.f, 480.f, 18.f, 350.f, 0.50f, 18.f, 8.f, 0.f, true, true, 2, true));

	OutRoster.Add(Make(Outer, "Siege_Engineer", TEXT("Breachwright Engineer"),
		TEXT("Dwarf"), TEXT("Fighter"), TEXT("Warrior"), TEXT("Huntmaster"), TEXT("Siegecraft"),
		TEXT("siege device / structure damage"),
		1, 0, 0, 1, 1, 3,
		110.f, 75.f, 115.f, 6.f, 15.f, 500.f, 15.f, 2200.f, 0.40f, 55.f, 0.f, 15.f, true, false, 1, false));

	OutRoster.Add(Make(Outer, "Defender_Warden", TEXT("Wallwarden"),
		TEXT("Human"), TEXT("Fighter"), TEXT("Warrior"), TEXT("Warlord"), TEXT("Fortress"),
		TEXT("repair / emplacement defense"),
		1, 0, 0, 1, 1, 3,
		130.f, 95.f, 110.f, 8.f, 15.f, 500.f, 16.f, 1800.f, 0.40f, 25.f, 0.f, 22.f, false, true, 2, false));

	// Key 0 — Minotaur silhouette (built-in horns until Fab mesh lands).
	OutRoster.Add(Make(Outer, "Minotaur_Bulwark", TEXT("Horned Gatebreaker"),
		TEXT("Minotaur"), TEXT("Fighter"), TEXT("Warrior"), TEXT("Warlord"), TEXT("Blade Weaving"),
		TEXT("minotaur melee bruiser / gate pressure"),
		3, 0, 1, 1, 0, 2,
		185.f, 60.f, 130.f, 5.f, 18.f, 500.f, 32.f, 280.f, 0.50f, 30.f, 0.f, 0.f, true, true, 1, true));
}
