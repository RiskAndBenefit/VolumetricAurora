// Copyright (c) 2026 R&B. All rights reserved.

#include "Customizations/VolumetricAuroraDetailsCustomization.h"
#include "Actors/VolumetricAurora.h"
#include "Data/AuroraPresetAsset.h"
#include "Core/AuroraPresetManager.h"
#include "Widgets/SavePresetAsWidget.h"
#include "Widgets/AuroraElementsPainterWidget.h"
#include "Widgets/AuroraPreviewViewport.h"
#include "VolumetricAuroraEditor.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Editor.h"

#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/SavePackage.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "Blueprint/UserWidget.h"
#include "Widgets/SWindow.h"
#include "WidgetBlueprint.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"

#include "PropertyEditorModule.h"
#include "IDetailsView.h"

#include "Widgets/AuroraTypeSelectorWidget.h"

#define LOCTEXT_NAMESPACE "VolumetricAuroraDetailsCustomization"

TSharedRef<IDetailCustomization> FVolumetricAuroraDetailsCustomization::MakeInstance()
{
	// Create shared pointer to customization instance
	// Unreal used TSharedRef for automatic memory management
	return MakeShareable(new FVolumetricAuroraDetailsCustomization);
}

void FVolumetricAuroraDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Get selected actors from details panel
	// DetailBuilder tracks which objects are being displayed
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

	// Cache AVolumetricAurora references
	SelectedAuroras.Empty();
	for (TWeakObjectPtr<UObject> Obj : CustomizedObjects)
	{
		if (AVolumetricAurora* Aurora = Cast<AVolumetricAurora>(Obj.Get()))
		{
			SelectedAuroras.Add(Aurora);
		}
	}

	if (SelectedAuroras.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid VolumetricAurora selected"));
		return;
	}
	FAssetRegistryModule& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	AssetRegistry.Get().OnAssetAdded().AddSP(this, &FVolumetricAuroraDetailsCustomization::OnPresetAdded);
	AssetRegistry.Get().OnAssetRemoved().AddSP(this, &FVolumetricAuroraDetailsCustomization::OnPresetRemoved);

	IDetailCategoryBuilder& AuroraCategory = DetailBuilder.EditCategory("Aurora");

	const FSlateBrush* NewPresetBrush = FAppStyle::GetBrush(TEXT("MainFrame.NewProject"));
	const FSlateBrush* SaveBrush = FAppStyle::GetBrush(TEXT("AssetEditor.SaveAsset"));
	const FSlateBrush* SaveAsBrush = FAppStyle::GetBrush(TEXT("AssetEditor.SaveAssetAs"));

	// 버튼 위치 추후 수정 예정
	AuroraCategory.AddCustomRow(FText::FromString("New Aurora Preset"))
		.WholeRowContent()
		[
			SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked(this, &FVolumetricAuroraDetailsCustomization::OnNewAuroraPresetButtonClicked)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
								.Image(NewPresetBrush)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(6.f, 0.f, 0.f, 0.f))
						[
							SNew(STextBlock)
								.Text(FText::FromString("New Aurora Preset"))
						]
				]
		];

	AuroraCategory.AddCustomRow(FText::FromString("SaveRow"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(0.f, 0.f, 1.f, 0.f)
				[
					/* Save Button */
					SNew(SButton)
						.OnClicked(this, &FVolumetricAuroraDetailsCustomization::OnSaveAuroraPresetButtonClicked)
						.HAlign(HAlign_Center)
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SImage)
										.Image(SaveBrush)
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(FMargin(6.f, 0.f, 0.f, 0.f))
								[
									SNew(STextBlock)
										.Text(FText::FromString("Save"))
								]
						]
				]

			+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					/* SaveAs Button */
					SNew(SButton)
						.OnClicked(this, &FVolumetricAuroraDetailsCustomization::OnSaveAsAuroraPresetButtonClicked)
						.HAlign(HAlign_Center)
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SImage)
										.Image(SaveAsBrush)
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(FMargin(6.f, 0.f, 0.f, 0.f))
								[
									SNew(STextBlock)
										.Text(FText::FromString("Save As..."))
								]
						]
				]
		];

	BuildPresetOptions();
	CreatePresetListView();
	BuildAuroraPresetSection(DetailBuilder);

	// Add "Reset Simulation" button
	AuroraCategory.AddCustomRow(LOCTEXT("ResetSimulation", "Reset Simulation"))
		.Visibility(TAttribute<EVisibility>::CreateLambda([this]()
			{
				return IsPotentialFlowPresetSelected()
					? EVisibility::Visible
					: EVisibility::Collapsed;
			}))
		.NameContent()
		[
			// Left side: Label
			SNew(STextBlock)
				.Text(LOCTEXT("ResetSimulationLabel", "Force Reset Flow"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(SButton)
				.Text(LOCTEXT("ResetBtn", "Reset Flow"))
				.OnClicked(this, &FVolumetricAuroraDetailsCustomization::OnResetClicked)
		];

	// Add "Capture Checkpoint" button
	AuroraCategory.AddCustomRow(LOCTEXT("CaptureCheckpoint", "Capture Checkpoint"))
		.Visibility(TAttribute<EVisibility>::CreateLambda([this]()
			{
				return IsPotentialFlowPresetSelected()
					? EVisibility::Visible
					: EVisibility::Collapsed;
			}))
		.NameContent()
		[
			// Left side: Label
			SNew(STextBlock)
				.Text(LOCTEXT("CaptureCheckpointLabel", "Simulation Checkpoint"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(SButton)
				.Text(LOCTEXT("CaptureBtn", "Capture Current State"))
				.OnClicked(this, &FVolumetricAuroraDetailsCustomization::OnCaptureCheckpointClicked)
		];

	// Add "Edit Elements Map" button
	AuroraCategory.AddCustomRow(LOCTEXT("EditElementRow", "Edit Elements Map"))
		.Visibility(TAttribute<EVisibility>::CreateLambda([this]()
			{
				return IsPotentialFlowPresetSelected()
					? EVisibility::Visible
					: EVisibility::Collapsed;
			}))
		.NameContent()
		[
			// Left side: Label
			SNew(STextBlock)
				.Text(LOCTEXT("EditElementsLabel", "Paint Elements Map"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			// Right side: Button
			SNew(SButton)
				.Text(LOCTEXT("EditElementsButton", "Edit Elements Map"))
				.ToolTipText(LOCTEXT("EditElementsTooltip",
					"Open texture paint mode to edit aurora elements. \n"
					"R channel = Emitter regions\n"
					"G channel = Fade zones"))
				.OnClicked(this, &FVolumetricAuroraDetailsCustomization::OnEditElementsMapClicked)
				// Only enable button if PotentialFlowPreset is selected
				.IsEnabled(this, &FVolumetricAuroraDetailsCustomization::IsPotentialFlowPresetSelected)
		];
}

void FVolumetricAuroraDetailsCustomization::OnPresetRemoved(const FAssetData& AssetData)
{
	if (IsAssetAuroraPreset(AssetData))
	{
		BuildPresetOptions();

	}
}

void FVolumetricAuroraDetailsCustomization::OnPresetAdded(const FAssetData& AssetData)
{
	if (IsAssetAuroraPreset(AssetData))
	{
		BuildPresetOptions();

		if (SelectedAuroras.IsValidIndex(0) && SelectedAuroras[0].IsValid())
		{
			if (!FVolumetricAuroraEditorModule::bIsCreatingAssetWhileSaving)
			{
				FScopedTransaction Transaction(FText::FromString("Apply Aurora Preset"));
				SelectedAuroras[0].Get()->ApplyPresetToTarget(Cast<UAuroraPresetBase>(AssetData.GetAsset()));
			}
		}
	}
}

bool FVolumetricAuroraDetailsCustomization::IsAssetAuroraPreset(const FAssetData& AssetData)
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	TArray<FTopLevelAssetPath> ParentPaths{ UAuroraPresetBase::StaticClass()->GetClassPathName() };
	TSet<FTopLevelAssetPath> DerivedClassNames;
	TSet<FTopLevelAssetPath> ExcludedClassNames;
	AssetRegistry.GetDerivedClassNames(ParentPaths, ExcludedClassNames, DerivedClassNames);

	return DerivedClassNames.Contains(AssetData.AssetClassPath);
}

FReply FVolumetricAuroraDetailsCustomization::OnResetClicked()
{
	if (SelectedAuroras.Num() == 0 || !SelectedAuroras[0].IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid VolumetricAurora selected"));
		return FReply::Handled();
	}

	AVolumetricAurora* Aurora = SelectedAuroras[0].Get();

	Aurora->ResetFlowSimulation();
	return FReply::Handled();
}

FReply FVolumetricAuroraDetailsCustomization::OnCaptureCheckpointClicked()
{
	// Validate selected Aurora actor exists
	if (SelectedAuroras.Num() == 0 || !SelectedAuroras[0].IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid VolumetricAurora selected"));
		return FReply::Handled();
	}

	AVolumetricAurora* Aurora = SelectedAuroras[0].Get();

	// Call wrapper function to capture checkpoint
	Aurora->CaptureFlowSimulationCheckpoint();

	return FReply::Handled();
}

FReply FVolumetricAuroraDetailsCustomization::OnEditElementsMapClicked()
{
	// ============================================================
	// Step 1: Validate selected aurora
	// ============================================================

	if (SelectedAuroras.Num() == 0 || !SelectedAuroras[0].IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid VolumetricAurora selected"));
		return FReply::Handled();
	}

	AVolumetricAurora* Aurora = SelectedAuroras[0].Get();

	// ============================================================
	// Step 2: Check for existing window FIRST
	// ============================================================

	// Prevent multiple paint windows from opening
	// If window is already open, bring it to front instead
	if (Aurora->EditorPaintWindow.IsValid())
	{
		// Check if window was actually destroyed (user closed it)
		if (!FSlateApplication::Get().FindWidgetWindow(Aurora->EditorPaintWindow.ToSharedRef()).IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("Cleaning up stale paint window reference"));
			Aurora->EditorPaintWindow.Reset();
			Aurora->EditorPaintWidgetInstance = nullptr;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Paint window already open, bringing to front"));
			Aurora->EditorPaintWindow->BringToFront();
			return FReply::Handled();
		}
	}

	// ============================================================
	// Step 3: Initialize RenderTarget
	// ============================================================

	// Initialize ElementsRenderTarget if not already created
	// This creates a 2048x2048 RGBA8 render target for painting
	Aurora->InitializeElementsRenderTarget();

	if (!Aurora->ElementsRenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to initialize ElementsRenderTarget"));
		return FReply::Handled();
	}

	// ============================================================
	// Step 4: Setup Preview System
	// ============================================================

	/**
	 * Preview System Setup:
	 * 1. CreatePreviewAurora: Spawns duplicate actor at fixed position
	 * 2. CreatePreviewSceneCapture: Sets up camera to render preview
	 * 3. UpdatePreviewAurora: Initial capture to populate preview image
	 */

	 // Create preview aurora actor
	AVolumetricAurora* PreviewAurora = Aurora->CreatePreviewAurora();
	if (!PreviewAurora)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create preview aurora"));
		return FReply::Handled();
	}

	// Setup scene capture component
	Aurora->CreatePreviewSceneCapture();
	if (!Aurora->PreviewSceneCapture)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create scene capture"));
		Aurora->DestroyPreviewAurora();	// Cleanup on failure
		return FReply::Handled();
	}

	// Perform initial capture for preview
	Aurora->UpdatePreviewAurora();

	UE_LOG(LogTemp, Log, TEXT("Preview aurora system initialized successfully"));

	// ============================================================
	// Step 4: Load WBP class using WidgetBlueprint asset
	// ============================================================

	// Blueprint asset path (not the generated class path)
	// This loads the .uasset file directly from plugin Content folder
	FString WidgetBlueprintPath = TEXT("/VolumetricAurora/Widgets/WBP_AuroraElementsPainter.WBP_AuroraElementsPainter");

	// StaticLoadObject: Load UWidgetBlueprint asset
	// UWidgetBlueprint contains the Blueprint editor data and GeneratedClass
	// UObject::StaticClass() allows loading any asset type
	UObject* LoadedObject = StaticLoadObject(
		UObject::StaticClass(),
		nullptr,
		*WidgetBlueprintPath
	);

	if (!LoadedObject)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WBP asset from path: %s"), *WidgetBlueprintPath);
		UE_LOG(LogTemp, Error, TEXT("Troubleshooting:"));
		UE_LOG(LogTemp, Error, TEXT("  1. Verify WBP exists in Content Browser"));
		UE_LOG(LogTemp, Error, TEXT("  2. Right-click WBP → Copy Reference"));
		UE_LOG(LogTemp, Error, TEXT("  3. Extract path between quotes: '/Path/To/WBP.WBP'"));
		UE_LOG(LogTemp, Error, TEXT("  4. Ensure WBP is saved and compiled"));
		Aurora->DestroyPreviewAurora();
		return FReply::Handled();
	}

	// Cast to UWidgetBlueprint to access GeneratedClass
	// WidgetBlueprint is the asset type that contains compiled widget code
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedObject);

	if (!WidgetBP)
	{
		Aurora->DestroyPreviewAurora();
		UE_LOG(LogTemp, Error, TEXT("Loaded object is not a WidgetBlueprint: %s"), *LoadedObject->GetClass()->GetName());
		UE_LOG(LogTemp, Error, TEXT("Expected UWidgetBlueprint but got different type"));
		return FReply::Handled();
	}

	// Get the compiled class from Blueprint
	// GeneratedClass is the runtime UClass created by Blueprint compiler
	// This is what CreateWidget needs to instantiate the widget
	UClass* WidgetClass = WidgetBP->GeneratedClass;

	if (!WidgetClass)
	{
		Aurora->DestroyPreviewAurora();
		UE_LOG(LogTemp, Error, TEXT("WidgetBlueprint has no GeneratedClass"));
		UE_LOG(LogTemp, Error, TEXT("Solution: Open WBP in Editor and click 'Compile' button"));
		return FReply::Handled();
	}

	UE_LOG(LogTemp, Log, TEXT("Successfully loaded WBP class: %s"), *WidgetClass->GetName());

	// ============================================================
	// Step 5: Create widget instance
	// ============================================================

	// Get Editor world context for widget creation
	// Editor widgets need Editor world, not game world
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();

	if (!EditorWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get Editor world"));
		Aurora->DestroyPreviewAurora();
		return FReply::Handled();
	}

	// CreateWidget<T>: Unreal's factory function for UserWidgets
	// Template parameter must match or be parent of actual class
	Aurora->EditorPaintWidgetInstance = CreateWidget<UUserWidget>(EditorWorld, WidgetClass);

	if (!Aurora->EditorPaintWidgetInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create widget instance"));
		Aurora->DestroyPreviewAurora();
		return FReply::Handled();
	}

	// ============================================================
	// Step 6: Set TargetAurora reference
	// ============================================================

	// Set TargetAurora so Blueprint can call UpdatePreview()
	// Cast to base class to access C++ property directly
	if (UAuroraElementsPainterWidget* PainterWidget = Cast<UAuroraElementsPainterWidget>(Aurora->EditorPaintWidgetInstance))
	{
		PainterWidget->TargetAurora = Aurora;
		UE_LOG(LogTemp, Log, TEXT("TargetAurora set to painter widget: %p"), Aurora);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to cast widget to UAuroraElementsPainterWidget"));
	}

	// ============================================================
	// Step 7: Initialize painter with RenderTarget
	// ============================================================

	// Call Blueprint-implemented InitializePainter function
	// UFunction::FindFunction: Reflection system lookup by name
	UFunction* InitFunc = Aurora->EditorPaintWidgetInstance->FindFunction(FName("InitializePainter"));

	if (InitFunc)
	{
		// ProcessEvent: Execute Blueprint function from C++
		// Params struct must match function signature exactly
		struct FInitPainterParams
		{
			UTextureRenderTarget2D* InRenderTarget;
		};

		FInitPainterParams Params;
		Params.InRenderTarget = Aurora->ElementsRenderTarget;

		// Execute the Blueprint function with parameters
		Aurora->EditorPaintWidgetInstance->ProcessEvent(InitFunc, &Params);

		UE_LOG(LogTemp, Log, TEXT("InitializePainter called successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializePainter function not found in WBP"));
		UE_LOG(LogTemp, Warning, TEXT("Make sure WBP inherits from UAuroraElementsPainterWidget"));
	}

	// ============================================================
	// Step 8: Pass Preview RenderTarget to Widget
	// ============================================================

	/**
	 * SetPreviewRenderTarget: Configure preview image in WBP
	 *
	 * FindFunctionByName: Looks up Blueprint-exposed function
	 * - Function must be marked UFUNCTION(BlueprintCallable)
	 * - Returns nullptr if function doesn't exist
	 *
	 * ProcessEvent: Executes UFunction with parameters
	 * - Similar to Blueprint "Call Function" node
	 * - Requires parameter struct matching function signature
	 */
	if (UFunction* SetPreviewFunc = WidgetClass->FindFunctionByName(TEXT("SetPreviewRenderTarget")))
	{
		// Parameter struct must match function signature exactly
		// Function: void SetPreviewRenderTarget(UTextureRenderTarget2D* RenderTarget)
		struct FSetPreviewRT_Params
		{
			UTextureRenderTarget2D* RenderTarget;
		};

		FSetPreviewRT_Params Params;
		Params.RenderTarget = Aurora->PreviewCaptureTarget;

		// ProcessEvent: Execute function with parameters
		// &Params: Pointer to parameter struct
		Aurora->EditorPaintWidgetInstance->ProcessEvent(SetPreviewFunc, &Params);

		UE_LOG(LogTemp, Log, TEXT("Preview render target passed to widget successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SetPreviewRenderTarget function not found in WBP"));
		UE_LOG(LogTemp, Warning, TEXT("Make sure function is marked UFUNCTION(BlueprintCallable)"));
	}

	// ============================================================
	// Step 9: Create Slate window
	// ============================================================

	/**
	 * SWindow: Top-level Slate window container
	 *
	 * Properties:
	 * - Title: Window title bar text
	 * - ClientSize: Initial window dimensions (2048x2048 for high-res painting)
	 * - SupportsMaximize/Minimize: Window chrome buttons
	 * - SizingRule: UserSized allows manual resize
	 */
	Aurora->EditorPaintWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Edit Elements Map")))
		.ClientSize(FVector2D(1600.f, 900.f))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		.SizingRule(ESizingRule::UserSized);

	// ============================================================
	// Step 10: Create interactive preview viewport
	// ============================================================

	/**
	 * SAuroraPreviewViewport: Custom Slate widget providing orbit camera controls
	 *
	 * Features:
	 * - Left Mouse Drag: Orbit camera around aurora volume
	 * - Mouse Wheel: Zoom in/out
	 * - Displays PreviewCaptureTarget (scene capture output)
	 *
	 * TargetAurora is passed to access PreviewSceneCapture for camera updates
	 */
	Aurora->EditorPreviewViewport = SNew(SAuroraPreviewViewport)
		.TargetAurora(Aurora)
		.RenderTarget(Aurora->PreviewCaptureTarget);

	// ============================================================
	// Step 10.5: Convert UMG Widget to Slate Widget
	// ============================================================

	/**
	 * TakeWidget(): Converts UUserWidget to Slate widget
	 *
	 * UMG (UUserWidget) and Slate are separate UI systems:
	 * - UMG: Blueprint-friendly, uses UObject-based widgets
	 * - Slate: C++ native, uses SWidget-based widgets
	 *
	 * To embed UMG widget in Slate layout, must convert via TakeWidget()
	 * This creates a wrapper SWidget that hosts the UMG content
	 */
	TSharedRef<SWidget> PainterSlateWidget = Aurora->EditorPaintWidgetInstance->TakeWidget();

	// ============================================================
	// Step 10.6: Create IDetailsView for embedded property editing
	// ============================================================

	TSharedRef<SWidget> EmbeddedDetailsWidget = CreateEmbeddedDetailsPanel(Aurora);

	// ============================================================
	// Step 10.7: Construct Nested SSplitter Layout
	// ============================================================

	/**
	 * SSplitter: Resizable split container
	 *
	 * Layout Structure:
	 * ┌─────────────────────────┬──────────────────────────────┐
	 * │   Preview Viewport      │                              │
	 * │   (Top 50%)             │   Painter Widget             │
	 * ├─────────────────────────┤   (Right 70%)                │
	 * │   Details Panel         │                              │
	 * │   (Bottom 50%)          │                              │
	 * └─────────────────────────┴──────────────────────────────┘
	 *        Left 30%
	 */
	Aurora->EditorPaintWindow->SetContent(
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)  // Outer: Horizontal split

		// ====================================================================
		// Left Region (30%): Split vertically again
		// ====================================================================
		+SSplitter::Slot()
		.Value(0.3f)
		[
			SNew(SSplitter)
				.Orientation(Orient_Vertical)  // Inner: Vertical split

				// ----------------------------------------------------------------
				// Left Top (50%): Preview Viewport
				// ----------------------------------------------------------------
				+SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SScaleBox)
						.Stretch(EStretch::ScaleToFit)
						[
							SNew(SBox)
								.WidthOverride(2048.f)
								.HeightOverride(2048.f)
								[
									Aurora->EditorPreviewViewport.ToSharedRef()
								]
						]
				]

			// ----------------------------------------------------------------
			// Left Bottom (50%): Aurora Detail Panel
			// ----------------------------------------------------------------
			+SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SBox)
						.Padding(4.f)  // Add padding
						[
							SNew(SBorder)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								.Padding(4.f)
								[
									SNew(SVerticalBox)

										// Header Title
										+ SVerticalBox::Slot()
										.AutoHeight()
										.Padding(0, 0, 0, 4)
										[
											SNew(STextBlock)
												.Text(LOCTEXT("AuroraPropertiesHeader", "Aurora Properties"))
												.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
										]

										// Detail View (Scrollable Area)
										+ SVerticalBox::Slot()
										.FillHeight(1.f)
										[
											EmbeddedDetailsWidget
										]
								]
						]
				]
		]

	// ====================================================================
	// Right Region (70%): Texture Painter Widget
	// ====================================================================
	+SSplitter::Slot()
		.Value(0.7f)
		[
			SNew(SBox)
				[
					PainterSlateWidget
				]
		]
		);

	// ============================================================
	// Step 11: Register window close callback
	// ============================================================

	// Capture weak reference to Aurora for safe callback access
	TWeakObjectPtr<AVolumetricAurora> WeakAurora = Aurora;

	/**
	 * Window close callback handles cleanup of all paint window resources
	 *
	 * ## Implementation Principle
	 * - IDetailsView is managed by TSharedPtr, releasing reference auto-cleans
	 * - SetTimerForNextTick ensures window close animation completes before cleanup
	 * - Skip deferred cleanup during engine shutdown to prevent crashes
	 *
	 * ## Cleanup Order
	 * 1. Clear viewport (holds references to SceneCapture)
	 * 2. Destroy preview system (uses aurora's resources)
	 * 3. Clear window references
	 * 4. Clear widget instance
	 */
	Aurora->EditorPaintWindow->SetOnWindowClosed(
		FOnWindowClosed::CreateLambda([WeakAurora](const TSharedRef<SWindow>& ClosedWindow) mutable
			{
				// Skip deferred cleanup if editor is shutting down
				if (!GEditor || IsEngineExitRequested())
				{
					// Perform immediate cleanup without deferring
					if (WeakAurora.IsValid())
					{
						AVolumetricAurora* AuroraPtr = WeakAurora.Get();
						AuroraPtr->EditorPreviewViewport.Reset();
						AuroraPtr->DestroyPreviewAurora();
						AuroraPtr->EditorPaintWindow.Reset();
						AuroraPtr->EditorPaintWidgetInstance = nullptr;
					}
					return;
				}

				// SetTimerForNextTick: Executes lambda on next frame
				// Ensures window close animation/logic completes before cleanup
				GEditor->GetTimerManager()->SetTimerForNextTick([WeakAurora]() mutable
					{
						// IsValid(): Check if aurora still exists before accessing
						if (WeakAurora.IsValid())
						{
							AVolumetricAurora* AuroraPtr = WeakAurora.Get();

							// Cleanup order matters:
							// 1. Clear viewport first (holds references to SceneCapture)
							// 2. Destroy preview system (uses aurora's resources)
							// 3. Clear window references
							// 4. Clear widget instance
							AuroraPtr->EditorPreviewViewport.Reset();
							AuroraPtr->DestroyPreviewAurora();
							AuroraPtr->EditorPaintWindow.Reset();
							AuroraPtr->EditorPaintWidgetInstance = nullptr;

							UE_LOG(LogTemp, Log, TEXT("Paint window closed, all resources cleaned up"));
						}
					});
			})
	);

	// ============================================================
	// Step 12: Show window
	// ============================================================

	// FSlateApplication: Singleton managing all Slate UI
	// AddWindow: Registers window with application and displays it
	FSlateApplication::Get().AddWindow(Aurora->EditorPaintWindow.ToSharedRef());

	UE_LOG(LogTemp, Log, TEXT("Paint window opened successfully"));

	return FReply::Handled();
}

FReply FVolumetricAuroraDetailsCustomization::OnNewAuroraPresetButtonClicked()
{
	if (!SelectedAuroras.IsValidIndex(0) || !SelectedAuroras[0].IsValid())
	{
		return FReply::Handled();
	}

	EAppReturnType::Type ReturnType = AskSaving(SelectedAuroras[0].Get());
	if (ReturnType == EAppReturnType::Cancel)
	{
		return FReply::Handled();
	}

	// If the selector window is already open, just bring it to front and reuse it
	if (AuroraTypeSelectorWindow.IsValid())
	{
		// Ensure the window is still alive in Slate (not a stale pointer)
		if (FSlateApplication::Get().FindWidgetWindow(AuroraTypeSelectorWindow.ToSharedRef()).IsValid())
		{
			AuroraTypeSelectorWindow->BringToFront(true);
			return FReply::Handled();
		}

		// Window was closed/destroyed, clear the stale reference
		AuroraTypeSelectorWindow.Reset();
	}

	// Capture a weak reference to the IDetailCustomization base class
	TWeakPtr<IDetailCustomization> WeakCustomization = StaticCastSharedRef<IDetailCustomization>(AsShared());
	GEditor->GetTimerManager()->SetTimerForNextTick([WeakCustomization]()
		{
			// Skip if editor is shutting down or customization was destroyed
			if (!GEditor || IsEngineExitRequested())
			{
				return;
			}

			TSharedPtr<IDetailCustomization> Pinned = WeakCustomization.Pin();
			if (!Pinned.IsValid())
			{
				return;
			}

			FVolumetricAuroraDetailsCustomization* Customization = static_cast<FVolumetricAuroraDetailsCustomization*>(Pinned.Get());

			TSharedRef<SWindow> NewWindow = SNew(SWindow)
				.Title(FText::FromString("Select Aurora Type"))
				.SizingRule(ESizingRule::Autosized)
				.SupportsMaximize(false)
				.SupportsMinimize(false);

			// Cache the window reference to prevent multiple instances
			Customization->AuroraTypeSelectorWindow = NewWindow;

			// Clear cached reference when the window is closed
			NewWindow->SetOnWindowClosed(
				FOnWindowClosed::CreateLambda([WeakCustomization](const TSharedRef<SWindow>&)
					{
						// Only reset if not shutting down and customization still alive
						if (!IsEngineExitRequested())
						{
							if (TSharedPtr<IDetailCustomization> StillAlive = WeakCustomization.Pin())
							{
								static_cast<FVolumetricAuroraDetailsCustomization*>(StillAlive.Get())->AuroraTypeSelectorWindow.Reset();
							}
						}
					})
			);

			NewWindow->SetContent(SNew(SAuroraTypeSelectorWidget)
				.InParentWindow(NewWindow));

			FSlateApplication::Get().AddWindow(NewWindow);
		});

	return FReply::Handled();
}

FReply FVolumetricAuroraDetailsCustomization::OnSaveAuroraPresetButtonClicked()
{
	if (!SelectedAuroras[0]->TargetAurora || !SelectedAuroras[0]->SourcePreset)
	{
		return FReply::Handled();
	}
	UAuroraPresetManager* Manager = GEditor->GetEditorSubsystem<UAuroraPresetManager>();

	Manager->SavePreset(SelectedAuroras[0]->TargetAurora, FAssetData(SelectedAuroras[0]->SourcePreset).AssetName.ToString());
	return FReply::Handled();
}

FReply FVolumetricAuroraDetailsCustomization::OnSaveAsAuroraPresetButtonClicked()
{
	if (!SelectedAuroras[0]->TargetAurora)
	{
		return FReply::Handled();
	}
	TSharedRef<SWindow> NewWindow = SNew(SWindow)
		.Title(FText::FromString("Save Preset As"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	NewWindow->SetContent(SNew(SSavePresetAsWidget)
		.ParentWindow(NewWindow)
		.SourcePreset(SelectedAuroras[0]->TargetAurora));

	FSlateApplication::Get().AddWindow(NewWindow);
	return FReply::Handled();
}

bool FVolumetricAuroraDetailsCustomization::IsPotentialFlowPresetSelected() const
{
	if (SelectedAuroras.Num() == 0 || !SelectedAuroras[0].IsValid())
	{
		return false;
	}

	AVolumetricAurora* Aurora = SelectedAuroras[0].Get();

	// Check if TargetAurora is UPotentialFlowAuroraPreset
	return Aurora->TargetAurora && Aurora->TargetAurora->IsA<UPotentialFlowAuroraPreset>();
}

void FVolumetricAuroraDetailsCustomization::BuildAuroraPresetSection(
	IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category =
		DetailBuilder.EditCategory(
			"Aurora",
			FText::FromString("Aurora"),
			ECategoryPriority::Important
		);

	Category.AddCustomRow(FText::FromString("PresetSelector"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)

				// Label
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 4, 0)
				[
					SNew(STextBlock)
						.Text(FText::FromString("Aurora Preset"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]

				// Custom ComboBox
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4, 6)
				[
					BuildPresetDropdown()
				]
		];
}

void FVolumetricAuroraDetailsCustomization::BuildPresetOptions()
{
	PresetOptions.Empty();

	FAssetRegistryModule& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssetsByClass(
		UAuroraPresetBase::StaticClass()->GetClassPathName(),
		Assets,
		true
	);

	// Assets starting with "Default" come first, then sort by creation time (Oldest -> Newest)
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	Assets.Sort([&PlatformFile](const FAssetData& A, const FAssetData& B)
		{
			// Check priority for names starting with "Default"
			const bool bStartsWithDefaultA = A.AssetName.ToString().StartsWith(TEXT("Default"), ESearchCase::IgnoreCase);
			const bool bStartsWithDefaultB = B.AssetName.ToString().StartsWith(TEXT("Default"), ESearchCase::IgnoreCase);

			if (bStartsWithDefaultA != bStartsWithDefaultB)
			{
				return bStartsWithDefaultA; // If A is "Default...", it comes first
			}

			// Fallback to creation time sorting
			auto GetAssetTime = [&PlatformFile](const FAssetData& Asset) -> FDateTime
				{
					const FString Filename = FPackageName::LongPackageNameToFilename(
						Asset.PackageName.ToString(),
						TEXT(".uasset")
					);

					FFileStatData Stat = PlatformFile.GetStatData(*Filename);

					return (Stat.CreationTime != FDateTime::MinValue()) ? Stat.CreationTime : Stat.ModificationTime;
				};

			const FDateTime TimeA = GetAssetTime(A);
			const FDateTime TimeB = GetAssetTime(B);

			if (TimeA == TimeB)
			{
				return A.AssetName.LexicalLess(B.AssetName);
			}

			return TimeA < TimeB;
		});

	auto AddPresetGroup =
		[this, &Assets](const FString& Header, UClass* PresetClass, bool bAddDivider)
		{
			bool bHasAssets = false;
			for (const FAssetData& Asset : Assets)
			{
				if (Asset.GetClass()->IsChildOf(PresetClass))
				{
					bHasAssets = true;
					break;
				}
			}

			if (!bHasAssets) return;

			if (bAddDivider)
			{
				PresetOptions.Add(MakeShared<FAuroraPresetComboItem>(
					EAuroraPresetItemType::Divider, TEXT("")));
			}

			PresetOptions.Add(MakeShared<FAuroraPresetComboItem>(
				EAuroraPresetItemType::Header, Header));

			for (const FAssetData& Asset : Assets)
			{
				if (Asset.GetClass()->IsChildOf(PresetClass))
				{
					if (UAuroraPresetBase* Preset = Cast<UAuroraPresetBase>(Asset.GetAsset()))
					{
						PresetOptions.Add(MakeShared<FAuroraPresetComboItem>(
							EAuroraPresetItemType::Option,
							Preset->GetName(),
							Preset));
					}
				}
			}
		};

	AddPresetGroup(TEXT("Noise Aurora"), UNoiseAuroraPreset::StaticClass(), false);
	AddPresetGroup(TEXT("Spline Aurora"), USplineAuroraPreset::StaticClass(), true);
	AddPresetGroup(TEXT("Flow Aurora"), UPotentialFlowAuroraPreset::StaticClass(), true);
}

static FTableRowStyle NoHoverRowStyle =
FTableRowStyle(FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row"))
.SetActiveBrush(FSlateNoResource())
.SetInactiveBrush(FSlateNoResource())
.SetActiveHoveredBrush(FSlateNoResource())
.SetInactiveHoveredBrush(FSlateNoResource())
.SetSelectorFocusedBrush(FSlateNoResource());

void FVolumetricAuroraDetailsCustomization::CreatePresetListView()
{
	if (PresetListViewContainer.IsValid())
	{
		return;
	}

	PresetListViewContainer = SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		.Padding(2)
		[

			SAssignNew(PresetListView, SListView<TSharedPtr<FAuroraPresetComboItem>>)
				.ListItemsSource(&PresetOptions)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &FVolumetricAuroraDetailsCustomization::GeneratePresetRow)
				.OnSelectionChanged(this, &FVolumetricAuroraDetailsCustomization::OnPresetRowSelected)
		];
}

TSharedRef<SWidget>
FVolumetricAuroraDetailsCustomization::BuildPresetDropdown()
{
	BuildPresetOptions();

	return
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SComboButton)
				.HasDownArrow(false)
				.OnGetMenuContent_Lambda([this]()
					{
						return PresetListViewContainer.ToSharedRef();
					})
				.ButtonContent()
				[
					SNew(SBorder)
						.BorderImage(FStyleDefaults::GetNoBrush())
						.BorderBackgroundColor(FLinearColor(0.12f, 0.12f, 0.12f))
						.Padding(FMargin(1, 3))
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
										.Text_Lambda([this]()
											{
												if (SelectedAuroras.IsValidIndex(0) && SelectedAuroras[0].IsValid() && SelectedAuroras[0]->SourcePreset)
												{
													UPackage* AssetPackage = SelectedAuroras[0]->SourcePreset->GetOutermost();
													FString PresetName = FPaths::GetBaseFilename(AssetPackage->GetName());

													if (!SelectedAuroras[0]->SourcePreset->IsIdentical(SelectedAuroras[0]->TargetAurora))
													{
														PresetName += TEXT("*");
													}
													return FText::FromString(PresetName);
												}

												return FText::FromString(TEXT("Select Preset"));
											})
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
										.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
								]

							+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(FMargin(0, 0))
								[
									SNew(SImage)
										.Image(FAppStyle::GetBrush("Icons.ChevronDown"))
										.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
								]
						]
				]
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(4.f, 0.f, 0.f, 0.f))
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(FText::FromString("Find in Content Browser"))
				.OnClicked_Lambda([this]()
					{
						if (SelectedAuroras.IsValidIndex(0) && SelectedAuroras[0].IsValid())
						{
							FContentBrowserModule& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

							TArray<UObject*> Assets;
							Assets.Add(SelectedAuroras[0].Get()->SourcePreset);
							ContentBrowser.Get().SyncBrowserToAssets(Assets);

						}
						return FReply::Handled();
					})
				[
					SNew(SBox)
						.WidthOverride(16.f)
						.HeightOverride(16.f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
								.Image(FAppStyle::GetBrush("Icons.BrowseContent"))
								.ColorAndOpacity(FSlateColor::UseForeground())
						]
				]

		];

}

TSharedRef<ITableRow>
FVolumetricAuroraDetailsCustomization::GeneratePresetRow(
	TSharedPtr<FAuroraPresetComboItem> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	// ----- OPTION -----
	if (Item->Type == EAuroraPresetItemType::Option)
	{
		return SNew(STableRow<TSharedPtr<FAuroraPresetComboItem>>, OwnerTable)
			.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row"))
			.Padding(FMargin(6, 3))
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("  ") + Item->DisplayName))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(FLinearColor::White)
			];
	}

	// ----- HEADER -----
	if (Item->Type == EAuroraPresetItemType::Header)
	{
		return SNew(STableRow<TSharedPtr<FAuroraPresetComboItem>>, OwnerTable)
			.IsEnabled(false)
			.Padding(FMargin(1, 0, 0, 0))
			[
				SNew(STextBlock)
					.Text(FText::FromString(Item->DisplayName))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FLinearColor(0.34f, 0.34f, 0.34f))
			];
	}

	// ----- DIVIDER -----
	return SNew(STableRow<TSharedPtr<FAuroraPresetComboItem>>, OwnerTable)
		.IsEnabled(false)
		.ShowSelection(false)
		.Padding(FMargin(0, 0, 0, 2))
		[
			SNew(SSeparator)
				.Thickness(1.f)
				.SeparatorImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.ColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f))
		];
}

void FVolumetricAuroraDetailsCustomization::OnPresetRowSelected(
	TSharedPtr<FAuroraPresetComboItem> Item,
	ESelectInfo::Type)
{
	if (!Item.IsValid())
		return;

	if (Item->Type != EAuroraPresetItemType::Option)
		return;

	SelectedPresetItem = Item;

	for (TWeakObjectPtr<AVolumetricAurora> AuroraPtr : SelectedAuroras)
	{
		if (!AuroraPtr.IsValid())
			continue;

		EAppReturnType::Type ReturnType = AskSaving(AuroraPtr.Get());
		if (ReturnType == EAppReturnType::Cancel)
		{
			if (PresetListView.IsValid())
			{
				PresetListView->ClearSelection();
			}
			return;
		}


		FScopedTransaction Transaction(FText::FromString("Apply Aurora Preset"));

		AuroraPtr->ApplyPresetToTarget(
			Item->PresetAsset.Get()
		);
	}


	if (PresetMenuAnchor.IsValid())
	{
		PresetMenuAnchor->SetIsOpen(false);
	}
}

EAppReturnType::Type FVolumetricAuroraDetailsCustomization::AskSaving(AVolumetricAurora* Aurora)
{
	if (Aurora && Aurora->SourcePreset)
	{
		if (!Aurora->SourcePreset->IsIdentical(Aurora->TargetAurora))
		{
			UPackage* AssetPackage = Aurora->SourcePreset->GetOutermost();
			FString PresetName = FPaths::GetBaseFilename(AssetPackage->GetName());

			FText DialogText = FText::Format(
				NSLOCTEXT("VolumetricAurora", "SaveBeforeSwitch", "The current changes differ from the '{0}' preset. Would you like to save?")
				, FText::FromString(PresetName));


			EAppReturnType::Type Reply = FMessageDialog::Open(
				EAppMsgType::YesNoCancel,
				DialogText
			);

			if (Reply == EAppReturnType::Yes)
			{
				UAuroraPresetManager* PresetManager = GEditor->GetEditorSubsystem<UAuroraPresetManager>();
				PresetManager->SavePreset(Aurora->TargetAurora, PresetName);
			}
			return Reply;
		}
	}
	return EAppReturnType::No;
}

/**
 * @brief Creates the Aurora Detail Panel to be embedded in the Edit Elements Map window.
 *
 * @param TargetAurora Original Aurora Actor (target for Details display)
 * @return Slate widget wrapping the IDetailsView
 *
 * ## Implementation Principle
 *
 * ### FDetailsViewArgs Configuration
 * FDetailsViewArgs determines the appearance and behavior of IDetailsView:
 * - bHideSelectionTip: Hides "Select an object..." message
 * - bAllowSearch: Shows/hides property search bar
 * - bShowScrollBar: Shows/hides scrollbar
 * - NameAreaSettings: Name area config (Hidden, ReadOnly, Editable)
 *
 * ### CreateDetailView Call
 * FPropertyEditorModule's CreateDetailView() creates IDetailsView instance
 * At this point, no editing target is assigned yet (empty state)
 *
 * ### SetObject Call
 * SetObject() assigns the target UObject for editing
 * Upon calling, all UPROPERTY fields are immediately displayed in the UI
 *
 * ### Category Filtering (Optional)
 * SetCategoriesToShow() or SetCategoriesToHide() can filter
 * which categories are displayed or hidden
 * Example: Show only Aurora-related categories, hide Transform etc.
 *
 * ### Change Callback Binding
 * Bind delegate to OnFinishedChangingProperties()
 * Invoked when user completes property editing, triggers synchronization logic
 */
TSharedRef<SWidget> FVolumetricAuroraDetailsCustomization::CreateEmbeddedDetailsPanel(
	AVolumetricAurora* TargetAurora)
{
	// ========================================================================
	// ### FDetailsViewArgs Configuration
	// ========================================================================
	/**
	 * Key FDetailsViewArgs options:
	 * - bUpdatesFromSelection: Auto-update when editor selection changes
	 * - bLockable: Show lock button (ignore selection changes)
	 * - bAllowSearch: Show search bar (useful for many properties)
	 * - bShowPropertyMatrixButton: Show Property Matrix button
	 * - bShowOptions: Show options dropdown menu
	 * - bShowModifiedPropertiesOption: "Modified properties only" filter option
	 */
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;	// Independent from editor selection
	DetailsViewArgs.bLockable = false;				// Lock button unnecessary
	DetailsViewArgs.bAllowSearch = true;			// Enable search
	DetailsViewArgs.bHideSelectionTip = true;		// Hide selection tip
	DetailsViewArgs.bShowObjectLabel = false;		// Hide actor label
	DetailsViewArgs.bShowScrollBar = true;			// Show scrollbar
	DetailsViewArgs.bShowOptions = false;			// Hide options menu (cleaner UI)
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;	// Hide name area

	// ========================================================================
	// ### Create IDetailsView Instance
	// ========================================================================
	/**
	 * FPropertyEditorModule is Unreal Editor's property editor system
	 * CreateDetailView() creates a standalone Details Panel widget
	 *
	 * Returns TSharedRef<IDetailsView>
	 * IDetailsView inherits from SCompoundWidget, directly insertable into Slate hierarchy
	 */
	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	EmbeddedAuroraDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	// ========================================================================
	// ### Set Target Object for Editing
	// ========================================================================
	/**
	 * When SetObject() is called, all UPROPERTYs of the UObject are analyzed
	 * and converted to appropriate UI widgets (numbers->sliders, colors->color pickers, etc.)
	 *
	 * Note: Setting TargetAurora->TargetAurora (Preset)
	 * would only allow editing Preset properties (choose based on intended usage)
	 */
	EmbeddedAuroraDetailsView->SetObject(TargetAurora->TargetAurora);

	// ========================================================================
	// ### Category Filtering (Optional)
	// ========================================================================
	/**
	 * Displaying all properties can clutter the UI
	 * Show only Aurora-related properties, hide Transform and base Actor properties
	 *
	 * Method 1: Specify allowed categories (pass)
	 * EmbeddedAuroraDetailsView->SetCategoriesToShow({ "Aurora", "Flow Simulation" });
	 *
	 * Method 2: Specify hidden categories (non pass)
	 * EmbeddedAuroraDetialsView->SetCategoriesToHide({ "Transform", "Actor" });
	 *
	 * Note: Category names must match UPROPERTY Category specifiers
	 */
	TArray<FName> CategoriesToShow;
	CategoriesToShow.Add(TEXT("Aurora"));		// Aurora-related properties
	CategoriesToShow.Add(TEXT("Aurora|Flow"));	// Subcategories can be specified
	CategoriesToShow.Add(TEXT("Aurora|Rendering"));
	// EmbeddedAuroraDetailsView->SetCategoriesToShow(CategoriesToShow);

	// ========================================================================
	// ### Bind Property Change Callback
	// ========================================================================
	/**
	 * OnFinishedChangingProperties: Fires when user completes value editing
	 * (Does not fires during slider dragging, only once when drag ends)
	 *
	 * FOnFinishedChangingProperties takes const FPropertyChangedEvent& Parameter
	 * (not FSimpleDelegate)
	 *
	 * This callback executes PreviewActor and original Actor synchronization logic
	 */
	EmbeddedAuroraDetailsView->OnFinishedChangingProperties().AddSP(
		this,
		&FVolumetricAuroraDetailsCustomization::OnEmbeddedDetailsPropertyChanged
	);

	// ========================================================================
	// IDetailsView is an SWidget, can be returned directly
	// ========================================================================
	return EmbeddedAuroraDetailsView.ToSharedRef();
}

// ============================================================================
// OnEmbeddedDetailsPropertyChanged Implementation
// ============================================================================

/**
 * @brief Called when property editing is completed in the Detail Panel.
 *
 * @param PropertyChangedEvent Information about the changed property
 *
 * ## Implementation Principle
 *
 * ### Synchronization Strategy
 * The Detail Panel directly edits the original Aurora Actor
 * Therefore, the original is already modified; only PreviewActor need synchronization
 *
 * ### Synchronization Methods
 * 1. Full PReset duplication:Simple but has overhead
 * 2. Copy specific properties only: Efficient buf requires per-property handling
 * 3. Copy property groups: Middle ground, category-based synchronization
 *
 * Method 1 is used here; optimize to Method 2 if performance is critical
 */
void FVolumetricAuroraDetailsCustomization::OnEmbeddedDetailsPropertyChanged(
	const FPropertyChangedEvent& PropertyChangedEvent)
{
	// ========================================================================
	// ### Validation
	// ========================================================================
	if (SelectedAuroras.Num() == 0 || !SelectedAuroras[0].IsValid())
	{
		return;
	}

	AVolumetricAurora* OriginalAurora = SelectedAuroras[0].Get();

	// Check if PreviewActor exists
	if (!OriginalAurora->PreviewAuroraActor.IsValid())
	{
		return;
	}

	AVolumetricAurora* PreviewAurora = OriginalAurora->PreviewAuroraActor.Get();

	// ========================================================================
	// ### Log Changed Property Info (for debugging)
	// ========================================================================
	/**
	 * Key PropertyChangedEvent members:
	 * - GetPropertyName(): Name of the changed property
	 * - GetMemberPropertyName(): Member name if property is inside a struct
	 * - ChangeType: Type of change (ValueSet, ArrayAdd, ArrayRemove, etc.)
	 * - Property: FProperty pointer (access to reflection info)
	 */
	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	UE_LOG(LogTemp, Verbose, TEXT("Embedded Detail Property Changed: %s"),
		*PropertyName.ToString());

	// ========================================================================
	// ### Preset Synchronization (Method 1: Full Duplication)
	// ========================================================================
	/**
	 * CopyPropertiesForUnrelatedObjects: Utility for copying properties between UObjects
	 *
	 * Caveats:
	 * - Must be the same class type
	 * - Transient properties are not copied
	 * - Pointer properties are shallow copied (reference only)
	 *
	 * Here we target the Preset object to synchronize
	 * Aurora Actor configuration values
	 */
	if (OriginalAurora->TargetAurora && PreviewAurora->TargetAurora)
	{
		// Copy Preset properties
		UEngine::CopyPropertiesForUnrelatedObjects(
			OriginalAurora->TargetAurora,   // Source
			PreviewAurora->TargetAurora     // Destination
		);
	}

	// ========================================================================
	// ### Update Original Aurora Actor
	// ========================================================================
	/**
	 * IMPORTANT: Must call UpdateMaterialTarget() on the ORIGINAL actor
	 * The Detail Panel modifies OriginalAurora->TargetAurora directly,
	 * but the changes won't reflect in the Editor viewport without this call.
	 *
	 * UpdateMaterialTarget() updates:
	 * - Material parameters on VolumeBox
	 * - VolumeBox position (Altitude)
	 * - VolumeBox scale (AuroraAreaExtent, AuroraHeight)
	 */
	OriginalAurora->UpdateMaterialTarget();

	// ========================================================================
	// ### Update Preview Aurora
	// ========================================================================
	/**
	 * UpdatePreviewAurora handles all necessary updates for the preview:
	 * - Sets bShapeTextureDirty = true internally
	 * - Calls PreviewActor->Tick() which triggers FlowTick()
	 * - FlowTick() processes dirty flag and updates simulation
	 * - Captures scene at the end
	 */
	OriginalAurora->UpdatePreviewAurora();
}

#undef LOCTEXT_NAMESPACE
