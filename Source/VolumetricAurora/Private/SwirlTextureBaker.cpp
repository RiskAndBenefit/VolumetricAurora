// Fill out your copyright notice in the Description page of Project Settings.
#include "SwirlTextureBaker.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureResource.h"
#include "RHIStaticStates.h"  // TStaticSamplerState 사용에 필수

#include "NoiseBakeCS.h"
#include "SimplexNoiseBakeCS.h"
#include "DistanceMapBakeCS.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "ShaderParameterUtils.h"
#include "RHI.h"
#include "RHIResources.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"
#include "HAL/FileManager.h"
#endif

namespace
{
	static FRDGTextureRef CreateUAVIntermediateTexture(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef ReferenceTexture,
		const TCHAR* DebugName)
	{
		const FRDGTextureDesc& RefDesc = ReferenceTexture->Desc;

		FRDGTextureDesc UAVDesc = FRDGTextureDesc::Create2D(
			RefDesc.Extent,
			RefDesc.Format,
			RefDesc.ClearValue,
			ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV
		);

		UAVDesc.NumMips = RefDesc.NumMips;
		UAVDesc.NumSamples = RefDesc.NumSamples;
		UAVDesc.ArraySize = RefDesc.ArraySize;

		return GraphBuilder.CreateTexture(UAVDesc, DebugName);
	}
}

void USwirlTextureBaker::RunComputeShaderOnRenderTarget(
	UTextureRenderTarget2D* RenderTarget,
	UTexture2D* InputTexture,
	float SwirlStrength,
	float SwirlRadius,
	FVector2D SwirlCenter)
{
	if (!RenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("RenderTarget is null!"));
		return;
	}

	if (!InputTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("InputTexture is null!"));
		return;
	}

	FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		UE_LOG(LogTemp, Error, TEXT("RenderTarget resource is null!"));
		return;
	}

	FTextureResource* InputResource = InputTexture->GetResource();
	if (!InputResource)
	{
		UE_LOG(LogTemp, Error, TEXT("InputTexture resource is null!"));
		return;
	}

	ENQUEUE_RENDER_COMMAND(RunSwirlComputeShader)(
		[RTResource, InputResource, SwirlStrength, SwirlRadius, SwirlCenter](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			// 1) External Output (RenderTarget) 등록 - UAV로 직접 쓰지 않음(안전)
			FRDGTextureRef ExternalOutputTexture = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(RTResource->GetRenderTargetTexture(), TEXT("SwirlOutputRT"))
			);

			// 2) UAV 가능한 임시 텍스처 생성 (여기에 Compute가 씀)
			FRDGTextureRef OutputUAVTexture = CreateUAVIntermediateTexture(
				GraphBuilder,
				ExternalOutputTexture,
				TEXT("SwirlOutput_UAV")
			);
			FRDGTextureUAVRef OutputUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputUAVTexture));

			// 3) Input 텍스처 등록 + SRV
			FRDGTextureRef InputRDGTexture = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(InputResource->TextureRHI, TEXT("SwirlInputTexture"))
			);
			FRDGTextureSRVRef InputSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputRDGTexture));

			// 4) Parameters
			FNoiseBakeCS::FParameters* PassParameters =
				GraphBuilder.AllocParameters<FNoiseBakeCS::FParameters>();
			PassParameters->RenderTarget = OutputUAV;
			PassParameters->InputNoiseTex = InputSRV;
			PassParameters->InputNoiseSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
			PassParameters->SwirlStrength = SwirlStrength;
			PassParameters->SwirlRadius = SwirlRadius;
			PassParameters->SwirlCenter = FVector2f(SwirlCenter.X, SwirlCenter.Y);

			TShaderMapRef<FNoiseBakeCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

			FIntVector GroupCount(
				(RTResource->GetSizeX() + 31) / 32,
				(RTResource->GetSizeY() + 31) / 32,
				1
			);

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("SwirlNoiseComputeShader"),
				ComputeShader,
				PassParameters,
				GroupCount
			);

			// 5) Compute 결과를 실제 RenderTarget로 복사
			{
				FRHICopyTextureInfo CopyInfo;
				AddCopyTexturePass(GraphBuilder, OutputUAVTexture, ExternalOutputTexture, CopyInfo);
			}

			GraphBuilder.Execute();
		}
		);

	UE_LOG(LogTemp, Warning, TEXT("Swirl Compute shader executed! Strength: %f, Radius: %f"), SwirlStrength, SwirlRadius);
}

void USwirlTextureBaker::BakeNoiseToRenderTarget(
	UTextureRenderTarget2D* RenderTarget,
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
	bool bIsDual)
{
	// 1) Validation
	if (!RenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("RenderTarget is null!"));
		return;
	}

	FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		UE_LOG(LogTemp, Error, TEXT("RenderTarget resource is null!"));
		return;
	}

	// 2) Render Thread
	ENQUEUE_RENDER_COMMAND(BakeSimplexNoise)(
		[
			RTResource,
			NoiseType,
			NoiseScale,
			Octaves,
			Persistence,
			Lacunarity,
			Offset,
			Time,
			Seed,
			Amplitude,
			Contrast,
			BaseFrequency,
			AnimDir,
			bIsDual
		]
	(FRHICommandListImmediate& RHICmdList)
	{
		FRDGBuilder GraphBuilder(RHICmdList);

		// 3) External Output (RenderTarget) 등록 - UAV로 직접 쓰지 않음(안전)
		FRDGTextureRef ExternalOutputTexture = GraphBuilder.RegisterExternalTexture(
			CreateRenderTarget(RTResource->GetRenderTargetTexture(), TEXT("SimplexNoiseOutput"))
		);

		// 4) UAV 가능한 임시 텍스처 생성 (여기에 Compute가 씀)
		FRDGTextureRef OutputUAVTexture = CreateUAVIntermediateTexture(
			GraphBuilder,
			ExternalOutputTexture,
			TEXT("SimplexNoiseOutput_UAV")
		);
		FRDGTextureUAVRef OutputUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputUAVTexture));

		// 5) Parameters
		FSimplexNoiseBakeCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FSimplexNoiseBakeCS::FParameters>();
		PassParameters->OutTexture = OutputUAV;
		PassParameters->NoiseType = static_cast<int32>(NoiseType);
		PassParameters->NoiseScale = NoiseScale;
		PassParameters->Octaves = Octaves;
		PassParameters->Persistence = Persistence;
		PassParameters->Lacunarity = Lacunarity;
		PassParameters->Offset = FVector2f(Offset.X, Offset.Y);
		PassParameters->Time = Time;
		PassParameters->Seed = Seed;
		PassParameters->Amplitude = Amplitude;
		PassParameters->Contrast = Contrast;
		PassParameters->BaseFrequency = BaseFrequency;
		PassParameters->AnimDir = FVector2f(AnimDir.X, AnimDir.Y);
		PassParameters->IsDual = (bIsDual ? 1 : 0);

		// 6) Shader + GroupCount
		TShaderMapRef<FSimplexNoiseBakeCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		FIntVector GroupCount(
			FMath::DivideAndRoundUp(RTResource->GetSizeX(), (uint32)8),
			FMath::DivideAndRoundUp(RTResource->GetSizeY(), (uint32)8),
			1
		);

		// 7) Compute Pass
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("BakeSimplexNoise"),
			ComputeShader,
			PassParameters,
			GroupCount
		);

		// 8) Compute 결과를 실제 RenderTarget로 복사
		{
			FRHICopyTextureInfo CopyInfo;
			AddCopyTexturePass(GraphBuilder, OutputUAVTexture, ExternalOutputTexture, CopyInfo);
		}

		GraphBuilder.Execute();
	}
		);
}

void USwirlTextureBaker::CreateNewNoise(
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
	bool bIsDual)
{
#if WITH_EDITOR
	// 1) 렌더타겟 생성 (임시)
	const int32 Width = 1024;
	const int32 Height = 1024;

	UTextureRenderTarget2D* TempRT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!TempRT)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateNewNoise: Failed to create TempRT."));
		return;
	}

	TempRT->SRGB = false;
	TempRT->ClearColor = FLinearColor::Black;
	TempRT->bAutoGenerateMips = false;

	// Unordered Access View의 약자입니다. 쉽게 말해서 GPU가 텍스처/버퍼에 "랜덤 위치에 쓰기(write)" 할 수 있게 만들어주는 쓰기용 뷰입니다.
	TempRT->bSupportsUAV = true;

	TempRT->InitCustomFormat(Width, Height, PF_R32_FLOAT, true);
	TempRT->UpdateResourceImmediate(true);

	// 2) 노이즈 생성(Compute)
	BakeNoiseToRenderTarget(
		TempRT,
		NoiseType,
		NoiseScale,
		Octaves,
		Persistence,
		Lacunarity,
		Offset,
		Time,
		Seed,
		Amplitude,
		Contrast,
		BaseFrequency,
		AnimDir,
		bIsDual
	);

	// 3) 렌더스레드 작업 완료 대기
	FRenderCommandFence Fence;
	Fence.BeginFence();
	Fence.Wait();

	// 4) 에셋 저장
	const FString PackagePath = TEXT("/Game/Generated");	// 경로 하드코딩
	const FString Guid = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString TypeName = StaticEnum<EVANoiseType>()->GetNameStringByValue((int64)NoiseType);
	const FString AssetName = FString::Printf(TEXT("Noise_%s_%s"), *TypeName, *Guid);

	UTexture2D* Saved = SaveRenderTargetAsTextureAsset(
		TempRT,
		PackagePath,
		AssetName,
		false
	);

	if (!Saved)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateNewNoise: Save failed."));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("CreateNewNoise: Saved %s/%s"), *PackagePath, *AssetName);
#else
	UE_LOG(LogTemp, Warning, TEXT("CreateNewNoise: Editor-only function (WITH_EDITOR)."));
#endif
}

#if WITH_EDITOR
UTexture2D* USwirlTextureBaker::SaveRenderTargetAsTextureAsset(
	UTextureRenderTarget2D* RenderTarget,
	const FString& PackagePath,
	const FString& AssetName,
	bool bOverwriteIfExists)
{
	if (!RenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveRenderTargetAsTextureAsset: RenderTarget is null."));
		return nullptr;
	}

	if (PackagePath.IsEmpty() || AssetName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("SaveRenderTargetAsTextureAsset: PackagePath/AssetName is empty."));
		return nullptr;
	}

	FString SanitizedPackagePath = PackagePath;
	if (!SanitizedPackagePath.StartsWith(TEXT("/Game")))
	{
		UE_LOG(LogTemp, Error, TEXT("SaveRenderTargetAsTextureAsset: PackagePath must start with /Game. Input: %s"), *PackagePath);
		return nullptr;
	}

	const FString LongPackageName = SanitizedPackagePath / AssetName;

	{
		const FString ExistingPackageFile = FPackageName::LongPackageNameToFilename(LongPackageName, FPackageName::GetAssetPackageExtension());
		if (IFileManager::Get().FileExists(*ExistingPackageFile))
		{
			if (!bOverwriteIfExists)
			{
				UE_LOG(LogTemp, Error, TEXT("SaveRenderTargetAsTextureAsset: Asset already exists: %s"), *LongPackageName);
				return nullptr;
			}
		}
	}

	// 1) 이 RenderTarget이 Texture2D로 변환 가능한지 + 어떤 포맷으로 가는지 확인
	ETextureSourceFormat OutSourceFormat = TSF_Invalid;
	EPixelFormat OutPixelFormat = PF_Unknown;
	FText ErrorMessage;

	if (!RenderTarget->CanConvertToTexture(OutSourceFormat, OutPixelFormat, &ErrorMessage))
	{
		UE_LOG(LogTemp, Error, TEXT("SaveRenderTargetAsTextureAsset: CanConvertToTexture failed: %s"), *ErrorMessage.ToString());
		return nullptr;
	}

	// PF_R16F RenderTarget이면 보통 TSF_R16F / PF_R16F 쪽이 기대값
	if (OutSourceFormat != TSF_R32F)
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveRenderTargetAsTextureAsset: SourceFormat is not TSF_R16F (got %d). Result may not be R16F."), (int32)OutSourceFormat);
	}
	if (OutPixelFormat != TSF_R32F)
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveRenderTargetAsTextureAsset: PixelFormat is not PF_R16F (got %d). Result may not be R16F."), (int32)OutPixelFormat);
	}

	// 2) 패키지 생성
	UPackage* Package = CreatePackage(*LongPackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveRenderTargetAsTextureAsset: Failed to create package: %s"), *LongPackageName);
		return nullptr;
	}

	Package->FullyLoad();

	// 3) Texture2D 에셋 생성 (빈 텍스처)
	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);
	if (!NewTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveRenderTargetAsTextureAsset: Failed to create UTexture2D."));
		return nullptr;
	}

	// 4) RenderTarget -> Texture2D로 픽셀 복사/변환 (핵심)
	// UpdateTexture2D가 Source를 채워주는 경로를 타게 합니다.
	RenderTarget->UpdateTexture(
		NewTexture,
		CTF_Default
	);

	// 5) 에셋 설정: float 유지 목적이면 Grayscale 압축은 피함
	NewTexture->SRGB = false;
	NewTexture->MipGenSettings = TMGS_NoMipmaps;
	NewTexture->CompressionSettings = TC_SingleFloat;
	NewTexture->CompressionNoAlpha = true;
	NewTexture->NeverStream = true;

#if WITH_EDITOR
	NewTexture->PostEditChange();
#endif
	NewTexture->UpdateResource();

	// 6) AssetRegistry 등록 + 저장
	FAssetRegistryModule::AssetCreated(NewTexture);
	Package->MarkPackageDirty();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(
		LongPackageName,
		FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_None;
	SaveArgs.Error = GError;

	const bool bSaved = UPackage::SavePackage(
		Package,
		NewTexture,
		*PackageFileName,
		SaveArgs
	);

	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveRenderTargetAsTextureAsset: SavePackage failed: %s"), *PackageFileName);
		return nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("Saved Texture Asset: %s"), *LongPackageName);
	return NewTexture;
}
#endif
