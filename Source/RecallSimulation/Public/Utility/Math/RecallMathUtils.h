// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

namespace Recall::Math::Utils
{
	
RECALLSIMULATION_API extern float FramesToSeconds(uint32 Frames);
RECALLSIMULATION_API extern uint32 SecondsToFrames(double Seconds);
RECALLSIMULATION_API extern float UnitsPerSecondToPerFrame(float UnitsPerSecond);
RECALLSIMULATION_API extern float UnitsPerSecondSquaredToPerFrameSquared(float UnitsPerSecondSquared);
RECALLSIMULATION_API extern FVector UnitsPerSecondSquaredToPerFrameSquared(const FVector& UnitsPerSecondSquared);
RECALLSIMULATION_API extern FVector UnitsPerSecondToPerFrame(const FVector& UnitsPerSecond);
RECALLSIMULATION_API extern float UnitsPerFrameToPerSecond(float UnitsPerFrame);
RECALLSIMULATION_API extern FVector UnitsPerFrameToPerSecond(const FVector& UnitsPerFrame);
	
RECALLSIMULATION_API extern FVector UnitsPerFrameToKilometersPerHour(const FVector& UnitsPerSecond);
RECALLSIMULATION_API extern float UnitsPerFrameToKilometersPerHour(float UnitsPerSecond);
RECALLSIMULATION_API extern float KilometersPerHourToUnitsPerFrame(float KilometersPerHour);
RECALLSIMULATION_API extern FVector KilometersPerHourToUnitsPerFrame(const FVector& KilometersPerHour);

} // namespace Recall::Math::Utils
