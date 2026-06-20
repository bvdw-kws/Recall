// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/PawnComponent.h"
#include "Engine/Engine.h"

#include "RecallViewTargetPawnComponent.generated.h"

class URecallGameSimViewComponent;

/**
 * Component that handles view target management and smooth camera transitions.
 * Caches the GameSimViewComponent and provides smooth blending when view targets change.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RECALLONLINE_API URecallViewTargetPawnComponent : public UPawnComponent
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Calculate camera with smooth transitions between view targets.
	 * @param DeltaTime Time since last frame
	 * @param OutResult The resulting view info
	 */
	void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult);

	/**
	 * Get the cached GameSimViewComponent.
	 */
	URecallGameSimViewComponent* GetGameSimViewComponent() const { return CachedGameSimViewComponent.Get(); }

	/**
	 * Get the current game sim view target.
	 */
	AActor* GetGameSimViewTarget() const;

	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	//~ End UActorComponent Interface

private:
	/**
	 * Calculate camera fallback when no valid view target is available.
	 */
	void CalcCameraFallback(float DeltaTime, FMinimalViewInfo& OutResult) const;
	
	/**
	 * Apply camera blending between previous and current view info.
	 * @return true if blend is complete and should clear bHasCachedViewInfo
	 */
	bool ApplyCameraBlending(const FMinimalViewInfo& NewViewInfo, FMinimalViewInfo& OutResult) const;
	
	/**
	 * Initialize camera blending when view target changes.
	 */
	void InitializeCameraBlending(bool bViewTargetChanged);

protected:
	/**
	 * Time in seconds to blend between camera transitions when view targets change.
	 */
	UPROPERTY(EditAnywhere, Category="Camera Transition", meta=(ClampMin="0.0", ClampMax="5.0"))
	float CameraBlendTime;

private:
	/**
	 * Cached reference to the GameSimViewComponent on the same pawn.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<URecallGameSimViewComponent> CachedGameSimViewComponent;

	// Camera transition state
	UPROPERTY(Transient)
	FMinimalViewInfo CachedPreviousViewInfo;
	
	UPROPERTY(Transient)
	bool bHasCachedViewInfo;
	
	UPROPERTY(Transient)
	float CameraBlendStartTime;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LastFrameGameSimViewTarget;
	
	UPROPERTY(Transient)
	FMinimalViewInfo LastFrameCameraInfo;
	
	UPROPERTY(Transient)
	bool bHasLastFrameCameraInfo;

};