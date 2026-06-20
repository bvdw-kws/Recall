// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassProcessor.h"
#include "MassEntityManager.h"
#include "RecallSignalTypes.h"
#include "Containers/StaticArray.h"
#include "Misc/SpinLock.h"
#include "RecallSignalProcessorBase.generated.h"

class URecallSignalSubsystem;

/** 
 * Processor for executing signals on each targeted entities
 * The derived classes only need to implement the method SignalEntities to actually received the raised signals for the entities they subscribed to 
 */
UCLASS(abstract)
class RECALLSIGNALS_API URecallSignalProcessorBase : public UMassProcessor
{
	GENERATED_BODY()

public:
	URecallSignalProcessorBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override;
	virtual void BeginDestroy() override;

	/** Configure the owned FMassEntityQuery instances to express processor queries requirements */
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override {}

	/**
	 * Actual method that derived class needs to implement to act on a signal that is raised for that frame
	 * @param EntitySubsystem is the system to execute the lambdas on each entity chunk
	 * @param Context is the execution context to be passed when executing the lambdas
	 * @param EntitySignals Look up to retrieve for each entities their raised signal via GetSignalsForEntity
	 */
	 virtual void SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context, FRecallSignalNameLookup& EntitySignals) PURE_VIRTUAL(URecallSignalProcessorBase::SignalEntities, );

	/**
	 * Callback that is being called when new signal is raised
	 * @param SignalName is the name of the signal being raised
	 * @param Entities are the targeted entities for this signal
	 */
	 virtual void OnSignalReceived(FName SignalName, TConstArrayView<FMassEntityHandle> Entities);

	/**
	 * Execution method for this processor
	 * @param EntitySubsystem is the system to execute the lambdas on each entity chunk
	 * @param Context is the execution context to be passed when executing the lambdas
	 */
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	/** 
	 * We don't want signaling processors to be pruned by default, since that can mess up signal processing 
	 * just after processor's creation (might miss some signals initially).
	 */
	virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override { return false; }


	/**
	 * To receive notification about a particular signal, you need to subscribe to it.
	 * @param SignalName is the name of the signal to receive notification about
	 */
	void SubscribeToSignal(const FName SignalName);

protected:
	FMassEntityQuery EntityQuery;

private:
	/** Index of our signals buffer stored in the signal system */
	UPROPERTY(transient)
	int32 SignalsBufferIndex = INDEX_NONE;

	UPROPERTY(transient)
	TWeakObjectPtr<class URecallSignalSubsystem> InternalSignalSystem;

	/** Lookup used to store and retrieve signals per entity, only used during processing */
	FRecallSignalNameLookup SignalNameLookup;

	/** List of all the registered signal names*/
	TArray<FName> RegisteredSignals;

	UE::FSpinLock ReceivedSignalLock;
};

