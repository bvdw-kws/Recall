// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityManager.h"
#include "MassSubsystemBase.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "RecallSignalTypes.h"

#include "RecallSignalSubsystem.generated.h"

namespace UE::RecallSignal 
{
	DECLARE_MULTICAST_DELEGATE_TwoParams(FSignalDelegate, FName /*SignalName*/, TConstArrayView<FMassEntityHandle> /*Entities*/);
} // UE::RecallSignal

/**
* A subsystem for handling Signals in Mass (with rollback support)
*/
UCLASS()
class RECALLSIGNALS_API URecallSignalSubsystem : public UMassSubsystemBase, public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()
	
public:

	/** 
	 * Retrieve the delegate dispatcher from the signal name
	 * @param SignalName is the name of the signal to get the delegate dispatcher from
	 */
	UE::RecallSignal::FSignalDelegate& GetSignalDelegateByName(FName SignalName)
	{
		return NamedSignals.FindOrAdd(SignalName);
	}

	/**
	 * Create the signals buffer used by signals processor
	 * @returns Index of the new signals buffer
	 */
	int32 CreateSignalsBuffer()
	{
		const int32 NewIndex = SignalsBuffersCount++;
		checkf(NewIndex < RecallSignalsProcessorsCount, TEXT("Please increase buffer count"));
		return NewIndex;
	}

	/**
	 * Retrieve the signals buffer used by signals processor
	 * @param Index is the index of the signals buffer to get
	 */
	FRecallSignalsBuffer& GetSignalsBuffer(int32 Index)
	{
		check(Index >= 0 && Index < SignalsBuffersCount);
		return SignalsBuffers[Index];
	}

	/**
	 * Inform a single entity of a signal being raised
	 * @param SignalName is the name of the signal raised
	 * @param Entity entity that should be informed that signal 'SignalName' was raised
	 */
	void SignalEntity(FName SignalName, const FMassEntityHandle Entity);

	/**
	 * Inform multiple entities of a signal being raised
	 * @param SignalName is the name of the signal raised
	 * @param Entities list of entities that should be informed that signal 'SignalName' was raised
	 */
	void SignalEntities(FName SignalName, TConstArrayView<FMassEntityHandle> Entities);

	/**
	 * Inform a single entity of a signal being raised in a certain amount of seconds
	 * @param SignalName is the name of the signal raised
	 * @param Entity entity that should be informed that signal 'SignalName' was raised
	 * @param DelayInSeconds is the amount of time before signaling the entity
	 */
	void DelaySignalEntity(FName SignalName, const FMassEntityHandle Entity, const float DelayInSeconds);

 	/**
	 * Inform multiple entities of a signal being raised in a certain amount of seconds
	 * @param SignalName is the name of the signal raised
	 * @param Entities being informed of the raised signal
	 * @param DelayInSeconds is the amount of time before signaling the entities
	 */
	void DelaySignalEntities(FName SignalName, TConstArrayView<FMassEntityHandle> Entities, const float DelayInSeconds);

	/**
	 * Inform single entity of a signal being raised asynchronously using the Mass Command Buffer
	 * @param Context is the Entity System execution context to push the async command
	 * @param SignalName is the name of the signal raised
	 * @param Entity entity that should be informed that signal 'SignalName' was raised
	 */
	void SignalEntityDeferred(FMassExecutionContext& Context, FName SignalName, const FMassEntityHandle Entity);

	/**
	 * Inform multiple entities of a signal being raised asynchronously using the Mass Command Buffer
	 * @param Context is the Entity System execution context to push the async command
	 * @param SignalName is the name of the signal raised
	 * @param Entities list of entities that should be informed that signal 'SignalName' was raised
	 */
	void SignalEntitiesDeferred(FMassExecutionContext& Context, FName SignalName, TConstArrayView<FMassEntityHandle> Entities);

 	/**
	 * Inform single entity of a signal being raised asynchronously using the Mass Command Buffer
	 * @param Context is the Entity System execution context to push the async command
	 * @param SignalName is the name of the signal raisedSignalsBuffersCount
	 * @param Entity entity that should be informed that signal 'SignalName' was raised
	 * @param DelayInSeconds is the amount of time before signaling the entities
	 */
	void DelaySignalEntityDeferred(FMassExecutionContext& Context, FName SignalName, const FMassEntityHandle Entity, const float DelayInSeconds);

 	/**
	 * Inform multiple entities of a signal being raised asynchronously using the Mass Command Buffer
	 * @param Context is the Entity System execution context to push the async command
	 * @param SignalName is the name of the signal raised
	 * @param Entities being informed of that signal was raised
	 * @param DelayInSeconds is the amount of time before signaling the entities
	 */
	void DelaySignalEntitiesDeferred(FMassExecutionContext& Context, FName SignalName, TConstArrayView<FMassEntityHandle> Entities, const float DelayInSeconds);

	/**
	 * Update our delayed signals and signal the Entities that waited more than the TargetTimestamp
	 */
	void UpdateDelayedSignals(float DeltaTime);

protected:
	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem implementation End	

	// IRecallSimulationReactSystemInterface implementation Begin
	void Reset() override final;
	void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

private:
	double TimeSeconds = 0.0;
	
	FCriticalSection NamedSignalsGuard;
	TMap<FName, UE::RecallSignal::FSignalDelegate> NamedSignals;

	FCriticalSection DelayedSignalsGuard;
	TArray<FRecallDelayedSignal, TFixedAllocator<RecallDelayedSignalsCount>> DelayedSignals;

	TStaticArray<FRecallSignalsBuffer, RecallSignalsProcessorsCount> SignalsBuffers;
	int32 SignalsBuffersCount = 0;
};

template<>
struct TMassExternalSubsystemTraits<URecallSignalSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
