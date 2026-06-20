// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallInputNetSerializers.h"

bool FRecallPlayerInputMetadata::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << LocalFrame;
	Ar << LocalConfirmFrame;
	Ar << RealTimeSeconds;
	
	bOutSuccess = true;
	return true;
}

bool FRecallPlayerInputBunch::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << PlayerId;
	Metadata.NetSerialize(Ar, Map, bOutSuccess);

	uint8 ItemCount = FrameInputs.Num();
	Ar << ItemCount;

	if (Ar.IsLoading())
	{
		FrameInputs.SetNum(ItemCount);
	}

	if (FrameInputs.Num())
	{
		uint32 Frame = FrameInputs[0].Frame;
		Ar << Frame;

		FrameInputs[0].Frame = Frame;
		FrameInputs[0].Input.NetSerialize(Ar, Map, bOutSuccess);

		for (int32 Index = 1; Index < FrameInputs.Num(); Index++)
		{
			const FRecallFrameInput& PreviousInput = FrameInputs[Index - 1];
			FRecallFrameInput& NewInput = FrameInputs[Index];

			/*
			uint32 NewFrame = Frame + static_cast<uint32>(Index);
			if (Ar.IsSaving())
			{
				checkf(NewFrame == NewInput.Frame, TEXT("Should be ordered by frame"));
			}
			else
			{
				NewInput.Frame = NewFrame;
			}
			 */

			Ar << NewInput.Frame;
			NewInput.Input.NetSerializeFromPrevious(PreviousInput.Input, Ar, Map, bOutSuccess);
		}
	}

	bOutSuccess = true;
	return true;
}

bool FRecallPlayerLatencyBunch::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << InputSentFrame;
	Ar << InputSentRealTimeSeconds;
	Ar << InputReceivedConfirmFrame;
	Ar << InputReceivedFrame;
	Ar << bInputReceivedHasAuthority;

	bOutSuccess = true;
	return true;
}
