// Copyright (c) 2026 R&B. All rights reserved.

///**
// * AuroraFlowSimulator.h
// * Main aurora flow simulation actor
// *
// * Provides real-time aurora particle simulation using compute shaders:
// * - Semi-Lagrangian advection for stable particle transport
// * - Multiple control point types (gravity, vortex, spiral, curl)
// * - Obstacle system with distance field-based deflection
// * - Aurora element-based emission/extinction control
// * - Frame-rate independent simulation via DeltaTime
// *
// * Architecture:
// * - Double-buffered render targets for ping-pong advection
// * - GPU-accelerated distance field baking (Jump Flooding Algorithm)
// * - Dynamic material preview system
// */
//
//#pragma once
//
//#include "CoreMinimal.h"
//#include "GameFramework/Actor.h"
//#include "FlowElement.h"
//#include "AuroraFlowSimulateCS.h"
//#include "AuroraFlowSimulator.generated.h"
//
//class UStaticMeshComponent;
//class UMaterialInterface;
//class UMaterialInstanceDynamic;
//
///**
// * Texture resolution options for simulation
// * Higher resolutions provide more detail but increase GPU cost
// */
//UENUM(BlueprintType)
//enum class ETextureResolution : uint8
//{
//	Res512		UMETA(DisplayName = "512"),
//	Res1024		UMETA(DisplayName = "1024"),
//	Res2048		UMETA(DisplayName = "2048"),
//	Res4096		UMETA(DisplayName = "4096"),
//	Res8192		UMETA(DisplayName = "8192")
//};
//
///**
// * Convert ETextureResolution enum to integer pixel dimension
// * @param Resolution Enum resolution value
// * @return Pixel dimension (e.g., 512, 1024, 2048)
// */
//inline int32 GetResolutionValue(ETextureResolution Resolution)
//{
//	switch (Resolution)
//	{
//	case ETextureResolution::Res512:  return 512;
//	case ETextureResolution::Res1024: return 1024;
//	case ETextureResolution::Res2048: return 2048;
//	case ETextureResolution::Res4096: return 4096;
//	case ETextureResolution::Res8192: return 8192;
//	default: return 512;
//	}
//}
//
///**
// * Aurora flow simulation actor
// * Manages compute shader-based particle simulation with preview visualization
// */
//UCLASS()
//class VOLUMETRICAURORA_API AAuroraFlowSimulator : public AActor
//{
//	GENERATED_BODY()
//
//public:
//	AAuroraFlowSimulator();
//
//protected:
//	virtual void BeginPlay() override;
//
//public:
//	virtual void Tick(float DeltaTime) override;
//	virtual bool ShouldTickIfViewportsOnly() const override;
//
//#if WITH_EDITOR
//	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
//#endif
//
//	/**
//	 * Mark control points as dirty for GPU buffer update
//	 * Call this after modifying ControlPoints array at runtime
//	 */
//	void MarkControlPointsDirty() { bControlPointsDirty = true; }
//
//	// ========================================================================
//	// Render Targets
//	// ========================================================================
//
//	/** Ping-pong buffer: write target for current simulation step */
//	UPROPERTY(VisibleAnywhere, Category = "Aurora|Debug")
//	UTextureRenderTarget2D* FrontBuffer = nullptr;
//
//	/** Ping-pong buffer: read source for current simulation step */
//	UPROPERTY(VisibleAnywhere, Category = "Aurora|Debug")
//	UTextureRenderTarget2D* BackBuffer = nullptr;
//
//	/** Baked obstacle distance field (RGBA: Normal.xy, Distance, Mask) */
//	UPROPERTY(VisibleAnywhere, Category = "Aurora|Debug")
//	UTextureRenderTarget2D* ObstacleMap = nullptr;
//
//	/** Display buffer for external use (snapshot before ping-pong swap) */
//	UPROPERTY(VisibleAnywhere, Category = "Aurora|Debug")
//	UTextureRenderTarget2D* DisplayBuffer = nullptr;
//
//	// ========================================================================
//	// Preview Visualization
//	// ========================================================================
//
//	/** Static mesh component for in-editor preview */
//	UPROPERTY(VisibleAnywhere, Category = "Aurora|Preview")
//	UStaticMeshComponent* PreviewPlane;
//
//	/** Material to use for preview rendering */
//	UPROPERTY(EditAnywhere, Category = "Aurora|Preview")
//	UMaterialInterface* PreviewMaterial;
//
//	// ========================================================================
//	// Simulation Parameters
//	// ========================================================================
//
//	/** Simulation texture resolution */
//	UPROPERTY(EditAnywhere, Category = "Aurora|Flow")
//	ETextureResolution SimulationResolution = ETextureResolution::Res512;
//
//	/** Velocity threshold for particle extinction (particles fade when velocity < threshold) */
//	UPROPERTY(EditAnywhere, Category = "Aurora|Flow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
//	float VelocityThreshold = 0.01f;
//
//	/** Base flow direction and speed in UV space */
//	UPROPERTY(EditAnywhere, Category = "Aurora|Flow")
//	FVector2D BaseFlow = FVector2D(0.1, 0.0);
//
//	// ========================================================================
//	// Control Points
//	// ========================================================================
//
//	/** Array of flow field control points (gravity, vortex, spiral, curl) */
//	UPROPERTY(EditAnywhere, Category = "Aurora|ControlPoints", meta = (TitleProperty = "Type"))
//	TArray<FFlowElement> ControlPoints;
//
//	// ========================================================================
//	// Obstacle System
//	// ========================================================================
//
//	/** Obstacle influence radius in UV space [0, 1] */
//	UPROPERTY(EditAnywhere, Category = "Aurora|Obstacles", meta = (ClampMin = "0.0", ClampMax = "1.0"))
//	float ObstacleInfluenceRadius = 0.05f;
//
//	// ========================================================================
//	// Aurora Elements
//	// ========================================================================
//
//	/** Aurora elements map (R=Emitter, G=Extinction zones) */
//	UPROPERTY(EditAnywhere, Category = "Aurora|Emitters")
//	UTexture* AuroraElementsMap = nullptr;
//
//private:
//	/** Execute aurora flow simulation compute shader pass */
//	void SimulateAuroraPass(float DeltaTime);
//
//	/** Bake obstacle distance field using Jump Flooding Algorithm */
//	void BakeDistanceMapToRenderTarget();
//
//	/** Dynamic material instance for preview rendering */
//	UPROPERTY()
//	UMaterialInstanceDynamic* DynamicMaterial = nullptr;
//
//	/** Cached GPU-ready control point data */
//	TArray<FControlPointGPU> CachedGPUData;
//
//	/** Flag indicating control points need GPU buffer update */
//	bool bControlPointsDirty = true;
//
//	/** Flag indicating aurora elements map needs update */
//	bool bAuroraElementsMapDirty = true;
//
//	/** Track previous aurora elements map to detect content changes */
//	UPROPERTY()
//	UTexture* PreviousAuroraElementsMap = nullptr;
//
//	/** Previous aurora elements resource pointer for change detection */
//	FTextureResource* PreviousAuroraElementsResource = nullptr;
//};
