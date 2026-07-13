// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Components/Pawn/RecallViewTargetPawnComponent.h"

#include "Camera/CameraTypes.h"
#include "Components/Pawn/RecallGameSimViewComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogRecallViewTarget, Log, All);

URecallViewTargetPawnComponent::URecallViewTargetPawnComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CameraBlendTime(0.3f)
	, bHasCachedViewInfo(false)
	, CameraBlendStartTime(0.0f)
	, bHasLastFrameCameraInfo(false)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URecallViewTargetPawnComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Cache the GameSimViewComponent from the same pawn
	if (APawn* OwnerPawn = GetPawn<APawn>())
	{
		CachedGameSimViewComponent = OwnerPawn->FindComponentByClass<URecallGameSimViewComponent>();
		if (!CachedGameSimViewComponent.IsValid())
		{
			UE_LOG(LogRecallViewTarget, Warning, TEXT("%hs: No URecallGameSimViewComponent found on pawn %s"), 
				__FUNCTION__, *OwnerPawn->GetName());
		}
	}
}

AActor* URecallViewTargetPawnComponent::GetGameSimViewTarget() const
{
	return CachedGameSimViewComponent.IsValid() ? CachedGameSimViewComponent->GetGameSimViewTarget() : nullptr;
}

void URecallViewTargetPawnComponent::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	const TWeakObjectPtr<AActor> CurrentGameSimViewTarget = GetGameSimViewTarget();
	const bool bViewTargetChanged = LastFrameGameSimViewTarget != CurrentGameSimViewTarget;
	
	// Use current target if valid, otherwise fall back to previous valid target (async spawn protection)
	const TWeakObjectPtr<AActor> EffectiveViewTarget = CurrentGameSimViewTarget.IsValid() ? 
		CurrentGameSimViewTarget : LastFrameGameSimViewTarget;
	
	if (EffectiveViewTarget.IsValid())
	{
		// Get the new camera info
		FMinimalViewInfo NewViewInfo;
		EffectiveViewTarget->CalcCamera(DeltaTime, NewViewInfo);
		
		// Only consider it a real view target change if we're using the current target (not a fallback)
		const bool bRealViewTargetChange = bViewTargetChanged && CurrentGameSimViewTarget.IsValid();
		
		// Initialize blending if view target changed
		InitializeCameraBlending(bRealViewTargetChange);
		
		// Apply blending and check if blend is complete
		const bool bBlendComplete = ApplyCameraBlending(NewViewInfo, OutResult);
		if (bBlendComplete)
		{
			bHasCachedViewInfo = false;
		}
		
		// Only update camera info when using current target (preserves previous camera for blending)
		if (CurrentGameSimViewTarget.IsValid())
		{
			LastFrameCameraInfo = OutResult;
			bHasLastFrameCameraInfo = true;
		}
	}
	else
	{
		CalcCameraFallback(DeltaTime, OutResult);
		
		// Clear cached info when no view target
		bHasLastFrameCameraInfo = false;
		bHasCachedViewInfo = false;
	}
	
	// Only update tracking when we have a valid current target (don't track null fallbacks)
	if (CurrentGameSimViewTarget.IsValid())
	{
		LastFrameGameSimViewTarget = CurrentGameSimViewTarget;
	}
}

void URecallViewTargetPawnComponent::CalcCameraFallback(float DeltaTime, FMinimalViewInfo& OutResult) const
{
	// No view target, use default pawn camera
	if (APawn* OwnerPawn = GetPawn<APawn>())
	{
		OwnerPawn->Super::CalcCamera(DeltaTime, OutResult);
	}
}

bool URecallViewTargetPawnComponent::ApplyCameraBlending(const FMinimalViewInfo& NewViewInfo, FMinimalViewInfo& OutResult) const
{
	if (bHasCachedViewInfo && CameraBlendTime > 0.0f)
	{
		const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		const float BlendAlpha = FMath::Clamp((CurrentTime - CameraBlendStartTime) / CameraBlendTime, 0.0f, 1.0f);
		
		if (BlendAlpha < 1.0f)
		{
			// Blend between cached previous view and new view
			OutResult.Location = FMath::Lerp(CachedPreviousViewInfo.Location, NewViewInfo.Location, BlendAlpha);
			OutResult.Rotation = FMath::Lerp(CachedPreviousViewInfo.Rotation, NewViewInfo.Rotation, BlendAlpha);
			OutResult.FOV = FMath::Lerp(CachedPreviousViewInfo.FOV, NewViewInfo.FOV, BlendAlpha);
			
			// Copy other fields from new view
			OutResult.AspectRatio = NewViewInfo.AspectRatio;
			OutResult.bConstrainAspectRatio = NewViewInfo.bConstrainAspectRatio;
			OutResult.bUseFieldOfViewForLOD = NewViewInfo.bUseFieldOfViewForLOD;
			OutResult.ProjectionMode = NewViewInfo.ProjectionMode;
			OutResult.OrthoWidth = NewViewInfo.OrthoWidth;
			OutResult.OrthoNearClipPlane = NewViewInfo.OrthoNearClipPlane;
			OutResult.OrthoFarClipPlane = NewViewInfo.OrthoFarClipPlane;
			
			return false; // Blend not complete
		}
		else
		{
			// Blend complete
			OutResult = NewViewInfo;
			return true; // Blend complete, caller should clear bHasCachedViewInfo
		}
	}
	else
	{
		// No blending, use new view directly
		OutResult = NewViewInfo;
		return false; // No blend was active
	}
}

void URecallViewTargetPawnComponent::InitializeCameraBlending(bool bViewTargetChanged)
{
	// Start blending if view target changed and we have previous camera info to blend from
	if (bViewTargetChanged && bHasLastFrameCameraInfo)
	{
		CachedPreviousViewInfo = LastFrameCameraInfo;
		CameraBlendStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		bHasCachedViewInfo = true;
	}
}

