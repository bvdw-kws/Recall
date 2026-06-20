// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

#include "RecallStreamingConfig.generated.h"

/**
 * Streaming configuration for entity loading/unloading
 */
USTRUCT()
struct RECALLCORE_API FRecallStreamingConfig
{
	GENERATED_BODY()

	/** Distance at which entities are loaded */
	UPROPERTY(EditAnywhere, config, Category="Streaming", meta=(ClampMin="100.0", Units=Centimeters))
	float LoadDistance = 5000.0f;

	/** Distance at which entities are unloaded (should be > LoadDistance) */
	UPROPERTY(EditAnywhere, config, Category="Streaming", meta=(ClampMin="100.0", Units=Centimeters))  
	float UnloadDistance = 6000.0f;

	/** How often to check streaming distances (seconds) */
	UPROPERTY(EditAnywhere, config, Category="Streaming", meta=(ClampMin="0.1", Units=Seconds))
	float UpdateInterval = 0.5f;

	/** Maximum entities to stream per frame (prevent hitches) */
	UPROPERTY(EditAnywhere, config, Category="Streaming", meta=(ClampMin="1"))
	int32 MaxEntitiesPerFrame = 10;

	/** Enable entity streaming globally (disabled by default for safety) */
	UPROPERTY(EditAnywhere, config, Category="Streaming")
	bool bEnableEntityStreaming = false;

	/** Enable visibility-based streaming (only spawn entities roughly in view direction) */
	UPROPERTY(EditAnywhere, config, Category="Streaming")
	bool bEnableVisibilityStreaming = false;

	/** Field of view angle for visibility streaming (degrees) */
	UPROPERTY(EditAnywhere, config, Category="Streaming", meta=(EditCondition="bEnableVisibilityStreaming", ClampMin="30.0", ClampMax="180.0"))
	float VisibilityFOV = 120.0f;

	/** Buffer distance behind view target for visibility streaming */
	UPROPERTY(EditAnywhere, config, Category="Streaming", meta=(EditCondition="bEnableVisibilityStreaming", ClampMin="0.0", Units=Centimeters))
	float VisibilityBackBuffer = 1000.0f;
};