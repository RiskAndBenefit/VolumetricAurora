// Copyright (c) 2026 R&B. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UTextureRenderTarget2D;
class AVolumetricAurora;

/**
 * @class SAuroraPreviewViewport
 * @brief Slate widget providing interactive orbit camera controls for Aurora preview
 *
 * This widget implements Material Editor-style viewport navigation:
 * - Left Mouse Button + Drag: Orbit camera around the focus point (like moon orbiting Earth)
 * - Mouse Scroll: Zoom in/out (adjust camera distance from focus point)
 *
 * The camera always looks at the center of the Aurora VolumeBox, creating an intuitive
 * navigation experience similar to Unreal Engine's Material Editor preview panel.
 *
 * Technical Implementation (Quaternion-based, Gimbal-lock Free):
 * - Camera orientation stored as FQuat to avoid gimbal lock issues
 * - Horizontal drag: Rotates around world Z-axis (global up)
 * - Vertical drag: Rotates around camera's local right vector
 * - No rotation angle limits - full 360-degree freedom in all directions
 * - Camera position = FocusPoint + CameraRotation.RotateVector(FVector(-Distance, 0, 0))
 *
 * Zoom Distance Constraints (Dynamic based on VolumeBox):
 * - MinDistance: VolumeScale.Z * 0.5f (camera at volume edge)
 * - DefaultDistance: VolumeScale.Z * 0.5f + MaxDimension * 60.f
 * - MaxDistance: VolumeScale.Z * 0.5f + MaxDimension * 120.f
 *
 * Integration:
 * - Receives render target from SceneCapture2D component
 * - Communicates camera changes to AVolumetricAurora via delegate/direct reference
 * - Updates are applied to PreviewSceneCapture component for real-time preview
 */
class VOLUMETRICAURORAEDITOR_API SAuroraPreviewViewport : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAuroraPreviewViewport)
		: _RenderTarget(nullptr)
		, _TargetAurora(nullptr)
	{}
		/** Render target to display (from SceneCapture2D) */
		SLATE_ARGUMENT(UTextureRenderTarget2D*, RenderTarget)

		/** Reference to VolumetricAurora actor for camera control */
		SLATE_ARGUMENT(AVolumetricAurora*, TargetAurora)
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the viewport widget with orbit camera controls
	 * @param InArgs Slate arguments containing RenderTarget and TargetAurora
	 */
	void Construct(const FArguments& InArgs);

	// ========================================================================
	// Mouse Input Handlers for Orbit Camera Control
	// ========================================================================

	/**
	 * @brief Handles mouse button press to initiate orbit/zoom operations
	 *
	 * Left Mouse Button: Begin orbit mode
	 * - Captures mouse to this widget for exclusive input handling
	 * - Enables high-precision mouse movement for smooth camera rotation
	 * - Hides cursor during drag for cleaner UX (like Unreal viewports)
	 *
	 * @param Geometry Widget geometry information
	 * @param MouseEvent Mouse event containing button and position data
	 * @return FReply indicating whether event was handled
	 */
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles mouse movement during orbit drag operation
	 *
	 * Quaternion-based rotation (gimbal-lock free):
	 * - Horizontal movement (Delta.X): Rotate around world Z-axis (yaw)
	 * - Vertical movement (Delta.Y): Rotate around camera's local right vector (pitch)
	 *
	 * The quaternion approach allows unlimited rotation in any direction without
	 * the singularities that occur at poles in spherical coordinate systems.
	 *
	 * @param MyGeometry Widget geometry information
	 * @param MouseEvent Mouse event containing cursor delta
	 * @return FReply indicating whether event was handled
	 */
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles mouse button release to end orbit operation
	 *
	 * Cleanup operations:
	 * - Release mouse capture
	 * - Restore cursor visibility
	 * - Reset cursor position to drag start location
	 *
	 * @param MyGeometry Widget geometry information
	 * @param MouseEvent Mouse event data
	 * @return FReply indicating whether event was handled
	 */
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles mouse wheel scroll for zoom control
	 *
	 * Zoom Implementation:
	 * - Scroll Up (positive delta): Decrease distance (zoom in)
	 * - Scroll Down (negative delta): Increase distance (zoom out)
	 * - Distance is clamped between dynamic MinZoomDistance and MaxZoomDistance
	 *
	 * Distance limits are calculated based on VolumeBox scale:
	 * - Min: VolumeScale.Z * 0.5f
	 * - Max: VolumeScale.Z * 0.5f + MaxDimension * 120.f
	 *
	 * @param MyGeometry Widget geometry information
	 * @param MouseEvent Mouse wheel event containing scroll delta
	 * @return FReply indicating whether event was handled
	 */
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// ========================================================================
	// Public Interface
	// ========================================================================

	/**
	 * @brief Updates the render target displayed in the viewport
	 * @param InRenderTarget New render target from SceneCapture2D
	 */
	void SetRenderTarget(UTextureRenderTarget2D* InRenderTarget);

	/**
	 * @brief Updates the target aurora reference
	 * @param InTargetAurora VolumetricAurora actor to control
	 */
	void SetTargetAurora(AVolumetricAurora* InTargetAurora);

	/**
	 * @brief Resets camera to default view position
	 *
	 * Default position: Top-down view looking at VolumeBox center
	 * - Rotation: Looking down at -80 degrees pitch, -90 degrees yaw
	 * - Distance: VolumeScale.Z * 0.5f + MaxDimension * 60.f
	 */
	void ResetCameraView();

private:
	// ========================================================================
	// Camera State (Yaw/Pitch with Roll Fixed to Zero)
	// ========================================================================
	//
	// Using separate Yaw/Pitch angles instead of accumulated quaternion to prevent
	// unintended roll accumulation. Roll is always reconstructed as zero, ensuring
	// the camera never "flips" unexpectedly during orbit operations.
	//
	// Camera orientation is reconstructed each frame from these angles:
	// 1. Apply Yaw rotation around world Z-axis
	// 2. Apply Pitch rotation around the resulting local Y-axis (right vector)
	// 3. Roll is implicitly zero

	/**
	 * @brief Camera yaw angle in degrees (horizontal orbit)
	 *
	 * Rotation around world Z-axis (up).
	 * - Positive: Counter-clockwise when viewed from above
	 * - No limits: Full 360-degree horizontal rotation
	 * - Default: -90 degrees (looking along -Y axis)
	 */
	float CameraYaw = -90.0f;

	/**
	 * @brief Camera pitch angle in degrees (vertical orbit)
	 *
	 * Rotation around camera's local right vector (Y-axis after yaw).
	 * - Positive: Looking upward
	 * - Negative: Looking downward
	 * - Clamped to (-89, +89) to prevent flip at poles
	 * - Default: -80 degrees (looking down from above)
	 */
	float CameraPitch = 80.0f;

	/**
	 * @brief Distance from camera to focus point (orbit radius)
	 *
	 * Range: Dynamic based on VolumeBox scale
	 * - Min: VolumeScale.Z * 0.5f
	 * - Max: VolumeScale.Z * 0.5f + MaxDimension * 120.f
	 */
	float CameraDistance = 10000.0f;

	// ========================================================================
	// Dynamic Zoom Constraints (Calculated from VolumeBox)
	// ========================================================================

	/**
	 * @brief Minimum allowed camera distance (closest zoom)
	 *
	 * Calculated as: VolumeScale.Z * 0.5f
	 * This places the camera at the edge of the volume when fully zoomed in.
	 */
	float MinZoomDistance = 1000.0f;

	/**
	 * @brief Maximum allowed camera distance (farthest zoom)
	 *
	 * Calculated as: VolumeScale.Z * 0.5f + MaxDimension * 120.f
	 * Allows viewing the entire aurora from a considerable distance.
	 */
	float MaxZoomDistance = 100000.0f;

	/**
	 * @brief Default camera distance for initial view
	 *
	 * Calculated as: VolumeScale.Z * 0.5f + MaxDimension * 60.f
	 * Provides a balanced initial view of the aurora.
	 */
	float DefaultZoomDistance = 10000.0f;

	// ========================================================================
	// Sensitivity Settings
	// ========================================================================

	/**
	 * @brief Mouse movement to camera rotation multiplier (degrees per pixel)
	 *
	 * Higher values = faster rotation
	 * Tuned to match Material Editor feel
	 */
	float OrbitSensitivity = 0.25f;

	/**
	 * @brief Scroll wheel to distance change multiplier
	 *
	 * Applied as percentage of current distance for consistent zoom feel
	 * ZoomSensitivity of 0.1 means each scroll notch changes distance by 10%
	 */
	float ZoomSensitivity = 0.1f;

	// ========================================================================
	// Drag State
	// ========================================================================

	/** True when LMB is held and orbiting is active */
	bool bIsOrbiting = false;

	/** Screen position where drag started (for cursor restoration) */
	FVector2D DragStartPosition;

	// ========================================================================
	// Widget References
	// ========================================================================

	/** Weak reference to target VolumetricAurora actor */
	TWeakObjectPtr<AVolumetricAurora> TargetAurora;

	/** Cached render target for display */
	TWeakObjectPtr<UTextureRenderTarget2D> RenderTarget;

	/** Image widget displaying the render target */
	TSharedPtr<class SImage> PreviewImage;

	// ========================================================================
	// Internal Helper Functions
	// ========================================================================

	/**
	 * @brief Applies current Yaw/Pitch angles to SceneCapture camera (FQuat-based)
	 *
	 * Builds camera orientation from separate quaternions to avoid FRotator
	 * pole singularities that cause 180° flips near ±90° pitch:
	 * 1. YawQuat: Rotation around world up (Z-axis)
	 * 2. PitchQuat: Rotation around right axis after yaw
	 * 3. CameraQuat = PitchQuat * YawQuat (no accumulation, no roll drift)
	 * 4. Apply transform via SetWorldRotation(FQuat), not FRotator
	 */
	void UpdateCameraTransform();

	/**
	 * @brief Gets the focus point (center of VolumeBox) in world space
	 * @return World position of focus point, or zero vector if invalid
	 */
	FVector GetFocusPoint() const;

	/**
	 * @brief Calculates dynamic zoom distance limits based on VolumeBox scale
	 *
	 * Updates MinZoomDistance, MaxZoomDistance, and DefaultZoomDistance
	 * based on the preview aurora's VolumeBox dimensions.
	 *
	 * Formulas:
	 * - MinZoomDistance = VolumeScale.Z * 0.5f
	 * - DefaultZoomDistance = VolumeScale.Z * 0.5f + MaxDimension * 60.f
	 * - MaxZoomDistance = VolumeScale.Z * 0.5f + MaxDimension * 120.f
	 */
	void CalculateZoomLimits();
};
