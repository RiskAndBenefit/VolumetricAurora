// Copyright (c) 2026 R&B. All rights reserved.

///**
// * AuroraFlowSimulator.cpp
// * Main aurora flow simulation actor implementation
// *
// * Manages GPU-accelerated particle simulation using compute shaders:
// * - Double-buffered ping-pong advection
// * - Distance field baking via Jump Flooding Algorithm
// * - Dynamic material preview system
// * - Automatic resource management and dirty tracking
// */
//
//#include "AuroraFlowSimulator.h"
//#include "RenderGraphBuilder.h"
//#include "RenderGraphUtils.h"
//#include "ShaderParameterUtils.h"
//#include "Engine/TextureRenderTarget2D.h"
//#include "AuroraFlowSimulateCS.h"
//#include "DistanceMapBakeCS.h"
//#include "Components/StaticMeshComponent.h"
//#include "Materials/MaterialInstanceDynamic.h"
//#include "UObject/ConstructorHelpers.h"
//
//// GPU Stats for profiling
//DECLARE_GPU_STAT_NAMED(AuroraFlowSimulate, TEXT("AuroraFlowSimulate"));
//DECLARE_GPU_STAT_NAMED(AuroraDistanceMapBake, TEXT("AuroraDistanceMapBake"));
//
//AAuroraFlowSimulator::AAuroraFlowSimulator()
//{
//	PrimaryActorTick.bCanEverTick = true;
//	PrimaryActorTick.bTickEvenWhenPaused = true;
//
//	// Root component
//	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
//
//	// Preview plane
//	PreviewPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewPlane"));
//	PreviewPlane->SetupAttachment(RootComponent);
//
//	// Load default plane mesh
//	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
//		TEXT("/Engine/BasicShapes/Plane.Plane")
//	);
//	if (PlaneMesh.Succeeded())
//	{
//		PreviewPlane->SetStaticMesh(PlaneMesh.Object);
//		PreviewPlane->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
//		PreviewPlane->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
//	}
//}
//
//void AAuroraFlowSimulator::BeginPlay()
//{
//	Super::BeginPlay();
//}
//
//void AAuroraFlowSimulator::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//	// Calculate resolution value
//	const int32 SimRes = GetResolutionValue(SimulationResolution);
//
//	// Create render targets or recreate on resolution change
//	if (!FrontBuffer || !FrontBuffer->bCanCreateUAV || FrontBuffer->SizeX != SimRes)
//	{
//		FrontBuffer = NewObject<UTextureRenderTarget2D>(this);
//		FrontBuffer->bCanCreateUAV = true;
//		FrontBuffer->InitCustomFormat(SimRes, SimRes, PF_FloatRGBA, false);
//		FrontBuffer->UpdateResourceImmediate();
//	}
//	if (!BackBuffer || !BackBuffer->bCanCreateUAV || BackBuffer->SizeX != SimRes)
//	{
//		BackBuffer = NewObject<UTextureRenderTarget2D>(this);
//		BackBuffer->bCanCreateUAV = true;
//		BackBuffer->InitCustomFormat(SimRes, SimRes, PF_FloatRGBA, false);
//		BackBuffer->UpdateResourceImmediate();
//	}
//
//	if (!ObstacleMap || !ObstacleMap->bCanCreateUAV || ObstacleMap->SizeX != SimRes)
//	{
//		ObstacleMap = NewObject<UTextureRenderTarget2D>(this);
//		ObstacleMap->bCanCreateUAV = true;
//		ObstacleMap->InitCustomFormat(SimRes, SimRes, PF_FloatRGBA, false);
//		ObstacleMap->UpdateResourceImmediate();
//		bAuroraElementsMapDirty = true;
//	}
//
//	// Connect material on first run
//	if (!DynamicMaterial && PreviewMaterial && PreviewPlane)
//	{
//		DynamicMaterial = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
//		PreviewPlane->SetMaterial(0, DynamicMaterial);
//	}
//
//	// Detect AuroraElementsMap changes (file replacement or content modification)
//	if (AuroraElementsMap != PreviousAuroraElementsMap)
//	{
//		// Replaced with different texture (or changed to nullptr)
//		bAuroraElementsMapDirty = true;
//		PreviousAuroraElementsMap = AuroraElementsMap;
//		PreviousAuroraElementsResource = AuroraElementsMap ? AuroraElementsMap->GetResource() : nullptr;
//		UE_LOG(LogTemp, Warning, TEXT("AuroraElementsMap changed to new texture: %p"), AuroraElementsMap);
//	}
//	else if (AuroraElementsMap && AuroraElementsMap->GetResource())
//	{
//		// Same texture but check if resource was updated
//		// Texture reimport recreates resource, so detect that
//		FTextureResource* CurrentResource = AuroraElementsMap->GetResource();
//
//		if (PreviousAuroraElementsResource != CurrentResource)
//		{
//			bAuroraElementsMapDirty = true;
//			PreviousAuroraElementsResource = CurrentResource;
//			UE_LOG(LogTemp, Warning, TEXT("AuroraElementsMap resource updated (reimported)"));
//		}
//	}
//
//	// Bake distance map when AuroraElementsMap changes
//	if (bAuroraElementsMapDirty)
//	{
//		BakeDistanceMapToRenderTarget();
//		bAuroraElementsMapDirty = false;
//	}
//
//	SimulateAuroraPass(DeltaTime);
//
//	// Connect FrontBuffer to material
//	if (DynamicMaterial && FrontBuffer)
//	{
//		DynamicMaterial->SetTextureParameterValue(TEXT("Texture"), FrontBuffer);
//	}
//	DisplayBuffer = FrontBuffer;
//
//	// Swap buffers for next frame
//	Swap(FrontBuffer, BackBuffer);
//}
//
//bool AAuroraFlowSimulator::ShouldTickIfViewportsOnly() const
//{
//	return true;
//}
//
//#if WITH_EDITOR
//void AAuroraFlowSimulator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
//{
//	Super::PostEditChangeProperty(PropertyChangedEvent);
//
//	FName MemberName = PropertyChangedEvent.GetMemberPropertyName();
//	if (MemberName == GET_MEMBER_NAME_CHECKED(AAuroraFlowSimulator, ControlPoints))
//	{
//		bControlPointsDirty = true;
//	}
//	else if (MemberName == GET_MEMBER_NAME_CHECKED(AAuroraFlowSimulator, AuroraElementsMap))
//	{
//		bAuroraElementsMapDirty = true;
//	}
//}
//#endif
//
//void AAuroraFlowSimulator::SimulateAuroraPass(float DeltaTime)
//{
//	// 1. Validation
//	if (!FrontBuffer || !BackBuffer)
//	{
//		UE_LOG(LogTemp, Error, TEXT("RenderTarget is null."));
//		return;
//	}
//
//	// CPU to GPU structure conversion
//	if (bControlPointsDirty)
//	{
//		CachedGPUData.Reset();
//
//		for (const FFlowElement& CP : ControlPoints)
//		{
//			FControlPointGPU GPU = {};
//			GPU.Position = FVector2f(CP.Position);
//			GPU.Type = static_cast<uint32>(CP.Type);
//
//			switch (CP.Type)
//			{
//			case EControlPointType::Gravity:
//				GPU.GravityStrength = CP.GravityStrength;
//				break;
//			case EControlPointType::Vortex:
//				GPU.CirculationStrength = CP.CirculationStrength;
//				break;
//			case EControlPointType::Spiral:
//				GPU.GravityStrength = CP.GravityStrength;
//				GPU.CirculationStrength = CP.CirculationStrength;
//				break;
//			case EControlPointType::Curl:
//				GPU.CurlFrequency = CP.CurlFrequency;
//				GPU.CurlAnimSpeed = CP.CurlAnimSpeed;
//				GPU.CurlOctaves = static_cast<uint32>(CP.CurlOctaves);
//				GPU.CurlLacunarity = CP.CurlLacunarity;
//				GPU.CurlGain = CP.CurlGain;
//				GPU.CurlAmplitude = CP.CurlAmplitude;
//				GPU.CurlStrength = CP.CurlStrength;
//				break;
//			case EControlPointType::Dipole:
//				GPU.DipoleDirection = FVector2f(CP.DipoleDirection.GetSafeNormal());
//				GPU.DipoleStrength = CP.DipoleStrength;
//				break;
//			default:
//				break;
//			}
//			CachedGPUData.Add(GPU);
//		}
//		bControlPointsDirty = false;
//	}
//	uint32 NumPoints = CachedGPUData.Num();
//
//	FTextureRenderTargetResource* FBResource =
//		FrontBuffer->GameThread_GetRenderTargetResource();
//	FTextureRenderTargetResource* BBResource =
//		BackBuffer->GameThread_GetRenderTargetResource();
//	if (!FBResource || !BBResource)
//	{
//		UE_LOG(LogTemp, Error, TEXT("RenderTarget Resource is null."));
//		return;
//	}
//
//	float WorldTime = GetWorld()->GetTimeSeconds();
//	float VelThreshold = VelocityThreshold;
//	FVector2f BaseFlowVec = FVector2f(BaseFlow);
//
//	// Obstacle parameters
//	FTextureRenderTargetResource* ObstacleTexResource = nullptr;
//	if (ObstacleMap)
//	{
//		ObstacleTexResource = ObstacleMap->GameThread_GetRenderTargetResource();
//		if (ObstacleTexResource)
//		{
//		}
//		else
//		{
//			UE_LOG(LogTemp, Warning, TEXT("AuroraFlowSimulator: ObstacleMap resource is null"));
//		}
//	}
//	else
//	{
//		UE_LOG(LogTemp, Warning, TEXT("AuroraFlowSimulator: No obstacle texture (Map=%p)"), ObstacleMap);
//	}
//	float ObstacleRadius = ObstacleInfluenceRadius;
//
//	// Emitter parameters
//	FTextureResource* AuroraElementsTexResource = nullptr;
//	if (AuroraElementsMap)
//	{
//		AuroraElementsTexResource = AuroraElementsMap->GetResource();
//	}
//	else
//	{
//		UE_LOG(LogTemp, Warning, TEXT("AuroraFlowSimulator: AuroraElementsMap is NULL!"));
//	}
//
//	// 2. Enqueue render command
//	ENQUEUE_RENDER_COMMAND(SimulateAuroraFlow)
//	(
//		[
//			FBResource,
//			BBResource,
//			WorldTime,
//			DeltaTime,
//			VelThreshold,
//			BaseFlowVec,
//			GPUData = CachedGPUData,
//			NumPoints,
//			ObstacleTexResource,
//			ObstacleRadius,
//			AuroraElementsTexResource
//		]
//		(FRHICommandListImmediate& RHICmdList)
//		{
//			SCOPED_DRAW_EVENTF(RHICmdList, AuroraFlowSimulate, TEXT("AuroraFlowSimulate"));
//			SCOPED_GPU_STAT(RHICmdList, AuroraFlowSimulate);
//
//			// Create RDG builder
//			FRDGBuilder GraphBuilder(RHICmdList);
//
//			// Register output texture
//			FRDGTextureRef FrontBufferTexture =
//				GraphBuilder.RegisterExternalTexture(
//					CreateRenderTarget(
//						FBResource->GetRenderTargetTexture(),
//						TEXT("FrontBuffer")
//					)
//			);
//			FRDGTextureUAVRef FrontBufferUAV =
//				GraphBuilder.CreateUAV(FrontBufferTexture);
//
//			// Add clear pass
//			// AddClearUAVPass(GraphBuilder, FrontBufferUAV, FLinearColor::Black);
//
//			// Register input texture
//			FRDGTextureRef BackBufferTexture =
//				GraphBuilder.RegisterExternalTexture(
//					CreateRenderTarget(
//						BBResource->GetRenderTargetTexture(),
//						TEXT("BackBuffer")
//					)
//			);
//			FRDGTextureSRVRef BackBufferSRV =
//				GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(BackBufferTexture));
//
//			// Create structured buffer (needs at least 1 element even if array is empty)
//			uint32 BufferCount = FMath::Max(NumPoints, 1u);
//			FRDGBufferDesc Desc = FRDGBufferDesc::CreateStructuredDesc(
//				sizeof(FControlPointGPU), BufferCount);
//			FRDGBufferRef ControlPointBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("ControlPoints"));
//
//			if (NumPoints > 0)
//			{
//				// Upload data
//				GraphBuilder.QueueBufferUpload(ControlPointBuffer,
//					GPUData.GetData(),
//					sizeof(FControlPointGPU) * NumPoints);
//			}
//
//			FRDGBufferSRVRef ControlPointSRV = GraphBuilder.CreateSRV(ControlPointBuffer);
//
//			// Setup parameters
//			FAuroraFlowSimulateCS::FParameters* PassParameters =
//				GraphBuilder.AllocParameters<FAuroraFlowSimulateCS::FParameters>();
//			PassParameters->FrontBuffer = FrontBufferUAV;
//			PassParameters->BackBuffer = BackBufferSRV;
//			PassParameters->SamplerState =
//				TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
//			PassParameters->Time = WorldTime;
//			PassParameters->DeltaTime = DeltaTime;
//			PassParameters->VelocityThreshold = VelThreshold;
//			PassParameters->BaseFlow = BaseFlowVec;
//			PassParameters->ControlPoints = ControlPointSRV;
//			PassParameters->NumControlPoints = NumPoints;
//
//			// Obstacle parameters
//			if (ObstacleTexResource)
//			{
//				PassParameters->ObstacleMap = ObstacleTexResource->TextureRHI;
//			}
//			else
//			{
//				PassParameters->ObstacleMap = GBlackTexture->TextureRHI;
//
//			}
//			PassParameters->ObstacleSampler =
//					TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
//			PassParameters->ObstacleInfluenceRadius = ObstacleRadius;
//			PassParameters->bHasObstacle = (ObstacleTexResource != nullptr) ? 1u : 0u;
//
//			if (AuroraElementsTexResource)
//			{
//				PassParameters->AuroraElementsMap = AuroraElementsTexResource->TextureRHI;
//			}
//			else
//			{
//				PassParameters->AuroraElementsMap = GBlackTexture->TextureRHI;
//			}
//			PassParameters->AuroraElementsSampler =
//				TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
//
//			TShaderMapRef<FAuroraFlowSimulateCS> ComputeShader(
//				GetGlobalShaderMap(GMaxRHIFeatureLevel)
//			);
//
//			FIntVector GroupCount(
//				FMath::DivideAndRoundUp(FBResource->GetSizeX(), 8u),
//				FMath::DivideAndRoundUp(FBResource->GetSizeY(), 8u),
//				1
//			);
//
//			FComputeShaderUtils::AddPass(
//				GraphBuilder,
//				RDG_EVENT_NAME("AuroraFlowSimulate"),
//				ComputeShader,
//				PassParameters,
//				GroupCount
//			);
//
//			GraphBuilder.Execute();
//		}
//	);
//}
//
//void AAuroraFlowSimulator::BakeDistanceMapToRenderTarget()
//{
//	// 1. Validation
//	if (!AuroraElementsMap)
//	{
//		return;
//	}
//
//	if (!ObstacleMap)
//	{
//		UE_LOG(LogTemp, Error, TEXT("ObstacleMap is null!"));
//		return;
//	}
//
//	FTextureRenderTargetResource* ObstacleRenderTargetResource =
//		ObstacleMap->GameThread_GetRenderTargetResource();
//	if (!ObstacleRenderTargetResource)
//	{
//		UE_LOG(LogTemp, Error, TEXT("Obstacle render target resource is null."));
//		return;
//	}
//
//	FTextureResource* ObstacleTextureResource = AuroraElementsMap->GetResource();
//	if (!ObstacleTextureResource)
//	{
//		UE_LOG(LogTemp, Error, TEXT("Obstacle texture resource is null."));
//		return;
//	}
//
//	const int32 TextureSize = ObstacleRenderTargetResource->GetSizeX();
//
//	UTextureRenderTarget2D* Result = ObstacleMap;
//
//	// 2. Enqueue render command
//	ENQUEUE_RENDER_COMMAND(BakeDistanceMap)(
//		[
//			ObstacleRenderTargetResource,
//			ObstacleTextureResource,
//			TextureSize,
//			Result
//		](FRHICommandListImmediate& RHICmdList)
//		{
//			SCOPED_DRAW_EVENTF(RHICmdList, AuroraDistanceMapBake, TEXT("AuroraDistanceMapBake"));
//			SCOPED_GPU_STAT(RHICmdList, AuroraDistanceMapBake);
//
//			FRDGBuilder GraphBuilder(RHICmdList);
//
//			// Register external output texture
//			FRDGTextureRef ExternalOutputTexture =
//				GraphBuilder.RegisterExternalTexture(
//					CreateRenderTarget(
//						ObstacleRenderTargetResource->GetRenderTargetTexture(),
//						TEXT("DistanceMapOutput"))
//				);
//
//			// Register input texture
//			FRDGTextureRef AuroraElementsTextureRef =
//				GraphBuilder.RegisterExternalTexture(
//					CreateRenderTarget(
//						ObstacleTextureResource->TextureRHI,
//						TEXT("AuroraElementsTextureRef")
//					)
//				);
//			FRDGTextureSRVRef AuroraElementsTextureSRV =
//				GraphBuilder.CreateSRV(
//					FRDGTextureSRVDesc::Create(
//						AuroraElementsTextureRef
//					)
//				);
//
//			// Create ping-pong buffers for JFA (seed coordinates)
//			FRDGTextureDesc SeedDesc = FRDGTextureDesc::Create2D(
//				FIntPoint(TextureSize, TextureSize),
//				PF_FloatRGBA,
//				FClearValueBinding::None,
//				ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV
//			);
//
//			FRDGTextureRef SeedBuffer0 = GraphBuilder.CreateTexture(SeedDesc, TEXT("SeedBuffer0"));
//			FRDGTextureRef SeedBuffer1 = GraphBuilder.CreateTexture(SeedDesc, TEXT("SeedBuffer1"));
//
//			// PASS 1: Initialize - Convert mask to seed coordinates
//			{
//				FDistanceMapInitCS::FParameters* InitParams =
//					GraphBuilder.AllocParameters<FDistanceMapInitCS::FParameters>();
//				InitParams->InputMask = AuroraElementsTextureSRV;
//				InitParams->InputSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
//				InitParams->SeedBuffer = GraphBuilder.CreateUAV(SeedBuffer0);
//
//				TShaderMapRef<FDistanceMapInitCS> InitShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
//
//				FIntVector GroupCount(
//					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
//					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
//					1
//				);
//
//				FComputeShaderUtils::AddPass(
//					GraphBuilder,
//					RDG_EVENT_NAME("DistanceMap_Initialize"),
//					InitShader,
//					InitParams,
//					GroupCount
//				);
//			}
//
//			// PASS 2: Jump Flooding - Multiple passes with decreasing step size
//			FRDGTextureRef CurrentInput = SeedBuffer0;
//			FRDGTextureRef CurrentOutput = SeedBuffer1;
//
//			// Calculate number of JFA passes: log2(TextureSize)
//			int32 NumPasses = FMath::CeilLogTwo(TextureSize);
//
//			for (int32 PassIndex = 0; PassIndex < NumPasses; ++PassIndex)
//			{
//				int32 JumpStep = 1 << (NumPasses - 1 - PassIndex);	// TextureSize/2, TextureSize/4, ..., 2, 1
//
//				FDistanceMapJFACS::FParameters* JFAParams =
//					GraphBuilder.AllocParameters<FDistanceMapJFACS::FParameters>();
//				JFAParams->InputSeeds =
//					GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(CurrentInput));
//				JFAParams->InputSampler =
//					TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
//				JFAParams->OutputSeeds = GraphBuilder.CreateUAV(CurrentOutput);
//				JFAParams->JumpStep = JumpStep;
//
//				TShaderMapRef<FDistanceMapJFACS> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
//
//				FIntVector GroupCount(
//					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
//					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
//					1
//				);
//
//				FComputeShaderUtils::AddPass(
//					GraphBuilder,
//					RDG_EVENT_NAME("DistanceMap_JFA_Pass%d_Step%d", PassIndex, JumpStep),
//					JFAShader,
//					JFAParams,
//					GroupCount
//				);
//
//				// Swap buffers for next pass
//				Swap(CurrentInput, CurrentOutput);
//			}
//
//			// JFA+1: Additional pass with step size 1 for improved accuracy
//			// This corrects any errors from the power-of-2 step sizes
//			{
//				FDistanceMapJFACS::FParameters* JFAParams =
//					GraphBuilder.AllocParameters<FDistanceMapJFACS::FParameters>();
//				JFAParams->InputSeeds =
//					GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(CurrentInput));
//				JFAParams->InputSampler =
//					TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
//				JFAParams->OutputSeeds = GraphBuilder.CreateUAV(CurrentOutput);
//				JFAParams->JumpStep = 1;
//
//				TShaderMapRef<FDistanceMapJFACS> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
//
//				FIntVector GroupCount(
//					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
//					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
//					1
//				);
//
//				FComputeShaderUtils::AddPass(
//					GraphBuilder,
//					RDG_EVENT_NAME("DistanceMap_JFA_Plus1"),
//					JFAShader,
//					JFAParams,
//					GroupCount
//				);
//
//				// Swap buffers for finalize pass
//				Swap(CurrentInput, CurrentOutput);
//			}
//
//			// PASS 3: Finalize - Calculate distance, normals, encode to RGBA
//			{
//				FDistanceMapFinalizeCS::FParameters* FinalParams =
//					GraphBuilder.AllocParameters<FDistanceMapFinalizeCS::FParameters>();
//				FinalParams->SeedBufferFinal =
//					GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(CurrentInput));
//				FinalParams->SeedSampler =
//					TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
//				FinalParams->MaskInput = AuroraElementsTextureSRV;
//				FinalParams->MaskSampler =
//					TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
//				FinalParams->OutputTexture = GraphBuilder.CreateUAV(ExternalOutputTexture);
//
//				TShaderMapRef<FDistanceMapFinalizeCS> FinalShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
//
//				FIntVector GroupCount(
//					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
//					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
//					1
//				);
//
//				FComputeShaderUtils::AddPass(
//					GraphBuilder,
//					RDG_EVENT_NAME("DistanceMap_Finalize"),
//					FinalShader,
//					FinalParams,
//					GroupCount
//				);
//			}
//
//			GraphBuilder.Execute();
//		}
//	);
//}
//
//// =============================================================================
//// DEPRECATED FUNCTIONS - Commented out for future reference
//// =============================================================================
//
///*
//void AAuroraFlowSimulator::UnsharpAuroraPass()
//{
//	// DEPRECATED: Unsharp masking had no visible effect on quality
//}
//
//void AAuroraFlowSimulator::BicubicUpscalePass()
//{
//	// DEPRECATED: Bicubic upscaling had no visible effect on quality
//}
//
//void AAuroraFlowSimulator::KawaseBlurPass()
//{
//	// DEPRECATED: Kawase blur had no visible effect on quality
//}
//*/
