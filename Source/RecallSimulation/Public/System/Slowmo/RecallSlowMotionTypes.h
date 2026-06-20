// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

#include "RecallSlowMotionTypes.generated.h"

#define MS_SLOW_MO_HANDLE_NULL 0

USTRUCT()
struct FRecallSlowMotionEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	uint32 Handle = MS_SLOW_MO_HANDLE_NULL;

	UPROPERTY(VisibleAnywhere)
	double StartFrame = 0.0;

	UPROPERTY(VisibleAnywhere)
	uint32 Duration = 1;

	UPROPERTY(VisibleAnywhere)
	TSoftObjectPtr<class UCurveFloat> TimeDilatationCurve;

	UPROPERTY(VisibleAnywhere)
	float TimeDilatation = 1.0f;
};

USTRUCT()
struct FRecallSlowMotionData
{
	GENERATED_BODY()

	// Internal frame to update our slow-motion events
	UPROPERTY(VisibleAnywhere)
	double ElapsedDilatedFrames = 0.0;

	// Only affect some parts of the game such as animation, movement, VFX...
	UPROPERTY(VisibleAnywhere)
	float TimeDilatation = 1.0f;

	// Active slow motion events
	UPROPERTY(VisibleAnywhere)
	TArray<FRecallSlowMotionEvent> Events;

	void Reset()
	{
		ElapsedDilatedFrames = 0.0;
		TimeDilatation = 1.0f;
		Events.Reset();
	}
};
