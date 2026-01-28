// Copyright (c) 2026 R&B. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "AuroraSplineComponent.generated.h"

/**
 *
 */
UCLASS(ClassGroup = (Aurora), meta = (BlueprintSpawnableComponent))
class VOLUMETRICAURORAEDITOR_API UAuroraSplineComponent : public USplineComponent
{
	GENERATED_BODY()

public:

	UAuroraSplineComponent();
protected:

	virtual void PostEditComponentMove(bool bFinished) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};
