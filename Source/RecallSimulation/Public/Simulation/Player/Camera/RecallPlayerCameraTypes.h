// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Collection/VariableCollectionTypes.h"

#include "RecallPlayerCameraTypes.generated.h"

/**
 * How a player camera selects which entities need an actor representation.
 * The selection is computed in the simulation from simulation data only (never from the real view), so it is deterministic.
 */
UENUM()
enum class ERecallCameraRenderFilter : uint8
{
	/** Every entity is considered rendered. */
	Off,

	/** Entities whose position is inside a world axis aligned box centered on the camera are considered rendered. */
	WorldBox,
};

USTRUCT()
struct RECALLSIMULATION_API FRecallPlayerCameraSettings
{
	GENERATED_BODY()

	/** How entities are filtered to know if they are rendered by this camera. */
	UPROPERTY(EditAnywhere)
	ERecallCameraRenderFilter RenderFilter = ERecallCameraRenderFilter::Off;

	/** Half extents (world axes) of the box centered on the camera. */
	UPROPERTY(EditAnywhere, meta=(EditCondition="RenderFilter == ERecallCameraRenderFilter::WorldBox", EditConditionHides, ClampMin = "0.0"))
	FVector RenderBoxExtent = FVector(2000.0, 2000.0, 2000.0);

	/** Extra margin added to the box for entities that are already rendered, to avoid flickering at the border. */
	UPROPERTY(EditAnywhere, meta=(EditCondition="RenderFilter == ERecallCameraRenderFilter::WorldBox", EditConditionHides, ClampMin = "0.0"))
	float RenderBoxHysteresis = 200.0f;

	UPROPERTY(EditAnywhere, meta=(ClampMin = "0.0"))
	FVariableCollectionFloat FollowInterpSpeed = 7.0f;
	
	UPROPERTY(EditAnywhere, meta=(EditCondition="!bUseControlRotation"))
	bool bUsePawnRotation = false;

	UPROPERTY(EditAnywhere)
	bool bUseControlRotation = false;
	
	UPROPERTY(EditAnywhere, meta=(ClampMin = "0.0"))
	FVariableCollectionFloat RotateInterpSpeed = 7.0f;
	
	UPROPERTY(EditAnywhere)
	FTransform Offset = FTransform::Identity;
};
