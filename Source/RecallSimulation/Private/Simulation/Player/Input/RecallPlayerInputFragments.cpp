// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Player/Input/RecallPlayerInputFragments.h"

TMap<ERecallControllerInputCommand, int32> FRecallPlayerInputFragment::InputCommandToIndex;

FRecallPlayerInputFragment::FRecallPlayerInputFragment()
{
	HeldInputHistory.Init(0, RECALL_INPUT_BUFFER_MAX_DURATION);
	PressedInputTimers.Init(RECALL_INPUT_BUFFER_MAX_DURATION, MAX_RECALL_INPUT_COUNT);
	
	if (InputCommandToIndex.Num() == 0)
	{
		for (int32 Index = 0; Index < MAX_RECALL_INPUT_COUNT; Index++)
		{
			InputCommandToIndex.Add(static_cast<ERecallControllerInputCommand>(1 << Index), Index);
		}		
	}
}

void FRecallPlayerInputFragment::ClearInputs()
{
	Inputs = 0;
}

bool FRecallPlayerInputFragment::IsInputHeld(ERecallControllerInputCommand InputID) const
{
	return EnumHasAnyFlags(Inputs, InputID);
}

float FRecallPlayerInputFragment::GetAxis1D(ERecallControllerInputCommand InputID) const
{
	return GetAxis2D(InputID).X;
}

FVector2f FRecallPlayerInputFragment::GetAxis2D(ERecallControllerInputCommand InputID) const
{
	const FVector2f* AxisInput = AxisInputMap.Find(static_cast<uint16>(InputID));
	if (AxisInput != nullptr)
	{
		return *AxisInput;
	}
	// Use boolean as axis
	return IsInputHeld(InputID) ? FVector2f(1.0f, 0.0f) : FVector2f::ZeroVector;
}

bool FRecallPlayerInputFragment::WasInputPressed(ERecallControllerInputCommand InputID, int32 InputBufferDuration /*= 1*/) const
{
	// We keep track of how many frames in the past a input triggered to give players input leeway.
	return PressedInputTimers[InputCommandToIndex[InputID]] < InputBufferDuration;
}

bool FRecallPlayerInputFragment::IsAnyInputPressed(int32 InputBufferDuration) const
{
	if (!LeftStickAxis.IsNearlyZero() || !RightStickAxis.IsNearlyZero() || Inputs != 0)
	{
		return true;
	}

	for (int32 Index = 0; Index < MAX_RECALL_INPUT_COUNT; Index++)
	{
		if (PressedInputTimers[Index] < InputBufferDuration)
		{
			return true;
		}
	}

	return false;
}

// This function clears out the input buffer, which is useful when you're getting the same input causing multiple actions.
void FRecallPlayerInputFragment::ClearInputBuffer()
{
	for (int32& Button : PressedInputTimers)
	{
		Button = RECALL_INPUT_BUFFER_MAX_DURATION;
	}
}

// This function clears out the input buffer for a single input
void FRecallPlayerInputFragment::ClearInputBuffer(ERecallControllerInputCommand InputID)
{
	PressedInputTimers[InputCommandToIndex[InputID]] = RECALL_INPUT_BUFFER_MAX_DURATION;
}
