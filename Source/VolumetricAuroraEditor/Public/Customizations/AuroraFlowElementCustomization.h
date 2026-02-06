// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IDetailGroup;

/**
 * @brief Property type customization for FAuroraFlowElement struct
 *
 * This customization reorganizes the Flow Element properties to include
 * a collapsible "Advanced" section within each array element, replacing
 * the legacy bShowAdvanced boolean approach with a cleaner UI grouping.
 *
 * @details Applied to all FAuroraFlowElement Instances in arrays,
 *			providing per-element advanced parameter organization.
 */
class FAuroraFlowElementCustomization : public IPropertyTypeCustomization
{
public:
	/**
	 * @brief Factory method to create instances of this customization
	 * @return Shared reference to new customization instance
	 */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/**
	 * @brief Customize the header row of the struct (collapsed state)
	 * @param StructPropertyHandle Handle to the entire struct property
	 * @param HeaderRow Builder for customizing the header display
	 * @param CustomizationUtils Utility functions for customization
	 */
	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils
	) override;

	/**
	 * @brief Customize the child properties (expanded state)
	 *
	 * Reorganizes properties into:
	 * - Base properties (Type, Range, Frequency, etc.)
	 * - Collapsible "Advanced" group (Lacunarity, Gain, Amplitude, etc.)
	 *
	 * @param StructPropertyHandle Handle to the entire struct property
	 * @param ChildBuilder Builder for adding child property rows
	 * @param CustomizationUtils Utility functions for customization
	 */
	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils
	) override;

private:
	/**
	 * @brief Helper to add basic (non-advanced) Curl properties
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param ChildBuilder Builder to add properties to
	 */
	void AddBasicCurlProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder
	);

	/**
	 * @brief Helper to add advanced Curl properties to the Advanced group
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param AdvancedGroup Group to add advanced properties to
	 */
	void AddAdvancedCurlProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailGroup& AdvancedGroup
	);

	/**
	 * @brief Helper to add basic (non-advanced) Warp properties
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param ChildBuilder Builder to add properties to
	 */
	void AddBasicWarpProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder
	);

	/**
	 * @brief Helper to add advanced Warp properties to the Advanced group
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param AdvancedGroup Group to add advanced properties to
	 */
	void AddAdvancedWarpProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailGroup& AdvancedGroup
	);

	/**
	 * @brief Helper to add Radial properties (Source/Sink/Spiral)
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param ChildBuilder Builder to add properties to
	 */
	void AddRadialProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder
	);

	/**
	 * @brief Helper to add Emission properties (Source/Emitter)
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param ChildBuilder Builder to add properties to
	 */
	void AddEmissionProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder
	);

	/**
	 * @brief Helper to add Rotation properties (Vortex/Spiral)
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param ChildBuilder Builder to add properties to
	 */
	void AddRotationProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder
	);

	/**
	 * @brief Helper to add Fade properties (Sink/Vortex/Spiral/Attenuator)
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param ChildBuilder Builder to add properties to
	 */
	void AddFadeProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder
	);

	/**
	 * @brief Helper to add Dipole properties
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param ChildBuilder Builder to add properties to
	 */
	void AddDipoleProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder
	);

	/**
	 * @brief Helper to add Debug Visualization properties
	 * @param StructPropertyHandle Handle to the FAuroraFlowElement struct
	 * @param ChildBuilder Builder to add properties to
	 */
	void AddDebugProperties(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder
	);
};