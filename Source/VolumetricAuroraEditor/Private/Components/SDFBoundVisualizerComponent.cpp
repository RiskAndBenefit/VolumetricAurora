// Copyright (c) 2026 R&B. All rights reserved.

#include "Components/SDFBoundVisualizerComponent.h"
#include "Components/SplineSDFTextureBakerComponent.h"

// Sets default values for this component's properties
USDFBoundVisualizerComponent::USDFBoundVisualizerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	bIsEditorOnly = true;



	// ...
}

// Called when the game starts
void USDFBoundVisualizerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void USDFBoundVisualizerComponent::PostEditComponentMove(bool bFinished)
{
	Super::PostEditComponentMove(bFinished);
	if (USplineSDFTextureBakerComponent* Parent = Cast<USplineSDFTextureBakerComponent>(GetAttachParent()))
	{
		Parent->SetBoundsVisualizerTransform();
	}
}

void USDFBoundVisualizerComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (USplineSDFTextureBakerComponent* Parent = Cast<USplineSDFTextureBakerComponent>(GetAttachParent()))
	{
		Parent->SetBoundsVisualizerTransform();
	}
}


// Called every frame
void USDFBoundVisualizerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
