// Copyright (c) 2026 R&B. All rights reserved.

#include "Data/AuroraPresetAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/SavePackage.h"

#if WITH_EDITOR
#include "Kismet/KismetRenderingLibrary.h"
#include "AssetToolsModule.h"
#include "PackageTools.h"
#include "Editor.h"
#endif

#if WITH_EDITOR
void UAuroraPresetBase::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Tail: the actual changed leaf property (e.g., X / Y)
	auto* TailNode = PropertyChangedEvent.PropertyChain.GetTail();
	FProperty* LeafProp = TailNode ? TailNode->GetValue() : nullptr;
	if (!LeafProp) return;

	// Walk the entire chain and check whether MidColorHeight is part of it
	bool bMidColorHeightInChain = false;

	for (auto* Node = PropertyChangedEvent.PropertyChain.GetHead();
		Node != nullptr;
		Node = Node->GetNextNode())
	{
		FProperty* P = Node->GetValue();
		if (!P) continue;

		if (P->GetFName() == GET_MEMBER_NAME_CHECKED(UAuroraPresetBase, MidColorHeight))
		{
			bMidColorHeightInChain = true;
			break;
		}
	}

	if (!bMidColorHeightInChain)
		return;

	// Clamp to [0, 1] and enforce X <= Y
	FVector2f& H = MidColorHeight;
	H.X = FMath::Clamp(H.X, 0.0f, 1.0f);
	H.Y = FMath::Clamp(H.Y, 0.0f, 1.0f);
	if (H.X > H.Y) H.X = H.Y;
}
#endif

bool UAuroraPresetBase::IsIdentical(UAuroraPresetBase* Other)
{
	if (Other && Other->GetClass() == this->GetClass())
	{
		for (TFieldIterator<FProperty> It(Other->GetClass()); It; ++It)
		{
			FProperty* Property = *It;

			if (Property->HasAnyPropertyFlags(CPF_Edit) && !Property->Identical_InContainer(this, Other))
			{
				return false;
			}
		}

	}
	return true;
}

void UAuroraPresetBase::CopyFrom(UAuroraPresetBase* Source)
{
	if (Source && Source->GetClass() == this->GetClass())
	{
		for (TFieldIterator<FProperty> It(Source->GetClass()); It; ++It)
		{
			FProperty* Property = *It;

			if (Property->HasAnyPropertyFlags(CPF_Edit))
			{
				void* DestPtr = Property->ContainerPtrToValuePtr<void>(this);
				void* SrcPtr = Property->ContainerPtrToValuePtr<void>(Source);

				Property->CopyCompleteValue(DestPtr, SrcPtr);
			}
		}

	}
}

void UAuroraPresetBase::UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance)
{
	MaterialInstance->SetTextureParameterValue(TEXT("ShapeTexture"), ShapeTexture);
	MaterialInstance->SetScalarParameterValue(TEXT("Intensity"), Intensity);
	MaterialInstance->SetScalarParameterValue(TEXT("Density"), Density);

	MaterialInstance->SetVectorParameterValue(TEXT("TopColor"), TopColor);
	MaterialInstance->SetVectorParameterValue(TEXT("MidColor"), MidColor);
	MaterialInstance->SetVectorParameterValue(TEXT("BottomColor"), BottomColor);
	MaterialInstance->SetVectorParameterValue(TEXT("MidColorHeight"), FVector3f(MidColorHeight, 0.0f));

	MaterialInstance->SetScalarParameterValue(TEXT("EdgeFadeMode"), static_cast<float>(EdgeFadeMode));
	MaterialInstance->SetScalarParameterValue(TEXT("EdgeFadeSoftness"), EdgeFadeSoftness);
	MaterialInstance->SetScalarParameterValue(TEXT("HeightFalloff"), HeightFalloff);

	MaterialInstance->SetScalarParameterValue(TEXT("bEnableFilmGrain"), bEnableFilmGrain);
	MaterialInstance->SetScalarParameterValue(TEXT("FilmGrainIntensity"), FilmGrainIntensity);

	MaterialInstance->SetScalarParameterValue(TEXT("bUseStraightAlphaPalette"), bUseStraightAlphaPalette);
	MaterialInstance->SetScalarParameterValue(TEXT("ColorShiftStrength"), ColorShiftStrength);

	MaterialInstance->SetScalarParameterValue(TEXT("bEnableStructureColoring"), bEnableStructureColoring);
	MaterialInstance->SetVectorParameterValue(TEXT("SoftTint"), SoftTint);
	MaterialInstance->SetVectorParameterValue(TEXT("CoreTint"), CoreTint);

	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveTonePivot"), EmissiveTonePivot);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveContrast"), EmissiveContrast);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveSaturation"), EmissiveSaturation);

	MaterialInstance->SetScalarParameterValue(TEXT("bEnableEmissiveStructure"), bEnableEmissiveStructure);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveStructurePivot"), EmissiveStructurePivot);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveStructureSelectivity"), EmissiveStructureSelectivity);
}

UNoiseAuroraPreset::UNoiseAuroraPreset()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultDF(TEXT("/VolumetricAurora/Textures/SimplexNoise/simplex_seamless_0_01"));

	if (DefaultDF.Succeeded())
	{
		ShapeTexture = DefaultDF.Object.Get();
	}
}

void UNoiseAuroraPreset::UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance)
{
	Super::UpdateMaterial(MaterialInstance);

	MaterialInstance->SetScalarParameterValue(TEXT("AuroraType"), 0);
	
	MaterialInstance->SetScalarParameterValue(TEXT("Smoothness"), Smoothness);
	MaterialInstance->SetScalarParameterValue(TEXT("Speed"), Speed);

	MaterialInstance->SetScalarParameterValue(TEXT("ShapeFrequency"), ShapeFrequency);
	MaterialInstance->SetVectorParameterValue(TEXT("ScrollVelocity"), FVector3f(ScrollVelocity, 0.0f));

	MaterialInstance->SetScalarParameterValue(TEXT("MaskFrequency"), MaskFrequency);
	MaterialInstance->SetVectorParameterValue(TEXT("MaskScrollVelocity"), FVector(MaskScrollVelocity, 0.0f));
	MaterialInstance->SetScalarParameterValue(TEXT("MaskOpacity"), MaskOpacity);
}

USplineAuroraPreset::USplineAuroraPreset()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultDF(TEXT("/VolumetricAurora/Textures/DFTextures/DFDefault"));

	if (DefaultDF.Succeeded())
	{
		ShapeTexture = DefaultDF.Object.Get();
	}
}

void USplineAuroraPreset::UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance)
{
	Super::UpdateMaterial(MaterialInstance);

	MaterialInstance->SetScalarParameterValue(TEXT("AuroraType"), 1);
	MaterialInstance->SetScalarParameterValue(TEXT("Thickness"), Thickness);
	MaterialInstance->SetScalarParameterValue(TEXT("Distortion"), Distortion);
	MaterialInstance->SetScalarParameterValue(TEXT("Speed"), Speed);
	MaterialInstance->SetScalarParameterValue(TEXT("DistortionHeightFalloff"), DistortionHeightFalloff);
}

#if WITH_EDITOR
void UPotentialFlowAuroraPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Null check
	if (!PropertyChangedEvent.Property) return;

	FName PropertyName = PropertyChangedEvent.Property->GetFName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPotentialFlowAuroraPreset, bDisplayControlPoints))
	{
		if (!bDisplayControlPoints) bDisplayAttenuationRange = false;
	}
}

void UPotentialFlowAuroraPreset::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// ============================================================================
	// How PostEditChangeChainProperty works with nested struct arrays:
	// ============================================================================
	// When editing a preset through a component reference (e.g., TargetAurora->ControlPoints[2].RadialAttenuationStart),
	// the PropertyChain contains the full path including the reference:
	//
	//   Head -> "TargetAurora" (UPotentialFlowAuroraPreset* reference in component)
	//        -> "ControlPoints" (TArray<FAuroraFlowElement>)
	//        -> "RadialAttenuationStart" (float) <- Tail (leaf property)
	//
	// Since the chain may start with the reference property (not ControlPoints),
	// we must walk the entire chain to find ControlPoints.
	//
	// GetArrayIndex(PropertyName): Searches the chain for an array property with
	// the given name and returns the index of the element being edited.
	// ============================================================================

	// Walk the chain to check if ControlPoints is being edited
	bool bControlPointsInChain = false;
	for (auto* Node = PropertyChangedEvent.PropertyChain.GetHead(); Node; Node = Node->GetNextNode())
	{
		FProperty* Prop = Node->GetValue();
		if (Prop && Prop->GetFName() == GET_MEMBER_NAME_CHECKED(UPotentialFlowAuroraPreset, ControlPoints))
		{
			bControlPointsInChain = true;
			break;
		}
	}

	if (!bControlPointsInChain)
	{
		return;
	}

	// Get the array index of the modified ControlPoints element
	// Returns INDEX_NONE (-1) if not editing an array element (e.g., adding/removing elements)
	const int32 ArrayIndex = PropertyChangedEvent.GetArrayIndex(TEXT("ControlPoints"));
	if (ArrayIndex == INDEX_NONE || !ControlPoints.IsValidIndex(ArrayIndex))
	{
		return;
	}

	FAuroraFlowElement& Element = ControlPoints[ArrayIndex];

	if (!Element.bDisplayControlPoint)
	{
		Element.bDisplayAttenuationRange = false;
	}
	
	// ============================================================================
	// Enforce AttenuationStart <= AttenuationEnd for all attenuation pairs
	// ============================================================================
	// Each control point type has its own attenuation parameters.
	// We clamp Start to never exceed End, preserving the user's End value.
	// This ensures valid interpolation range for distance-based falloff.
	// ============================================================================

	// Radial attenuation (Source, Sink, Spiral)
	if (Element.RadialAttenuationStart > Element.RadialAttenuationEnd)
	{
		Element.RadialAttenuationStart = Element.RadialAttenuationEnd;
	}

	// Emission attenuation (Source, Emitter)
	if (Element.EmissionAttenuationStart > Element.EmissionAttenuationEnd)
	{
		Element.EmissionAttenuationStart = Element.EmissionAttenuationEnd;
	}

	// Rotation attenuation (Vortex, Spiral)
	if (Element.RotationAttenuationStart > Element.RotationAttenuationEnd)
	{
		Element.RotationAttenuationStart = Element.RotationAttenuationEnd;
	}

	// Fade attenuation (Sink, Vortex, Spiral, Attenuator)
	if (Element.FadeAttenuationStart > Element.FadeAttenuationEnd)
	{
		Element.FadeAttenuationStart = Element.FadeAttenuationEnd;
	}

	// Dipole attenuation
	if (Element.DipoleAttenuationStart > Element.DipoleAttenuationEnd)
	{
		Element.DipoleAttenuationStart = Element.DipoleAttenuationEnd;
	}

	// Curl attenuation
	if (Element.CurlAttenuationStart > Element.CurlAttenuationEnd)
	{
		Element.CurlAttenuationStart = Element.CurlAttenuationEnd;
	}
}
#endif

void UPotentialFlowAuroraPreset::UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance)
{
	Super::UpdateMaterial(MaterialInstance);

	MaterialInstance->SetScalarParameterValue(TEXT("AuroraType"), 2);

	if (DisplayBuffer)
	{
		MaterialInstance->SetTextureParameterValue(TEXT("ShapeTexture"), Cast<UTexture>(DisplayBuffer));
	}
}

#if WITH_EDITOR
void UPotentialFlowAuroraPreset::CaptureSimulationCheckpoint(FString TargetAuroraName, float CurrentSimulationTime)
{
	// Validate render target exists
	if (!FrontBuffer)
	{
		UE_LOG(LogTemp, Warning, TEXT("CaptureSimulationCheckpoint failed: FrontBuffer is null"));
		return;
	}

	// Get dimensions from source RenderTarget
	int32 Width = FrontBuffer->SizeX;
	int32 Height = FrontBuffer->SizeY;
	
	// Define checkpoint texture save path
	FString PackageName = FString::Printf(
		TEXT("/VolumetricAurora/Textures/FlowCheckpoints/CheckpointTexture_%s_%s"),
		*TargetAuroraName,
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))
	);
	FString TextureName = FPaths::GetBaseFilename(PackageName);

	// Create package
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package"));
		return;
	}
	Package->FullyLoad();

	// Create UTexture2D with HDR format for FloatRGBA data
	UTexture2D* NewTexture = NewObject<UTexture2D>(
		Package,
		*TextureName,
		RF_Public | RF_Standalone | RF_MarkAsRootSet
	);

	if (!NewTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UTexture2D"));
		return;
	}

	// Configure texture properties for HDR float data
	NewTexture->SRGB = false;
	NewTexture->CompressionSettings = TC_HDR;
	NewTexture->MipGenSettings = TMGS_NoMipmaps;

	// Read float data from RenderTarget
	FTextureRenderTargetResource* RTResource = FrontBuffer->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get RenderTarget resource"));
		return;
	}

	TArray<FLinearColor> FloatPixels;
	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	ReadFlags.SetLinearToGamma(false);
	RTResource->ReadLinearColorPixels(FloatPixels, ReadFlags);

	// Initialize Source data with RGBA16F format (required for serialization)
	NewTexture->Source.Init(Width, Height, 1, 1, TSF_RGBA16F);

	// Lock Source mip and write float16 data
	uint8* SourceMipData = NewTexture->Source.LockMip(0);
	FFloat16Color* DestData = reinterpret_cast<FFloat16Color*>(SourceMipData);

	// Convert FLinearColor to FFloat16Color
	for (int32 i = 0; i < FloatPixels.Num(); i++)
	{
		DestData[i] = FFloat16Color(FloatPixels[i]);
	}

	NewTexture->Source.UnlockMip(0);

	// UpdateResource will automatically generate PlatformData from Source
	NewTexture->UpdateResource();

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;

	if (UPackage::SavePackage(Package, NewTexture, *PackageFileName, SaveArgs))
	{
		FAssetRegistryModule::AssetCreated(NewTexture);

		SimulationCheckpointTexture = NewTexture;
		SimulationCheckpointTime = CurrentSimulationTime;
		SimulationCheckpointResolution = Width;

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Checkpoint saved: %s (Time: %.2f, Resolution: %dx%d)"),
			*PackageFileName,
			CurrentSimulationTime,
			Width,
			Height
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save checkpoint package"));
	}
}
#endif

bool UPotentialFlowAuroraPreset::HasValidCheckpoint() const
{
	return SimulationCheckpointTexture != nullptr && SimulationCheckpointTexture->IsValidLowLevel();
}