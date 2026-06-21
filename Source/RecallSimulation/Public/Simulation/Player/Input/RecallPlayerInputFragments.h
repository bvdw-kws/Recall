// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityTypes.h"
#include "System/Input/RecallInputQueueTypes.h"

#include "RecallPlayerInputFragments.generated.h"

// Defines the longest duration of time a input can be accepted.
constexpr int32 RECALL_INPUT_BUFFER_MAX_DURATION = 120;

USTRUCT()
struct RECALLSIMULATION_API FRecallPlayerInputFragment : public FMassFragment
{
	GENERATED_BODY()
	
public:
	FRecallPlayerInputFragment();
	
public:
	UPROPERTY(VisibleAnywhere, meta=(Bitmask, BitmaskEnum="/Script/RecallCore.ERecallControllerInputCommand"))
	uint16 Inputs = 0;

	// A ring buffer tracking the history of held inputs
	UPROPERTY(VisibleAnywhere, meta=(Bitmask, BitmaskEnum="/Script/RecallCore.ERecallControllerInputCommand"))
	TArray<uint16> HeldInputHistory;

	// Tracks the most recent held input index.
	UPROPERTY(VisibleAnywhere)
	int32 HeldInputBackIndex = 0;
	
	// Positive edge inputs
	UPROPERTY(VisibleAnywhere, meta=(Bitmask, BitmaskEnum="/Script/RecallCore.ERecallControllerInputCommand"))
	uint16 PressedInputs = 0;

	// Elapsed time in frames since an input was pressed.
	UPROPERTY(VisibleAnywhere)
	TArray<int32> PressedInputTimers;

	UPROPERTY(VisibleAnywhere)
	FVector2f LeftStickAxis = FVector2f::ZeroVector;
	
	UPROPERTY(VisibleAnywhere)
	FVector2f RightStickAxis = FVector2f::ZeroVector;

	// Axis inputs stored into a map
	UPROPERTY(VisibleAnywhere)
	TMap<uint16, FVector2f> AxisInputMap;

private:
	// A map of input commands to array index to avoid calculating the index many times.
	static TMap<ERecallControllerInputCommand, int32> InputCommandToIndex;

public:
	void ClearInputs();

	bool IsInputHeld(ERecallControllerInputCommand InputID) const;
	float GetAxis1D(ERecallControllerInputCommand InputID) const;
	FVector2f GetAxis2D(ERecallControllerInputCommand InputID) const;
	
	bool WasInputPressed(ERecallControllerInputCommand InputID, int32 InputBufferDuration = 1) const;

	bool IsAnyInputPressed(int32 InputBufferDuration = 1) const;

	// This function clears out the input buffer, which is useful when you're getting the same input causing multiple actions.
	void ClearInputBuffer();

	// This function clears out the input buffer for a single input
	void ClearInputBuffer(ERecallControllerInputCommand InputID);
};

template <>
struct TMassFragmentTraits<FRecallPlayerInputFragment> final
{ enum { AuthorAcceptsItsNotTriviallyCopyable = true }; };
