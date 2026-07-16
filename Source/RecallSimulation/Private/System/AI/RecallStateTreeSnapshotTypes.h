// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "StateTreeDelegate.h"
#include "System/AI/RecallStateTreeInstanceTypes.h"

#include "RecallStateTreeSnapshotTypes.generated.h"

struct FRecallStateTreeInstanceDataItem;

USTRUCT()
struct FRecallInstancedPropertyBagWrapper : public FInstancedPropertyBag
{
	GENERATED_BODY()

	FInstancedStruct& GetMutableValue() { return Value; }
};

USTRUCT()
struct FRecallStateTreeActiveStatesSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<uint32> StateIDs;
	
	void Save(const FStateTreeActiveStates& ActiveStates, uint32 UniqueIdGenerator);
	void Restore(FStateTreeActiveStates& ActiveStates) const;
};

USTRUCT()
struct FRecallStateTreeActiveFrameSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	uint32 FrameID = 0;

	UPROPERTY(VisibleAnywhere)
	int32 NumCurrentlyActiveStates = 0;

	/** Last entered evaluator/task node index (non-UPROPERTY on FStateTreeExecutionFrame, must be saved explicitly). */
	UPROPERTY(VisibleAnywhere)
	int32 ActiveNodeIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere)
	FRecallStateTreeActiveStatesSnapshot ActiveStates;
	
	UPROPERTY(VisibleAnywhere)
	FStateTreeTasksCompletionStatus ActiveTasksStatus;

	/**
	 * Path of the state trees for the active frames.
	 */
	UPROPERTY(VisibleAnywhere, meta=(AllowedClasses="/Script/StateTreeModule.StateTree"))
	FSoftObjectPath StateTreePath;

	void Save(const FStateTreeExecutionFrame& ActiveFrame, uint32 UniqueIdGenerator);
	void Restore(FStateTreeExecutionFrame& ActiveFrame) const;
};

/*
* Snapshot of a state tree instance data item.
*/
USTRUCT()
struct FRecallStateTreeInstanceDataArraySnapshot
{
	GENERATED_BODY()

public:
	void Restore(UObject& InOwner, FRecallStateTreeInstanceDataItem& Item) const;
	void Save(const FRecallStateTreeInstanceDataItem& Item);

protected:
	/** Struct instances */
	UPROPERTY(VisibleAnywhere)
	TArray<FInstancedStruct> Structs;

	/** Execution state of the state tree instance. */
	UPROPERTY(VisibleAnywhere)
	FStateTreeExecutionState ExecutionState;

	/**
	 * Snapshot of active frames.
	 */
	UPROPERTY(VisibleAnywhere)
	TArray<FRecallStateTreeActiveFrameSnapshot> ActiveFrames;

	/** Events */
	UPROPERTY(VisibleAnywhere)
	TArray<FStateTreeEvent> Events;

	/** Requested transitions */
	UPROPERTY(VisibleAnywhere)
	TArray<FStateTreeTransitionRequest> TransitionRequests;

	/** Delegates broadcasted but not yet consumed by a transition. */
	UPROPERTY(VisibleAnywhere)
	TArray<FStateTreeDelegateDispatcher> BroadcastedDelegates;

	/** Global parameters */
	UPROPERTY(VisibleAnywhere)
	TArray<uint8> GlobalParametersMemory;

	/** Execution runtime data structs (new in 5.8, transient in FStateTreeInstanceStorage) */
	UPROPERTY(VisibleAnywhere)
	TArray<FInstancedStruct> ExecutionRuntimeStructs;

	UPROPERTY(VisibleAnywhere)
	int32 Generation = 0;

	UPROPERTY(VisibleAnywhere)
	uint32 UniqueIdGenerator = 0;

};

/*
* Snapshot of all our state tree instance data items.
*/
USTRUCT()
struct FRecallStateTreeInstanceDataSnapshot
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	TArray<int32> InstanceDataFreelist;

	void GetInstanceDataArray(UObject& InOwner, TArray<FRecallStateTreeInstanceDataItem, TFixedAllocator<RECALL_STATE_TREE_INSTANCE_MAX>>& InstanceDataArray) const;
	void SetInstanceDataArray(const TArray<FRecallStateTreeInstanceDataItem, TFixedAllocator<RECALL_STATE_TREE_INSTANCE_MAX>>& InstanceDataArray);

protected:
	UPROPERTY(VisibleAnywhere)
	TArray<FRecallStateTreeInstanceDataArraySnapshot> ItemSnapshots;
};
