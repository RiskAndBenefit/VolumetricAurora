// Copyright Epic Games, Inc. All Rights Reserved.

#include "VolumetricAuroraDetailsCustomization.h"
#include "VolumetricAurora.h"
#include "AuroraPresetAsset.h"
#include "AuroraPresetManager.h"
#include "SSavePresetAsWidget.h"
#include "AuroraElementsPainterWidget.h"
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

#include "SAuroraTypeSelectorWidget.h"

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
	
	// // Add custom UI to "Aurora|Preset Details|Flow" category
	// IDetailCategoryBuilder& FlowCategory = DetailBuilder.EditCategory(
	// 	TEXT("Aurora|PresetDetails|Flow"),
	// 	FText::GetEmpty(),
	// 	ECategoryPriority::Important
	// );

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

	// 프리셋 커스텀 UI 관련 (추후 완성 시 주석 해제)
	/*DetailBuilder.HideProperty(
		GET_MEMBER_NAME_CHECKED(AVolumetricAurora, CurrentPresetName));*/

	BuildPresetOptions();
	CreatePresetListView();
	BuildAuroraPresetSection(DetailBuilder);
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
	// Step 2: Initialize RenderTarget
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
	// Step 3: Check for existing window
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
			Aurora->DestroyPreviewAurora();
			return FReply::Handled();
		}
	}

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

	// SNew: Slate macro for creating widgets (similar to MakeShareable)
	// SWindow: Top-level window container
	Aurora->EditorPaintWindow = SNew(SWindow)
		.Title(LOCTEXT("PaintWindowTitle", "Aurora Elements Painter"))
		.ClientSize(FVector2D(2048, 2048))		// Window size in pixels
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		.SizingRule(ESizingRule::UserSized)					// Allow user to resize
		.IsTopmostWindow(false)								// Not always on top
		.FocusWhenFirstShown(true);							// Auto-focus when opened

	// ============================================================
	// Step 10: Convert UMG widget to Slate widget
	// ============================================================

	// TakeWidget(): Converts UUserWidget (UMG) to SWidget (Slate)
	// This creates the Slate widget tree from Blueprint hierarchy
	// IMPORTANT: Can only be called once per widget instance
	TSharedRef<SWidget> SlateWidget = Aurora->EditorPaintWidgetInstance->TakeWidget();

	Aurora->EditorPaintWindow->SetContent(
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SlateWidget
		]
	);
	
	// Set window content to our UMG widget
	Aurora->EditorPaintWindow->SetContent(SlateWidget);

	// ============================================================
	// Step 11: Register window close callback
	// ============================================================

	// Capture weak reference to Aurora for safe callback access
	TWeakObjectPtr<AVolumetricAurora> WeakAurora = Aurora;

	/**
	 * SetOnWindowClosed: Callback fired when user closes window
	 *
	 * Use lambda to capture weak pointer:
	 * - Safe: Won't crash if aurora is destroyed before window closes
	 * - Clean: Automatically cleans up all preview resources
	 *
	 * Timer usage: Delay cleanup by one frame to ensure window close completes
	 */
	// OnWindowClosed is called when user closes the window
	Aurora->EditorPaintWindow->SetOnWindowClosed(
		FOnWindowClosed::CreateLambda([WeakAurora](const TSharedRef<SWindow>& ClosedWindow) mutable
		{
			if (GEditor)
			{
				// SetTimerForNextTick: Executes lambda on next frame
				// Ensures window close animation/logic completes before cleanup
				GEditor->GetTimerManager()->SetTimerForNextTick([WeakAurora]() mutable
				{
					// IsValid(): Check if aurora still exists before accessing
					if (WeakAurora.IsValid())
					{
						AVolumetricAurora* AuroraPtr = WeakAurora.Get();

						// Cleanup order matters:
						// 1. Destroy preview system first (uses aurora's resources)
						// 2. Clear window references
						// 3. Clear widget instance
						AuroraPtr->DestroyPreviewAurora();
						AuroraPtr->EditorPaintWindow.Reset();
						AuroraPtr->EditorPaintWidgetInstance = nullptr;

						UE_LOG(LogTemp, Log, TEXT("Paint window closed, all resources cleaned up"));
					}
				});
			}
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

FReply FVolumetricAuroraDetailsCustomization::OnBakeToTextureClicked()
{
	if (SelectedAuroras.Num() == 0 || !SelectedAuroras[0].IsValid())
	{
		return FReply::Handled();
	}

	AVolumetricAurora* Aurora = SelectedAuroras[0].Get();

	if (!Aurora->ElementsRenderTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("No render target to bake"));
		return FReply::Handled();
	}

	UTextureRenderTarget2D* RT = Aurora->ElementsRenderTarget;

	// Step 1: Read pixels from render target
	// Allocate CPU-side buffer for pixel data
	TArray<FColor> SurfaceData;

	// Get RenderTarget's GPU resource
	FTextureRenderTargetResource* RTResource = RT->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get RenderTarget resource"));
		return FReply::Handled();
	}

	// Read pixels from GPU to CPU
	// This is a blocking operation (waits for GPU to finish rendering)
	if (!RTResource->ReadPixels(SurfaceData))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read pixels from RenderTarget"));
		return FReply::Handled();
	}

	// Step 2: Create Texture2D asset
	// Generate unique asset name with timestamp
	FString AssetName = FString::Printf(
		TEXT("T_AuroraElements_%s_%s"),
		*Aurora->GetName(),
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))
	);

	// Asset path in Content Browser (Plugin Content folder)
	// /VolumetricAurora/ refers to plugin mount point, not /Game/
	FString PackagePath = TEXT("/VolumetricAurora/Textures/FlowElementMapTextures/");
	FString PackageName = PackagePath + AssetName;

	// Create package (container for asset)
	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	// Create Texture2D object
	UTexture2D* NewTexture = NewObject<UTexture2D>(
		Package,
		*AssetName,
		RF_Public | RF_Standalone | RF_MarkAsRootSet
	);

	// Step 3: Configure Texture2D properties
	NewTexture->Source.Init(
		RT->SizeX,			// Width
		RT->SizeY,			// Height
		1,					// NumSlices (1 for 2D texture)
		1,					// NumMips (generate mips later)
		TSF_BGRA8			// Source format (matches FColor)
	);

	// Step 4: Copy pixel data to Texture2D
	// Lock texture for writing
	uint8* MipData = NewTexture->Source.LockMip(0);

	// Copy pixel array
	FMemory::Memcpy(
		MipData,
		SurfaceData.GetData(),
		SurfaceData.Num() * sizeof(FColor)
	);

	// Unlock texture
	NewTexture->Source.UnlockMip(0);

	// Step 5: Update texture settings
	NewTexture->SRGB = false;							// Linear color space (for data texture)
	NewTexture->CompressionSettings = TC_VectorDisplacementmap;	// RGB only, ignore alpha
	NewTexture->MipGenSettings = TMGS_NoMipmaps;		// No mipmaps for UI texture
	NewTexture->AddressX = TA_Clamp;					// Clamp at edges
	NewTexture->AddressY = TA_Clamp;
	NewTexture->Filter = TF_Bilinear;					// Bilinear filtering
	NewTexture->AlphaCoverageThresholds = FVector4(0, 0, 0, 0);	// Disable alpha channel display

	// Step 6: Build texture (compile for GPU)
	NewTexture->UpdateResource();

	// Step 7: Save asset to disk
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	// Save package using UE5-compatible API
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension()
	);

	// Configure save parameters using FSavePackageArgs structure
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bForceByteSwapping = false;
	SaveArgs.bWarnOfLongFilename = true;

	// UPackage::Save returns FSavePackageResultStruct in UE5
	FSavePackageResultStruct SaveResult = UPackage::Save(
		Package,
		NewTexture,
		*PackageFileName,
		SaveArgs
	);

	// Check if save was successful
	bool bSaved = (SaveResult.Result == ESavePackageResult::Success);

	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("Baked texture saved: %s"), *PackageName);

		// Optional: Auto-assign baked texture to AuroraElementsMap
		if (UPotentialFlowAuroraPreset* FlowPreset = Cast<UPotentialFlowAuroraPreset>(Aurora->TargetAurora))
		{
			FlowPreset->Modify();	// Mark for undo
			FlowPreset->AuroraElementsMap = NewTexture;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save baked texture"));
	}
	
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

	GEditor->GetTimerManager()->SetTimerForNextTick([]()
		{
			TSharedRef<SWindow> NewWindow = SNew(SWindow)
				.Title(FText::FromString("Select Aurora Type"))
				.SizingRule(ESizingRule::Autosized)
				.SupportsMaximize(false)
				.SupportsMinimize(false);

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

	auto AddPresetGroup =
		[this, &Assets](const FString& Header, UClass* PresetClass, bool bAddDivider)
		{
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
					if (UAuroraPresetBase* Preset =
						Cast<UAuroraPresetBase>(Asset.GetAsset()))
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
		+SHorizontalBox::Slot()
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
					.Padding(FMargin(1, 2))
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
		.Padding(FMargin(6.f, 0.f, 0.f, 0.f))
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
					SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.BrowseContent"))
						.ColorAndOpacity(FSlateColor::UseForeground())
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

#undef LOCTEXT_NAMESPACE
