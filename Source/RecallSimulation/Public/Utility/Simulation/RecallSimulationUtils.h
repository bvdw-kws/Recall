// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

namespace Recall::Simulation::Utils
{

RECALLSIMULATION_API extern uint32 GetFrame(const UObject* WorldContextObject);
RECALLSIMULATION_API extern double GetTimeSeconds(const UObject* WorldContextObject);
RECALLSIMULATION_API extern int32 GetFramesPerSeconds(const UObject* WorldContextObject);
RECALLSIMULATION_API extern float GetFixedDeltaTime(const UObject* WorldContextObject);
RECALLSIMULATION_API extern float GetDilatedFixedDeltaTime(const UObject* WorldContextObject);

RECALLSIMULATION_API extern float GetRepresentationDeltaFrame(const UObject* WorldContextObject);
RECALLSIMULATION_API extern float GetRepresentationSpeedScale(const UObject* WorldContextObject);

RECALLSIMULATION_API extern bool HasSimulationStarted(const UObject* WorldContextObject);
RECALLSIMULATION_API extern bool IsSimulationProcessingPhase(const UObject* WorldContextObject);
RECALLSIMULATION_API extern bool IsSimulationRenderPhase(const UObject* WorldContextObject);
RECALLSIMULATION_API extern void CheckSimulationProcessingPhase(const UObject* WorldContextObject, FString Reason = FString());

/** Minimum duration for async tasks that we run on the gamethread */
RECALLSIMULATION_API extern int32 GetMaxStepCount(const UObject* WorldContextObject);

} // namespace Recall::Simulation::Utils
