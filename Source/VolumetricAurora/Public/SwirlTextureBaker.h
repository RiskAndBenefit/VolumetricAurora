// Copyright (c) 2026 R&B. All rights reserved.

// SwirlTextureBaker.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "NoiseTypes.h"
#include "SwirlTextureBaker.generated.h"

/*
 * Utility class that applies swirl distortion to input textures using compute shaders.
 */

UCLASS()
class VOLUMETRICAURORA_API USwirlTextureBaker : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Texture|Swirl")
	static void RunComputeShaderOnRenderTarget(
		UTextureRenderTarget2D* RenderTarget,
		UTexture2D* InputTexture,
		float SwirlStrength = 1.0f,
		float SwirlRadius = 0.5f,
		FVector2D SwirlCenter = FVector2D(0.5f, 0.5f)
	);

	UFUNCTION(BlueprintCallable, Category = "Texture|Noise")
	static void BakeNoiseToRenderTarget(
		UTextureRenderTarget2D* RenderTarget,
		EVANoiseType NoiseType = EVANoiseType::Simplex,
		float NoiseScale = 1.0f,
		int32 Octaves = 4,
		float Persistence = 0.5f,
		float Lacunarity = 2.0f,
		FVector2D Offset = FVector2D::ZeroVector,
		float Time = 0.0f,
		float Seed = 0.0f,
		float Amplitude = 1.0f,
		float Contrast = 1.0f,
		UPARAM(DisplayName = "Base Frequency (Curl Only)", meta = (ToolTip = "Curl Noise 전용: 기본 주파수"))
		float BaseFrequency = 1.0f,
		UPARAM(DisplayName = "Anim Dir (Curl Only)", meta = (ToolTip = "Curl Noise 전용: 애니메이션 방향"))
		FVector2D AnimDir = FVector2D(0.1f, 0.15f),
		bool bIsDual = false
	);

	UFUNCTION(BlueprintCallable, Category = "Swirl|Bake")
	static void CreateNewNoise(
		EVANoiseType NoiseType,
		float NoiseScale,
		int32 Octaves,
		float Persistence,
		float Lacunarity,
		FVector2D Offset,
		float Time,
		float Seed,
		float Amplitude,
		float Contrast,
		float BaseFrequency,
		FVector2D AnimDir,
		bool bIsDual);

#if WITH_EDITOR
	// RenderTarget을 Texture2D 에셋으로 생성하고 Content Browser에 저장합니다.
	// PackagePath 예: "/Game/Generated"
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Swirl|Bake")
	static UTexture2D* SaveRenderTargetAsTextureAsset(
		UTextureRenderTarget2D* RenderTarget,
		const FString& PackagePath,
		const FString& AssetName,
		bool bOverwriteIfExists = false
	);
#endif
};