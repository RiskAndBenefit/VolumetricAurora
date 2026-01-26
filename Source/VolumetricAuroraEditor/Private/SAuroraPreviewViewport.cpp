// Copyright (c) 2026 R&B. All rights reserved.

#include "SAuroraPreviewViewport.h"
#include "VolumetricAurora.h"

#include "Widgets/Images/SImage.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Framework/Application/SlateApplication.h"
#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAuroraPreviewViewport::Construct(const FArguments& InArgs)
{
	// Store references from construction arguments
	TargetAurora = InArgs._TargetAurora;
	RenderTarget = InArgs._RenderTarget;

	// ========================================================================
	// Calculate Dynamic Zoom Limits Based on VolumeBox Scale
	// ========================================================================
	//
	// The zoom distance limits are calculated from the preview aurora's
	// VolumeBox dimensions to ensure appropriate viewing range for any
	// aurora size configuration.

	CalculateZoomLimits();

	// ========================================================================
	// Initialize Camera State
	// ========================================================================
	//
	// CameraYaw and CameraPitch are initialized to default values (-90, -80)
	// in the class definition, providing a top-down view.
	// Only CameraDistance needs to be set here based on calculated defaults.

	CameraDistance = DefaultZoomDistance;

	// ========================================================================
	// Build Widget Hierarchy
	// ========================================================================
	//
	// Structure:
	// - SImage: Displays the render target from SceneCapture2D
	//
	// The image fills the entire widget area and receives all mouse input
	// for orbit/zoom controls.

	ChildSlot
	[
		SAssignNew(PreviewImage, SImage)
			.Image_Lambda([this]() -> const FSlateBrush*
			{
				// Dynamic brush creation from render target
				// Returns empty brush if render target is invalid
				static FSlateBrush DynamicBrush;

				if (RenderTarget.IsValid())
				{
					DynamicBrush.SetResourceObject(RenderTarget.Get());
					DynamicBrush.ImageSize = FVector2D(
						RenderTarget->SizeX,
						RenderTarget->SizeY
					);
				}

				return &DynamicBrush;
			})
	];

	// Apply initial camera transform
	UpdateCameraTransform();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

// ============================================================================
// Mouse Input Handlers
// ============================================================================

FReply SAuroraPreviewViewport::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	// ========================================================================
	// Left Mouse Button: Begin Orbit Mode
	// ========================================================================
	//
	// When user presses LMB, we enter orbit mode:
	// 1. Capture mouse to receive all mouse events (even outside widget bounds)
	// 2. Enable high-precision mouse movement for smooth camera rotation
	// 3. Hide cursor for cleaner visual feedback during drag
	// 4. Store start position for cursor restoration on release

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsOrbiting = true;
		DragStartPosition = MouseEvent.GetScreenSpacePosition();

		// Hide cursor during drag (matches Unreal Editor viewport behavior)
		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);

		// CaptureMouse: Ensures this widget receives all mouse events
		// UseHighPrecisionMouseMovement: Provides sub-pixel mouse delta for smooth rotation
		return FReply::Handled()
			.CaptureMouse(SharedThis(this))
			.UseHighPrecisionMouseMovement(SharedThis(this));
	}

	return FReply::Unhandled();
}

FReply SAuroraPreviewViewport::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// ========================================================================
	// Yaw/Pitch Orbit Camera (Unlimited Rotation, Roll Fixed to Zero)
	// ========================================================================
	//
	// Mouse delta directly updates Yaw and Pitch angles:
	//
	// 1. Horizontal movement (Delta.X) -> Yaw change
	//    - No limits: Full 360-degree horizontal rotation
	//
	// 2. Vertical movement (Delta.Y) -> Pitch change
	//    - No limits: Can rotate past poles (top/bottom)
	//    - When crossing ±90 degrees, camera smoothly transitions to other side
	//
	// Roll is always zero. When pitch crosses poles, yaw is adjusted by 180
	// degrees to maintain correct orientation without any "flip" sensation.

	if (!bIsOrbiting)
	{
		return FReply::Unhandled();
	}

	// Get cursor movement delta since last frame
	FVector2D Delta = MouseEvent.GetCursorDelta();

	// ========================================================================
	// Update Yaw (Horizontal Orbit)
	// ========================================================================

	CameraYaw += Delta.X * OrbitSensitivity;
	// Keep yaw bounded for numerical stability
	CameraYaw = FMath::UnwindDegrees(CameraYaw);

	// ========================================================================
	// Update Pitch (Vertical Orbit)
	// ========================================================================
	//
	// No clamping - unlimited vertical rotation allowed

	CameraPitch += -Delta.Y * OrbitSensitivity;

	// Apply new camera transform to SceneCapture component
	UpdateCameraTransform();

	return FReply::Handled();
}

FReply SAuroraPreviewViewport::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// ========================================================================
	// End Orbit Mode on LMB Release
	// ========================================================================
	//
	// Cleanup operations:
	// 1. Restore cursor visibility
	// 2. Reset cursor to original drag start position
	// 3. Release mouse capture

	if (bIsOrbiting && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsOrbiting = false;

		// Restore cursor visibility
		FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(true);

		// Reset cursor to drag start position
		// This prevents cursor from jumping to unexpected location after orbit
		FSlateApplication::Get().SetCursorPos(DragStartPosition);

		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply SAuroraPreviewViewport::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// ========================================================================
	// Zoom Control via Mouse Wheel
	// ========================================================================
	//
	// Scroll wheel adjusts camera distance from focus point:
	// - Scroll Up (positive delta): Zoom in (decrease distance)
	// - Scroll Down (negative delta): Zoom out (increase distance)
	//
	// Distance change is proportional to current distance for consistent zoom feel:
	// - At close range: Small absolute distance change
	// - At far range: Larger absolute distance change
	//
	// Distance limits are dynamically calculated based on VolumeBox scale

	float WheelDelta = MouseEvent.GetWheelDelta();

	// Calculate distance change as percentage of current distance
	// Negative sign: Scroll up (positive delta) should zoom IN (decrease distance)
	float DistanceChange = -WheelDelta * ZoomSensitivity * CameraDistance;

	// Apply distance change with dynamic clamping
	CameraDistance += DistanceChange;
	CameraDistance = FMath::Clamp(CameraDistance, MinZoomDistance, MaxZoomDistance);

	// Update camera position
	UpdateCameraTransform();

	return FReply::Handled();
}

// ============================================================================
// Public Interface
// ============================================================================

void SAuroraPreviewViewport::SetRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
	RenderTarget = InRenderTarget;

	// Force widget redraw with new render target
	if (PreviewImage.IsValid())
	{
		PreviewImage->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SAuroraPreviewViewport::SetTargetAurora(AVolumetricAurora* InTargetAurora)
{
	TargetAurora = InTargetAurora;

	// Recalculate zoom limits for new aurora
	CalculateZoomLimits();

	// Reset camera view for new aurora
	ResetCameraView();
}

void SAuroraPreviewViewport::ResetCameraView()
{
	// ========================================================================
	// Reset to Default Camera Position
	// ========================================================================
	//
	// Default view is nearly top-down, matching the original fixed camera setup.
	// Yaw = -90 degrees (looking along -Y axis)
	// Pitch = -80 degrees (looking down from above)

	// Recalculate zoom limits in case VolumeBox changed
	CalculateZoomLimits();

	// Reset Yaw/Pitch to default values
	CameraYaw = -90.0f;
	CameraPitch = -80.0f;

	// Set distance to default value
	CameraDistance = DefaultZoomDistance;

	UpdateCameraTransform();
}

// ============================================================================
// Internal Helper Functions
// ============================================================================

void SAuroraPreviewViewport::UpdateCameraTransform()
{
	// ========================================================================
	// Apply Yaw/Pitch to SceneCapture Camera (FQuat-based, No FRotator)
	// ========================================================================
	//
	// Camera orientation is built from separate Yaw and Pitch quaternions:
	// 1. YawQuat: Rotation around world up (Z-axis)
	// 2. PitchQuat: Rotation around the right axis after yaw
	// 3. Final quaternion = PitchQuat * YawQuat (no accumulation, no roll drift)
	//
	// IMPORTANT: Using FQuat directly instead of FRotator avoids the
	// ±90° pole singularity where FRotator normalization causes 180° flips.

	if (!TargetAurora.IsValid())
	{
		return;
	}

	USceneCaptureComponent2D* SceneCapture = TargetAurora->PreviewSceneCapture;
	if (!SceneCapture)
	{
		return;
	}

	FVector FocusPoint = GetFocusPoint();

	// ========================================================================
	// Build Camera Quaternion from Yaw + Pitch (Roll = 0)
	// ========================================================================

	const float YawRad = FMath::DegreesToRadians(CameraYaw);
	const float PitchRad = FMath::DegreesToRadians(CameraPitch);

	// Yaw around world up
	const FQuat YawQuat(FVector::UpVector, YawRad);

	// Pitch around the right axis after yaw (keeps roll fixed)
	const FVector RightAfterYaw = YawQuat.RotateVector(FVector::RightVector);
	const FQuat PitchQuat(RightAfterYaw, PitchRad);

	// Combine: apply yaw first, then pitch
	const FQuat CameraQuat = PitchQuat * YawQuat;

	// ========================================================================
	// Calculate Camera Position and Apply Transform
	// ========================================================================

	// Camera looks along +X in UE, position it behind focus point
	const FVector Forward = CameraQuat.RotateVector(FVector::ForwardVector);
	const FVector CameraPosition = FocusPoint - Forward * CameraDistance;

	SceneCapture->SetWorldLocation(CameraPosition);
	SceneCapture->SetWorldRotation(CameraQuat);  // Use FQuat directly, not FRotator

	// Manually trigger scene capture (preview world doesn't auto-tick)
	SceneCapture->CaptureScene();
}

FVector SAuroraPreviewViewport::GetFocusPoint() const
{
	// ========================================================================
	// Get Camera Focus Point
	// ========================================================================
	//
	// The focus point is the center of the preview aurora's VolumeBox.
	// This is where the camera orbits around and always looks at.
	//
	// Fallback: Returns zero vector if references are invalid

	if (!TargetAurora.IsValid())
	{
		return FVector::ZeroVector;
	}

	// Get focus point from preview aurora actor
	if (!TargetAurora->PreviewAuroraActor.IsValid())
	{
		return FVector::ZeroVector;
	}

	AVolumetricAurora* PreviewActor = TargetAurora->PreviewAuroraActor.Get();
	if (!PreviewActor->VolumeBox)
	{
		return FVector::ZeroVector;
	}

	// Return center of VolumeBox
	return PreviewActor->VolumeBox->GetComponentLocation();
}

void SAuroraPreviewViewport::CalculateZoomLimits()
{
	// ========================================================================
	// Calculate Dynamic Zoom Distance Limits from VolumeBox Scale
	// ========================================================================
	//
	// The zoom limits are calculated based on the aurora's VolumeBox dimensions
	// to ensure appropriate viewing range for any aurora size:
	//
	// - MinZoomDistance: VolumeScale.Z * 0.5f
	//   Places camera at the edge of volume when fully zoomed in
	//
	// - DefaultZoomDistance: VolumeScale.Z * 0.5f + MaxDimension * 60.f
	//   Provides balanced initial view (matches original camera offset)
	//
	// - MaxZoomDistance: VolumeScale.Z * 0.5f + MaxDimension * 120.f
	//   Allows viewing entire aurora from considerable distance

	if (!TargetAurora.IsValid() || !TargetAurora->PreviewAuroraActor.IsValid())
	{
		// Fallback to default values if references invalid
		MinZoomDistance = 1000.0f;
		DefaultZoomDistance = 10000.0f;
		MaxZoomDistance = 100000.0f;
		return;
	}

	AVolumetricAurora* PreviewActor = TargetAurora->PreviewAuroraActor.Get();
	if (!PreviewActor->VolumeBox)
	{
		MinZoomDistance = 1000.0f;
		DefaultZoomDistance = 10000.0f;
		MaxZoomDistance = 100000.0f;
		return;
	}

	// Get VolumeBox scale
	FVector VolumeScale = PreviewActor->VolumeBox->GetComponentScale();

	// Calculate MaxDimension (larger of X and Y scales)
	float MaxDimension = FMath::Max(VolumeScale.X, VolumeScale.Y);

	// Calculate zoom limits based on volume dimensions
	// Base offset from volume center: VolumeScale.Z * 0.5f (half height)
	float BaseOffset = VolumeScale.Z * 0.5f;

	MinZoomDistance = BaseOffset;
	DefaultZoomDistance = BaseOffset + MaxDimension * 60.0f;
	MaxZoomDistance = BaseOffset + MaxDimension * 120.0f;

	// Ensure minimum is at least some positive value to prevent camera going inside
	MinZoomDistance = FMath::Max(MinZoomDistance, 100.0f);

	UE_LOG(LogTemp, Log, TEXT("SAuroraPreviewViewport: Zoom limits calculated"));
	UE_LOG(LogTemp, Log, TEXT("  - VolumeScale: (%.1f, %.1f, %.1f)"), VolumeScale.X, VolumeScale.Y, VolumeScale.Z);
	UE_LOG(LogTemp, Log, TEXT("  - MinZoom: %.1f, Default: %.1f, MaxZoom: %.1f"),
		MinZoomDistance, DefaultZoomDistance, MaxZoomDistance);
}
