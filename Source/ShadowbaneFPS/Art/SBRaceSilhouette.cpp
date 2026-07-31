// Copyright shadowbanefps.

#include "Art/SBRaceSilhouette.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"

namespace
{
	void Hide(UStaticMeshComponent* Mesh)
	{
		if (Mesh)
		{
			Mesh->SetHiddenInGame(true);
			Mesh->SetRelativeScale3D(FVector(0.01f));
		}
	}

	void ShowAt(UStaticMeshComponent* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		if (!Mesh)
		{
			return;
		}
		Mesh->SetHiddenInGame(false);
		Mesh->SetRelativeLocation(Loc);
		Mesh->SetRelativeRotation(Rot);
		Mesh->SetRelativeScale3D(Scale);
	}
}

void SBRaceSilhouette::Apply(const FString& RaceName, const FParts& Parts, FLinearColor& InOutBodyTint)
{
	const FString Race = RaceName.TrimStartAndEnd().ToLower();

	Hide(Parts.FeatureL);
	Hide(Parts.FeatureR);
	Hide(Parts.FeatureExtra);

	// Defaults = Human
	float CapsuleRadius = 42.f;
	float CapsuleHalfHeight = 96.f;
	FVector BodyLoc(0.f, 0.f, -20.f);
	FVector BodyScale(0.7f, 0.7f, 1.6f);
	FVector HeadLoc(0.f, 0.f, 78.f);
	FVector HeadScale(0.45f, 0.45f, 0.45f);

	if (Race.Contains(TEXT("minotaur")) || Race.Contains(TEXT("minot")))
	{
		CapsuleRadius = 52.f;
		CapsuleHalfHeight = 102.f;
		BodyLoc = FVector(0.f, 0.f, -10.f);
		BodyScale = FVector(1.3f, 1.1f, 1.9f);
		HeadLoc = FVector(8.f, 0.f, 95.f);
		HeadScale = FVector(0.68f, 0.62f, 0.62f);
		ShowAt(Parts.FeatureL, FVector(10.f, -34.f, 130.f), FRotator(25.f, 0.f, -35.f), FVector(0.24f, 0.24f, 0.9f));
		ShowAt(Parts.FeatureR, FVector(10.f, 34.f, 130.f), FRotator(25.f, 0.f, 35.f), FVector(0.24f, 0.24f, 0.9f));
		ShowAt(Parts.FeatureExtra, FVector(45.f, 0.f, 88.f), FRotator(0.f, 0.f, 0.f), FVector(0.7f, 0.45f, 0.35f));
		InOutBodyTint = FLinearColor(
			FMath::Min(1.f, InOutBodyTint.R * 0.55f + 0.35f),
			FMath::Min(1.f, InOutBodyTint.G * 0.45f + 0.22f),
			FMath::Min(1.f, InOutBodyTint.B * 0.35f + 0.08f));
	}
	else if (Race.Contains(TEXT("dwarf")))
	{
		CapsuleRadius = 48.f;
		CapsuleHalfHeight = 72.f;
		BodyLoc = FVector(0.f, 0.f, -28.f);
		BodyScale = FVector(1.12f, 1.12f, 1.25f);
		HeadLoc = FVector(0.f, 0.f, 48.f);
		HeadScale = FVector(0.64f, 0.64f, 0.58f);
		ShowAt(Parts.FeatureExtra, FVector(20.f, 0.f, 28.f), FRotator(0.f, 0.f, 0.f), FVector(0.45f, 0.7f, 0.5f)); // beard slab
		InOutBodyTint = FLinearColor(
			FMath::Min(1.f, InOutBodyTint.R * 0.7f + 0.2f),
			FMath::Min(1.f, InOutBodyTint.G * 0.65f + 0.15f),
			FMath::Min(1.f, InOutBodyTint.B * 0.55f + 0.1f));
	}
	else if (Race.Contains(TEXT("high elf")) || Race.Contains(TEXT("highelf")))
	{
		// Former Aelfborn silhouette — hybrid ears, mid height.
		CapsuleRadius = 40.f;
		CapsuleHalfHeight = 98.f;
		BodyLoc = FVector(0.f, 0.f, -18.f);
		BodyScale = FVector(0.62f, 0.62f, 1.7f);
		HeadLoc = FVector(0.f, 0.f, 85.f);
		HeadScale = FVector(0.42f, 0.42f, 0.42f);
		ShowAt(Parts.FeatureL, FVector(0.f, -22.f, 92.f), FRotator(0.f, 0.f, -40.f), FVector(0.1f, 0.22f, 0.38f));
		ShowAt(Parts.FeatureR, FVector(0.f, 22.f, 92.f), FRotator(0.f, 0.f, 40.f), FVector(0.1f, 0.22f, 0.38f));
	}
	else if (Race.Contains(TEXT("elf")))
	{
		CapsuleRadius = 38.f;
		CapsuleHalfHeight = 105.f;
		BodyLoc = FVector(0.f, 0.f, -15.f);
		BodyScale = FVector(0.62f, 0.62f, 2.05f);
		HeadLoc = FVector(0.f, 0.f, 95.f);
		HeadScale = FVector(0.4f, 0.4f, 0.42f);
		ShowAt(Parts.FeatureL, FVector(0.f, -26.f, 102.f), FRotator(0.f, 0.f, -50.f), FVector(0.11f, 0.28f, 0.48f));
		ShowAt(Parts.FeatureR, FVector(0.f, 26.f, 102.f), FRotator(0.f, 0.f, 50.f), FVector(0.11f, 0.28f, 0.48f));
		InOutBodyTint = FLinearColor(
			FMath::Min(1.f, InOutBodyTint.R * 0.85f + 0.05f),
			FMath::Min(1.f, InOutBodyTint.G * 0.9f + 0.1f),
			FMath::Min(1.f, InOutBodyTint.B * 0.75f + 0.15f));
	}
	else if (Race.Contains(TEXT("nightshade")))
	{
		CapsuleRadius = 40.f;
		CapsuleHalfHeight = 96.f;
		BodyLoc = FVector(0.f, 0.f, -20.f);
		BodyScale = FVector(0.6f, 0.6f, 1.65f);
		HeadLoc = FVector(0.f, 0.f, 78.f);
		HeadScale = FVector(0.4f, 0.4f, 0.4f);
		ShowAt(Parts.FeatureExtra, FVector(0.f, 0.f, 98.f), FRotator(180.f, 0.f, 0.f), FVector(0.7f, 0.7f, 0.58f)); // hood
		InOutBodyTint = FLinearColor(
			InOutBodyTint.R * 0.25f + 0.12f,
			InOutBodyTint.G * 0.2f + 0.05f,
			InOutBodyTint.B * 0.45f + 0.28f);
	}
	else if (Race.Contains(TEXT("accipitridae")) || Race.Contains(TEXT("aracoix")))
	{
		CapsuleRadius = 40.f;
		CapsuleHalfHeight = 100.f;
		BodyScale = FVector(0.65f, 0.65f, 1.75f);
		ShowAt(Parts.FeatureL, FVector(-8.f, -40.f, 70.f), FRotator(0.f, 0.f, -20.f), FVector(0.2f, 0.55f, 0.15f));
		ShowAt(Parts.FeatureR, FVector(-8.f, 40.f, 70.f), FRotator(0.f, 0.f, 20.f), FVector(0.2f, 0.55f, 0.15f));
	}
	else if (Race.Contains(TEXT("badawian")) || Race.Contains(TEXT("irekei")))
	{
		CapsuleRadius = 39.f;
		CapsuleHalfHeight = 100.f;
		BodyScale = FVector(0.64f, 0.64f, 1.85f);
		InOutBodyTint = FLinearColor(
			FMath::Min(1.f, InOutBodyTint.R * 0.7f + 0.35f),
			FMath::Min(1.f, InOutBodyTint.G * 0.45f + 0.12f),
			FMath::Min(1.f, InOutBodyTint.B * 0.35f + 0.05f));
	}
	else if (Race.Contains(TEXT("shedim")) || Race.Contains(TEXT("nephilim")))
	{
		CapsuleRadius = 48.f;
		CapsuleHalfHeight = 110.f;
		BodyScale = FVector(0.85f, 0.8f, 2.1f);
		HeadScale = FVector(0.5f, 0.5f, 0.5f);
	}
	else if (Race.Contains(TEXT("centaur")))
	{
		// Interim: tall humanoid + elongated rear slab until a real centaur mesh lands.
		CapsuleRadius = 48.f;
		CapsuleHalfHeight = 100.f;
		BodyScale = FVector(0.75f, 0.7f, 1.5f);
		ShowAt(Parts.FeatureExtra, FVector(-55.f, 0.f, -40.f), FRotator(0.f, 0.f, 0.f), FVector(1.2f, 0.55f, 0.7f));
		InOutBodyTint = FLinearColor(
			FMath::Min(1.f, InOutBodyTint.R * 0.6f + 0.3f),
			FMath::Min(1.f, InOutBodyTint.G * 0.55f + 0.2f),
			FMath::Min(1.f, InOutBodyTint.B * 0.4f + 0.1f));
	}
	// else Human defaults

	if (Parts.Body)
	{
		Parts.Body->SetRelativeLocation(BodyLoc);
		Parts.Body->SetRelativeScale3D(BodyScale);
		Parts.Body->SetHiddenInGame(false);
	}
	if (Parts.Head)
	{
		ShowAt(Parts.Head, HeadLoc, FRotator::ZeroRotator, HeadScale);
	}

	if (Parts.Capsule)
	{
		Parts.Capsule->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight, true);
	}
}
