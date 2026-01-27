// Copyright (c) 2026 R&B. All rights reserved.

#include "Data/AuroraPresetAsset.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	MaterialInstance->SetTextureParameterValue(TEXT("Noise"), NoiseTexture);
	MaterialInstance->SetScalarParameterValue(TEXT("Intensity"), Intensity);
	MaterialInstance->SetVectorParameterValue(TEXT("Speed"), FVector3f(Speed, 0.0f));
	MaterialInstance->SetScalarParameterValue(TEXT("BaseDensity"), BaseDensity);
	MaterialInstance->SetScalarParameterValue(TEXT("BaseScale"), BaseScale);

	MaterialInstance->SetVectorParameterValue(TEXT("TopColor"), TopColor);
	MaterialInstance->SetVectorParameterValue(TEXT("MidColor"), MidColor);
	MaterialInstance->SetVectorParameterValue(TEXT("BottomColor"), BottomColor);

	MaterialInstance->SetScalarParameterValue(TEXT("FadeType"), static_cast<float>(FadeType));
	MaterialInstance->SetScalarParameterValue(TEXT("FadeStartRatio"), FadeStartRatio);
	MaterialInstance->SetScalarParameterValue(TEXT("HeightFalloff"), HeightFalloff);
	MaterialInstance->SetVectorParameterValue(TEXT("MidColorHeight"), FVector3f(MidColorHeight, 0.0f));

	MaterialInstance->SetScalarParameterValue(TEXT("EnableFilmGrain"), bEnableFilmGrain);
	MaterialInstance->SetScalarParameterValue(TEXT("FilmGrainIntensity"), FilmGrainIntensity);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveTonePivot"), EmissiveTonePivot);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveContrast"), EmissiveContrast);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveSaturation"), EmissiveSaturation);
}

UNoiseAuroraPreset::UNoiseAuroraPreset()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultSDF(TEXT("/VolumetricAurora/Textures/SimplexNoise/simplex_seamless_0_01"));

	if (DefaultSDF.Succeeded())
	{
		NoiseTexture = DefaultSDF.Object.Get();
	}
}

void UNoiseAuroraPreset::UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance)
{
	Super::UpdateMaterial(MaterialInstance);

	MaterialInstance->SetScalarParameterValue(TEXT("bIsDual"), 1.0f);
	MaterialInstance->SetScalarParameterValue(TEXT("bIsSpline"), 0.0f);
	MaterialInstance->SetScalarParameterValue(TEXT("Smoothness"), Smoothness);
	MaterialInstance->SetScalarParameterValue(TEXT("Activity"), Activity);
	MaterialInstance->SetScalarParameterValue(TEXT("MaskScaleMultiplier"), MaskScaleMultiplier);
	MaterialInstance->SetVectorParameterValue(TEXT("MaskSpeedMultiplier"), FVector(MaskSpeedMultiplier, 0.0f));
	MaterialInstance->SetScalarParameterValue(TEXT("MaskOpacity"), MaskOpacity);

	MaterialInstance->SetScalarParameterValue(TEXT("UseStructureColoring"), bUseStructureColoring);
	MaterialInstance->SetVectorParameterValue(TEXT("SoftTint"), SoftTint);
	MaterialInstance->SetVectorParameterValue(TEXT("CoreTint"), CoreTint);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveStructurePivot"), EmissiveStructurePivot);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveStructureSelectivity"), EmissiveStructureSelectivity);
	MaterialInstance->SetScalarParameterValue(TEXT("EmissiveStructureAmount"), EmissiveStructureAmount);
}

USplineAuroraPreset::USplineAuroraPreset()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultSDF(TEXT("/VolumetricAurora/Textures/SDFTextures/SDFDefault"));

	if (DefaultSDF.Succeeded())
	{
		NoiseTexture = DefaultSDF.Object.Get();
	}
}

void USplineAuroraPreset::UpdateMaterial(UMaterialInstanceDynamic* MaterialInstance)
{
	Super::UpdateMaterial(MaterialInstance);

	MaterialInstance->SetScalarParameterValue(TEXT("bIsSpline"), 1.0f);
	MaterialInstance->SetScalarParameterValue(TEXT("bIsDual"), 0.0f);
	MaterialInstance->SetScalarParameterValue(TEXT("Thickness"), Thickness);
	MaterialInstance->SetScalarParameterValue(TEXT("Distortion"), Distortion);
	MaterialInstance->SetScalarParameterValue(TEXT("DistortionSpeed"), DistortionSpeed);
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

	MaterialInstance->SetScalarParameterValue(TEXT("bIsSpline"), 0.0f);
	MaterialInstance->SetScalarParameterValue(TEXT("bIsDual"), 0.0f);

	if (DisplayBuffer)
	{
		MaterialInstance->SetTextureParameterValue(TEXT("Noise"), Cast<UTexture>(DisplayBuffer));
	}
}
