// Copyright (c) 2026 R&B. All rights reserved.

#pragma once


#include "CoreMinimal.h"
#include "Components/DFBakerComponentBase.h"
#include "Components/SceneComponent.h"
#include "Types/AuroraTypes.h"
#include "SplineDFTextureBakerComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), HideCategories = (Rendering, Tags, Physics, LOD, Navigation, Cooking, Activation, AssetUserData))
class VOLUMETRICAURORAEDITOR_API USplineDFTextureBakerComponent : public UDFBakerComponentBase
{
	GENERATED_BODY()

public:	

	USplineDFTextureBakerComponent();
	
	UPROPERTY()
	UStaticMeshComponent* BoundsVisualizer;

	UPROPERTY(EditAnywhere, Category = "DFBaker")
	bool bAlwaysShowDebugLine = false;

	// Sets default values for this component's properties

	float MapSize = 50000.0f;

	void FocusOnVisualizer();

	void AddSplineComponent();

	void MakeDFTexture(const FString& NewTextureName);

	void SetBoundsVisualizerTransform();

	void DispatchDFBakeCS(const TArray<FVector4f>& SplinePoints, const TArray<FAuroraSplineInfo>& SplineInfos, const FString& NewTextureName);

	void UpdateVisualizerState();

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
