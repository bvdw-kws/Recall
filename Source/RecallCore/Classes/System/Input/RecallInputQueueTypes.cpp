// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Input/RecallInputQueueTypes.h"

DEFINE_LOG_CATEGORY(LogRecallInput);

UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_BlockControl,		"State.BlockControl",		"Can not receive most inputs");

bool FRecallInput::operator==(const FRecallInput& Other) const
{
	if (AxisInputMap.Num() != Other.AxisInputMap.Num())
	{
		return false;
	}
	
	for (const TPair<uint16, FVector2D_NetQuantizeDirection>& AxisInputPair : AxisInputMap)
	{
		const FVector2D_NetQuantizeDirection* OtherAxisInputPtr = Other.AxisInputMap.Find(AxisInputPair.Key);			
		if (OtherAxisInputPtr == nullptr || AxisInputPair.Value != *OtherAxisInputPtr)
		{
			return false;
		}
	}
	
	return InputCommand == Other.InputCommand
		&& LeftStickAxis == Other.LeftStickAxis
		&& RightStickAxis == Other.RightStickAxis
		&& MousePosition == Other.MousePosition
		&& Options == Other.Options;
}

FString FRecallInput::ToString() const
{
	return FString::Printf(TEXT("InputCommand: %d, LS: %s, RS: %s, Mouse: %s, AxisCount: %d, Options: %s"),
		InputCommand, *LeftStickAxis.ToString(), *RightStickAxis.ToString(), *MousePosition.ToString(), AxisInputMap.Num(), *Options);
}

static void SerializeAxisInputCommands(TMap<uint16, FVector2D_NetQuantizeDirection>& AxisInputMap,
	FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint8 AxisCount = AxisInputMap.Num();
	Ar << AxisCount;

	if (Ar.IsLoading())
	{
		AxisInputMap.Reserve(AxisCount);
	}

	TArray<uint16> AxisInputCommands;
	AxisInputMap.GenerateKeyArray(AxisInputCommands);

	for (int32 Index = 0; Index < AxisCount; Index++)
	{
		uint16 AxisInputCommand = Ar.IsSaving() ? AxisInputCommands[Index] : 0;
		Ar << AxisInputCommand;		
		AxisInputMap.FindOrAdd(AxisInputCommand).NetSerialize(Ar, Map, bOutSuccess);
	}
}

bool FRecallInput::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint8 B = (InputCommand != 0);
	Ar.SerializeBits(&B, 1);
	if (B)
	{
		Ar << InputCommand;
	}
	else
	{
		InputCommand = 0;
	}

	LeftStickAxis.NetSerialize(Ar, Map, bOutSuccess);
	RightStickAxis.NetSerialize(Ar, Map, bOutSuccess);
	MousePosition.NetSerialize(Ar, Map, bOutSuccess);

	SerializeAxisInputCommands(AxisInputMap, Ar, Map, bOutSuccess);
	
	Ar << Options;
	bOutSuccess = true;
	return true;
}

template<typename T>
static void SerializeInputParameterFromPrevious(const T& Old, T& New, FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint8 B = (Old != New);
	Ar.SerializeBits(&B, 1);
	if (B)
	{
		Ar << New;
	}
	else
	{
		New = Old;
	}
}

template<typename T>
static void NetSerializeInputParameterFromPrevious(const T& Old, T& New, FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint8 B = (Old != New);
	Ar.SerializeBits(&B, 1);
	if (B)
	{
		New.NetSerialize(Ar, Map, bOutSuccess);
	}
	else
	{
		New = Old;
	}
}

bool FRecallInput::NetSerializeFromPrevious(const FRecallInput& PreviousInput, FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	SerializeInputParameterFromPrevious(PreviousInput.InputCommand, InputCommand, Ar, Map, bOutSuccess);
	NetSerializeInputParameterFromPrevious(PreviousInput.LeftStickAxis, LeftStickAxis, Ar, Map, bOutSuccess);
	NetSerializeInputParameterFromPrevious(PreviousInput.RightStickAxis, RightStickAxis, Ar, Map, bOutSuccess);
	NetSerializeInputParameterFromPrevious(PreviousInput.MousePosition, MousePosition, Ar, Map, bOutSuccess);
	SerializeAxisInputCommands(AxisInputMap, Ar, Map, bOutSuccess); // TODO
	SerializeInputParameterFromPrevious(PreviousInput.Options, Options, Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

FRecallFrameInput::FRecallFrameInput(uint32 InFrame, const FRecallInput& InInput)
	: Frame(InFrame)
	, Input(InInput)
{
}

bool FRecallFrameInput::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Frame;
	Input.NetSerialize(Ar, Map, bOutSuccess);
	bOutSuccess = true;
	return true;
}

uint32  FRecallInputQueueItemEntry::LastFrame() const
{
	check(FrameCount > 0);
	return FrameStart + (FrameCount - 1);
}

bool FRecallInputQueueItemEntry::IsNextFrame(uint32 Frame) const
{
	return IsBeforeFrame(Frame)
		&& IsDuringFrame(Frame - 1);
}

bool FRecallInputQueueItemEntry::IsDuringFrame(uint32 Frame) const
{
	return Frame >= FrameStart && Frame < FrameStart + FrameCount;
}

bool FRecallInputQueueItemEntry::IsBeforeFrame(uint32 Frame) const
{
	return FrameStart + FrameCount <= Frame;
}

bool FRecallInputQueueItemEntry::IsAfterFrame(uint32 Frame) const
{
	return FrameStart > Frame;
}

bool FRecallInputQueueArray::PushFrameInput(const FRecallFrameInput& FrameInput)
{
	int32 InsertIndex = INDEX_NONE;

	for (int32 ControlIndex = Items.Num() - 1; ControlIndex >= 0; ControlIndex--)
	{
		FRecallInputQueueItemEntry& PreviousControl = Items[ControlIndex];

		if (PreviousControl.IsAfterFrame(FrameInput.Frame))
		{
			InsertIndex = ControlIndex;
			continue;
		}
		else if (PreviousControl.IsDuringFrame(FrameInput.Frame))
		{
			if (PreviousControl.Input != FrameInput.Input)
			{
				UE_LOG(LogRecallInput, Error,
					TEXT("%hs Already received a different input for this frame"), __FUNCTION__);
			}
			return false;
		}
		else if (PreviousControl.IsNextFrame(FrameInput.Frame))
		{
			if (PreviousControl.Input == FrameInput.Input)
			{
				PreviousControl.FrameCount++;
				MarkItemDirty(PreviousControl);
				return true;
			}
			else
			{
				break;
			}
		}
		else if (PreviousControl.IsBeforeFrame(FrameInput.Frame))
		{
			break;
		}
	}

	FRecallInputQueueItemEntry Control;
	Control.FrameStart = FrameInput.Frame;
	Control.Input = FrameInput.Input;

	if (InsertIndex == INDEX_NONE)
	{
		MarkItemDirty(Items.Add_GetRef(Control));
	}
	else
	{
		MarkItemDirty(Items.Insert_GetRef(Control, InsertIndex));
	}
	return true;
}

bool FRecallInputQueueArray::FindInput(uint32 Frame, FRecallInput& OutInput) const
{
	for (const FRecallInputQueueItemEntry& Item : Items)
	{
		if (Item.IsBeforeFrame(Frame))
		{
			continue;
		}
		else if (Item.IsDuringFrame(Frame))
		{
			OutInput = Item.Input;
			return true;
		}
		else if (Item.IsAfterFrame(Frame))
		{
			break;
		}
	}

	return false;
}

void FRecallInputQueueArray::RemoveAllAfterFrame(uint32 Frame)
{
	bool bDirty = false;

	for (int32 ControlIndex = Items.Num() - 1; ControlIndex >= 0; ControlIndex--)
	{
		FRecallInputQueueItemEntry& Item = Items[ControlIndex];

		if (Item.IsAfterFrame(Frame))
		{
			Items.RemoveAt(ControlIndex);
			bDirty = true;
		}
		if (Item.IsDuringFrame(Frame))
		{
			Item.FrameCount = (Frame - Item.FrameStart) + 1;
			MarkItemDirty(Item);
			break;
		}
		else if (Item.IsBeforeFrame(Frame))
		{
			break;
		}
	}

	if (bDirty)
	{
		MarkArrayDirty();
	}
}

bool FRecallInputQueueArray::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FRecallInputQueueItemEntry, FRecallInputQueueArray>(Items, DeltaParms, *this);
}

bool FRecallInputQueueArray::operator!=(const FRecallInputQueueArray& Other) const 
{ 
	return !(*this == Other); 
}

bool FRecallInputQueueArray::operator==(const FRecallInputQueueArray& Other) const
{
	return Items == Other.Items;
}

FRecallInputQueuePlayerEntry* FRecallInputQueue::FindPlayerEntry(const FString& PlayerId)
{
	return PlayerEntries.FindByPredicate([&PlayerId](const FRecallInputQueuePlayerEntry& PlayerEntry)
		{
			return PlayerEntry.PlayerId == PlayerId;
		});
}

const FRecallInputQueuePlayerEntry* FRecallInputQueue::FindPlayerEntry(const FString& PlayerId) const
{
	return PlayerEntries.FindByPredicate([&PlayerId](const FRecallInputQueuePlayerEntry& PlayerEntry)
		{
			return PlayerEntry.PlayerId == PlayerId;
		});
}

void FRecallInputQueue::RemoveInputPastFrame(uint32 Frame)
{
	for (FRecallInputQueuePlayerEntry& PlayerEntry : PlayerEntries)
	{
		PlayerEntry.Queue.RemoveAllAfterFrame(Frame);
	}
}
