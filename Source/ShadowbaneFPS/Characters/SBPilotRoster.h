// Copyright shadowbanefps.
//
// Runtime factory for the curated ~10-character pilot roster (design doc §3, §12).
// Built in C++ so the project is playable before any .uasset data assets exist.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SBPilotRoster.generated.h"

class USBCharacterArchetype;

UCLASS()
class SHADOWBANEFPS_API USBPilotRoster : public UObject
{
	GENERATED_BODY()

public:
	/** Creates the default pilot roster owned by Outer (typically the GameMode). */
	static void BuildDefaultRoster(UObject* Outer, TArray<TObjectPtr<USBCharacterArchetype>>& OutRoster);

private:
	static USBCharacterArchetype* Make(
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
		int32 DuplicateLimit);
};
