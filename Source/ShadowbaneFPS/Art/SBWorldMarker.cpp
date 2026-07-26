// Copyright shadowbanefps.

#include "SBWorldMarker.h"
#include "SBPlaceholderArt.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ASBWorldMarker::ASBWorldMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	SetRootComponent(Disc);
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.12f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Disc->SetStaticMesh(Cylinder.Object);
	}

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	LabelText->SetupAttachment(RootComponent);
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextCenter);
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	LabelText->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	LabelText->SetWorldSize(48.f);
	LabelText->SetTextRenderColor(FColor::White);
	LabelText->SetText(FText::FromString(TEXT("MARK")));
}

void ASBWorldMarker::BeginPlay()
{
	Super::BeginPlay();
}

void ASBWorldMarker::Configure(const FString& Label, FLinearColor Color, float Scale)
{
	SetActorScale3D(FVector(Scale));
	LabelText->SetText(FText::FromString(Label));
	USBPlaceholderArt::ApplySolidColor(Disc, Color);
}

void ASBWorldMarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Billboard the label toward the local player camera.
	if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (const APlayerCameraManager* Cam = PC->PlayerCameraManager)
		{
			const FVector ToCam = Cam->GetCameraLocation() - LabelText->GetComponentLocation();
			const FRotator Look = ToCam.Rotation();
			LabelText->SetWorldRotation(FRotator(0.f, Look.Yaw + 180.f, 0.f));
		}
	}

	// Gentle bob so markers read as "alive" placeholder FX.
	const float Bob = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.5f) * 8.f;
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, 80.f + Bob));
}
