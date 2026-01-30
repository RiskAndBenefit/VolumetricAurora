// Copyright (c) 2026 R&B. All rights reserved.

#include "Components/DFBoundVisualizerComponent.h"
#include "Components/SplineDFTextureBakerComponent.h"

// Sets default values for this component's properties
UDFBoundVisualizerComponent::UDFBoundVisualizerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	bIsEditorOnly = true;



	// ...
}

// Called when the game starts
void UDFBoundVisualizerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UDFBoundVisualizerComponent::PostEditComponentMove(bool bFinished)
{
	Super::PostEditComponentMove(bFinished);
	if (USplineDFTextureBakerComponent* Parent = Cast<USplineDFTextureBakerComponent>(GetAttachParent()))
	{
		Parent->SetBoundsVisualizerTransform();
	}
}

void UDFBoundVisualizerComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (USplineDFTextureBakerComponent* Parent = Cast<USplineDFTextureBakerComponent>(GetAttachParent()))
	{
		Parent->SetBoundsVisualizerTransform();
	}
}


// Called every frame
void UDFBoundVisualizerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
