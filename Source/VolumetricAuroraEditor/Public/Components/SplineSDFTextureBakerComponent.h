// Copyright (c) 2026 R&B. All rights reserved.

#pragma once


#include "CoreMinimal.h"
#include "Components/SDFBakerComponentBase.h"
#include "Components/SceneComponent.h"
#include "Types/AuroraTypes.h"
#include "SplineSDFTextureBakerComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), HideCategories = (Rendering, Tags, Physics, LOD, Navigation, Cooking, Activation, AssetUserData))
class VOLUMETRICAURORAEDITOR_API USplineSDFTextureBakerComponent : public USDFBakerComponentBase
{
	GENERATED_BODY()

public:	

	USplineSDFTextureBakerComponent();
	
	UPROPERTY()
	UStaticMeshComponent* BoundsVisualizer;


	// Sets default values for this component's properties

	float MapSize = 50000.0f;


	void AddSplineComponent();

	void MakeSDFTexture(const FString& NewTextureName);

	void SetBoundsVisualizerTransform();

	void DispatchSDFBakeCS(const TArray<FVector4f>& SplinePoints, const TArray<FAuroraSplineInfo>& SplineInfos, const FString& NewTextureName);

	void OnSelectionChanged(UObject* Selected);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void OnRegister() override;

	virtual void OnUnregister() override;

	virtual void PostEditComponentMove(bool bFinished) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


};
