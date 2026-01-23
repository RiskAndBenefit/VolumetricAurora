#include "VolumetricAuroraStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateBrush.h"
#include "Engine/Texture2D.h"

TSharedPtr<FSlateStyleSet> FVolumetricAuroraStyle::StyleSet = nullptr;

FName FVolumetricAuroraStyle::GetStyleSetName()
{
	return FName(TEXT("AuroraStyle"));
}

void FVolumetricAuroraStyle::Initialize()
{
	if (!StyleSet.IsValid())
	{
		StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

		// 콘텐츠 폴더의 텍스처 로드
		// 경로: /VolumetricAurora/Icon/T_Aurora_Icon.T_Aurora_Icon
		UTexture2D* IconTexture = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/VolumetricAurora/Icon/T_Aurora_Icon.T_Aurora_Icon")
		);

		if (IconTexture)
		{
			// 아웃라이너 아이콘 (16x16)
			StyleSet->Set(
				"ClassIcon.VolumetricAurora",
				new FSlateImageBrush(IconTexture, FVector2D(16.0f, 16.0f))
			);

			// 썸네일 아이콘 (32x32)
			StyleSet->Set(
				"ClassThumbnail.VolumetricAurora",
				new FSlateImageBrush(IconTexture, FVector2D(32.0f, 32.0f))
			);

			FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load Aurora Icon at: /VolumetricAurora/Icon/T_Aurora_Icon.T_Aurora_Icon"));
		}
	}
}

void FVolumetricAuroraStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(GetStyleSetName());
		StyleSet.Reset();
	}
}

const ISlateStyle& FVolumetricAuroraStyle::Get()
{
	if (!StyleSet.IsValid())
	{
		Initialize();
	}
	return *StyleSet;
}
