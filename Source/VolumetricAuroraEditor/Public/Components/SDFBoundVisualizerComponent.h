// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "SDFBoundVisualizerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VOLUMETRICAURORAEDITOR_API USDFBoundVisualizerComponent : public UStaticMeshComponent
{
	GENERATED_BODY()
public:	
	// Sets default values for this component's properties
	USDFBoundVisualizerComponent();



protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void PostEditComponentMove(bool bFinished) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	
};
