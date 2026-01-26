// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"

// Forward declarations
class IDetailLayoutBuilder;
class AVolumetricAurora;
class UUserWidget;
class SWindow;
class UAuroraPresetBase;


enum class EAuroraPresetItemType : uint8
{
	Divider,
	Header,
	Option
};

struct FAuroraPresetComboItem
{
	EAuroraPresetItemType Type;
	FString DisplayName;
	TWeakObjectPtr<UAuroraPresetBase> PresetAsset;

	FAuroraPresetComboItem(
		EAuroraPresetItemType InType,
		const FString& InName,
		UAuroraPresetBase* InPreset = nullptr)
		: Type(InType)
		, DisplayName(InName)
		, PresetAsset(InPreset)
	{
	}
};

/**
 * @brief Custom details panel for AVolumetricAurora
 *
 * Adds "Edit Elements Map" button to Details Panel.
 * When clicked, spawns a paint proxy and enters Mesh Paint Mode.
 */
class FVolumetricAuroraDetailsCustomization : public IDetailCustomization
{
public:
	/**
	 * @brief Factory method for creating instances
	 * Called by PropertyEditor module when registering customization
	 */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/**
	 * @brief Customize the details panel layout
	 *
	 * @param DetailBuilder Interface for modifying details panel structure
	 */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	void OnPresetRemoved(const FAssetData& AssetData);

	void OnPresetAdded(const FAssetData& AssetData);

	bool IsAssetAuroraPreset(const FAssetData& AssetData);

	EAppReturnType::Type AskSaving(AVolumetricAurora* Aurora);

private:
	/**
	 * @brief Handle "Reset Simulation" button click
	 *
	 * Set force reset simulation flag to true
	 */
	FReply OnResetClicked();
	
	/**
	 * @brief Handle "Edit Elements Map" button click
	 *
	 * Workflow:
	 * 1. Initialize render target if needed
	 * 2. Spawn paint proxy actor
	 * 3. Enter mesh paint mode
	 *
	 * @return FReply::Handled() if successful
	 */
	FReply OnEditElementsMapClicked();

	/**
	 * @brief Handle "Bake to Texture" button click
	 *
	 * Converts render target to permanent Texture2D asset
	 */
	FReply OnBakeToTextureClicked();

	FReply OnNewAuroraPresetButtonClicked();

	FReply OnSaveAuroraPresetButtonClicked();

	FReply OnSaveAsAuroraPresetButtonClicked();

	/**
	 * @brief Check if PotentialFlowAuroraPreset is selected
	 * Used to show/hide paint buttons conditionally
	 */
	bool IsPotentialFlowPresetSelected() const;

	/**
	 * @brief Cached reference to selected VolumetricAurora actor(s)
	 */
	TArray<TWeakObjectPtr<AVolumetricAurora>> SelectedAuroras;

	/**
	 * @brief Reference to opened paint window
	 *
	 * Cached to prevent multiple windows from opening
	 * Reset when window is closed.
	 */
	TSharedPtr<SWindow> PaintWindow;

	/**
	 * @brief Reference to painter widget instance
	 *
	 * Kept alive while paint window is open.
	 * Used for RenderTarget updates and communication with WBP.
	 */
	TObjectPtr<UUserWidget> PaintWidgetInstance;

	/**
	 * @brief Reference to the Aurora Type Selector window
	 *
	 * Cached to prevent opening multiple selector windows.
	 * If the window is already open, it will be brought to front instead of creating a new one.
	 * Reset when the window is closed.
	 */
	TSharedPtr<SWindow> AuroraTypeSelectorWindow;

	// ==========================
	// Aurora Preset Combo UI
	// ==========================

	/** Combo box option items (headers, dividers, preset entries) */
	TArray<TSharedPtr<FAuroraPresetComboItem>> PresetOptions;

	/** About preset options */
	TSharedPtr<FAuroraPresetComboItem> SelectedPresetItem;
	TSharedPtr<SListView<TSharedPtr<FAuroraPresetComboItem>>> PresetListView;

	/** Dropdown menu root & anchor */
	TSharedPtr<SBorder> PresetListViewContainer;
	TSharedPtr<SMenuAnchor> PresetMenuAnchor;

	/** Builds the Aurora preset selector row in the details panel */
	void BuildAuroraPresetSection(IDetailLayoutBuilder& DetailBuilder);

	/** Collects and groups available preset assets into combo options */
	void BuildPresetOptions();

	/** Builds the preset dropdown widget */
	void CreatePresetListView();
	TSharedRef<SWidget> BuildPresetDropdown();

	/** Generates a row widget for each preset option in the list view */
	TSharedRef<ITableRow> GeneratePresetRow(TSharedPtr<FAuroraPresetComboItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Handles selection of a preset option from the dropdown list */
	void OnPresetRowSelected(TSharedPtr<FAuroraPresetComboItem> Item, ESelectInfo::Type);
};