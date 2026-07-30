// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SBCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USBCharacterArchetype;
class ASBPlayerState;

/**
 * Shared pilot pawn. Archetype data (stats / role) is applied at spawn so the
 * curated roster can ship without a unique Blueprint per character (§3).
 */
UCLASS()
class SHADOWBANEFPS_API ASBCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASBCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Server-only. Applies archetype combat stats and stores the selection. */
	void ApplyArchetype(USBCharacterArchetype* Archetype);

	/** Server-authority fire used by bots (no client RPC). */
	void BotFire();

	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	USBCharacterArchetype* GetArchetype() const { return Archetype; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** Floating dummy "rune" disc — role-colored placeholder VFX. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> RuneDisc;

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Stats")
	float Health = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Archetype, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<USBCharacterArchetype> Archetype = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DefaultAttackDamage = 18.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DefaultAttackRange = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DefaultAttackInterval = 0.35f;

	float AttackDamage = 18.f;
	float AttackRange = 2500.f;
	float AttackInterval = 0.35f;
	float StructureDamage = 25.f;
	float HealPerSecond = 0.f;
	float RepairPerSecond = 0.f;
	float LastFireTime = -1000.f;

	/** Last power that damaged this pawn — used for kill attribution telemetry. */
	FName LastDamagePowerId = NAME_None;

	/** Batches heal telemetry so aura ticks do not flood the CSV. */
	TMap<TWeakObjectPtr<ASBCharacter>, float> PendingHealTelemetry;
	float HealTelemetryFlushAmount = 12.f;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void OnFirePressed();
	void OnInteractPressed();
	void OnPingPressed();

	UFUNCTION(Server, Reliable)
	void ServerFire();

	UFUNCTION(Server, Reliable)
	void ServerInteract();

	void PerformFire();
	void PerformSupportTick(float DeltaSeconds);
	void Die(AController* KillerController);

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_Archetype();

	void UpdatePlaceholderVisuals();
};
