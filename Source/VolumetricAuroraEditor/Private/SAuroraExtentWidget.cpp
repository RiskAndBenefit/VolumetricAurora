// Fill out your copyright notice in the Description page of Project Settings.


#include "SAuroraExtentWidget.h"
#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAuroraExtentWidget::Construct(const FArguments& InArgs)
{
	Value = InArgs._Value;
	OnValueChanged = InArgs._OnValueChanged;
	Sensitivity = InArgs._Sensitivity;
	
	ChildSlot
	[
		// Populate the widget
		SAssignNew(TextBlock, STextBlock)
			.Text_Lambda([this]() -> FText
				{
					return FText::AsNumber(Value.Get());
				})
	];
	
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SAuroraExtentWidget::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = true;
		NewValue = Value.Get();

		StartPos = MouseEvent.GetScreenSpacePosition();

		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);
		// SharedThis -> SharedRef of this
		return FReply::Handled()
			.CaptureMouse(SharedThis(this))
			.UseHighPrecisionMouseMovement(SharedThis(this));
	}
	return FReply::Unhandled();
}

FReply SAuroraExtentWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bIsDragging)
	{
		return FReply::Unhandled();
	}
	float Delta = MouseEvent.GetCursorDelta().X;

	NewValue += (Delta * Sensitivity);

	if (OnValueChanged.IsBound())
	{
		OnValueChanged.Execute(NewValue);
	}

	
	return FReply::Handled();
}

FReply SAuroraExtentWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		bIsDragging = false;
		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(true);
		FSlateApplication::Get().SetCursorPos(StartPos);		
		return FReply::Handled()
			.ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}
