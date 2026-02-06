// Copyright (c) 2026 R&B. All rights reserved.

#include "Customizations/AuroraFlowElementCustomization.h"
#include "Types/AuroraFlowElement.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"

#define LOCTEXT_NAMESPACE "AuroraFlowElementCustomization"

TSharedRef<IPropertyTypeCustomization> FAuroraFlowElementCustomization::MakeInstance()
{
	return MakeShareable(new FAuroraFlowElementCustomization);
}

void FAuroraFlowElementCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils
)
{
	// Use default header row (displays "Element [N]" for array elements)
	// No custom header needed - leave default behavior
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		StructPropertyHandle->CreatePropertyValueWidget()
	];
}

void FAuroraFlowElementCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils
)
{
	// ========================================
	// Common Properties (always visible)
	// ========================================

	/**
	 * Add Type property - determines which flow element type (Curl, Warp, etc.)
	 */
	TSharedPtr<IPropertyHandle> TypeProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, Type));
	if (TypeProperty.IsValid())
	{
		ChildBuilder.AddProperty(TypeProperty.ToSharedRef());
	}

	/**
	 * Add Range property - defines spatial extent of the flow element
	 */
	TSharedPtr<IPropertyHandle> RangeProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, Range));
	if (RangeProperty.IsValid())
	{
		ChildBuilder.AddProperty(RangeProperty.ToSharedRef());
	}

	/**
	 * Add Position property - location of flow element in world space
	 */
	TSharedPtr<IPropertyHandle> PositionProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, Position));
	if (PositionProperty.IsValid())
	{
		ChildBuilder.AddProperty(PositionProperty.ToSharedRef());
	}

	// ========================================
	// Type-Specific Properties (in logical order)
	// ========================================

	// Radial-based control points (Source, Sink, Spiral)
	AddRadialProperties(StructPropertyHandle, ChildBuilder);

	// Emission properties (Source, Emitter)
	AddEmissionProperties(StructPropertyHandle, ChildBuilder);

	// Rotation properties (Vortex, Spiral)
	AddRotationProperties(StructPropertyHandle, ChildBuilder);

	// Fade properties (Sink, Vortex, Spiral, Attenuator)
	AddFadeProperties(StructPropertyHandle, ChildBuilder);

	// Dipole properties
	AddDipoleProperties(StructPropertyHandle, ChildBuilder);

	// Curl Properties
	AddBasicCurlProperties(StructPropertyHandle, ChildBuilder);

	// Warp Properties
	AddBasicWarpProperties(StructPropertyHandle, ChildBuilder);

    // ========================================
    // Advanced Group (Collapsible Section)
    // Only shown for Curl and Warp types
    // ========================================

    /**
     * Check current Type value and conditions to determine if Advanced section should be shown
     * Advanced section is only shown when there are actually visible advanced properties
     */
    TSharedPtr<IPropertyHandle> TypePropertyForCheck = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, Type));

    // Register property change callback to refresh UI when Type changes
    // This ensures Advanced section appears/disappears dynamically when user changes Type
    if (TypePropertyForCheck.IsValid())
    {
        TSharedPtr<IPropertyUtilities> PropUtils = CustomizationUtils.GetPropertyUtilities();
        TypePropertyForCheck->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([PropUtils]()
        {
            // Force refresh the details panel when Type property changes
            // This will re-run CustomizeChildren and properly show/hide Advanced section
            if (PropUtils.IsValid())
            {
                PropUtils->RequestRefresh();
            }
        }));
    }

    bool bShowAdvancedSection = false;
    if (TypePropertyForCheck.IsValid())
    {
        uint8 TypeValueAsUint8 = 0;
        if (TypePropertyForCheck->GetValue(TypeValueAsUint8) == FPropertyAccess::Success)
        {
            EControlPointType TypeValue = static_cast<EControlPointType>(TypeValueAsUint8);

            if (TypeValue == EControlPointType::Curl)
            {
                /**
                 * For Curl: Always show Advanced section.
                 * Individual properties (Lacunarity, Gain, Amplitude) have EditCondition
                 * "CurlOctaves > 1" and will be hidden/shown automatically.
                 * This ensures dynamic visibility when CurlOctaves changes.
                 */
                bShowAdvancedSection = true;
            }
            else if (TypeValue == EControlPointType::Warp)
            {
                /**
                 * For Warp: Most advanced properties (Falloff, Contrast, AnimAmplitude, XScale, YScale)
                 * are always visible when Type == Warp, so always show Advanced section
                 */
                bShowAdvancedSection = true;
            }
        }
    }

    // Only create Advanced group if there are visible advanced properties
    if (bShowAdvancedSection)
    {
        /**
         * Create collapsible "Advanced" group for advanced parameters
         * - Initially collapsed to reduce UI clutter
         * - Contains Lacunarity, Gain, Amplitude, and other advanced parameters
         */
        IDetailGroup& AdvancedGroup = ChildBuilder.AddGroup(
            FName("Advanced"),                           // Internal name
            LOCTEXT("AdvancedGroup", "Advanced"),        // Display name
            false                                        // bForAdvanced flag
        );

        // Add type-specific advanced properties to the group
        // Only add properties relevant to the current Type to avoid unnecessary UI elements
        if (TypePropertyForCheck.IsValid())
        {
            uint8 TypeValueAsUint8 = 0;
            if (TypePropertyForCheck->GetValue(TypeValueAsUint8) == FPropertyAccess::Success)
            {
                EControlPointType TypeValue = static_cast<EControlPointType>(TypeValueAsUint8);

                if (TypeValue == EControlPointType::Curl)
                {
                    // Add advanced Curl properties to the group
                    AddAdvancedCurlProperties(StructPropertyHandle, AdvancedGroup);
                }
                else if (TypeValue == EControlPointType::Warp)
                {
                    // Add advanced Warp properties to the group
                    AddAdvancedWarpProperties(StructPropertyHandle, AdvancedGroup);
                }
            }
        }
    }

	// ========================================
	// Debug Visualization Properties (always at the end)
	// ========================================

	AddDebugProperties(StructPropertyHandle, ChildBuilder);
}

void FAuroraFlowElementCustomization::AddBasicCurlProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder)
{
	/**
	 * Curl Frequency - base frequency of curl noise pattern
	 */
	TSharedPtr<IPropertyHandle> CurlFrequencyProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlFrequency));
	if (CurlFrequencyProperty.IsValid())
	{
		ChildBuilder.AddProperty(CurlFrequencyProperty.ToSharedRef());
	}

	/**
	 * Curl Animation Speed - temporal evolution rate
	 */
	TSharedPtr<IPropertyHandle> CurlAnimationSpeedProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlAnimationSpeed));
	if (CurlAnimationSpeedProperty.IsValid())
	{
		ChildBuilder.AddProperty(CurlAnimationSpeedProperty.ToSharedRef());
	}

	/**
	 * Curl Octaves - number of noise octaves for detail layering
	 */
	TSharedPtr<IPropertyHandle> CurlOctavesProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlOctaves));
	if (CurlOctavesProperty.IsValid())
	{
		ChildBuilder.AddProperty(CurlOctavesProperty.ToSharedRef());
	}

	/**
	 * Curl Strength - intensity of curl effect
	 */
	TSharedPtr<IPropertyHandle> CurlStrengthProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlStrength));
	if (CurlStrengthProperty.IsValid())
	{
		ChildBuilder.AddProperty(CurlStrengthProperty.ToSharedRef());
	}

	/**
	 * Curl Attenuation Start - distance where curl attenuation begins
	 */
	TSharedPtr<IPropertyHandle> CurlAttenuationStartProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlAttenuationStart));
	if (CurlAttenuationStartProperty.IsValid())
	{
		ChildBuilder.AddProperty(CurlAttenuationStartProperty.ToSharedRef());
	}

	/**
	 * Curl Attenuation End - distance where curl influence reaches zero
	 */
	TSharedPtr<IPropertyHandle> CurlAttenuationEndProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlAttenuationEnd));
	if (CurlAttenuationEndProperty.IsValid())
	{
		ChildBuilder.AddProperty(CurlAttenuationEndProperty.ToSharedRef());
	}

	/**
	 * Curl Exponent - falloff exponent for curl influence
	 */
	TSharedPtr<IPropertyHandle> CurlExponentProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlExponent));
	if (CurlExponentProperty.IsValid())
	{
		ChildBuilder.AddProperty(CurlExponentProperty.ToSharedRef());
	}
}

void FAuroraFlowElementCustomization::AddAdvancedCurlProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailGroup& AdvancedGroup)
{
	/**
	 * Curl Lacunarity - frequency multiplier between octaves
	 * Controls how quickly noise detail increases per octave
	 */
	TSharedPtr<IPropertyHandle> LacunarityProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlLacunarity));
	if (LacunarityProperty.IsValid())
	{
		AdvancedGroup.AddPropertyRow(LacunarityProperty.ToSharedRef());
	}

	/**
	 * Curl Gain - amplitude multiplier between octaves
	 * Controls how quickly noise amplitude decreases per octave
	 */
	TSharedPtr<IPropertyHandle> GainProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlGain));
	if (GainProperty.IsValid())
	{
		AdvancedGroup.AddPropertyRow(GainProperty.ToSharedRef());
	}

	/**
	 * Curl Amplitude - overall amplitude scaling factor
	 */
	TSharedPtr<IPropertyHandle> AmplitudeProperty = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, CurlAmplitude));
	if (AmplitudeProperty.IsValid())
	{
		AdvancedGroup.AddPropertyRow(AmplitudeProperty.ToSharedRef());
	}

	// TODO: Add remaining advanced Curl properties (Turbulence, etc.)
}

void FAuroraFlowElementCustomization::AddBasicWarpProperties(
    TSharedRef<IPropertyHandle> StructPropertyHandle,
    IDetailChildrenBuilder& ChildBuilder)
{
    /**
     * Warp Iterations - number of iterative domain warp passes
     */
    TSharedPtr<IPropertyHandle> WarpIterationsProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpIterations));
    if (WarpIterationsProperty.IsValid())
    {
            ChildBuilder.AddProperty(WarpIterationsProperty.ToSharedRef());
    }

    /**
     * Warp Displacement - initial warp displacement strength
     */
    TSharedPtr<IPropertyHandle> WarpDisplacementProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpDisplacement));
    if (WarpDisplacementProperty.IsValid())
    {
            ChildBuilder.AddProperty(WarpDisplacementProperty.ToSharedRef());
    }

    /**
     * Warp Animation Speed - animation speed multiplier
     */
    TSharedPtr<IPropertyHandle> WarpAnimationSpeedProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpAnimationSpeed));
    if (WarpAnimationSpeedProperty.IsValid())
    {
            ChildBuilder.AddProperty(WarpAnimationSpeedProperty.ToSharedRef());
    }

    /**
     * Warp Octaves - FBM octave count for warp noise
     */
    TSharedPtr<IPropertyHandle> WarpOctavesProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpOctaves));
    if (WarpOctavesProperty.IsValid())
    {
            ChildBuilder.AddProperty(WarpOctavesProperty.ToSharedRef());
    }

    /**
     * Warp Noise Scale - result noise sample scale
     */
    TSharedPtr<IPropertyHandle> WarpNoiseScaleProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpNoiseScale));
    if (WarpNoiseScaleProperty.IsValid())
    {
            ChildBuilder.AddProperty(WarpNoiseScaleProperty.ToSharedRef());
    }

    /**
     * Warp Flow Strength - final flow strength multiplier
     */
    TSharedPtr<IPropertyHandle> WarpFlowStrengthProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpFlowStrength));
    if (WarpFlowStrengthProperty.IsValid())
    {
            ChildBuilder.AddProperty(WarpFlowStrengthProperty.ToSharedRef());
    }

    /**
     * Warp Attenuation Start - distance where warp attenuation begins
     */
    TSharedPtr<IPropertyHandle> WarpAttenuationStartProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpAttenuationStart));
    if (WarpAttenuationStartProperty.IsValid())
    {
            ChildBuilder.AddProperty(WarpAttenuationStartProperty.ToSharedRef());
    }

    /**
     * Warp Attenuation End - distance where warp influence reaches zero
     */
    TSharedPtr<IPropertyHandle> WarpAttenuationEndProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpAttenuationEnd));
    if (WarpAttenuationEndProperty.IsValid())
    {
            ChildBuilder.AddProperty(WarpAttenuationEndProperty.ToSharedRef());
    }

    /**
     * Warp Exponent - falloff exponent for warp influence
     */
    TSharedPtr<IPropertyHandle> WarpExponentProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpExponent));
    if (WarpExponentProperty.IsValid())
    {
            ChildBuilder.AddProperty(WarpExponentProperty.ToSharedRef());
    }
}

void FAuroraFlowElementCustomization::AddAdvancedWarpProperties(
    TSharedRef<IPropertyHandle> StructPropertyHandle,
    IDetailGroup& AdvancedGroup)
{
    /**
     * Warp Lacunarity - frequency multiplier per octave (visible when WarpOctaves > 1)
     */
    TSharedPtr<IPropertyHandle> WarpLacunarityProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpLacunarity));
    if (WarpLacunarityProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpLacunarityProperty.ToSharedRef());
    }

    /**
     * Warp Gain - amplitude multiplier per octave (visible when WarpOctaves > 1)
     */
    TSharedPtr<IPropertyHandle> WarpGainProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpGain));
    if (WarpGainProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpGainProperty.ToSharedRef());
    }

    /**
     * Warp Initial Amplitude - initial FBM amplitude (visible when WarpOctaves > 1)
     */
    TSharedPtr<IPropertyHandle> WarpInitialAmplitudeProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpInitialAmplitude));
    if (WarpInitialAmplitudeProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpInitialAmplitudeProperty.ToSharedRef());
    }

    /**
     * Warp Falloff - strength decay per warp iteration
     */
    TSharedPtr<IPropertyHandle> WarpFalloffProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpFalloff));
    if (WarpFalloffProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpFalloffProperty.ToSharedRef());
    }

    /**
     * Warp Contrast - output contrast adjustment
     */
    TSharedPtr<IPropertyHandle> WarpContrastProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpContrast));
    if (WarpContrastProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpContrastProperty.ToSharedRef());
    }

    /**
     * Warp Anim Amplitude - animation displacement magnitude
     */
    TSharedPtr<IPropertyHandle> WarpAnimAmplitudeProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpAnimAmplitude));
    if (WarpAnimAmplitudeProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpAnimAmplitudeProperty.ToSharedRef());
    }

    /**
     * Warp X Scale - X-axis warping noise scale
     */
    TSharedPtr<IPropertyHandle> WarpXScaleProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpXScale));
    if (WarpXScaleProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpXScaleProperty.ToSharedRef());
    }

    /**
     * Warp Y Scale - Y-axis warping noise scale
     */
    TSharedPtr<IPropertyHandle> WarpYScaleProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpYScale));
    if (WarpYScaleProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpYScaleProperty.ToSharedRef());
    }

    /**
     * Warp X Offset - X-axis noise sampling offset
     */
    TSharedPtr<IPropertyHandle> WarpXOffsetProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpXOffset));
    if (WarpXOffsetProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpXOffsetProperty.ToSharedRef());
    }

    /**
     * Warp Y Offset - Y-axis noise sampling offset
     */
    TSharedPtr<IPropertyHandle> WarpYOffsetProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpYOffset));
    if (WarpYOffsetProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpYOffsetProperty.ToSharedRef());
    }

    /**
     * Warp Noise Offset - result noise sampling offset
     */
    TSharedPtr<IPropertyHandle> WarpNoiseOffsetProperty = StructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, WarpNoiseOffset));
    if (WarpNoiseOffsetProperty.IsValid())
    {
            AdvancedGroup.AddPropertyRow(WarpNoiseOffsetProperty.ToSharedRef());
    }
}

void FAuroraFlowElementCustomization::AddRadialProperties(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder)
{
	/**
	 * Radial Strength - outward/inward flow intensity
	 * Visible for: Source, Sink, Spiral
	 */
	TSharedPtr<IPropertyHandle> RadialStrengthProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, RadialStrength));
	if (RadialStrengthProperty.IsValid())
	{
		ChildBuilder.AddProperty(RadialStrengthProperty.ToSharedRef());
	}

	/**
	 * Radial Attenuation Start - distance where radial attenuation begins
	 */
	TSharedPtr<IPropertyHandle> RadialAttenuationStartProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, RadialAttenuationStart));
	if (RadialAttenuationStartProperty.IsValid())
	{
		ChildBuilder.AddProperty(RadialAttenuationStartProperty.ToSharedRef());
	}

	/**
	 * Radial Attenuation End - distance where radial influence reaches zero
	 */
	TSharedPtr<IPropertyHandle> RadialAttenuationEndProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, RadialAttenuationEnd));
	if (RadialAttenuationEndProperty.IsValid())
	{
		ChildBuilder.AddProperty(RadialAttenuationEndProperty.ToSharedRef());
	}

	/**
	 * Radial Exponent - falloff exponent for radial influence
	 */
	TSharedPtr<IPropertyHandle> RadialExponentProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, RadialExponent));
	if (RadialExponentProperty.IsValid())
	{
		ChildBuilder.AddProperty(RadialExponentProperty.ToSharedRef());
	}
}

void FAuroraFlowElementCustomization::AddEmissionProperties(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder)
{
	/**
	 * Emission Strength - density emission strength
	 * Visible for: Source, Emitter
	 */
	TSharedPtr<IPropertyHandle> EmissionStrengthProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, EmissionStrength));
	if (EmissionStrengthProperty.IsValid())
	{
		ChildBuilder.AddProperty(EmissionStrengthProperty.ToSharedRef());
	}

	/**
	 * Emission Attenuation Start - distance where emission attenuation begins
	 */
	TSharedPtr<IPropertyHandle> EmissionAttenuationStartProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, EmissionAttenuationStart));
	if (EmissionAttenuationStartProperty.IsValid())
	{
		ChildBuilder.AddProperty(EmissionAttenuationStartProperty.ToSharedRef());
	}

	/**
	 * Emission Attenuation End - distance where emission influence reaches zero
	 */
	TSharedPtr<IPropertyHandle> EmissionAttenuationEndProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, EmissionAttenuationEnd));
	if (EmissionAttenuationEndProperty.IsValid())
	{
		ChildBuilder.AddProperty(EmissionAttenuationEndProperty.ToSharedRef());
	}

	/**
	 * Emission Exponent - falloff exponent for emission influence
	 */
	TSharedPtr<IPropertyHandle> EmissionExponentProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, EmissionExponent));
	if (EmissionExponentProperty.IsValid())
	{
		ChildBuilder.AddProperty(EmissionExponentProperty.ToSharedRef());
	}
}

void FAuroraFlowElementCustomization::AddRotationProperties(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder)
{
	/**
	 * Rotation Strength - rotational flow intensity
	 * Visible for: Vortex, Spiral
	 */
	TSharedPtr<IPropertyHandle> RotationStrengthProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, RotationStrength));
	if (RotationStrengthProperty.IsValid())
	{
		ChildBuilder.AddProperty(RotationStrengthProperty.ToSharedRef());
	}

	/**
	 * Rotation Attenuation Start - distance where rotation attenuation begins
	 */
	TSharedPtr<IPropertyHandle> RotationAttenuationStartProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, RotationAttenuationStart));
	if (RotationAttenuationStartProperty.IsValid())
	{
		ChildBuilder.AddProperty(RotationAttenuationStartProperty.ToSharedRef());
	}

	/**
	 * Rotation Attenuation End - distance where rotation influence reaches zero
	 */
	TSharedPtr<IPropertyHandle> RotationAttenuationEndProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, RotationAttenuationEnd));
	if (RotationAttenuationEndProperty.IsValid())
	{
		ChildBuilder.AddProperty(RotationAttenuationEndProperty.ToSharedRef());
	}

	/**
	 * Rotation Exponent - falloff exponent for rotation influence
	 */
	TSharedPtr<IPropertyHandle> RotationExponentProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, RotationExponent));
	if (RotationExponentProperty.IsValid())
	{
		ChildBuilder.AddProperty(RotationExponentProperty.ToSharedRef());
	}
}

void FAuroraFlowElementCustomization::AddFadeProperties(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder)
{
	/**
	 * Fade Strength - density fade strength
	 * Visible for: Sink, Vortex, Spiral, Attenuator
	 */
	TSharedPtr<IPropertyHandle> FadeStrengthProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, FadeStrength));
	if (FadeStrengthProperty.IsValid())
	{
		ChildBuilder.AddProperty(FadeStrengthProperty.ToSharedRef());
	}

	/**
	 * Fade Attenuation Start - distance where fade attenuation begins
	 */
	TSharedPtr<IPropertyHandle> FadeAttenuationStartProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, FadeAttenuationStart));
	if (FadeAttenuationStartProperty.IsValid())
	{
		ChildBuilder.AddProperty(FadeAttenuationStartProperty.ToSharedRef());
	}

	/**
	 * Fade Attenuation End - distance where fade influence reaches zero
	 */
	TSharedPtr<IPropertyHandle> FadeAttenuationEndProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, FadeAttenuationEnd));
	if (FadeAttenuationEndProperty.IsValid())
	{
		ChildBuilder.AddProperty(FadeAttenuationEndProperty.ToSharedRef());
	}

	/**
	 * Fade Exponent - falloff exponent for fade influence
	 */
	TSharedPtr<IPropertyHandle> FadeExponentProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, FadeExponent));
	if (FadeExponentProperty.IsValid())
	{
		ChildBuilder.AddProperty(FadeExponentProperty.ToSharedRef());
	}
}

void FAuroraFlowElementCustomization::AddDipoleProperties(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder)
{
	/**
	 * Dipole Direction - axis direction for dipole flow
	 * Visible for: Dipole
	 */
	TSharedPtr<IPropertyHandle> DipoleDirectionProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, DipoleDirection));
	if (DipoleDirectionProperty.IsValid())
	{
		ChildBuilder.AddProperty(DipoleDirectionProperty.ToSharedRef());
	}

	/**
	 * Dipole Strength - dipole moment strength
	 */
	TSharedPtr<IPropertyHandle> DipoleStrengthProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, DipoleStrength));
	if (DipoleStrengthProperty.IsValid())
	{
		ChildBuilder.AddProperty(DipoleStrengthProperty.ToSharedRef());
	}

	/**
	 * Dipole Attenuation Start - distance where dipole attenuation begins
	 */
	TSharedPtr<IPropertyHandle> DipoleAttenuationStartProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, DipoleAttenuationStart));
	if (DipoleAttenuationStartProperty.IsValid())
	{
		ChildBuilder.AddProperty(DipoleAttenuationStartProperty.ToSharedRef());
	}

	/**
	 * Dipole Attenuation End - distance where dipole influence reaches zero
	 */
	TSharedPtr<IPropertyHandle> DipoleAttenuationEndProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, DipoleAttenuationEnd));
	if (DipoleAttenuationEndProperty.IsValid())
	{
		ChildBuilder.AddProperty(DipoleAttenuationEndProperty.ToSharedRef());
	}

	/**
	 * Dipole Exponent - falloff exponent for dipole influence
	 */
	TSharedPtr<IPropertyHandle> DipoleExponentProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, DipoleExponent));
	if (DipoleExponentProperty.IsValid())
	{
		ChildBuilder.AddProperty(DipoleExponentProperty.ToSharedRef());
	}
}

void FAuroraFlowElementCustomization::AddDebugProperties(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder)
{
	/**
	 * Display Control Point - visualize control point location
	 */
	TSharedPtr<IPropertyHandle> DisplayControlPointProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, bDisplayControlPoint));
	if (DisplayControlPointProperty.IsValid())
	{
		ChildBuilder.AddProperty(DisplayControlPointProperty.ToSharedRef());
	}

	/**
	 * Display Attenuation Range - visualize attenuation range
	 */
	TSharedPtr<IPropertyHandle> DisplayAttenuationRangeProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FAuroraFlowElement, bDisplayAttenuationRange));
	if (DisplayAttenuationRangeProperty.IsValid())
	{
		ChildBuilder.AddProperty(DisplayAttenuationRangeProperty.ToSharedRef());
	}
}

#undef LOCTEXT_NAMESPACE