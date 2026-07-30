// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SBPlayerController.generated.h"

class USBCharacterArchetype;

/**
 * Player-facing match controls: archetype select while dead, respawn request,
 * and future lobby / intel UI hooks (design doc §3, §9, §11).
 */
UCLASS()
class SHADOWBANEFPS_API ASBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASBPlayerController();

	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintPure, Category = "Siege")
	float GetRespawnTimeRemaining() const { return RespawnTimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "Siege")
	bool CanRespawn() const { return bCanRespawn; }

	/** Server sets this when a death starts the respawn countdown. */
	void ServerBeginRespawnCountdown(float DelaySeconds);

	UFUNCTION(Server, Reliable)
	void ServerSelectArchetypeByIndex(int32 RosterIndex);

	UFUNCTION(Server, Reliable)
	void ServerRequestRespawn();

	/** Admin free-cam spectate for bot-populated playtests. */
	UFUNCTION(BlueprintCallable, Category = "Siege|Admin")
	void EnterAdminSpectate();

	UFUNCTION(BlueprintPure, Category = "Siege|Admin")
	bool IsAdminSpectator() const { return bAdminSpectator; }

	UFUNCTION(BlueprintPure, Category = "Siege|Admin")
	bool WantsAdminSpectate() const { return bWantAdminSpectate; }

	void SetWantsAdminSpectate(bool bWant) { bWantAdminSpectate = bWant; }

	/** Console: AddBots 8 */
	UFUNCTION(Exec)
	void AddBots(int32 Count = 8);

	/** Console: AdminSpectate */
	UFUNCTION(Exec)
	void AdminSpectate();

protected:
	virtual void PlayerTick(float DeltaTime) override;
	virtual void BeginPlay() override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	float RespawnTimeRemaining = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	bool bCanRespawn = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege|Admin")
	bool bAdminSpectator = false;

	bool bWantAdminSpectate = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void SelectArchetypeSlot0();
	void SelectArchetypeSlot1();
	void SelectArchetypeSlot2();
	void SelectArchetypeSlot3();
	void SelectArchetypeSlot4();
	void SelectArchetypeSlot5();
	void SelectArchetypeSlot6();
	void SelectArchetypeSlot7();
	void SelectArchetypeSlot8();
	void SelectArchetypeSlot9();
	void RequestRespawnPressed();
};
