// Copyright (c) 2026 R&B. All rights reserved.

#include "Components/SplineDFTextureBakerComponent.h"

#include "Actors/VolumetricAurora.h"
#include "Components/DFBoundVisualizerComponent.h"
#include "Components/AuroraSplineComponent.h"
#include "Selection.h"
#include "FileHelpers.h"
#include "Components/SplineComponent.h"
#include "RenderTargetPool.h"
#include "RenderGraphUtils.h"
#include "ComputeShaders/AuroraDFBakeCS.h"
#include "Engine/TextureRenderTarget2D.h"

#include "EditorViewportClient.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"


// Sets default values for this component's properties
USplineDFTextureBakerComponent::USplineDFTextureBakerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	bIsEditorOnly = true;

	// ...
}

void USplineDFTextureBakerComponent::FocusOnVisualizer()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<IAssetViewport> ActiveViewport = LevelEditorModule.GetFirstActiveViewport();
	
	if (ActiveViewport.IsValid())
	{
		FEditorViewportClient& ViewportClient = ActiveViewport->GetAssetViewportClient();

		FVector TargetLocation = BoundsVisualizer->GetComponentLocation();
		FVector CameraLocation = FVector(0.0f, 0.0f, -50000.0f);

		ViewportClient.SetViewLocation(TargetLocation + CameraLocation);
		
		ViewportClient.SetViewRotation(FVector(0.0f, 0.0f, 1.0f).Rotation());

		ViewportClient.Invalidate();
		
	}

}

void USplineDFTextureBakerComponent::AddSplineComponent()
{
	AActor* Owner = GetOwner();

	if (!Owner) return;

	FScopedTransaction Transaction(FText::FromString("New Spline Component"));

	Owner->Modify();

	USplineComponent* NewSpline = NewObject<UAuroraSplineComponent>(Owner, UAuroraSplineComponent::StaticClass(), MakeUniqueObjectName(Owner, UAuroraSplineComponent::StaticClass()), RF_Transactional);

	if (NewSpline)
	{


		NewSpline->CreationMethod = EComponentCreationMethod::Instance;

		NewSpline->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);

		NewSpline->OnComponentCreated();

		NewSpline->RegisterComponent();

		NewSpline->bIsEditorOnly = true;

		NewSpline->SetWorldScale3D(FVector(250.0, 250.0, 1.0));

		Owner->AddInstanceComponent(NewSpline);

		NewSpline->Modify();

		Owner->PostEditChange();

		if (GEditor)
		{
			GEditor->SelectComponent(NewSpline, true, true);
		}
	}
}

void USplineDFTextureBakerComponent::MakeDFTexture(const FString& NewTextureName)
{
	TArray<USplineComponent*> SplineComponents;

	TArray<USceneComponent*> Children;
	GetChildrenComponents(true, Children);

	for (USceneComponent* Child : Children)
	{
		if (USplineComponent* SplineComponent = Cast<USplineComponent>(Child))
		{
			SplineComponents.Add(SplineComponent);
		}
	}

	int32 SplineStartIndex = 0;
	TArray<FVector4f> SplinePoints;
	TArray<FAuroraSplineInfo> SplineInfos;
	SplinePoints.Reserve(100);
	for (USplineComponent* SplineComponent : SplineComponents)
	{
		int32 SplinePointCount = SplineComponent->GetNumberOfSplinePoints();
		FAuroraSplineInfo SplineInfo;
		SplineInfo.PointCount = (SplinePointCount - 1) * 3 + 1;
		SplineInfo.StartIndex = SplineStartIndex;
		SplineStartIndex += SplineInfo.PointCount;
		SplineInfos.Add(SplineInfo);

		for (int32 Index = 0; Index < SplinePointCount - 1; Index++)
		{
			FVector Location0 = SplineComponent->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
			SplinePoints.Add(FVector4f(Location0.X, Location0.Y, Location0.Z, 10.0f));
			FVector LeaveTangent = SplineComponent->GetLeaveTangentAtSplinePoint(Index, ESplineCoordinateSpace::World);
			FVector Location1 = Location0 + (LeaveTangent / 3.0);
			SplinePoints.Add(FVector4f(Location1.X, Location1.Y, Location1.Z, 10.0f));

			FVector Location3 = SplineComponent->GetLocationAtSplinePoint(Index + 1, ESplineCoordinateSpace::World);
			FVector ArriveTangent = SplineComponent->GetArriveTangentAtSplinePoint(Index + 1, ESplineCoordinateSpace::World);
			FVector Location2 = Location3 - (ArriveTangent / 3.0);
			SplinePoints.Add(FVector4f(Location2.X, Location2.Y, Location2.Z, 10.0f));
		}
		FVector LastLocation = SplineComponent->GetLocationAtSplinePoint(SplinePointCount - 1, ESplineCoordinateSpace::World);
		SplinePoints.Add(FVector4f(LastLocation.X, LastLocation.Y, LastLocation.Z, 10.0f));
	}

	DispatchDFBakeCS(SplinePoints, SplineInfos, NewTextureName);
}


void USplineDFTextureBakerComponent::DispatchDFBakeCS(const TArray<FVector4f>& SplinePoints, const TArray<FAuroraSplineInfo>& SplineInfos, const FString& NewTextureName)
{

	AVolumetricAurora* Owner = Cast<AVolumetricAurora>(GetOwner());

	if (!Owner)
	{
		return;
	}
	UTextureRenderTarget2D* DFTextureRT = NewObject<UTextureRenderTarget2D>(this, NAME_None, RF_Transient);

	DFTextureRT->RenderTargetFormat = RTF_R16f;
	DFTextureRT->ClearColor = FLinearColor::Black;
	DFTextureRT->bAutoGenerateMips = false;

	DFTextureRT->AddressX = TA_Clamp;
	DFTextureRT->AddressY = TA_Clamp;

	DFTextureRT->bCanCreateUAV = true;


	DFTextureRT->InitCustomFormat(512, 512, PF_R16F, true);

	DFTextureRT->UpdateResource();

	if (!DFTextureRT)
	{
		return;
	}

	FTextureRenderTargetResource* RTResource = DFTextureRT->GameThread_GetRenderTargetResource();

	ENQUEUE_RENDER_COMMAND(BakeAuroraDF) (
		[RTResource, SplinePoints, SplineInfos, CenterPosition = this->GetComponentLocation(), MapSize = this->MapSize](FRHICommandListImmediate& RHICmdList)
		{
			
			FRDGBuilder GraphBuilder(RHICmdList);
			FRHITexture* TextureRHI = RTResource->GetRenderTargetTexture();
		
			FRDGTextureRef RDGTexture = GraphBuilder.RegisterExternalTexture(
				CreateRenderTarget(TextureRHI, TEXT("AuroraTexture"))
			);
		
			FRDGTextureUAVRef DFUAV = GraphBuilder.CreateUAV(RDGTexture);
		
			FRDGBufferRef PointBuffer = CreateStructuredBuffer(
				GraphBuilder, TEXT("AuroraPointBuffer"), sizeof(FVector4f),
				SplinePoints.Num(), SplinePoints.GetData(), sizeof(FVector4f) * SplinePoints.Num()
			);
			FRDGBufferRef SplineBuffer = CreateStructuredBuffer(
				GraphBuilder, TEXT("AuroraSplineBuffer"), sizeof(FAuroraSplineInfo),
				SplineInfos.Num(), SplineInfos.GetData(), sizeof(FAuroraSplineInfo) * SplineInfos.Num()
			);
	
			auto* PassParameters = GraphBuilder.AllocParameters<FAuroraDFBakeCS::FParameters>();
			PassParameters->InPoints = GraphBuilder.CreateSRV(PointBuffer);
			PassParameters->InSplines = GraphBuilder.CreateSRV(SplineBuffer);
			PassParameters->InSplineCount = SplineInfos.Num();
			PassParameters->OutDF = DFUAV;
			PassParameters->InMapSize = MapSize;

			FVector2f SnapPosition;
			float PixelWorldSize = MapSize / RTResource->GetSizeX();
			SnapPosition.X = FMath::Floor(CenterPosition.X / PixelWorldSize) * PixelWorldSize;
			SnapPosition.Y = FMath::Floor(CenterPosition.Y / PixelWorldSize) * PixelWorldSize;
			PassParameters->InMapCenter = SnapPosition;

			TShaderMapRef<FAuroraDFBakeCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("BakeAuroraDF"),
				ComputeShader,
				PassParameters,
				FIntVector(FMath::DivideAndRoundUp(RTResource->GetSizeX(), (uint32)8), FMath::DivideAndRoundUp(RTResource->GetSizeY(), (uint32)8), 1)
			);
		
			GraphBuilder.Execute();
		}
		);



	FlushRenderingCommands();

	FString TexturePath = Owner->GetPluginPath() + "/Textures/DFTextures/" + NewTextureName;

	UTexture2D* TargetTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath, nullptr, LOAD_NoWarn | LOAD_Quiet));

	UPackage* Package = nullptr;

	if (TargetTexture)
	{
		FString AbsolutePath = FPackageName::LongPackageNameToFilename(TexturePath, FPackageName::GetAssetPackageExtension());
		if (IPlatformFile::GetPlatformPhysical().IsReadOnly(*AbsolutePath))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Cannot overwrite the default Texture\n\nPlease save as different name")), FText::FromString(TEXT("Attempt to overwrite default texture")));
			return;
		}
		Package = TargetTexture->GetPackage();
	}
	else
	{
		Package = CreatePackage(*TexturePath);
		if (!Package)
		{
			return;
		}
		Package->FullyLoad();
	}

	UTexture2D* NewTexture = DFTextureRT->ConstructTexture2D(Package, NewTextureName, RF_Public | RF_Standalone, CTF_Default);

	if (NewTexture)
	{
		NewTexture->Modify();

		NewTexture->CompressionSettings = TextureCompressionSettings::TC_HalfFloat;
		NewTexture->CompressionNone = true;
		NewTexture->SRGB = false;
		NewTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

		NewTexture->AddressX = TA_Clamp;
		NewTexture->AddressY = TA_Clamp;



		NewTexture->PostEditChange();

		NewTexture->UpdateResource();

		Package->MarkPackageDirty();


		TArray<UPackage*> Packages;
		Packages.Add(Package);
		UEditorLoadingAndSavingUtils::SavePackages(Packages, false);
	}
}

// Called when the game starts
void USplineDFTextureBakerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void USplineDFTextureBakerComponent::OnRegister()
{
	Super::OnRegister();
	if (GEditor)
	{
		GEditor->GetSelectedActors()->SelectionChangedEvent.AddUObject(this, &USplineDFTextureBakerComponent::OnSelectionChanged);
		GEditor->GetSelectedComponents()->SelectionChangedEvent.AddUObject(this, &USplineDFTextureBakerComponent::OnSelectionChanged);
	}

	if (!BoundsVisualizer)
	{
		BoundsVisualizer = NewObject<UDFBoundVisualizerComponent>(GetOwner(), TEXT("BoundsVisualizer"));

		if (BoundsVisualizer)
		{
			BoundsVisualizer->SetupAttachment(this);
			BoundsVisualizer->RegisterComponent();

			UStaticMesh* PlaneMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/VolumetricAurora/Meshes/Plane")));
			if (PlaneMesh)
			{
				BoundsVisualizer->SetStaticMesh(PlaneMesh);
			}
			BoundsVisualizer->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
			BoundsVisualizer->SetCastShadow(false);
			SetHiddenInGame(true);

			UMaterial* WireframeMat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, TEXT("/VolumetricAurora/Materials/DFVisualizer/WireframeMaterial")));
			if (WireframeMat)
			{
				BoundsVisualizer->SetMaterial(0, WireframeMat);
			}

			SetBoundsVisualizerTransform();
		}
	}
}

void USplineDFTextureBakerComponent::OnUnregister()
{
	Super::OnUnregister();
	if (GEditor)
	{
		GEditor->GetSelectedActors()->SelectionChangedEvent.RemoveAll(this);
		GEditor->GetSelectedComponents()->SelectionChangedEvent.RemoveAll(this);
	}
}

void USplineDFTextureBakerComponent::OnSelectionChanged(UObject* Selected)
{
	UpdateVisualizerState();
}

void USplineDFTextureBakerComponent::UpdateVisualizerState()
{
	if (BoundsVisualizer)
	{
		bool bShouldShow = IsSelected() || bAlwaysShowDebugLine;

		if (!bShouldShow)
		{
			TArray<USceneComponent*> Children;
			GetChildrenComponents(true, Children);
			for (auto* Child : Children)
			{
				if (Child->IsSelected())
				{
					bShouldShow = true;
					break;
				}
			}
		}

		if (BoundsVisualizer->GetVisibleFlag() != bShouldShow)
		{
			BoundsVisualizer->SetVisibility(bShouldShow);
		}

		TArray<USplineComponent*> SplineComponents;

		TArray<USceneComponent*> Children;
		GetChildrenComponents(true, Children);

		for (USceneComponent* Child : Children)
		{
			if (USplineComponent* SplineComponent = Cast<USplineComponent>(Child))
			{
				SplineComponent->SetDrawDebug(bShouldShow);
			}
		}
	}
}

void USplineDFTextureBakerComponent::PostEditComponentMove(bool bFinished)
{
	Super::PostEditComponentMove(bFinished);

	SetBoundsVisualizerTransform();
}

void USplineDFTextureBakerComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SetBoundsVisualizerTransform();

	UpdateVisualizerState();
}

void USplineDFTextureBakerComponent::SetBoundsVisualizerTransform()
{
	if (BoundsVisualizer)
	{
		FTransform BakerTransform = this->GetComponentTransform();
		BakerTransform.SetScale3D(FVector(MapSize / 100.0f, MapSize / 100.0f, 1));
		BakerTransform.SetRotation(FQuat::Identity);
		BoundsVisualizer->SetWorldTransform(BakerTransform);
	}
}


// Called every frame
void USplineDFTextureBakerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

