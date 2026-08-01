// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Characters/SBCharacterCalculator.h"
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

	/** shadowbanefps-style creation builder (while dead). Toggle with C. */
	UFUNCTION(BlueprintPure, Category = "Siege|Builder")
	bool IsCreationBuilderOpen() const { return bCreationBuilderOpen; }

	UFUNCTION(BlueprintPure, Category = "Siege|Builder")
	FSBCharacterBuildState GetCreationBuild() const;

	UFUNCTION(Server, Reliable)
	void ServerConfirmCreationBuild(const FString& Race, const FString& BaseClass, const FString& Prestige, const FString& Discipline, const FString& HeroName);

	/** Sets hero display name (replicates via PlayerState::SetPlayerName). */
	UFUNCTION(Server, Reliable)
	void ServerSetHeroName(const FString& HeroName);

	/** Local draft name shown in the creation builder. */
	UFUNCTION(BlueprintPure, Category = "Siege|Builder")
	FString GetCreationHeroName() const { return CreationHeroName; }

	UFUNCTION(BlueprintPure, Category = "Siege|Builder")
	bool IsHeroNameEditing() const { return bHeroNameEditing; }

	void SetCreationHeroName(const FString& InName);

	/** Type into hero name while builder is open (N toggles edit). */
	virtual bool InputKey(const FInputKeyParams& Params) override;

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

	/** Console: SBName MyHero — set display name (and builder draft). */
	UFUNCTION(Exec)
	void SBName(const FString& NewName);

	/** Console: SBDebugMsg "text" — push an on-screen upper-left message. */
	UFUNCTION(Exec)
	void SBDebugMsg(const FString& Message);

	/** Console / key: spawn test bots on the listen/dedicated server. */
	UFUNCTION(Exec)
	void SpawnTestBots(int32 Count = 6);

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
	void TryCastOrSelectArchetype(int32 SlotIndex);
	void RequestCastSpell(int32 SlotIndex);
	void RecallPressed();
	void RequestRespawnPressed();
	void SpawnTestBotsPressed();

	void ToggleCreationBuilder();
	void BuilderCycleRaceNext();
	void BuilderCycleRacePrev();
	void BuilderCycleBaseNext();
	void BuilderCycleBasePrev();
	void BuilderCyclePrestigeNext();
	void BuilderCyclePrestigePrev();
	void BuilderCycleDisciplineNext();
	void BuilderCycleDisciplinePrev();
	void BuilderConfirm();
	void BuilderToggleNameEdit();
	void EnsureCreationBuildInitialized();
	static FString SanitizeHeroName(const FString& InName);

	bool bCreationBuilderOpen = false;
	bool bHeroNameEditing = false;
	FSBCharacterBuildState CreationBuild;
	FString CreationHeroName;
};
