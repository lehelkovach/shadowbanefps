// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/SBTypes.h"
#include "SBDestructibleStructure.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBOnStructureStateChanged, ESBStructureState, NewState);

/**
 * A gate, breach wall, emplacement, beacon, or barricade with intact / damaged /
 * destroyed states and clear gameplay consequences (design doc §7).
 */
UCLASS()
class SHADOWBANEFPS_API ASBDestructibleStructure : public AActor
{
	GENERATED_BODY()

public:
	ASBDestructibleStructure();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void ApplyStructureDamage(float Amount, AController* Instigator = nullptr, FName AttackerArchetype = NAME_None, FName PowerId = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void Repair(float Amount);

	UFUNCTION(BlueprintPure, Category = "Siege")
	ESBStructureState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	UPROPERTY(BlueprintAssignable, Category = "Siege")
	FSBOnStructureStateChanged OnStructureStateChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siege")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege", meta = (ClampMin = "1.0"))
	float MaxHealth = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamagedThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege")
	bool bDestructionIsPermanent = true;

	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Siege")
	ESBStructureState State = ESBStructureState::Intact;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	float Health = 1000.f;

	virtual void BeginPlay() override;

	void RefreshState();
	void ApplyVisualState();

	UFUNCTION()
	void OnRep_State();
};
