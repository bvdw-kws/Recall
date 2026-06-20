// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Math/RecallMathUtils.h"

#include "Settings/RecallSimulationSettings.h"

namespace Recall::Math::Utils
{
	
float FramesToSeconds(uint32 Frames)
{
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const float FramesPerSeconds = static_cast<float>(SimulationSettings->FramesPerSeconds);
	return static_cast<float>(Frames) / FramesPerSeconds;
}
	
uint32 SecondsToFrames(double Seconds)
{
	check(Seconds >= 0.0);
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const double FramesPerSeconds = static_cast<double>(SimulationSettings->FramesPerSeconds);
	return static_cast<uint32>(FMath::FloorToInt64(Seconds * FramesPerSeconds));
}
	
float UnitsPerSecondToPerFrame(float UnitsPerSecond)
{
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const float FramesPerSeconds = static_cast<float>(SimulationSettings->FramesPerSeconds);
	return UnitsPerSecond / FramesPerSeconds;
}

float UnitsPerSecondSquaredToPerFrameSquared(float UnitsPerSecondSquared)
{
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const float FramesPerSeconds = static_cast<float>(SimulationSettings->FramesPerSeconds);
	return UnitsPerSecondSquared / (FramesPerSeconds * FramesPerSeconds);
}

FVector UnitsPerSecondSquaredToPerFrameSquared(const FVector& UnitsPerSecondSquared)
{
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const float FramesPerSeconds = static_cast<float>(SimulationSettings->FramesPerSeconds);
	return UnitsPerSecondSquared / (FramesPerSeconds * FramesPerSeconds);
}

FVector UnitsPerSecondToPerFrame(const FVector& UnitsPerSecond)
{
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const double FramesPerSeconds = static_cast<double>(SimulationSettings->FramesPerSeconds);
	return UnitsPerSecond / FramesPerSeconds;
}

float UnitsPerFrameToPerSecond(float UnitsPerFrame)
{
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const float FramesPerSeconds = static_cast<float>(SimulationSettings->FramesPerSeconds);
	return UnitsPerFrame * FramesPerSeconds;
}

FVector UnitsPerFrameToPerSecond(const FVector& UnitsPerFrame)
{
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const double FramesPerSeconds = static_cast<double>(SimulationSettings->FramesPerSeconds);
	return UnitsPerFrame * FramesPerSeconds;
}
	
FVector UnitsPerFrameToKilometersPerHour(const FVector& UnitsPerSecond)
{
	const FVector SpeedCentimetersPerSecond = UnitsPerFrameToPerSecond(UnitsPerSecond);
	return SpeedCentimetersPerSecond * 0.036;
}
	
float UnitsPerFrameToKilometersPerHour(float UnitsPerSecond)
{
	const float SpeedCentimetersPerSecond = UnitsPerFrameToPerSecond(UnitsPerSecond);
	return SpeedCentimetersPerSecond * 0.036f;	
}
	
float KilometersPerHourToUnitsPerFrame(float KilometersPerHour)
{
	const float CentimetersPerSecond = KilometersPerHour / 0.036;
	return UnitsPerSecondToPerFrame(CentimetersPerSecond);
}

FVector KilometersPerHourToUnitsPerFrame(const FVector& KilometersPerHour)
{
	const FVector CentimetersPerSecond = KilometersPerHour / 0.036;
	return UnitsPerSecondToPerFrame(CentimetersPerSecond);
}
	
} // namespace Recall::Math::Utils
