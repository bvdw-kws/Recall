// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSignalTypes.h"
#include "MassEntityManager.h"

uint64 FRecallSignalNameLookup::GetOrAddSignalName(const FName SignalName)
{
	int32 Index = SignalNames.Find(SignalName);
	if (Index == INDEX_NONE)
	{
		if (SignalNames.Num() >= MaxSignalNames)
		{
			return 0;
		}
		Index = SignalNames.Add(SignalName);
	}
	return 1ULL << Index;
}

void FRecallSignalNameLookup::AddSignalToEntity(const FMassEntityHandle Entity, const uint64 SignalFlag)
{
	uint64& Signals = EntitySignals.FindOrAdd(Entity, 0);
	Signals |= SignalFlag;
}

void FRecallSignalNameLookup::GetSignalsForEntity(const FMassEntityHandle Entity, TArray<FName>& OutSignals) const
{
	OutSignals.Reset();
	if (const uint64* Signals = EntitySignals.Find(Entity))
	{
		const uint64 Start = FMath::CountTrailingZeros64(*Signals);
		const uint64 End = FMath::CountLeadingZeros64(*Signals);
		for (uint64 i = Start; i < 64 - End; i++)
		{
			const uint64 SignalFlag = 1ULL << i;
			if ((*Signals) & SignalFlag)
			{
				OutSignals.Add(SignalNames[(uint32)i]);
			}
		}
	}
}

void FRecallSignalNameLookup::Reset()
{
	SignalNames.Reset();
	EntitySignals.Reset();
}

void FRecallSignalSnapshot::SetSignalsBuffers(const TStaticArray<FRecallSignalsBuffer, RecallSignalsProcessorsCount>& InSignalsBuffers, int32 InBufferCount)
{
	BufferCount = InBufferCount;
	SignalsBuffers.SetNum(BufferCount);

	for (int32 Idx = 0; Idx < BufferCount; Idx++)
	{
		SignalsBuffers[Idx] = InSignalsBuffers[Idx];
	}
}

void FRecallSignalSnapshot::SetDelayedSignals(const TArray<FRecallDelayedSignal, TFixedAllocator<RecallDelayedSignalsCount>>& InDelayedSignals)
{
	DelayedSignals.SetNum(InDelayedSignals.Num());

	for (int32 Idx = 0; Idx < InDelayedSignals.Num(); Idx++)
	{
		DelayedSignals[Idx] = InDelayedSignals[Idx];
	}
}

void FRecallSignalSnapshot::GetDelayedSignals(TArray<FRecallDelayedSignal, TFixedAllocator<RecallDelayedSignalsCount>>& InDelayedSignals) const
{
	InDelayedSignals.SetNum(DelayedSignals.Num());

	for (int32 Idx = 0; Idx < DelayedSignals.Num(); Idx++)
	{
		InDelayedSignals[Idx] = DelayedSignals[Idx];
	}
}

void FRecallSignalSnapshot::GetSignalsBuffers(TStaticArray<FRecallSignalsBuffer, RecallSignalsProcessorsCount>& OutSignalsBuffers, int32& OutBufferCount) const
{
	OutBufferCount = BufferCount;

	for (int32 Idx = 0; Idx < BufferCount; Idx++)
	{
		OutSignalsBuffers[Idx] = SignalsBuffers[Idx];
	}
}
