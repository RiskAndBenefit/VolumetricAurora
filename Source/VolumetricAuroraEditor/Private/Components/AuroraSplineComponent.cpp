// Copyright (c) 2026 R&B. All rights reserved.
#include "Components/AuroraSplineComponent.h"

UAuroraSplineComponent::UAuroraSplineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	bIsEditorOnly = true;
}

void UAuroraSplineComponent::PostEditComponentMove(bool bFinished)
{
	Super::PostEditComponentMove(bFinished);

	for (int Index = 0; Index < GetNumberOfSplinePoints(); Index++)
	{
		FVector Position = GetSplinePointAt(Index, ESplineCoordinateSpace::Local).Position;

		Position.Z = 0.0f;

		SetLocationAtSplinePoint(Index, Position, ESplineCoordinateSpace::Local);
	}
	FVector CompLocation = GetRelativeLocation();
	CompLocation.Z = 0.0f;

	SetRelativeLocation(CompLocation);
}

void UAuroraSplineComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	for (int Index = 0; Index < GetNumberOfSplinePoints(); Index++)
	{
		FVector Position = GetSplinePointAt(Index, ESplineCoordinateSpace::Local).Position;

		Position.Z = 0.0f;

		SetLocationAtSplinePoint(Index, Position, ESplineCoordinateSpace::Local);
	}
	FVector CompLocation = GetRelativeLocation();
	CompLocation.Z = 0.0f;

	SetRelativeLocation(CompLocation);
}
