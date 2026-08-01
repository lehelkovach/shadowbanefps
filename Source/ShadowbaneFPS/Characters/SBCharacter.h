// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SBCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USBCharacterArchetype;
class ASBPlayerState;
class UInputMappingContext;
class UInputAction;
class UProceduralMeshComponent;
class USkeletalMeshComponent;
class UAnimMontage;
class UAnimSequence;

/**
 * Shared pilot pawn. Archetype data (stats / role) is applied at spawn so the
 * curated roster can ship without a unique Blueprint per character (§3).
 * Melee builds use a server-authority sphere sweep; ranged use hitscan.
 */
UCLASS()
class SHADOWBANEFPS_API ASBCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASBCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void PawnClientRestart() override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Server-only. Applies archetype combat stats and stores the selection. */
	void ApplyArchetype(USBCharacterArchetype* Archetype);

	/** Overlay shadowbanefps calculator vitals after ApplyArchetype. */
	void ApplyCreationVitals(const struct FSBCreationFpsVitals& Vitals, const FString& OverlayRace = FString());

	/** Server-authority attack used by bots (no client RPC). */
	void BotFire();

	/** Client → server: cast rune slot 0–3 (QWER). */
	UFUNCTION(Server, Reliable)
	void ServerCastSpell(int32 SlotIndex);

	/** Client → server: LoL-style recall channel (B). Cancels on move / damage / attack. */
	UFUNCTION(Server, Reliable)
	void ServerStartRecall();

	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Siege|Recall")
	bool IsRecalling() const { return bRecalling; }

	UFUNCTION(BlueprintPure, Category = "Siege|Recall")
	float GetRecallTimeRemaining() const { return RecallTimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Siege|Vitals")
	float GetMana() const { return Mana; }

	UFUNCTION(BlueprintPure, Category = "Siege|Vitals")
	float GetMaxMana() const { return MaxMana; }

	UFUNCTION(BlueprintPure, Category = "Siege|Vitals")
	float GetStamina() const { return Stamina; }

	UFUNCTION(BlueprintPure, Category = "Siege|Vitals")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "Siege|Vitals")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	USBCharacterArchetype* GetArchetype() const { return Archetype; }

	UFUNCTION(BlueprintPure, Category = "Siege|Combat")
	bool UsesMelee() const { return bUsesMelee; }

	UFUNCTION(BlueprintPure, Category = "Siege|Combat")
	bool IsBowEquipped() const { return bBowEquipped; }

	UFUNCTION(BlueprintPure, Category = "Siege|Combat")
	bool IsFirstPersonAiming() const { return bFirstPersonAim; }

	UFUNCTION(BlueprintPure, Category = "Siege|Combat")
	float GetAimCharge01() const { return AimCharge01; }

	UFUNCTION(BlueprintPure, Category = "Siege|Combat")
	bool IsCasting() const { return bCasting; }

	/** Local: begin hold-to-aim fireball (slot 0). */
	void BeginFireballAim();
	/** Local: release held fireball. */
	void ReleaseFireballAim();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** Race head (sphere). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	/** Race features: horns / ears (L/R) and snout / hood / beard (Extra). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> FeatureL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> FeatureR;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> FeatureExtra;

	/** Floating dummy "rune" disc — role-colored placeholder VFX. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> RuneDisc;

	/** Cast-pose hands (spheres) pushed forward during rune invocations. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Cast")
	TObjectPtr<UStaticMeshComponent> HandL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Cast")
	TObjectPtr<UStaticMeshComponent> HandR;

	/**
	 * Melee weapon assembly (engine shapes → axe). FP on camera, TP on body for peers/bots.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Melee")
	TObjectPtr<USceneComponent> FpWeaponPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Melee")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Melee")
	TObjectPtr<UStaticMeshComponent> AxeBladeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Melee")
	TObjectPtr<USceneComponent> TpWeaponPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Melee")
	TObjectPtr<UStaticMeshComponent> TpWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Melee")
	TObjectPtr<UStaticMeshComponent> TpAxeBladeMesh;

	/** Imported free sword (skeletal). Preferred over static axe when loaded. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Melee")
	TObjectPtr<USkeletalMeshComponent> TpSwordMesh;

	/** Crescent battle-axe procedural mesh (replaces cube mallet). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh|Melee")
	TObjectPtr<UProceduralMeshComponent> TpBattleAxe;

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Stats")
	float Health = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 100.f;

	/** Shadowbane-style resource pools (0–100 display). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats|Vitals")
	float Mana = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats|Vitals")
	float MaxMana = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats|Vitals")
	float Stamina = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats|Vitals")
	float MaxStamina = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Dead, BlueprintReadOnly, Category = "Stats")
	bool bDead = false;

	UPROPERTY(ReplicatedUsing = OnRep_Archetype, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<USBCharacterArchetype> Archetype = nullptr;

	/** Replicated independently so clients can build their race silhouette before asset data resolves. */
	UPROPERTY(ReplicatedUsing = OnRep_RaceName, BlueprintReadOnly, Category = "Stats")
	FString RaceName = TEXT("Human");

	UPROPERTY(ReplicatedUsing = OnRep_UsesMelee, BlueprintReadOnly, Category = "Combat")
	bool bUsesMelee = false;

	/** Melee builds can Q-swap to bow; ranged builds start with bow. */
	UPROPERTY(ReplicatedUsing = OnRep_BowEquipped, BlueprintReadOnly, Category = "Combat")
	bool bBowEquipped = false;

	/** True when Epic mannequin skeletal mesh + AnimBP loaded (replaces box body). */
	bool bUsingHeroMesh = false;

	/** Local/cosmetic: FP ADS for bow draw or fireball aim. */
	bool bFirstPersonAim = false;
	bool bDrawingBow = false;
	bool bAimingFireball = false;
	float AimCharge01 = 0.f;
	float BowDrawSeconds = 0.9f;
	float FireballChargeSeconds = 1.15f;
	float DefaultSpringArmLength = 320.f;
	float AimSpringArmLength = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DefaultAttackDamage = 18.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DefaultAttackRange = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DefaultAttackInterval = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Melee")
	float MeleeSweepRadius = 60.f;

	float AttackDamage = 18.f;
	float AttackRange = 2500.f;
	float AttackInterval = 0.35f;
	float StructureDamage = 25.f;
	float HealPerSecond = 0.f;
	float RepairPerSecond = 0.f;
	float LastFireTime = -1000.f;
	float LastSpellTime = -1000.f;
	float SpellCooldown = 1.15f;
	float MeleeSwingAnimRemaining = 0.f;
	float MeleeSwingAnimDuration = 0.55f;
	float CastFlashRemaining = 0.f;
	float CastInvokeDuration = 1.05f;
	/** Shadowbane-style: spells channel while rooted; move/damage cancels. Archers stay mobile. */
	UPROPERTY(ReplicatedUsing = OnRep_Casting, BlueprintReadOnly, Category = "Combat|Cast")
	bool bCasting = false;
	int32 PendingCastSlot = -1;
	float CastTimeRemaining = 0.f;
	float SpellCastDuration = 1.05f;
	float CastMoveCancelSpeed = 40.f;
	float WalkSpeedBeforeCast = 600.f;
	FVector LastSwingBladePos = FVector::ZeroVector;
	bool bSwingTrailValid = false;
	TObjectPtr<UAnimMontage> CachedMeleeMontage = nullptr;
	TObjectPtr<UAnimSequence> CachedMeleeSwingSequence = nullptr;
	/** True while a skeletal montage/slot anim is driving the swing (skip procedural arm bones). */
	bool bMeleeMontagePlaying = false;
	FDelegateHandle HeroBonesFinalizedHandle;
	FRotator HeroMeshBaseRelativeRot = FRotator(0.f, -90.f, 0.f);

	/** Runtime Enhanced Input (no Content IMC assets required). */
	UPROPERTY()
	TObjectPtr<UInputMappingContext> MoveMappingContext = nullptr;
	UPROPERTY()
	TObjectPtr<UInputAction> MoveForwardAction = nullptr;
	UPROPERTY()
	TObjectPtr<UInputAction> MoveRightAction = nullptr;
	UPROPERTY()
	TObjectPtr<UInputAction> LookAction = nullptr;
	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction = nullptr;
	UPROPERTY()
	TObjectPtr<UInputAction> FireAction = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_Recalling, BlueprintReadOnly, Category = "Siege|Recall")
	bool bRecalling = false;

	float RecallTimeRemaining = 0.f;
	float RecallChannelDuration = 6.5f;
	float LastRecallCompleteTime = -1000.f;
	float RecallCooldown = 12.f;
	float RecallMoveCancelSpeed = 40.f;

	/** Last power that damaged this pawn — used for kill attribution telemetry. */
	FName LastDamagePowerId = NAME_None;

	/** Batches heal telemetry so aura ticks do not flood the CSV. */
	TMap<TWeakObjectPtr<ASBCharacter>, float> PendingHealTelemetry;
	float HealTelemetryFlushAmount = 12.f;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void OnMoveForwardTriggered(const FInputActionValue& Value);
	void OnMoveRightTriggered(const FInputActionValue& Value);
	void OnLookTriggered(const FInputActionValue& Value);
	void OnFirePressed();
	void OnFireReleased();
	void OnToggleBowPressed();
	void OnInteractPressed();
	void OnPingPressed();
	void EnsureEnhancedInputAssets();
	void PollKeyboardMovement();
	void PollMouseLook();
	void EnsureGameplayInputMode();
	void ApplyMeleeBodyPose(float Alpha01);

	UFUNCTION(Server, Reliable)
	void ServerFire();

	UFUNCTION(Server, Reliable)
	void ServerFireBow(float Charge01);

	UFUNCTION(Server, Reliable)
	void ServerToggleBow();

	UFUNCTION(Server, Reliable)
	void ServerCastChargedFireball(float Charge01);

	UFUNCTION(Server, Reliable)
	void ServerInteract();

	/** Cosmetic swing for all peers — damage already applied on server. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastMeleeSwing();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBowShot(float Charge01);

	/** Cast yell + rune flash for all peers (cast start / interrupt messaging). */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastCastSpell(int32 SlotIndex);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastCastInterrupted();

	UFUNCTION(Client, Unreliable)
	void ClientCombatHint(const FString& Message);

	/** Shows a world-space combat number on every connected client's local HUD. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShowDamage(float Amount, FVector Loc, bool bHeal);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRecallEvent(bool bStarted, bool bCompleted);

	void PerformAttack();
	void PerformHitscanFire();
	void PerformBowShot(float Charge01);
	void PerformMeleeSwing();
	void PerformSpell(int32 SlotIndex);
	void BeginSpellCast(int32 SlotIndex);
	void TickSpellCast(float DeltaSeconds);
	void CancelSpellCast(const TCHAR* Reason);
	void CompleteSpellCast();
	void ApplyCastRoot(bool bRoot);
	void PerformSpellBolt();
	void PerformSpellBoltCharged(float Charge01);
	void PerformSpellHealPulse();
	void PerformSpellShockwave();
	void PerformSpellReinforce();
	void StartRecall();
	void CancelRecall(const TCHAR* Reason);
	void CompleteRecall();
	void TickRecall(float DeltaSeconds);
	void PerformSupportTick(float DeltaSeconds);
	void TickVitals(float DeltaSeconds);
	void Die(AController* KillerController);
	void ApplyDeathPose();
	void ClearDeathPose();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDeathPose();

	float ManaRegenPerSecond = 8.f;
	float StaminaRegenPerSecond = 14.f;
	float SpellManaCost = 18.f;
	float MeleeStaminaCost = 8.f;
	float CorpseLifeSeconds = 9.5f;

	void SetFirstPersonAim(bool bAim);
	void TickAimCharge(float DeltaSeconds);
	void UpdateFpAimVisuals();
	void CancelAimModes(const TCHAR* Reason);

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_Dead();

	UFUNCTION()
	void OnRep_Archetype();

	UFUNCTION()
	void OnRep_RaceName();

	UFUNCTION()
	void OnRep_UsesMelee();

	UFUNCTION()
	void OnRep_BowEquipped();

	UFUNCTION()
	void OnRep_Casting();

	UFUNCTION()
	void OnRep_Recalling();

	void UpdatePlaceholderVisuals();
	void UpdateWeaponVisibility();
	void TickMeleeSwingVisual(float DeltaSeconds);
	void ApplyMeleeIdlePose();
	void TickCastInvokeVisual(float DeltaSeconds);
	void SetupHeroMeshForRace();
	void SetPlaceholderBodyVisible(bool bVisible);
	void EnsureWeaponInHand();
	void PlayMeleeAttackAnimation();
	void EnsureMeleeSwingAnimAssets();
	void BindHeroBoneSwingOverlay();
	void UnbindHeroBoneSwingOverlay();
	void OnHeroBonesFinalized();
	void ApplyProceduralMeleeArmBones(float Alpha01);
	void ApplyHeroRaceMaterials();
};
