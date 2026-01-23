// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class VOLUMETRICAURORAEDITOR_API SAuroraExtentWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAuroraExtentWidget)
	{}
		SLATE_ATTRIBUTE(float, Value)
		// FOnFloatValueChanged OnValueChanged -> OnValueChanged_Lambda(Lambda){_OnValueChanged = Lambda;}
		SLATE_EVENT(FOnFloatValueChanged, OnValueChanged)
		SLATE_ARGUMENT(float, Sensitivity)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;


private:
	TAttribute<float> Value = 0.0f;

	float NewValue = 0.0f;

	float Sensitivity = 0.0f;

	FVector2D StartPos;

	FOnFloatValueChanged OnValueChanged;

	TSharedPtr<STextBlock> TextBlock;

	bool bIsDragging = false;
};
