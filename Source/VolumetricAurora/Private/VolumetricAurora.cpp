// Copyright (c) 2026 R&B. All rights reserved.

#include "VolumetricAurora.h"

#include "VolumetricAuroraModule.h"
#include "TextureResource.h"
#include "SystemTextures.h" 
#include "RenderGraphBuilder.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "Math/UnrealMathUtility.h"
#include "SDFBakerComponentBase.h"
#include "RenderTargetPool.h"
#include "RenderGraphUtils.h"
#include "AuroraSDFBakeCS.h"
#include "AuroraFlowSimulateCS.h"
#include "ChartCreation.h"
#include "DistanceMapBakeCS.h"
#include "ShaderParameterUtils.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "DrawDebugHelpers.h"
#include "Math/MathFwd.h"
#include "Kismet/KismetRenderingLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Interfaces/IPluginManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/GameplayStatics.h"
#endif

// Sets default values
AVolumetricAurora::AVolumetricAurora()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	VolumeBox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VolumeBox"));
	VolumeBox->SetupAttachment(RootComponent);
	VolumeBox->ComponentTags.Add(TEXT("Volume"));
	// Set VolumeBox size similar to UDS Aurora (hardcoded)
	VolumeBox->SetRelativeLocation(FVector(0.0f, 0.0f, 110000.0f));
	VolumeBox->SetRelativeScale3D(FVector(50000.0f, 50000.0f, 1200.0f));

#if WITH_EDITOR
	//SDFBakerComponent = CreateDefaultSubobject<USplineSDFTextureBakerComponent>(TEXT("SplineSDFTextureBakerComponent"));
	//if (SDFBakerComponent && RootComponent)
	//{
	//	SDFBakerComponent->SetupAttachment(RootComponent);
	//}
#endif

	static ConstructorHelpers::FObjectFinder<UAuroraPresetBase> DefaultPreset(
		TEXT("/VolumetricAurora/AuroraPresets/DefaultNoise"));

	if (DefaultPreset.Object)
	{
		DefaultAurora = DefaultPreset.Object;
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(
		TEXT("/VolumetricAurora/Meshes/InnerCube.InnerCube")
	);
	if (MeshFinder.Succeeded())
	{
		VolumeBox->SetStaticMesh(MeshFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(
		TEXT("/VolumetricAurora/Materials/AuroraMaterial")
	);
	if (MatFinder.Succeeded())
	{
		VolumeBox->SetMaterial(0, MatFinder.Object);
	}

#if WITH_EDITORONLY_DATA
	// 에디터 전용: Billboard 컴포넌트 추가
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));

	if (SpriteComponent)
	{
		// 텍스처 로드 및 설정
		static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(
			TEXT("/VolumetricAurora/Icon/T_Aurora_Icon.T_Aurora_Icon")  // 경로는 프로젝트에 맞게
		);

		if (IconTexture.Succeeded())
		{
			SpriteComponent->SetSprite(IconTexture.Object);
		}

		SpriteComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
		SpriteComponent->SetRelativeScale3D(FVector(0.75)); // 아이콘 크기 조절
		SpriteComponent->ScreenSize = 0.003f;	// 최소 크기 (가까이 갈 때 작아질 수 있는 크기)
		SpriteComponent->SetupAttachment(SceneRoot);
		SpriteComponent->bIsScreenSizeScaled = true; // 에디터에서 화면 크기에 상관없이 일정하게 보이도록 설정
		SpriteComponent->SetUsingAbsoluteScale(true);
		SpriteComponent->bReceivesDecals = false;
	}
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
void AVolumetricAurora::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Null check
	if (!PropertyChangedEvent.Property)
	{
		UpdateMaterialTarget();
		return;
	}

	FName PropertyName = PropertyChangedEvent.Property->GetFName();

	// MemberProperty check for top-level changed member
	if (PropertyChangedEvent.MemberProperty)
	{
		FName MemberName = PropertyChangedEvent.MemberProperty->GetFName();

		// TargetAurora related property changed
		if (MemberName == GET_MEMBER_NAME_CHECKED(AVolumetricAurora, TargetAurora))
		{
			// Check if TargetAurora is PotentialFlowAuroraPreset
			if (UPotentialFlowAuroraPreset* FlowPreset = Cast<UPotentialFlowAuroraPreset>(TargetAurora))
			{
				// ControlPoints, AuroraElementsMap etc. may have changed
				bControlPointsDirty = true;

				// Check AuroraElementsMap change (by property name)
				if (PropertyName == GET_MEMBER_NAME_CHECKED(UPotentialFlowAuroraPreset, AuroraElementsMap))
				{
					bAuroraElementsMapDirty = true;
				}

				UE_LOG(
					LogTemp,
					Log,
					TEXT("PotentialFlow preset property changed : %s"),
					*PropertyName.ToString()
				);
			}
		}
	}

	UpdateMaterialTarget();
}

void AVolumetricAurora::PostActorCreated()
{
	Super::PostActorCreated();
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VolumetricAurora"));
	if (Plugin.IsValid())
	{
		PluginPath = TEXT("/") + Plugin->GetName();
		PresetFolder = PluginPath + TEXT("/") + TEXT("AuroraPresets");
	}

	if (!TargetAurora || !SourcePreset)
	{

		ApplyPresetToTarget(DefaultAurora);
	}
}

void AVolumetricAurora::PostLoad()
{
	Super::PostLoad();
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VolumetricAurora"));
	if (Plugin.IsValid())
	{
		PluginPath = TEXT("/") + Plugin->GetName();
		PresetFolder = PluginPath + TEXT("/") + TEXT("AuroraPresets");
	}

	if (!TargetAurora || !SourcePreset)
	{

		ApplyPresetToTarget(DefaultAurora);
	}
}

#endif
void AVolumetricAurora::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
#if WITH_EDITOR
	if (!SDFBakerComponent && GIsEditor && !IsTemplate())
	{
		UClass* SDFBakerClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Script/VolumetricAuroraEditor.SplineSDFTextureBakerComponent"));

		if (SDFBakerClass)
		{
			SDFBakerComponent = NewObject<USDFBakerComponentBase>(this, SDFBakerClass);
			if (SDFBakerComponent && RootComponent)
			{
				SDFBakerComponent->CreationMethod = EComponentCreationMethod::Instance;
				SDFBakerComponent->SetFlags(RF_Transactional);

				this->AddInstanceComponent(SDFBakerComponent);

				SDFBakerComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
				SDFBakerComponent->OnComponentCreated();
				SDFBakerComponent->RegisterComponent();

				PostEditChange();
			}
		}
	}
#endif


	if (!AuroraMaterialDynamic)
	{
		AuroraMaterialDynamic = VolumeBox->CreateAndSetMaterialInstanceDynamic(0);
	}
	UpdateMaterialTarget();
}

void AVolumetricAurora::BeginPlay()
{
	Super::BeginPlay();

	// TODO: Need modification later (allow user to bake SDF texture to different path)
	//MakeSDFTexture();

	// Ensure preset exists (simulation data)
	if (!TargetAurora)
	{
		ApplyPresetToTarget(DefaultAurora);
	}

	// Ensure MID exists in PIE (rendering target)
	if (!AuroraMaterialDynamic && VolumeBox)
	{
		AuroraMaterialDynamic = VolumeBox->CreateAndSetMaterialInstanceDynamic(0);
	}
}

// Called every frame
bool AVolumetricAurora::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AVolumetricAurora::FlowTick(float DeltaTime)
{

#if WITH_EDITOR
	RenderControlPointsDebug();
#endif

	UPotentialFlowAuroraPreset* FlowPreset = Cast<UPotentialFlowAuroraPreset>(TargetAurora);

	FlowSimulationAccumulatedTime += DeltaTime * FlowPreset->FlowTimeScale;

	// Calculate resolution value
	const int32 SimRes = GetResolutionValue(FlowPreset->SimulationResolution);

	// Create render targets or recreate on resolution change
	if (!FlowPreset->FrontBuffer || !FlowPreset->FrontBuffer->bCanCreateUAV || FlowPreset->FrontBuffer->SizeX != SimRes)
	{
		FlowPreset->FrontBuffer = NewObject<UTextureRenderTarget2D>(this);
		FlowPreset->FrontBuffer->bCanCreateUAV = true;
		FlowPreset->FrontBuffer->InitCustomFormat(SimRes, SimRes, PF_FloatRGBA, false);
		FlowPreset->FrontBuffer->UpdateResourceImmediate();
	}
	if (!FlowPreset->BackBuffer || !FlowPreset->BackBuffer->bCanCreateUAV || FlowPreset->BackBuffer->SizeX != SimRes)
	{
		FlowPreset->BackBuffer = NewObject<UTextureRenderTarget2D>(this);
		FlowPreset->BackBuffer->bCanCreateUAV = true;
		FlowPreset->BackBuffer->InitCustomFormat(SimRes, SimRes, PF_FloatRGBA, false);
		FlowPreset->BackBuffer->UpdateResourceImmediate();
	}

	if (!FlowPreset->ObstacleMap || !FlowPreset->ObstacleMap->bCanCreateUAV || FlowPreset->ObstacleMap->SizeX != SimRes)
	{
		FlowPreset->ObstacleMap = NewObject<UTextureRenderTarget2D>(this);
		FlowPreset->ObstacleMap->bCanCreateUAV = true;
		FlowPreset->ObstacleMap->InitCustomFormat(SimRes, SimRes, PF_FloatRGBA, false);
		FlowPreset->ObstacleMap->UpdateResourceImmediate();
		bAuroraElementsMapDirty = true;
	}

	//// Connect material on first run
	//if (!DynamicMaterial && PreviewMaterial && PreviewPlane)
	//{
	//	DynamicMaterial = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
	//	PreviewPlane->SetMaterial(0, DynamicMaterial);
	//}

	// Detect AuroraElementsMap changes (file replacement or content modification)
	if (FlowPreset->AuroraElementsMap != PreviousAuroraElementsMap)
	{
		// Replaced with different texture (or changed to nullptr)
		bAuroraElementsMapDirty = true;
		PreviousAuroraElementsMap = FlowPreset->AuroraElementsMap;
		PreviousAuroraElementsResource = FlowPreset->AuroraElementsMap ? FlowPreset->AuroraElementsMap->GetResource() : nullptr;
		//UE_LOG(LogTemp, Warning, TEXT("AuroraElementsMap changed to new texture: %p"), AuroraElementsMap);
	}
	else if (FlowPreset->AuroraElementsMap && FlowPreset->AuroraElementsMap->GetResource())
	{
		// Same texture but check if resource was updated
		// Texture reimport recreates resource, so detect that
		FTextureResource* CurrentResource = FlowPreset->AuroraElementsMap->GetResource();

		if (PreviousAuroraElementsResource != CurrentResource)
		{
			bAuroraElementsMapDirty = true;
			PreviousAuroraElementsResource = CurrentResource;
			UE_LOG(LogTemp, Warning, TEXT("AuroraElementsMap resource updated (reimported)"));
		}
	}

	// Bake distance map when AuroraElementsMap changes
	if (bAuroraElementsMapDirty)
	{
		BakeDistanceMapToRenderTarget(FlowPreset);
		bAuroraElementsMapDirty = false;
	}

	SimulateAuroraPass(FlowPreset, DeltaTime);

	// No post-processing: DisplayBuffer = FrontBuffer
	FlowPreset->DisplayBuffer = FlowPreset->FrontBuffer;

	// Swap buffers for next frame
	Swap(FlowPreset->FrontBuffer, FlowPreset->BackBuffer);
}

void AVolumetricAurora::NormalizeFlowSimulationParameter()
{

}


void AVolumetricAurora::SimulateAuroraPass(UPotentialFlowAuroraPreset* FlowPreset, float DeltaTime)
{
	// 1. Validation
	if (!FlowPreset->FrontBuffer || !FlowPreset->BackBuffer)
	{
		UE_LOG(LogTemp, Error, TEXT("RenderTarget is null."));
		return;
	}

	bool bResetSimulation = bForceResetSimulation;
	bForceResetSimulation = false;

	// CPU to GPU structure conversion
	if (bControlPointsDirty)
	{
		// Restart Simulation only if bResetSimulationOnElementChange is enabled
		if (FlowPreset->bResetSimulationOnElementChange)
		{
			bResetSimulation = true;
		}

		ControlPointsInfo.Reset();
		SingleForceControlPoints.Reset();
		DoubleForceControlPoints.Reset();
		TripleForceControlPoints.Reset();
		DipoleControlPoints.Reset();
		CurlControlPoints.Reset();
		WarpControlPoints.Reset();

		for (const FFlowElement& CP : FlowPreset->ControlPoints)
		{
			FControlPointInfoGPU CPInfo = {};

			CPInfo.Type = static_cast<uint32>(CP.Type);
			CPInfo.IsGlobal = (CP.Range == EControlPointRange::Global) ? 1u : 0u;
			CPInfo.Position = FVector2f(CP.Position);

			ControlPointsInfo.Add(CPInfo);

			switch (CP.Type)
			{
			case EControlPointType::Source:
			{
				FDoubleForceControlPointGPU SourceCP = {};
				SourceCP.ForceStrength = CP.RadialStrength;
				SourceCP.AttenuationStart = CP.RadialAttenuationStart;
				SourceCP.AttenuationEnd = CP.RadialAttenuationEnd;
				SourceCP.AttenuationExponent = CP.RadialExponent;
				SourceCP.ForceStrength2 = CP.EmissionStrength;
				SourceCP.AttenuationStart2 = CP.EmissionAttenuationStart;
				SourceCP.AttenuationEnd2 = CP.EmissionAttenuationEnd;
				SourceCP.AttenuationExponent2 = CP.EmissionExponent;
				DoubleForceControlPoints.Add(SourceCP);
				break;
			}
			case EControlPointType::Sink:
			{
				FDoubleForceControlPointGPU SinkCP = {};
				SinkCP.ForceStrength = -CP.RadialStrength;
				SinkCP.AttenuationStart = CP.RadialAttenuationStart;
				SinkCP.AttenuationEnd = CP.RadialAttenuationEnd;
				SinkCP.AttenuationExponent = CP.RadialExponent;
				SinkCP.ForceStrength2 = CP.FadeStrength * AuroraFlowConstants::FadeStrengthScale;
				SinkCP.AttenuationStart2 = CP.FadeAttenuationStart;
				SinkCP.AttenuationEnd2 = CP.FadeAttenuationEnd;
				SinkCP.AttenuationExponent2 = CP.FadeExponent;
				DoubleForceControlPoints.Add(SinkCP);
				break;
			}
			case EControlPointType::Vortex:
			{
				FDoubleForceControlPointGPU VortexCP = {};
				VortexCP.ForceStrength = CP.RotationStrength;
				VortexCP.AttenuationStart = CP.RotationAttenuationStart;
				VortexCP.AttenuationEnd = CP.RotationAttenuationEnd;
				VortexCP.AttenuationExponent = CP.RotationExponent;
				VortexCP.ForceStrength2 = CP.FadeStrength * AuroraFlowConstants::FadeStrengthScale;
				VortexCP.AttenuationStart2 = CP.FadeAttenuationStart;
				VortexCP.AttenuationEnd2 = CP.FadeAttenuationEnd;
				VortexCP.AttenuationExponent2 = CP.FadeExponent;
				DoubleForceControlPoints.Add(VortexCP);
				break;
			}
			case EControlPointType::Spiral:
			{
				FTripleForceControlPointGPU SpiralCP = {};
				SpiralCP.ForceStrength = CP.RadialStrength;
				SpiralCP.AttenuationStart = CP.RadialAttenuationStart;
				SpiralCP.AttenuationEnd = CP.RadialAttenuationEnd;
				SpiralCP.AttenuationExponent = CP.RadialExponent;
				SpiralCP.ForceStrength2 = CP.RotationStrength;
				SpiralCP.AttenuationStart2 = CP.RotationAttenuationStart;
				SpiralCP.AttenuationEnd2 = CP.RotationAttenuationEnd;
				SpiralCP.AttenuationExponent2 = CP.RotationExponent;
				SpiralCP.ForceStrength3 = CP.FadeStrength * AuroraFlowConstants::FadeStrengthScale;
				SpiralCP.AttenuationStart3 = CP.FadeAttenuationStart;
				SpiralCP.AttenuationEnd3 = CP.FadeAttenuationEnd;
				SpiralCP.AttenuationExponent3 = CP.FadeExponent;
				TripleForceControlPoints.Add(SpiralCP);
				break;
			}
			case EControlPointType::Dipole:
			{
				FDipoleControlPointGPU DipoleCP = {};
				DipoleCP.ForceStrength = CP.DipoleStrength * AuroraFlowConstants::DipoleStrengthScale;
				DipoleCP.AttenuationStart = CP.DipoleAttenuationStart;
				DipoleCP.AttenuationEnd = CP.DipoleAttenuationEnd;
				DipoleCP.AttenuationExponent = CP.DipoleExponent;
				DipoleCP.DipoleDirection = FVector2f(CP.DipoleDirection.GetSafeNormal());
				DipoleControlPoints.Add(DipoleCP);
				break;
			}
			case EControlPointType::Curl:
			{
				FCurlControlPointGPU CurlCP = {};
				CurlCP.CurlFrequency = CP.CurlFrequency * AuroraFlowConstants::CurlFrequencyScale;
				CurlCP.CurlAnimationSpeed = CP.CurlAnimationSpeed * AuroraFlowConstants::CurlAnimSpeedScale;
				CurlCP.CurlOctaves = static_cast<uint32>(CP.CurlOctaves);
				CurlCP.CurlLacunarity = CP.CurlLacunarity;
				CurlCP.CurlGain = CP.CurlGain;
				CurlCP.CurlAmplitude = CP.CurlAmplitude;
				CurlCP.ForceStrength = CP.CurlStrength * AuroraFlowConstants::CurlStrengthScale;
				CurlCP.AttenuationStart = CP.CurlAttenuationStart;
				CurlCP.AttenuationEnd = CP.CurlAttenuationEnd;
				CurlCP.AttenuationExponent = CP.CurlExponent;
				CurlControlPoints.Add(CurlCP);
				break;
			}
			case EControlPointType::Warp:
			{
				FWarpControlPointGPU WarpCP = {};
				WarpCP.Iterations = static_cast<uint32>(CP.WarpIterations);
				WarpCP.Displacement = CP.WarpDisplacement;
				WarpCP.Falloff = CP.WarpFalloff;
				WarpCP.Contrast = CP.WarpContrast;
				WarpCP.AnimAmplitude = CP.WarpAnimAmplitude;
				WarpCP.AnimationSpeed = CP.WarpAnimationSpeed;
				WarpCP.XScale = CP.WarpXScale;
				WarpCP.YScale = CP.WarpYScale;
				WarpCP.NoiseScale = CP.WarpNoiseScale;
				WarpCP.XOffset = FVector2f(CP.WarpXOffset);
				WarpCP.YOffset = FVector2f(CP.WarpYOffset);
				WarpCP.NoiseOffset = FVector2f(CP.WarpNoiseOffset);
				WarpCP.Octaves = static_cast<uint32>(CP.WarpOctaves);
				WarpCP.Lacunarity = CP.WarpLacunarity;
				WarpCP.Gain = CP.WarpGain;
				WarpCP.InitialAmplitude = CP.WarpInitialAmplitude;
				WarpCP.FlowStrength = CP.WarpFlowStrength;
				WarpCP.AttenuationStart = CP.WarpAttenuationStart;
				WarpCP.AttenuationEnd = CP.WarpAttenuationEnd;
				WarpCP.AttenuationExponent = CP.WarpExponent;
				WarpControlPoints.Add(WarpCP);
				break;
			}
			case EControlPointType::Emitter:
			{
				FSingleForceControlPointGPU EmitterCP = {};
				EmitterCP.ForceStrength = CP.EmissionStrength;
				EmitterCP.AttenuationStart = CP.EmissionAttenuationStart;
				EmitterCP.AttenuationEnd = CP.EmissionAttenuationEnd;
				EmitterCP.AttenuationExponent = CP.EmissionExponent;
				SingleForceControlPoints.Add(EmitterCP);
				break;
			}
			case EControlPointType::Attenuator:
			{
				FSingleForceControlPointGPU AttenuatorCP = {};
				AttenuatorCP.ForceStrength = CP.FadeStrength * AuroraFlowConstants::FadeStrengthScale;
				AttenuatorCP.AttenuationStart = CP.FadeAttenuationStart;
				AttenuatorCP.AttenuationEnd = CP.FadeAttenuationEnd;
				AttenuatorCP.AttenuationExponent = CP.FadeExponent;
				SingleForceControlPoints.Add(AttenuatorCP);
				break;
			}
			default:
				break;
			}
		}
		bControlPointsDirty = false;
	}

	FTextureRenderTargetResource* FBResource =
		FlowPreset->FrontBuffer->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* BBResource =
		FlowPreset->BackBuffer->GameThread_GetRenderTargetResource();

	if (!FBResource || !BBResource)
	{
		UE_LOG(LogTemp, Error, TEXT("RenderTarget Resource is null."));
		return;
	}

	FVector2f BaseFlowVec = FVector2f(FlowPreset->BaseFlow * AuroraFlowConstants::BaseFlowScale);

	// Obstacle parameters
	FTextureRenderTargetResource* ObstacleTexResource = nullptr;
	if (FlowPreset->ObstacleMap)
	{
		ObstacleTexResource = FlowPreset->ObstacleMap->GameThread_GetRenderTargetResource();
		if (!ObstacleTexResource)
		{
			UE_LOG(LogTemp, Warning, TEXT("AuroraFlowSimulator: ObstacleMap resource is null"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AuroraFlowSimulator: No obstacle texture (Map=%p)"), FlowPreset->ObstacleMap);
	}

	// Emitter parameters
	FTextureResource* AuroraElementsTexResource = nullptr;
	if (FlowPreset->AuroraElementsMap)
	{
		AuroraElementsTexResource = FlowPreset->AuroraElementsMap->GetResource();
	}

	// 2. Enqueue render command
	ENQUEUE_RENDER_COMMAND(SimulateAuroraFlow)
		(
			[
				FBResource,
				BBResource,
				AccumulatedTime = FlowSimulationAccumulatedTime,
				TickTime = DeltaTime * FlowPreset->FlowTimeScale,
				BaseFlowVec,
				CPInfo = ControlPointsInfo,
				SingleForceCP = SingleForceControlPoints,
				DoubleForceCP = DoubleForceControlPoints,
				TripleForceCP = TripleForceControlPoints,
				DipoleCP = DipoleControlPoints,
				CurlCP = CurlControlPoints,
				WarpCP = WarpControlPoints,
				ObstacleTexResource,
				AuroraElementsTexResource,
				FlowPreset,
				bResetSimulation
			]
	(FRHICommandListImmediate& RHICmdList)
	{
		SCOPED_DRAW_EVENTF(RHICmdList, AuroraFlowSimulate, TEXT("AuroraFlowSimulate"));
		SCOPED_GPU_STAT(RHICmdList, AuroraFlowSimulate);

		FRDGBuilder GraphBuilder(RHICmdList);

		FRDGTextureRef FrontBufferTexture =
			GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(FBResource->GetRenderTargetTexture(), TEXT("FrontBuffer"))
			);
		FRDGTextureUAVRef FrontBufferUAV = GraphBuilder.CreateUAV(FrontBufferTexture);

		FRDGTextureRef BackBufferTexture =
			GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(BBResource->GetRenderTargetTexture(), TEXT("BackBuffer"))
			);

		if (bResetSimulation)
		{
			AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(BackBufferTexture), FLinearColor::Black);
		}

		FRDGTextureSRVRef BackBufferSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(BackBufferTexture));

		uint32 NumControlPoints = CPInfo.Num();
		uint32 BufferCount = FMath::Max(NumControlPoints, 1u);
		FRDGBufferDesc Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FControlPointInfoGPU), BufferCount);
		FRDGBufferRef ControlPointInfoBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("ControlPointsInfo"));
		if (NumControlPoints > 0)
			GraphBuilder.QueueBufferUpload(ControlPointInfoBuffer, CPInfo.GetData(), sizeof(FControlPointInfoGPU)* NumControlPoints);
		FRDGBufferSRVRef ControlPointsInfoSRV = GraphBuilder.CreateSRV(ControlPointInfoBuffer);

		uint32 NumSingleForceCP = SingleForceCP.Num();
		BufferCount = FMath::Max(NumSingleForceCP, 1u);
		Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FSingleForceControlPointGPU), BufferCount);
		FRDGBufferRef SingleForceCPBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("SingleForceControlPoint"));
		if (NumSingleForceCP > 0)
			GraphBuilder.QueueBufferUpload(SingleForceCPBuffer, SingleForceCP.GetData(), sizeof(FSingleForceControlPointGPU)* NumSingleForceCP);
		FRDGBufferSRVRef SingleForceCPSRV = GraphBuilder.CreateSRV(SingleForceCPBuffer);

		uint32 NumDoubleForceCP = DoubleForceCP.Num();
		BufferCount = FMath::Max(NumDoubleForceCP, 1u);
		Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FDoubleForceControlPointGPU), BufferCount);
		FRDGBufferRef DoubleForceCPBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("DoubleForceControlPoint"));
		if (NumDoubleForceCP > 0)
			GraphBuilder.QueueBufferUpload(DoubleForceCPBuffer, DoubleForceCP.GetData(), sizeof(FDoubleForceControlPointGPU)* NumDoubleForceCP);
		FRDGBufferSRVRef DoubleForceCPSRV = GraphBuilder.CreateSRV(DoubleForceCPBuffer);

		uint32 NumTripleForceCP = TripleForceCP.Num();
		BufferCount = FMath::Max(NumTripleForceCP, 1u);
		Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FTripleForceControlPointGPU), BufferCount);
		FRDGBufferRef TripleForceCPBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("TripleForceControlPoint"));
		if (NumTripleForceCP > 0)
			GraphBuilder.QueueBufferUpload(TripleForceCPBuffer, TripleForceCP.GetData(), sizeof(FTripleForceControlPointGPU)* NumTripleForceCP);
		FRDGBufferSRVRef TripleForceCPSRV = GraphBuilder.CreateSRV(TripleForceCPBuffer);

		uint32 NumDipoleCP = DipoleCP.Num();
		BufferCount = FMath::Max(NumDipoleCP, 1u);
		Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FDipoleControlPointGPU), BufferCount);
		FRDGBufferRef DipoleCPBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("DipoleControlPoint"));
		if (NumDipoleCP > 0)
			GraphBuilder.QueueBufferUpload(DipoleCPBuffer, DipoleCP.GetData(), sizeof(FDipoleControlPointGPU)* NumDipoleCP);
		FRDGBufferSRVRef DipoleCPSRV = GraphBuilder.CreateSRV(DipoleCPBuffer);

		uint32 NumCurlCP = CurlCP.Num();
		BufferCount = FMath::Max(NumCurlCP, 1u);
		Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FCurlControlPointGPU), BufferCount);
		FRDGBufferRef CurlCPBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("CurlControlPoint"));
		if (NumCurlCP > 0)
			GraphBuilder.QueueBufferUpload(CurlCPBuffer, CurlCP.GetData(), sizeof(FCurlControlPointGPU)* NumCurlCP);
		FRDGBufferSRVRef CurlCPSRV = GraphBuilder.CreateSRV(CurlCPBuffer);

		uint32 NumWarpCP = WarpCP.Num();
		BufferCount = FMath::Max(NumWarpCP, 1u);
		Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FWarpControlPointGPU), BufferCount);
		FRDGBufferRef WarpCPBuffer = GraphBuilder.CreateBuffer(Desc, TEXT("WarpControlPoint"));
		if (NumWarpCP > 0)
			GraphBuilder.QueueBufferUpload(WarpCPBuffer, WarpCP.GetData(), sizeof(FWarpControlPointGPU)* NumWarpCP);
		FRDGBufferSRVRef WarpCPSRV = GraphBuilder.CreateSRV(WarpCPBuffer);

		FAuroraFlowSimulateCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FAuroraFlowSimulateCS::FParameters>();
		PassParameters->FrontBuffer = FrontBufferUAV;
		PassParameters->BackBuffer = BackBufferSRV;
		PassParameters->Sampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->Time = AccumulatedTime;
		// restrict delta time upper limit to consistent simulation(120 fps)
		PassParameters->DeltaTime = FMath::Min(TickTime, 0.0083f);
		PassParameters->BaseFlow = BaseFlowVec;
		PassParameters->ControlPointsInfo = ControlPointsInfoSRV;
		PassParameters->NumControlPoints = NumControlPoints;
		PassParameters->SingleForceControlPoints = SingleForceCPSRV;
		PassParameters->DoubleForceControlPoints = DoubleForceCPSRV;
		PassParameters->TripleForceControlPoints = TripleForceCPSRV;
		PassParameters->DipoleControlPoints = DipoleCPSRV;
		PassParameters->CurlControlPoints = CurlCPSRV;
		PassParameters->WarpControlPoints = WarpCPSRV;

		// Obstacle Texture 안전 처리
		if (ObstacleTexResource)
		{
			PassParameters->ObstacleMap = ObstacleTexResource->TextureRHI;
		}
		else
		{
			PassParameters->ObstacleMap = GSystemTextures.BlackDummy->GetRHI();
		}
		PassParameters->ObstacleSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->bHasObstacle = (ObstacleTexResource != nullptr) ? 1u : 0u;

		// Aurora Elements Texture 안전 처리
		if (AuroraElementsTexResource)
		{
			PassParameters->AuroraElementsMap = AuroraElementsTexResource->TextureRHI;
		}
		else
		{
			PassParameters->AuroraElementsMap = GSystemTextures.BlackDummy->GetRHI();
		}
		PassParameters->AuroraElementsSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

		PassParameters->EmitterNoiseFrequency = FlowPreset->EmitterNoiseFrequency * AuroraFlowConstants::EmitterNoiseFrequencyMultiplier;
		PassParameters->EmitterNoiseSpeed = FlowPreset->EmitterNoiseSpeed * AuroraFlowConstants::EmitterNoiseSpeedScale;
		PassParameters->EmitterNoiseStrength = FlowPreset->EmitterNoiseStrength;

		TShaderMapRef<FAuroraFlowSimulateCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

		// DivideAndRoundUp: 8u -> 8 (int 타입으로 수정)
		FIntVector GroupCount(
			FMath::DivideAndRoundUp(FBResource->GetSizeX(), 8u),
			FMath::DivideAndRoundUp(FBResource->GetSizeY(), 8u),
			1
		);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("AuroraFlowSimulate"),
			ComputeShader,
			PassParameters,
			GroupCount
		);

		GraphBuilder.Execute();
	});
}

void AVolumetricAurora::BakeDistanceMapToRenderTarget(UPotentialFlowAuroraPreset* FlowPreset)
{
	// 1. Validation
	if (!FlowPreset->AuroraElementsMap)
	{
		return;
	}

	if (!FlowPreset->ObstacleMap)
	{
		UE_LOG(LogTemp, Error, TEXT("ObstacleMap is null!"));
		return;
	}

	FTextureRenderTargetResource* ObstacleRenderTargetResource =
		FlowPreset->ObstacleMap->GameThread_GetRenderTargetResource();
	if (!ObstacleRenderTargetResource)
	{
		UE_LOG(LogTemp, Error, TEXT("Obstacle render target resource is null."));
		return;
	}

	FTextureResource* ObstacleTextureResource = FlowPreset->AuroraElementsMap->GetResource();
	if (!ObstacleTextureResource)
	{
		UE_LOG(LogTemp, Error, TEXT("Obstacle texture resource is null."));
		return;
	}

	const int32 TextureSize = ObstacleRenderTargetResource->GetSizeX();

	UTextureRenderTarget2D* Result = FlowPreset->ObstacleMap;

	// 2. Enqueue render command
	ENQUEUE_RENDER_COMMAND(BakeDistanceMap)(
		[
			ObstacleRenderTargetResource,
			ObstacleTextureResource,
			TextureSize,
			Result
		](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_DRAW_EVENTF(RHICmdList, AuroraDistanceMapBake, TEXT("AuroraDistanceMapBake"));
			SCOPED_GPU_STAT(RHICmdList, AuroraDistanceMapBake);

			FRDGBuilder GraphBuilder(RHICmdList);

			// Register external output texture
			FRDGTextureRef ExternalOutputTexture =
				GraphBuilder.RegisterExternalTexture(
					CreateRenderTarget(
						ObstacleRenderTargetResource->GetRenderTargetTexture(),
						TEXT("DistanceMapOutput"))
				);

			// Register input texture
			FRDGTextureRef AuroraElementsTextureRef =
				GraphBuilder.RegisterExternalTexture(
					CreateRenderTarget(
						ObstacleTextureResource->TextureRHI,
						TEXT("AuroraElementsTextureRef")
					)
				);
			FRDGTextureSRVRef AuroraElementsTextureSRV =
				GraphBuilder.CreateSRV(
					FRDGTextureSRVDesc::Create(
						AuroraElementsTextureRef
					)
				);

			// Create ping-pong buffers for JFA (seed coordinates)
			FRDGTextureDesc SeedDesc = FRDGTextureDesc::Create2D(
				FIntPoint(TextureSize, TextureSize),
				PF_FloatRGBA,
				FClearValueBinding::None,
				ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV
			);

			FRDGTextureRef SeedBuffer0 = GraphBuilder.CreateTexture(SeedDesc, TEXT("SeedBuffer0"));
			FRDGTextureRef SeedBuffer1 = GraphBuilder.CreateTexture(SeedDesc, TEXT("SeedBuffer1"));

			// PASS 1: Initialize - Convert mask to seed coordinates
			{
				FDistanceMapInitCS::FParameters* InitParams =
					GraphBuilder.AllocParameters<FDistanceMapInitCS::FParameters>();
				InitParams->InputMask = AuroraElementsTextureSRV;
				InitParams->InputSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				InitParams->SeedBuffer = GraphBuilder.CreateUAV(SeedBuffer0);

				TShaderMapRef<FDistanceMapInitCS> InitShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

				FIntVector GroupCount(
					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
					1
				);

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("DistanceMap_Initialize"),
					InitShader,
					InitParams,
					GroupCount
				);
			}

			// PASS 2: Jump Flooding - Multiple passes with decreasing step size
			FRDGTextureRef CurrentInput = SeedBuffer0;
			FRDGTextureRef CurrentOutput = SeedBuffer1;

			// Calculate number of JFA passes: log2(TextureSize)
			int32 NumPasses = FMath::CeilLogTwo(TextureSize);

			for (int32 PassIndex = 0; PassIndex < NumPasses; ++PassIndex)
			{
				int32 JumpStep = 1 << (NumPasses - 1 - PassIndex);	// TextureSize/2, TextureSize/4, ..., 2, 1

				FDistanceMapJFACS::FParameters* JFAParams =
					GraphBuilder.AllocParameters<FDistanceMapJFACS::FParameters>();
				JFAParams->InputSeeds =
					GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(CurrentInput));
				JFAParams->InputSampler =
					TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				JFAParams->OutputSeeds = GraphBuilder.CreateUAV(CurrentOutput);
				JFAParams->JumpStep = JumpStep;

				TShaderMapRef<FDistanceMapJFACS> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

				FIntVector GroupCount(
					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
					1
				);

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("DistanceMap_JFA_Pass%d_Step%d", PassIndex, JumpStep),
					JFAShader,
					JFAParams,
					GroupCount
				);

				// Swap buffers for next pass
				Swap(CurrentInput, CurrentOutput);
			}

			// JFA+1: Additional pass with step size 1 for improved accuracy
			// This corrects any errors from the power-of-2 step sizes
			{
				FDistanceMapJFACS::FParameters* JFAParams =
					GraphBuilder.AllocParameters<FDistanceMapJFACS::FParameters>();
				JFAParams->InputSeeds =
					GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(CurrentInput));
				JFAParams->InputSampler =
					TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				JFAParams->OutputSeeds = GraphBuilder.CreateUAV(CurrentOutput);
				JFAParams->JumpStep = 1;

				TShaderMapRef<FDistanceMapJFACS> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

				FIntVector GroupCount(
					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
					1
				);

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("DistanceMap_JFA_Plus1"),
					JFAShader,
					JFAParams,
					GroupCount
				);

				// Swap buffers for finalize pass
				Swap(CurrentInput, CurrentOutput);
			}

			// PASS 3: Finalize - Calculate distance, normals, encode to RGBA
			{
				FDistanceMapFinalizeCS::FParameters* FinalParams =
					GraphBuilder.AllocParameters<FDistanceMapFinalizeCS::FParameters>();
				FinalParams->SeedBufferFinal =
					GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(CurrentInput));
				FinalParams->SeedSampler =
					TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				FinalParams->MaskInput = AuroraElementsTextureSRV;
				FinalParams->MaskSampler =
					TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
				FinalParams->OutputTexture = GraphBuilder.CreateUAV(ExternalOutputTexture);

				TShaderMapRef<FDistanceMapFinalizeCS> FinalShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

				FIntVector GroupCount(
					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
					FMath::DivideAndRoundUp((uint32)TextureSize, 8u),
					1
				);

				FComputeShaderUtils::AddPass(
					GraphBuilder,
					RDG_EVENT_NAME("DistanceMap_Finalize"),
					FinalShader,
					FinalParams,
					GroupCount
				);
			}

			GraphBuilder.Execute();
		}
		);
}

#if WITH_EDITOR
void AVolumetricAurora::RenderControlPointsDebug() const
{
	// Control Points Debug Display using DrawDebugSphere
	if (!GetWorld() || !TargetAurora)
	{
		return;
	}

	UPotentialFlowAuroraPreset* PFAuroraPreset =
		Cast<UPotentialFlowAuroraPreset>(TargetAurora);
	if (!PFAuroraPreset || !PFAuroraPreset->bDisplayControlPoints) return;

	// Use DisplaySize as sphere radius
	float SphereRadius = PFAuroraPreset->DisplaySize * 10000.0f;
	
	// Find Volume
	UStaticMeshComponent* Volume = nullptr;
	TArray<UActorComponent*> Comps =
		GetComponentsByTag(UStaticMeshComponent::StaticClass(), TEXT("Volume"));
	
	if (Comps.Num() == 0) return;
	Volume = Cast<UStaticMeshComponent>(Comps[0]);
	if (!Volume) return;

	
	int PointWithoutPositionNum = 0;

	// Store Control Point position and type info only
	for (const FFlowElement& ControlPoint : PFAuroraPreset->ControlPoints)
	{
		if (ControlPoint.Type == EControlPointType::None)
		{
			continue;
		}

		bool bIsGlobal = ControlPoint.Range == EControlPointRange::Global;

		// Calculate position
		const FVector Extent = Volume->Bounds.BoxExtent;
		const FVector Center = Volume->Bounds.Origin;

		FVector Pos = Center;

		if ((ControlPoint.Type == EControlPointType::Curl
			|| ControlPoint.Type == EControlPointType::Warp)
			&& bIsGlobal)
		{
			// Curl: Separate placement (on edge)
			Pos.X += 1.1f * Extent.X;
			Pos.Y += (0.125f * PointWithoutPositionNum - 0.5f) * 2.0f * Extent.Y;
			Pos.Z += PFAuroraPreset->DisplayZPos;
			PointWithoutPositionNum++;
		}
		else
		{
			// Others: Reflect struct Position (0~1 -> inside volume)
			Pos.X += (ControlPoint.Position.X - 0.5f) * 2.0f * Extent.X;
			Pos.Y += (ControlPoint.Position.Y - 0.5f) * 2.0f * Extent.Y;
			Pos.Z += PFAuroraPreset->DisplayZPos;
		}

		// Assign color based on EControlPointType
		FColor DisplayColor = FColor::White;
		bool bHasSecondForce = false;
		bool bHasThirdForce = false;
		float AttenuationStart = 0.f;
		float AttenuationEnd = 0.f;
		float AttenuationStart2 = 0.f;
		float AttenuationEnd2 = 0.f;
		float AttenuationStart3 = 0.f;
		float AttenuationEnd3 = 0.f;

		switch (ControlPoint.Type)
		{
		case EControlPointType::Source:
			DisplayColor = FColor::Green;
			AttenuationStart = ControlPoint.RadialAttenuationStart;
			AttenuationEnd = ControlPoint.RadialAttenuationEnd;
			bHasSecondForce = true;
			AttenuationStart2 = ControlPoint.EmissionAttenuationStart;
			AttenuationEnd2 = ControlPoint.EmissionAttenuationEnd;
			break;
		case EControlPointType::Sink:
			DisplayColor = FColor::Red;
			AttenuationStart = ControlPoint.RadialAttenuationStart;
			AttenuationEnd = ControlPoint.RadialAttenuationEnd;
			bHasSecondForce = true;
			AttenuationStart2 = ControlPoint.FadeAttenuationStart;
			AttenuationEnd2 = ControlPoint.FadeAttenuationEnd;
			break;
		case EControlPointType::Dipole:
			DisplayColor = FColor::Blue;
			AttenuationStart = ControlPoint.DipoleAttenuationStart;
			AttenuationEnd = ControlPoint.DipoleAttenuationEnd;
			break;
		case EControlPointType::Vortex:
			DisplayColor = FColor::Cyan;
			AttenuationStart = ControlPoint.RotationAttenuationStart;
			AttenuationEnd = ControlPoint.RotationAttenuationEnd;
			bHasSecondForce = true;
			AttenuationStart2 = ControlPoint.FadeAttenuationStart;
			AttenuationEnd2 = ControlPoint.FadeAttenuationEnd;
			break;
		case EControlPointType::Spiral:
			DisplayColor = FColor::Magenta;
			AttenuationStart = ControlPoint.RotationAttenuationStart;
			AttenuationEnd = ControlPoint.RotationAttenuationEnd;
			bHasSecondForce = true;
			AttenuationStart2 = ControlPoint.FadeAttenuationStart;
			AttenuationEnd2 = ControlPoint.FadeAttenuationEnd;
			bHasThirdForce = true;
			AttenuationStart3 = ControlPoint.RadialAttenuationStart;
			AttenuationEnd3 = ControlPoint.RadialAttenuationEnd;
			break;
		case EControlPointType::Curl:
			DisplayColor = FColor::Yellow;
			AttenuationStart = ControlPoint.CurlAttenuationStart;
			AttenuationEnd = ControlPoint.CurlAttenuationEnd;
			break;
		case EControlPointType::Warp:
			DisplayColor = FColor::Orange;
			AttenuationStart = ControlPoint.WarpAttenuationStart;
			AttenuationEnd = ControlPoint.WarpAttenuationEnd;
			break;
		default:
			DisplayColor = FColor::White;
			break;
		}

		if (ControlPoint.bDisplayControlPoint)
		{
			// DrawDebugSphere - High thickness setting for better visibility
			DrawDebugSphere(
				GetWorld(),
				Pos,
				SphereRadius,
				8,
				DisplayColor,
				false,
				0.0f,
				0,
				1000.0f
			);
		}
		
		if (!bIsGlobal
			&& PFAuroraPreset->bDisplayAttenuationRange
			&& ControlPoint.bDisplayAttenuationRange)
		{
			DrawDebugCircle(
				GetWorld(),
				Pos,
				AttenuationStart * Extent.X * 2.f,
				128,
				DisplayColor,
				false,
				-1.f,
				0,
				1000.0f,
				FVector(1, 0, 0),
				FVector(0, 1, 0),
				false
			);

			DrawDebugCircle(
				GetWorld(),
				Pos,
				AttenuationEnd * Extent.X * 2.f,
				128,
				DisplayColor,
				false,
				-1.f,
				0,
				1000.0f,
				FVector(1, 0, 0),
				FVector(0, 1, 0),
				false
			);

			if (bHasSecondForce)
			{
				DrawDebugCircle(
					GetWorld(),
					Pos,
					AttenuationStart2 * Extent.X * 2.f,
					128,
					FColor::Silver,
					false,
					-1.f,
					0,
					1000.0f,
					FVector(1, 0, 0),
					FVector(0, 1, 0),
					false
				);

				DrawDebugCircle(
					GetWorld(),
					Pos,
					AttenuationEnd2 * Extent.X * 2.f,
					128,
					FColor::Silver,
					false,
					-1.f,
					0,
					1000.0f,
					FVector(1, 0, 0),
					FVector(0, 1, 0),
					false
				);
			}

			if (bHasThirdForce)
			{
				DrawDebugCircle(
				GetWorld(),
				Pos,
				AttenuationStart3 * Extent.X * 2.f,
				128,
				FColor::Turquoise,
				false,
				-1.f,
				0,
				1000.0f,
				FVector(1, 0, 0),
				FVector(0, 1, 0),
				false
			);

				DrawDebugCircle(
					GetWorld(),
					Pos,
					AttenuationEnd3 * Extent.X * 2.f,
					128,
					FColor::Turquoise,
					false,
					-1.f,
					0,
					1000.0f,
					FVector(1, 0, 0),
					FVector(0, 1, 0),
					false
				);
			}
		}
	}
}

/**
* @brief Initialize render target for texture painting
*
* Creates a 2048x2048 RGBA8 render target if it doesn't exist.
* This render target will be used as the paint canvas.
*/
void AVolumetricAurora::InitializeElementsRenderTarget()
{
	if (ElementsRenderTarget)
	{
		return;	// Already initialized
	}

	// Create new render target object
	ElementsRenderTarget = NewObject<UTextureRenderTarget2D>(
		this,
		UTextureRenderTarget2D::StaticClass(),
		NAME_None,
		RF_Transient
	);

	if (ElementsRenderTarget)
	{
		ElementsRenderTarget->InitAutoFormat(2048, 2048);
		ElementsRenderTarget->AddressX = TA_Clamp;
		ElementsRenderTarget->AddressY = TA_Clamp;
		ElementsRenderTarget->Filter = TF_Bilinear;
		ElementsRenderTarget->ClearColor = FLinearColor::Black;
		ElementsRenderTarget->UpdateResourceImmediate(true);

		// Copy existing AuroraElementsMap to ElementsRenderTarget
		if (UPotentialFlowAuroraPreset* FlowPreset = Cast<UPotentialFlowAuroraPreset>(TargetAurora))
		{
			if (FlowPreset->AuroraElementsMap)
			{
				UMaterialInstanceDynamic* CopyMaterial = CreateSimpleMaterialForTextureCopy(FlowPreset->AuroraElementsMap);
				if (CopyMaterial)
				{
					UKismetRenderingLibrary::DrawMaterialToRenderTarget(
						this,
						ElementsRenderTarget,
						CopyMaterial
					);
				}
			}
		}
	}

	// ========================================================================
	// Initialize Preview Capture Target
	// ========================================================================

	/**
	 * PreviewCaptureTarget: Receives output from SceneCapture2D
	 *
	 * Resolution choice:
	 * - 1024x1024: Good balance for UI preview
	 * - Lower (512): Faster but less detail
	 * - Higher (2048): Better quality but higher memory/performance cost
	 */
	if (!PreviewCaptureTarget)
	{
		PreviewCaptureTarget = NewObject<UTextureRenderTarget2D>(this);

		// InitAutoFormat: Automatically selects format based on platform
		// Usually picks RTF_RGBA8 (32-bit color) for preview purpose
		PreviewCaptureTarget->InitAutoFormat(1024, 1024);

		// UpdateResourceImmediate: Creates GPU resource immediately
		// Parameter true = flush render commands, ensures resource is ready
		PreviewCaptureTarget->UpdateResourceImmediate(true);

		UE_LOG(LogTemp, Log, TEXT("PreviewCaptureTarget initialized: 1024x1024"));
	}
}

UMaterialInstanceDynamic* AVolumetricAurora::CreateSimpleMaterialForTextureCopy(UTexture* SourceTexture)
{
	if (!SourceTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSimpleMaterialForTextureCopy: SourceTexture is null"));
		return nullptr;
	}

	// Try to load simple texture copy material from plugin
	FString MaterialPath = TEXT("/VolumetricAurora/Materials/M_Copy");
	UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, *MaterialPath);

	if (!BaseMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSimpleMaterialForTextureCopy: Failed to load material at %s"), *MaterialPath);
		UE_LOG(LogTemp, Error, TEXT("Please create M_Copy material in VolumetricAurora/Content/Materials/"));
		UE_LOG(LogTemp, Error, TEXT("Material should have a TextureSampleParameter2D named 'SourceTexture' connected to Emissive Color"));
		return nullptr;
	}

	UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);

	if (DynMaterial)
	{
		DynMaterial->SetTextureParameterValue(FName("SourceTexture"), SourceTexture);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSimpleMaterialForTextureCopy: Failed to create MaterialInstanceDynamic"));
	}

	return DynMaterial;
}

// ============================================================================
// Preview Aurora System Implementation
// ============================================================================

AVolumetricAurora* AVolumetricAurora::CreatePreviewAurora()
{
	UE_LOG(LogTemp, Log, TEXT("CreatePreviewAurora: Starting..."));

	// === Step 1: Create Dedicated Preview World ===
	// Clean up existing preview if any
	if (PreviewAuroraActor.IsValid() || PreviewWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreatePreviewAurora: Cleaning up existing preview"));
		DestroyPreviewAurora();
	}

	// Create transient package for preview world to avoid name conflicts
	PreviewPackage = NewObject<UPackage>(nullptr, *FString::Printf(TEXT("/Temp/AuroraPreview_%d"), FMath::Rand()), RF_Transient);
	PreviewPackage->AddToRoot();

	// Create world with no name to avoid conflicts
	PreviewWorld = NewObject<UWorld>(PreviewPackage, NAME_None, RF_Transient);
	PreviewWorld->WorldType = EWorldType::EditorPreview;
	PreviewWorld->AddToRoot();

	// Create PersistentLevel manually to have full control
	PreviewWorld->PersistentLevel = NewObject<ULevel>(PreviewWorld, TEXT("PersistentLevel"), RF_Transient);
	PreviewWorld->PersistentLevel->OwningWorld = PreviewWorld;
	PreviewWorld->PersistentLevel->Model = NewObject<UModel>(PreviewWorld->PersistentLevel, NAME_None, RF_Transient);
	PreviewWorld->PersistentLevel->bIsVisible = true;

	// Add level to world
	PreviewWorld->AddLevel(PreviewWorld->PersistentLevel);

	// CRITICAL: Set current level before spawning any actors
	// Without this, SpawnActor will fail with "Assertion failed: CurrentLevel"
	PreviewWorld->SetCurrentLevel(PreviewWorld->PersistentLevel);

	// Create WorldSettings manually
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = PreviewWorld->PersistentLevel; // Explicitly specify level
	SpawnInfo.Name = TEXT("WorldSettings");
	AWorldSettings* WorldSettings = PreviewWorld->SpawnActor<AWorldSettings>(AWorldSettings::StaticClass(), SpawnInfo);

	if (!WorldSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewAurora: Failed to spawn WorldSettings"));
		return nullptr;
	}

	PreviewWorld->PersistentLevel->SetWorldSettings(WorldSettings);

	// Initialize world with proper values
	FWorldInitializationValues IVS;
	IVS.RequiresHitProxies(false);
	IVS.ShouldSimulatePhysics(false);
	IVS.EnableTraceCollision(false);
	IVS.CreateNavigation(false);
	IVS.CreateAISystem(false);
	IVS.AllowAudioPlayback(false);
	IVS.CreatePhysicsScene(true);

	PreviewWorld->InitWorld(IVS);

	UE_LOG(LogTemp, Log, TEXT("CreatePreviewAurora: Preview world created successfully in package: %s"), *PreviewPackage->GetName());

	// === Step 2: Validate TargetAurora ===
	if (!TargetAurora)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewAurora: TargetAurora is null"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("CreatePreviewAurora: TargetAurora type: %s"), *TargetAurora->GetClass()->GetName());

	// === Step 3: Define Preview Actor Transform ===
	FVector PreviewLocation(0.f, 0.f, 1000.f);
	FRotator PreviewRotation(0.f, 0.f, 0.f);

	// === Step 4: Spawn Preview Actor in Preview World ===
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AVolumetricAurora* PreviewActor = PreviewWorld->SpawnActor<AVolumetricAurora>(
		AVolumetricAurora::StaticClass(),
		PreviewLocation,
		PreviewRotation,
		SpawnParams
	);

	if (!PreviewActor)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewAurora: Failed to spawn preview actor in Preview World"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("CreatePreviewAurora: Preview actor spawned in Preview World: %s"), *PreviewActor->GetName());

	// === Step 5: Copy Preset Settings ===
	UAuroraPresetBase* DuplicatedPreset = DuplicateObject(TargetAurora, PreviewActor);
	if (!DuplicatedPreset)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewAurora: Failed to duplicate preset"));
		PreviewActor->Destroy();
		return nullptr;
	}

	PreviewActor->TargetAurora = DuplicatedPreset;

	UE_LOG(LogTemp, Log, TEXT("CreatePreviewAurora: Preset duplicated"));
	UE_LOG(LogTemp, Log, TEXT("  - Original TargetAurora: %p"), TargetAurora.Get());
	UE_LOG(LogTemp, Log, TEXT("  - Preview TargetAurora: %p"), PreviewActor->TargetAurora.Get());

	// === Step 6: Create separate RenderTargets for preview simulation ===
	// DuplicateObject only does shallow copy, so RenderTargets are shared with original
	// We need separate RenderTargets to prevent preview simulation from affecting original
	if (UPotentialFlowAuroraPreset* OriginalFlowPreset = Cast<UPotentialFlowAuroraPreset>(TargetAurora))
	{
		if (UPotentialFlowAuroraPreset* PreviewFlowPreset = Cast<UPotentialFlowAuroraPreset>(PreviewActor->TargetAurora))
		{
			// Get simulation resolution from original preset
			int32 SimResolution = GetResolutionValue(OriginalFlowPreset->SimulationResolution);

			// Create new RenderTargets for preview (prevents sharing with original)
			PreviewFlowPreset->FrontBuffer = NewObject<UTextureRenderTarget2D>(PreviewActor);
			PreviewFlowPreset->FrontBuffer->InitAutoFormat(SimResolution, SimResolution);
			PreviewFlowPreset->FrontBuffer->UpdateResourceImmediate(true);

			PreviewFlowPreset->BackBuffer = NewObject<UTextureRenderTarget2D>(PreviewActor);
			PreviewFlowPreset->BackBuffer->InitAutoFormat(SimResolution, SimResolution);
			PreviewFlowPreset->BackBuffer->UpdateResourceImmediate(true);

			PreviewFlowPreset->ObstacleMap = NewObject<UTextureRenderTarget2D>(PreviewActor);
			PreviewFlowPreset->ObstacleMap->InitAutoFormat(SimResolution, SimResolution);
			PreviewFlowPreset->ObstacleMap->UpdateResourceImmediate(true);

			PreviewFlowPreset->DisplayBuffer = nullptr; // Will be set to FrontBuffer after simulation

			UE_LOG(LogTemp, Log, TEXT("CreatePreviewAurora: Separate RenderTargets created for preview"));
			UE_LOG(LogTemp, Log, TEXT("  - Original FrontBuffer: %p, Preview FrontBuffer: %p"),
				OriginalFlowPreset->FrontBuffer, PreviewFlowPreset->FrontBuffer);
		}
	}

	// === Step 7: Connect ElementsRenderTarget ===
	// Share ElementsRenderTarget with preview actor
	if (UPotentialFlowAuroraPreset* OriginalFlowPreset = Cast<UPotentialFlowAuroraPreset>(TargetAurora))
	{
		if (UPotentialFlowAuroraPreset* PreviewFlowPreset = Cast<UPotentialFlowAuroraPreset>(PreviewActor->TargetAurora))
		{
			PreviewFlowPreset->AuroraElementsMap = OriginalFlowPreset->AuroraElementsMap;
			PreviewActor->bAuroraElementsMapDirty = true;
		}
	}

	// === Step 8: Material Instance Check ===
	if (!PreviewActor->AuroraMaterialDynamic && PreviewActor->VolumeBox)
	{
		PreviewActor->AuroraMaterialDynamic = PreviewActor->VolumeBox->CreateAndSetMaterialInstanceDynamic(0);
	}

	// === Step 9: Run initial simulation to setup DisplayBuffer ===
	PreviewActor->FlowTick(0.016f);

	// === Step 10: Update Material Parameters ===
	PreviewActor->UpdateMaterialTarget();

	// === Step 11: Configure Preview Actor ===
	PreviewActor->SetActorEnableCollision(false);

#if WITH_EDITORONLY_DATA
	if (PreviewActor->SpriteComponent)
	{
		PreviewActor->SpriteComponent->SetVisibility(false);
	}
#endif

	// Hide all SDFBoundVisualizerComponents (wireframe plane visualizers)
	TArray<UActorComponent*> BoundVisualizers;
	PreviewActor->GetComponents(UStaticMeshComponent::StaticClass(), BoundVisualizers);
	for (UActorComponent* Comp : BoundVisualizers)
	{
		if (Comp->GetClass()->GetName().Contains(TEXT("SDFBoundVisualizer")))
		{
			Cast<UStaticMeshComponent>(Comp)->SetVisibility(false);
			Cast<UStaticMeshComponent>(Comp)->SetHiddenInGame(true);
			Comp->SetActive(false);
		}
	}

	// === Step 12: Enable ticking for simulation ===
	PreviewActor->PrimaryActorTick.bCanEverTick = true;
	PreviewActor->PrimaryActorTick.bStartWithTickEnabled = true;
	PreviewActor->PrimaryActorTick.TickGroup = TG_PrePhysics;
	PreviewActor->SetActorTickEnabled(true);
	PreviewActor->RegisterAllActorTickFunctions(true, true);

	// === Step 13: Store Weak Reference ===
	PreviewAuroraActor = PreviewActor;

	return PreviewActor;
}

void AVolumetricAurora::CreatePreviewSceneCapture()
{
	// === Step 1: Validate Prerequisites ===
	if (!PreviewWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewSceneCapture: Preview world not available"));
		return;
	}

	if (!PreviewAuroraActor.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewSceneCapture: PreviewAuroraActor is invalid"));
		return;
	}

	if (!PreviewCaptureTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewSceneCapture: PreviewCaptureTarget is null"));
		return;
	}

	AVolumetricAurora* PreviewActor = PreviewAuroraActor.Get();

	// === Step 2: Create Scene Capture Component ===
	// CRITICAL: Outer must be in same World as PreviewActor for capture to work
	// Use PreviewActor as Outer instead of 'this' (original actor may be in different world)
	PreviewSceneCapture = NewObject<USceneCaptureComponent2D>(PreviewActor);

	if (!PreviewSceneCapture)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewSceneCapture: Failed to create component"));
		return;
	}

	// RegisterComponentWithWorld: Adds component to world's component list
	// Required for component to function (tick, render, etc.)
	// DO NOT Attach to Actor - keep it independent to avoid affecting Preview Actor rendering
	PreviewSceneCapture->RegisterComponentWithWorld(PreviewWorld);

	// === Step 3: Configure Capture Settings ===

	// TextureTarget: Output destination for captured pixels
	PreviewSceneCapture->TextureTarget = PreviewCaptureTarget;

	// CaptureSource: What stage of rendering pipeline to capture
	// SCS_FinalColorLDR: Post-processed, tonemapped final image (what you see on screen)
	// Other options: SCS_SceneColorHDR (before tonemapping), SCS_BakeColor, etc.
	PreviewSceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	// Capture every frame for real-time preview
	PreviewSceneCapture->bCaptureEveryFrame = true;
	PreviewSceneCapture->bCaptureOnMovement = false;

	// === Step 4: Set Camera Transform ===
	PreviewActor = PreviewAuroraActor.Get();
	FVector VolumeBoxLocation = PreviewActor->VolumeBox->GetComponentLocation();
	FVector VolumeBoxScale = PreviewActor->VolumeBox->GetComponentScale();
	FVector CameraLocation = VolumeBoxLocation;
	CameraLocation.Z += VolumeBoxScale.Z * 0.5f;
	CameraLocation.Z += VolumeBoxScale.X * 60.f;
	FRotator CameraRotation(-90.f, -90.f, 0.f);

	PreviewSceneCapture->SetWorldLocation(CameraLocation);
	PreviewSceneCapture->SetWorldRotation(CameraRotation);

	// === Perspective Projection Settings ===
	PreviewSceneCapture->ProjectionType = ECameraProjectionMode::Perspective;
	PreviewSceneCapture->FOVAngle = 90.f;

	// === Step 5: Render Mode ===
	PreviewSceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	PreviewSceneCapture->ShowOnlyActors.Empty();
	PreviewSceneCapture->ShowOnlyComponents.Empty();

	// === Step 6: Configure ShowFlags ===
	PreviewSceneCapture->ShowFlags.SetAtmosphere(true);
	PreviewSceneCapture->ShowFlags.SetFog(true);
	PreviewSceneCapture->ShowFlags.SetVolumetricFog(true);
	PreviewSceneCapture->ShowFlags.SetTranslucency(true);
	PreviewSceneCapture->ShowFlags.SetPostProcessing(true);
}

void AVolumetricAurora::UpdatePreviewAurora()
{
	if (!PreviewAuroraActor.IsValid() || !PreviewWorld)
	{
		return;
	}

	AVolumetricAurora* PreviewPtr = PreviewAuroraActor.Get();

	// Update ElementsMap Reference
	if (UPotentialFlowAuroraPreset* FlowPreset = Cast<UPotentialFlowAuroraPreset>(PreviewPtr->TargetAurora))
	{
		FlowPreset->AuroraElementsMap = ElementsRenderTarget;
		PreviewPtr->bAuroraElementsMapDirty = true;
	}

	// Manually tick the preview actor
	// EditorPreview worlds don't auto-tick, so we directly call actor's Tick()
	constexpr float DeltaTime = 0.016f; // ~60 FPS
	if (PreviewPtr->GetClass()->HasAnyClassFlags(CLASS_NeedsDeferredDependencyLoading))
	{
		PreviewPtr->GetClass()->GetDefaultObject();
	}
	PreviewPtr->Tick(DeltaTime);

	// Update world time for material parameters and effects
	PreviewWorld->TimeSeconds += DeltaTime;
	PreviewWorld->RealTimeSeconds += DeltaTime;

	// Capture Scene
	if (PreviewSceneCapture)
	{
		PreviewSceneCapture->CaptureScene();
	}
}

void AVolumetricAurora::DestroyPreviewAurora()
{
	// Step 1: Destroy Scene Capture Component
	if (PreviewSceneCapture)
	{
		PreviewSceneCapture->DestroyComponent();
		PreviewSceneCapture = nullptr;
	}

	// Step 2: Destroy Preview Actor
	if (PreviewAuroraActor.IsValid())
	{
		PreviewAuroraActor.Get()->Destroy();
	}
	PreviewAuroraActor.Reset();

	// Step 3: Destroy Preview World
	if (PreviewWorld)
	{
		// Remove from root to allow garbage collection
		PreviewWorld->RemoveFromRoot();

		// Destroy the world and all its contents
		PreviewWorld->CleanupWorld();

		PreviewWorld = nullptr;

		UE_LOG(LogTemp, Log, TEXT("DestroyPreviewAurora: Preview world destroyed"));
	}

	// Step 4: Destroy Preview Package
	if (PreviewPackage)
	{
		PreviewPackage->RemoveFromRoot();
		PreviewPackage->ClearFlags(RF_Standalone);
		PreviewPackage = nullptr;

		UE_LOG(LogTemp, Log, TEXT("DestroyPreviewAurora: Preview package destroyed"));
	}
}

#endif

void AVolumetricAurora::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bAuroraPlaying)
	{
		AuroraAccumulatedTime += DeltaTime * AuroraTimeScale;
	}

	UpdateMaterialTimeParameter();

	if (TargetAurora && TargetAurora->GetClass() == UPotentialFlowAuroraPreset::StaticClass() && AuroraMaterialDynamic)
	{
		FlowTick(DeltaTime);
		TargetAurora->UpdateMaterial(AuroraMaterialDynamic);
	}
}

void AVolumetricAurora::UpdateMaterialTarget()
{
	if (!TargetAurora || !AuroraMaterialDynamic)
	{
		return;
	}

	TargetAurora->UpdateMaterial(AuroraMaterialDynamic);
	VolumeBox->SetRelativeLocation(FVector(0.0f, 0.0f, TargetAurora->Altitude * 100000.0f));
	VolumeBox->SetWorldRotation(FQuat::Identity);
	VolumeBox->SetRelativeScale3D(FVector(TargetAurora->AuroraAreaExtent, TargetAurora->AuroraAreaExtent, TargetAurora->AuroraHeight) * 1000.0f);
}

void AVolumetricAurora::UpdateMaterialTimeParameter()
{
	if (!AuroraMaterialDynamic)
	{
		return;
	}

	AuroraMaterialDynamic->SetScalarParameterValue(
		TEXT("AuroraTime"),
		AuroraAccumulatedTime
	);
}

#if WITH_EDITOR

void AVolumetricAurora::ResetFlowSimulation()
{
	bForceResetSimulation = true;
}

#endif

void AVolumetricAurora::ApplyPresetToTarget(UAuroraPresetBase* InPreset)
{
	if (!InPreset)	return;

	Modify(); // Undo / Redo

	TargetAurora = DuplicateObject<UAuroraPresetBase>(InPreset, this);
	SourcePreset = InPreset;
	if (TargetAurora)
	{
		TargetAurora->ClearFlags(RF_Standalone);
		TargetAurora->SetFlags(RF_Transactional);

		if (VolumeBox)
		{
			AuroraMaterialDynamic = VolumeBox->CreateAndSetMaterialInstanceDynamic(0);
		}

		UpdateMaterialTarget();
		UpdateMaterialTimeParameter();
	}

	bControlPointsDirty = true;
	bAuroraElementsMapDirty = true;
}

FString AVolumetricAurora::GetPluginPath()
{
	return PluginPath;
}

