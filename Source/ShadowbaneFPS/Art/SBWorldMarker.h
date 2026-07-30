// Copyright shadowbanefps.
//
// Floating colored marker used as a dummy "rune / objective icon" in the world.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SBWorldMarker.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class SHADOWBANEFPS_API ASBWorldMarker : public AActor
{
	GENERATED_BODY()

public:
	ASBWorldMarker();

	UFUNCTION(BlueprintCallable, Category = "Siege|Art")
	void Configure(const FString& Label, FLinearColor Color, float Scale = 1.f);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Disc;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> LabelText;

	virtual void BeginPlay() override;
};
