// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "MassEntityManager.h"

#include "RecallSignalTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRecallSignals, Log, All)

namespace Recall::Signals::ProcessorGroupNames
{

	const FName UpdateDelayedSignals = FName(TEXT("RecallUpdateDelayedSignals"));
	const FName SignalEntities = FName(TEXT("RecallSignalEntities"));

} // namespace Recall::StateTree::ProcessorGroupNames

/**
 * RecallSignalNameLookup stores list of Signal names for each entity. The names are stored per entity as a bitmask,
 * you can allocate new name using GetOrAddSignalName(). This limits the names to 64.
 */
struct RECALLSIGNALS_API FRecallSignalNameLookup
{
	/** Max number of names each entity can contain */
	static constexpr int32 MaxSignalNames = 64;

	/** 
	 * Retrieve if it is already registered or adds new signal to the lookup and return the bitflag for that Signal
	 * @SignalName is the name of the signal to retrieve or add to the lookup.
	 * @return bitflag describing the name, or 0 if max names is reached. 
	 */
	uint64 GetOrAddSignalName(const FName SignalName);

	/**
	 * Adds specified Signal name bitflag to an entity 
	 * @param Entity is the entity where the signal has been raised
	 * @param SignalFlag is the actual bitflag describing the signal
	 */
	void AddSignalToEntity(const FMassEntityHandle Entity, const uint64 SignalFlag);

	/** 
	 * Retrieve for a specific entity the raised signal this frame
	 * @return Array of signal names raised for this entity 
	 */
	void GetSignalsForEntity(const FMassEntityHandle Entity, TArray<FName>& OutSignals) const;

	/** Empties the name lookup and entity signals */
	void Reset();

protected:
	/** Array of Signal names */
	TArray<FName> SignalNames;

	/** Map from entity id to name bitmask */
	TMap<FMassEntityHandle, uint64> EntitySignals;
};

USTRUCT()
struct FRecallDelayedSignal
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName SignalName;
	UPROPERTY()
	TArray<FMassEntityHandle> Entities;
	UPROPERTY()
	double TargetTimestamp = 0.0;
};

/** Stores a range of indices in the SignaledEntities TArray of Entities and the associated signal name */
USTRUCT()
struct FRecallEntitySignalRange
{
	GENERATED_BODY()

	UPROPERTY()
	FName SignalName;
	UPROPERTY()
	int32 Begin = 0;
	UPROPERTY()
	int32 End = 0;
	UPROPERTY()
	bool bProcessed = false;
};

USTRUCT()
struct FRecallFrameReceivedSignals
{
	GENERATED_BODY()

	/** Received signals are double buffered as we can receive new one while processing them */
	UPROPERTY()
	TArray<FRecallEntitySignalRange> ReceivedSignalRanges;

	/** the list of all signaled entities, can contain duplicates */
	UPROPERTY()
	TArray<FMassEntityHandle> SignaledEntities;
};

static constexpr int32 RecallSignalsBuffersCount = 2;

USTRUCT()
struct FRecallSignalsBuffer
{
	GENERATED_BODY()

	FRecallSignalsBuffer()
	{
		FrameReceivedSignals.SetNum(RecallSignalsBuffersCount);
	}

	/** Double buffer frame received signal as we can receive new signals as we are processing them */
	UPROPERTY()
	TArray<FRecallFrameReceivedSignals> FrameReceivedSignals;

	/** Current frame buffer index of FrameReceivedSignals */
	UPROPERTY()
	int32 CurrentFrameBufferIndex = 0;
};

static constexpr int32 RecallSignalsProcessorsCount = 64;
static constexpr int32 RecallDelayedSignalsCount = 1024;

USTRUCT()
struct FRecallSignalSnapshot
{
	GENERATED_BODY()

	FRecallSignalSnapshot() = default;

public:
	UPROPERTY()
	double TimeSeconds = 0.0;

public:
	void SetSignalsBuffers(const TStaticArray<FRecallSignalsBuffer, RecallSignalsProcessorsCount>& InSignalsBuffers, int32 InBufferCount);
	void GetSignalsBuffers(TStaticArray<FRecallSignalsBuffer, RecallSignalsProcessorsCount>& OutSignalsBuffers, int32& OutBufferCount) const;

	void SetDelayedSignals(const TArray<FRecallDelayedSignal, TFixedAllocator<RecallDelayedSignalsCount>>& InDelayedSignals);
	void GetDelayedSignals(TArray<FRecallDelayedSignal, TFixedAllocator<RecallDelayedSignalsCount>>& InDelayedSignals) const;

protected:
	UPROPERTY()
	TArray<FRecallDelayedSignal> DelayedSignals;
	UPROPERTY()
	TArray<FRecallSignalsBuffer> SignalsBuffers;
	UPROPERTY()
	int32 BufferCount = 0;
};
