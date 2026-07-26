// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SBTypes.h"
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
	USBCharacterArchetype* GetSelectedArchetype() const;

	UFUNCTION(BlueprintCallable, Category = "Siege")
	FName GetSelectedArchetypeId() const { return SelectedArchetypeId; }

	void SetSelectedArchetype(USBCharacterArchetype* NewArchetype);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	bool IsAlive() const { return bAlive; }

	void SetAlive(bool bNewAlive);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Team, BlueprintReadOnly, Category = "Siege")
	ESBTeam Team = ESBTeam::Unassigned;

	UPROPERTY(ReplicatedUsing = OnRep_ArchetypeId, BlueprintReadOnly, Category = "Siege")
	FName SelectedArchetypeId = NAME_None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	bool bAlive = false;

	/** Resolved locally from SelectedArchetypeId; not replicated. */
	UPROPERTY(Transient)
	mutable TObjectPtr<USBCharacterArchetype> CachedArchetype = nullptr;

	UFUNCTION()
	void OnRep_Team();

	UFUNCTION()
	void OnRep_ArchetypeId();
};
