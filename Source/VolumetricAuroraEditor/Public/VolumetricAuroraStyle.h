// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "Styling/SlateStyle.h"

class FVolumetricAuroraStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static const class ISlateStyle& Get();
	static FName GetStyleSetName();

private:
	static TSharedPtr<class FSlateStyleSet> StyleSet;
};
