// Copyright shadowbanefps.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SBTypes.h"
#include "SBPlayerState.generated.h"

class USBCharacterArchetype;

/**
 * Per-player match state: team assignment and the currently selected pre-built
 * character. See design doc §3 (selection) and §9 (post-death switching).
 *
 * HIDDEN COMPOSITION NOTE (design doc §4):
 * The selected archetype is authoritative on the server. It must only be
 * disclosed to teammates and to enemies who have *observed* the player. The raw
 * identity should NOT be blanket-replicated to opponents. See SBSiegeGameMode /
 * the intel system for how disclosure is gated per-connection. The property here
 * is replicated for owner + teammates; enemy-facing knowledge is delivered
 * separately as short-lived "observed" intel markers.
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

	/** Server-only. Assigns the player's side. */
	void SetTeam(ESBTeam NewTeam);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	USBCharacterArchetype* GetSelectedArchetype() const { return SelectedArchetype; }

	/** Server-only. Sets the pre-built character this player is (re)deploying as. */
	void SetSelectedArchetype(USBCharacterArchetype* NewArchetype);

	UFUNCTION(BlueprintCallable, Category = "Siege")
	bool IsAlive() const { return bAlive; }

	void SetAlive(bool bNewAlive) { bAlive = bNewAlive; }

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Team, BlueprintReadOnly, Category = "Siege")
	ESBTeam Team = ESBTeam::Unassigned;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	TObjectPtr<USBCharacterArchetype> SelectedArchetype = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Siege")
	bool bAlive = false;

	UFUNCTION()
	void OnRep_Team();
};
