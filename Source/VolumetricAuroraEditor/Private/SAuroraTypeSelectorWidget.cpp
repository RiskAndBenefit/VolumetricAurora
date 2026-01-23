// Fill out your copyright notice in the Description page of Project Settings.


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
	return SNew(SBorder)
		// SelectedIndex랑 현재 그리고 있는 오로라 인덱스랑 같으면 백그라운드 노란색으로 칠하고 아니면 투명
		.BorderBackgroundColor_Lambda([this, Index]()
			{
				return SelectedIndex == Index ? FLinearColor::Yellow : FLinearColor::Transparent;
			})
		[
			SNew(SButton)
				// 현재 그리고 있는 오로라 버튼 클릭하면 SelectedIndex 바꿔줌
				.OnClicked_Lambda([this, Index]()
					{
						SelectedIndex = Index;
						return FReply::Handled();
					})
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							// 이미지 크기 강제설정
							SNew(SBox).WidthOverride(256).HeightOverride(512)
								[
									SNew(SScaleBox)
										.Stretch(EStretch::ScaleToFill)
										[
											SNew(SImage)
												.Image(&InAuroraType.AuroraBrush)
										]
								]
						]
					+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SNew(SHorizontalBox)
								+SHorizontalBox::Slot().HAlign(HAlign_Center)
								[
								SNew(STextBlock)
									.Text(FText::FromString(InAuroraType.Name))
									.Font(FAppStyle::GetFontStyle("NormalFont"))
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
	UAuroraPresetBase* TargetAsset = Cast<UAuroraPresetBase>(StaticLoadObject(UAuroraPresetBase::StaticClass(), nullptr, *(PresetPath + TEXT("/") + NewPresetName)));
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

	NoiseType.Name = TEXT("Noise Type");
	NoiseType.Description = TEXT("Noise Type Aurora Description");
	NoiseType.PresetClass = UNoiseAuroraPreset::StaticClass();

	SplineType.Name = TEXT("Spline Type");
	SplineType.Description = TEXT("Spline Type Aurora Description");
	SplineType.PresetClass = USplineAuroraPreset::StaticClass();

	FlowType.Name = TEXT("Flow Type");
	FlowType.Description = TEXT("Flow Type Aurora Description");
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
		}
		AuroraTypes.Add(FlowType);
	}
	
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
