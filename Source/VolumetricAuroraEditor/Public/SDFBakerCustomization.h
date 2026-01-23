#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"


class FSDFBakerCustomization : public IDetailCustomization
{

public:

	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	FReply OnAddSplineComponentButtonClicked();

	FReply OnSaveAsButtonClicked();

	class USplineSDFTextureBakerComponent* SelectedComponent = nullptr;
};