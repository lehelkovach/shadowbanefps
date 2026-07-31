// Copyright shadowbanefps.
//
// Attacker-operated field siege device (battering ram / ballista stand-in).
// Crew with Interact; while crewed it pounds a tagged structure (main gate).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SBSiegeWeapon.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class ASBDestructibleStructure;
class ASBCharacter;

UCLASS()
class SHADOWBANEFPS_API ASBSiegeWeapon : public AActor
{
	GENERATED_BODY()

public:
	ASBSiegeWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Siege")
	bool TryCrew(ASBCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void ReleaseCrew();

	UFUNCTION(BlueprintPure, Category = "Siege")
	bool IsCrewed() const { return CrewCharacter != nullptr; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege")
	FName TargetStructureTag = FName(TEXT("MainGate"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege", meta = (ClampMin = "1.0"))
	float DamagePerPulse = 85.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege", meta = (ClampMin = "0.2"))
	float PulseInterval = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siege", meta = (ClampMin = "1.0"))
	float MaxHealth = 600.f;

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void ApplyWeaponDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	void ConfigureWeapon(float InMaxHealth, float InDamagePerPulse, float InPulseInterval);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siege")
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siege")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siege")
	TObjectPtr<UBoxComponent> UseVolume;

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Siege")
	float Health = 600.f;

	UPROPERTY(ReplicatedUsing = OnRep_Crewed, BlueprintReadOnly, Category = "Siege")
	bool bCrewed = false;

	UPROPERTY()
	TObjectPtr<ASBCharacter> CrewCharacter;

	float PulseCooldown = 0.f;

	virtual void BeginPlay() override;

	ASBDestructibleStructure* FindTargetStructure() const;
	void PulseAttack();

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_Crewed();

	void ApplyVisuals();
};
