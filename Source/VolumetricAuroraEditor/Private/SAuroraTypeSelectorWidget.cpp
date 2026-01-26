// Copyright (c) 2026 R&B. All rights reserved.

#include "SAuroraTypeSelectorWidget.h"
#include "SlateOptMacros.h"
#include "VolumetricAurora.h"
#include "AuroraPresetManager.h"
#include "VolumetricAuroraEditor.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Layout/SScaleBox.h"
#include "AssetToolsModule.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAuroraTypeSelectorWidget::Construct(const FArguments& InArgs)
{

	ParentWindow = InArgs._InParentWindow;

	PresetManager = GEditor->GetEditorSubsystem<UAuroraPresetManager>();

	InitAuroraTypes();

	// Initialize per-type hover fade animations (one curve per aurora type)
	HoverSeqs.SetNum(AuroraTypes.Num());
	HoverCurves.SetNum(AuroraTypes.Num());

	for (int32 i = 0; i < AuroraTypes.Num(); i++)
	{
		HoverSeqs[i] = FCurveSequence();
		HoverCurves[i] = HoverSeqs[i].AddCurve(0.f, 0.15f, ECurveEaseFunction::QuadInOut);
	}

	// 모든 SCompoundWidget은 하나의 자식 위젯만 가짐, 그게 ChildSlot
	ChildSlot
	[
		// VerticalBox -> 위에서 아래로 쌓이는 박스, + -> 아래에 줄 하나 추가
		SNew(SVerticalBox)

			+SVerticalBox::Slot()   // 슬롯에 그림, 버튼 넣는것
			.AutoHeight()			// 내용물 크기만큼 높이 설정
			.Padding(10)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 5) [CreateAuroraTypeButton(0, AuroraTypes[0])]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 5) [CreateAuroraTypeButton(1, AuroraTypes[1])]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 5) [CreateAuroraTypeButton(2, AuroraTypes[2])]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.f)
			[
				SNew(STextBlock)
					.Text_Lambda([this]() { return ValidationResult.ErrorMessage; })
					.ColorAndOpacity(FLinearColor::Red)
					.Visibility_Lambda([this]()
						{
							return ValidationResult.bIsValid ? EVisibility::Hidden : EVisibility::Visible;
						})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.f)
			[
				SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(FText::FromString("New Preset Name: "))
					]
					+SHorizontalBox::Slot()
					.MaxWidth(615)
					[
						SAssignNew(PresetNameBox, SEditableTextBox)
							.OnTextChanged(this, &SAuroraTypeSelectorWidget::OnTextChanged)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.f)
					[
						SNew(SButton).Text(FText::FromString("Confirm")).OnClicked(this, &SAuroraTypeSelectorWidget::OnAuroraConfirmClicked)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton).Text(FText::FromString("Cancel")).OnClicked(this, &SAuroraTypeSelectorWidget::OnAuroraCancelClicked)
					]
			]
		// Populate the widget
	];
	
}

TSharedRef<SWidget> SAuroraTypeSelectorWidget::CreateAuroraTypeButton(int Index, const FAuroraTypeInfo& InAuroraType)
{
	return SNew(SBox)
		.WidthOverride(256)
		.HeightOverride(512)
	[
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			// SelectedIndex랑 현재 그리고 있는 오로라 인덱스랑 같으면 테두리 하얀색으로 칠하고 아니면 투명 (호버시에는 회색)
			.BorderBackgroundColor_Lambda([this, Index]()
				{
					const bool bSelected = (SelectedIndex == Index);
					const float tHover = HoverCurves.IsValidIndex(Index) ? HoverCurves[Index].GetLerp() : 0.f;

					if (bSelected)
					{
						return FLinearColor(1.f, 1.f, 1.f, 0.7f);
					}

					return FLinearColor(0.95f, 0.95f, 0.95f, 0.2f * tHover);
				})
			/*.Padding_Lambda([this, Index]()
				{
					const bool bSelected = (SelectedIndex == Index);
					const float tHover = HoverCurves.IsValidIndex(Index) ? HoverCurves[Index].GetLerp() : 0.f;

					return (bSelected || tHover > 0.f) ? FMargin(1.f) : FMargin(0.f);
				})*/
			[
				SNew(SButton)
					.ContentPadding(0)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.OnHovered_Lambda([this, Index]() { HandleTypeHovered(Index); })
					.OnUnhovered_Lambda([this, Index]() { HandleTypeUnhovered(Index); })
					// 현재 그리고 있는 오로라 버튼 클릭하면 SelectedIndex 바꿔줌
					.OnClicked_Lambda([this, Index]()
						{
							SelectedIndex = Index;
							HandleTypeHovered(Index);
							return FReply::Handled();
						})
					[
						SNew(SOverlay)
						.Clipping(EWidgetClipping::ClipToBoundsAlways)

						// Image Layer
						+ SOverlay::Slot()
						[
							SNew(SBox)
								.RenderTransformPivot(FVector2D(0.5f, 0.5f))
								.RenderTransform(TAttribute<TOptional<FSlateRenderTransform>>::CreateLambda([this, Index]()
									{
										const float tHover = HoverCurves.IsValidIndex(Index) ? HoverCurves[Index].GetLerp() : 0.f;
										const float Scale = FMath::Lerp(1.0f, 1.04f, tHover);
										return FSlateRenderTransform(FScale2D(Scale, Scale));
									}))
								[
									SNew(SScaleBox)
										.Stretch(EStretch::ScaleToFill)
										[
											SNew(SImage)
												.Image(&InAuroraType.AuroraBrush)
										]
								]
						]

						// Description Layer
						+ SOverlay::Slot()
						[
							SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
								.Visibility(EVisibility::HitTestInvisible)

								// Fade the panel by scaling the background alpha
								.BorderBackgroundColor_Lambda([this, Index]()
									{
										const bool bSelected = (SelectedIndex == Index);
										const float tHover = HoverCurves.IsValidIndex(Index) ? HoverCurves[Index].GetLerp() : 0.f;
										const float t = bSelected ? 1.f : tHover;	// Stay visible when selected

										return FLinearColor(0.f, 0.f, 0.f, 0.4f * t);	// Semi-transparent black overlay
									})
								.Padding(FMargin(14.f))
								[
									SNew(STextBlock)
										.Text(FText::FromString(InAuroraType.Description))
										.AutoWrapText(true)
										.LineHeightPercentage(1.5f)
										.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))

										// Fade the text along with the background
										.ColorAndOpacity_Lambda([this, Index]()
											{
												const bool bSelected = (SelectedIndex == Index);
												const float tHover = HoverCurves.IsValidIndex(Index) ? HoverCurves[Index].GetLerp() : 0.f;
												const float t = bSelected ? 1.f : tHover;

												return FLinearColor(1.f, 1.f, 1.f, t);
											})
								]
						]

						// Type Name Layer
						+ SOverlay::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Bottom)
							.Padding(FMargin(0.f, 0.f, 0.f, 0.f))
							[
								SNew(SOverlay)

								+ SOverlay::Slot()
								[
									SNew(SBorder)
										.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
										.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.7f))
										.Padding(FMargin(0.f, 20.f))
								]

								+ SOverlay::Slot()
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
										.Text(FText::FromString(InAuroraType.Name))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(FLinearColor(1, 1, 1, 0.9f))
										.ShadowOffset(FVector2D(1.f, 1.f))
										.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 1.0f))
								]
							]
					]
			]
	];
}

void SAuroraTypeSelectorWidget::OnTextChanged(const FText& InText)
{
	ValidationResult = PresetManager->ValidatePresetName(InText.ToString());
}
FReply SAuroraTypeSelectorWidget::OnAuroraConfirmClicked()
{

	if (!PresetNameBox.IsValid())
	{
		return FReply::Handled();
	}
	FString NewPresetName = PresetNameBox->GetText().ToString();

	ValidationResult = PresetManager->ValidatePresetName(NewPresetName);
	if (SelectedIndex == -1)
	{
		ValidationResult.bIsValid = false;
		ValidationResult.ErrorMessage = INVTEXT("Aurora type must selected");
	}

	if (!ValidationResult.bIsValid)
	{
		return FReply::Unhandled();
	}

	FString PresetPath = PluginPath + TEXT("/AuroraPresets");
	UAuroraPresetBase* TargetAsset = Cast<UAuroraPresetBase>(StaticLoadObject(UAuroraPresetBase::StaticClass(), nullptr, *(PresetPath + TEXT("/") + NewPresetName), nullptr, LOAD_NoWarn | LOAD_Quiet));
	if (TargetAsset)
	{
		ValidationResult.bIsValid = false;
		ValidationResult.ErrorMessage = FText::FromString(TEXT("Preset name \"") + NewPresetName + TEXT("\" already exist"));
		return FReply::Handled();
	}
	if (!TargetAsset)
	{
		TGuardValue<bool> DelegateGuard(FVolumetricAuroraEditorModule::bIsCreatingAssetWhileSaving, true);
		// FAssetToolsModule gets AssetTools interface, remains after module unloads
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		TargetAsset = Cast<UAuroraPresetBase>(AssetTools.CreateAsset(NewPresetName, PresetPath, AuroraTypes[SelectedIndex].PresetClass, nullptr));
	}
	if (TargetAsset)
	{
		PresetManager = GEditor->GetEditorSubsystem<UAuroraPresetManager>();
		PresetManager->SavePreset(TargetAsset, NewPresetName);
	}

	if (ParentWindow.IsValid())
	{
		ParentWindow.ToSharedRef()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SAuroraTypeSelectorWidget::OnAuroraCancelClicked()
{
	if (ParentWindow.IsValid())
	{
		ParentWindow.ToSharedRef()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SAuroraTypeSelectorWidget::InitAuroraTypes()
{
	AuroraTypes.Empty();
	FAuroraTypeInfo NoiseType;
	FAuroraTypeInfo SplineType;
	FAuroraTypeInfo FlowType;

	NoiseType.Name = TEXT("NOISE AURORA");
	NoiseType.Description = TEXT("Creates a classic curtain aurora with soft, layered bands and lively motion. \nGreat for general-purpose shots, from gentle drifts to more intense activity.");
	NoiseType.PresetClass = UNoiseAuroraPreset::StaticClass();

	SplineType.Name = TEXT("SPLINE AURORA");
	SplineType.Description = TEXT("Creates a stylized ribbon-like aurora guided by splines. \nIdeal for long sweeping arcs, sharp vertical streaks, and distinctive shapes.");
	SplineType.PresetClass = USplineAuroraPreset::StaticClass();

	FlowType.Name = TEXT("FLOW AURORA");
	FlowType.Description = TEXT("Creates a dynamic aurora controlled by points and shaped by forces. \nSuited for storm-like scenes, featuring vortex swirls and turbulence.");
	FlowType.PresetClass = UPotentialFlowAuroraPreset::StaticClass();

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VolumetricAurora"));
	if (Plugin.IsValid())
	{
		PluginPath = TEXT("/") + Plugin->GetName();
		FString TexturePath = PluginPath + TEXT("/Textures/AuroraSelectorTextures/");

		UTexture2D* NoiseTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *(TexturePath + TEXT("NoiseAuroraTexture"))));
		if (NoiseTexture)
		{
			NoiseType.AuroraBrush.SetResourceObject(NoiseTexture);
			NoiseType.AuroraBrush.ImageSize = FVector2D(1024, 900);
		}
		AuroraTypes.Add(NoiseType);

		UTexture2D* SplineTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *(TexturePath + TEXT("SplineAuroraTexture"))));
		if (SplineTexture)
		{
			SplineType.AuroraBrush.SetResourceObject(SplineTexture);
			SplineType.AuroraBrush.ImageSize = FVector2D(1024, 900);
		}
		AuroraTypes.Add(SplineType);

		UTexture2D* FlowTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *(TexturePath + TEXT("FlowAuroraTexture"))));
		if (FlowTexture)
		{
			FlowType.AuroraBrush.SetResourceObject(FlowTexture);
			FlowType.AuroraBrush.ImageSize = FVector2D(1024, 900);
			FlowType.AuroraBrush.Tiling = ESlateBrushTileType::NoTile;		// TMP: 썸네일 이미지 모두 확정되면 추후 이미지 사이즈 방식 결정   
		}
		AuroraTypes.Add(FlowType);
	}
	
}

void SAuroraTypeSelectorWidget::HandleTypeHovered(int32 Index)
{
	if (HoverSeqs.IsValidIndex(Index))
	{
		HoverSeqs[Index].Play(AsShared());
	}
}

void SAuroraTypeSelectorWidget::HandleTypeUnhovered(int32 Index)
{
	if (HoverSeqs.IsValidIndex(Index))
	{
		HoverSeqs[Index].Reverse();
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
