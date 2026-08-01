// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SBTypes.h"
#include "Characters/SBCharacterCalculator.h"
#include "SBPlayerState.generated.h"

class USBCharacterArchetype;

/**
 * Per-player match state. Selected character is replicated by ArchetypeId so
 * clients can resolve against the shared roster without needing cooked assets.
 * Full enemy roster disclosure is still a separate intel concern (§4).
 */
UCLASS()
class SHADOWBANEFPS_API ASBPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASBPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Siege")
	ESBTeam GetTeam() const { return Team; }

	void SetTeam(ESBTeam NewTeam);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	int32 GetKillCount() const { return KillCount; }

	UFUNCTION(BlueprintCallable, Category = "Siege")
	int32 GetDeathCount() const { return DeathCount; }

	void AddKill();
	void AddDeath();

	UFUNCTION(BlueprintCallable, Category = "Siege")
	USBCharacterArchetype* GetSelectedArchetype() const;

	UFUNCTION(BlueprintCallable, Category = "Siege")
	FName GetSelectedArchetypeId() const { return SelectedArchetypeId; }

	void SetSelectedArchetype(USBCharacterArchetype* NewArchetype);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	bool IsAlive() const { return bAlive; }

	void SetAlive(bool bNewAlive);

	/** shadowbanefps-derived FPS vitals overlay applied on next spawn. */
	void SetCreationVitalsOverlay(const FSBCreationFpsVitals& Vitals, const FString& Race, const FString& Base, const FString& Prestige, const FString& Discipline = FString());
	bool ConsumeCreationVitalsOverlay(FSBCreationFpsVitals& OutVitals);
	bool HasCreationVitalsOverlay() const { return bHasCreationVitals; }
	FString GetCreationSummary() const;
	FString GetCreationRace() const { return CreationRace; }

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Team, BlueprintReadOnly, Category = "Siege")
	ESBTeam Team = ESBTeam::Unassigned;

	UPROPERTY(ReplicatedUsing = OnRep_ArchetypeId, BlueprintReadOnly, Category = "Siege")
	FName SelectedArchetypeId = NAME_None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	bool bAlive = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	int32 KillCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	int32 DeathCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege|Builder")
	bool bHasCreationVitals = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege|Builder")
	FSBCreationFpsVitals CreationVitals;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege|Builder")
	FString CreationRace;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege|Builder")
	FString CreationBaseClass;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege|Builder")
	FString CreationPrestige;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege|Builder")
	FString CreationDiscipline;

	/** Resolved locally from SelectedArchetypeId; not replicated. */
	UPROPERTY(Transient)
	mutable TObjectPtr<USBCharacterArchetype> CachedArchetype = nullptr;

	UFUNCTION()
	void OnRep_Team();

	UFUNCTION()
	void OnRep_ArchetypeId();
};
