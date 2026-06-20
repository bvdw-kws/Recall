// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallStateTreeSnapshotTypes.h"

#include "Async/ParallelFor.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "StateTree.h"

//----------------------------------------------------------------------//
// FRecallStateTreeActiveStatesSnapshot
//----------------------------------------------------------------------//
void FRecallStateTreeActiveStatesSnapshot::Save(const FStateTreeActiveStates& ActiveStates, uint32 UniqueIdGenerator)
{
	StateIDs.SetNum(ActiveStates.NumStates);

	for (uint32 UniqueId = UniqueIdGenerator; UniqueId > 0; --UniqueId)
	{
		for (int32 StateIndex = 0; StateIndex < ActiveStates.NumStates; ++StateIndex)
		{
			if (ActiveStates.StateIDs[StateIndex] == UE::StateTree::FActiveStateID(UniqueId))
			{
				StateIDs[StateIndex] = UniqueId;
				break;
			}
		}
	}
}

void FRecallStateTreeActiveStatesSnapshot::Restore(FStateTreeActiveStates& ActiveStates) const
{
	for (int32 StateIndex = 0; StateIndex < ActiveStates.NumStates; ++StateIndex)
	{
		ActiveStates.StateIDs[StateIndex] = UE::StateTree::FActiveStateID(StateIDs[StateIndex]);
	}
}

//----------------------------------------------------------------------//
// FRecallStateTreeActiveFrameSnapshot
//----------------------------------------------------------------------//
void FRecallStateTreeActiveFrameSnapshot::Save(const FStateTreeExecutionFrame& ActiveFrame, uint32 UniqueIdGenerator)
{
	if (ActiveFrame.StateTree)
	{
		StateTreePath = ActiveFrame.StateTree->GetPathName();
	}

	for (uint32 UniqueId = UniqueIdGenerator; UniqueId > 0; --UniqueId)
	{
		if (ActiveFrame.FrameID == UE::StateTree::FActiveFrameID(UniqueId))
		{
			FrameID = UniqueId;
			break;
		}
	}

	NumCurrentlyActiveStates = ActiveFrame.NumCurrentlyActiveStates;
	ActiveTasksStatus = ActiveFrame.ActiveTasksStatus;
	
	ActiveStates.Save(ActiveFrame.ActiveStates, UniqueIdGenerator);
}

void FRecallStateTreeActiveFrameSnapshot::Restore(FStateTreeExecutionFrame& ActiveFrame) const
{
	if (!ActiveFrame.StateTree)
	{
		ActiveFrame.StateTree = Cast<UStateTree>(StateTreePath.ResolveObject());
		ensureAlwaysMsgf(ActiveFrame.StateTree,
			TEXT("%hs Failed to resolve state tree: %s"), __FUNCTION__, *StateTreePath.ToString());
	}

	ActiveFrame.FrameID = UE::StateTree::FActiveFrameID(FrameID);
	ActiveFrame.NumCurrentlyActiveStates = NumCurrentlyActiveStates;
	ActiveFrame.ActiveTasksStatus = ActiveTasksStatus;
	
	ActiveStates.Restore(ActiveFrame.ActiveStates);
}

//----------------------------------------------------------------------//
// FRecallStateTreeInstanceDataArraySnapshot
//----------------------------------------------------------------------//
void FRecallStateTreeInstanceDataArraySnapshot::Restore(UObject& InOwner, FRecallStateTreeInstanceDataItem& Item) const
{
	Item.Generation = Generation;

	TArray<FInstancedStruct> DestStructs;
	DestStructs.SetNum(Structs.Num());

	for (int32 StructIndex = 0; StructIndex < Structs.Num(); StructIndex++)
	{
		const FInstancedStruct& Struct = Structs[StructIndex];
		DestStructs[StructIndex].InitializeAs(Struct.GetScriptStruct(), Struct.GetMemory());
	}

	Item.InstanceData = FStateTreeInstanceData();
	Item.InstanceData.Init(InOwner, DestStructs);
	Item.InstanceData.GetMutableStorage().GetMutableExecutionState() = ExecutionState;

	FRecallStateTreeInstanceStorageWrapper& InstanceStorageWrapper = *(FRecallStateTreeInstanceStorageWrapper*)(const void*)&Item.InstanceData.GetMutableStorage();
	InstanceStorageWrapper.SetUniqueIdGenerator(UniqueIdGenerator);
	
	for (int32 ActiveFrameIndex = 0; ActiveFrameIndex < ActiveFrames.Num(); ActiveFrameIndex++)
	{
		FStateTreeExecutionState& ItemExecState = Item.InstanceData.GetMutableStorage().GetMutableExecutionState();
		check(ItemExecState.ActiveFrames.IsValidIndex(ActiveFrameIndex))

		// State tree might not have been loaded yet when the snapshot was deserialized.
		FStateTreeExecutionFrame& ActiveFrame = ItemExecState.ActiveFrames[ActiveFrameIndex];
		ActiveFrames[ActiveFrameIndex].Restore(ActiveFrame);
	}
	
	for (const FStateTreeEvent& Event : Events)
	{
		Item.InstanceData.GetMutableEventQueue().SendEvent(&InOwner, Event.Tag, Event.Payload, Event.Origin);
	}

	for (const FStateTreeTransitionRequest& TransitionRequest : TransitionRequests)
	{
		Item.InstanceData.GetMutableStorage().AddTransitionRequest(&InOwner, TransitionRequest);
	}

	FInstancedPropertyBag InstancedPropertyBag;

	FMemoryReader MemoryReader(GlobalParametersMemory, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryReader, false);
	InstancedPropertyBag.Serialize(Ar);

	Item.InstanceData.GetMutableStorage().SetGlobalParameters(InstancedPropertyBag);
}

void FRecallStateTreeInstanceDataArraySnapshot::Save(const FRecallStateTreeInstanceDataItem& Item)
{
	check(Item.InstanceData.IsOwningEventQueue());

	Generation = Item.Generation;

	Structs.SetNum(Item.InstanceData.Num());

	for (int32 StructIndex = 0; StructIndex < Item.InstanceData.Num(); StructIndex++)
	{
		const FConstStructView SrcStruct = Item.InstanceData.GetStruct(StructIndex);
		Structs[StructIndex].InitializeAs(SrcStruct.GetScriptStruct(), SrcStruct.GetMemory());
	}

	ExecutionState = Item.InstanceData.GetStorage().GetExecutionState();

	const FRecallStateTreeInstanceStorageWrapper& InstanceStorageWrapper = *(const FRecallStateTreeInstanceStorageWrapper*)(const void*)&Item.InstanceData.GetStorage();
	UniqueIdGenerator = InstanceStorageWrapper.GetUniqueIdGenerator();
	
	ActiveFrames.SetNum(ExecutionState.ActiveFrames.Num());

	for (int32 ActiveFrameIndex = 0; ActiveFrameIndex < ExecutionState.ActiveFrames.Num(); ActiveFrameIndex++)
	{
		const FStateTreeExecutionFrame& ActiveFrame = ExecutionState.ActiveFrames[ActiveFrameIndex];
		ActiveFrames[ActiveFrameIndex].Save(ActiveFrame, UniqueIdGenerator);
	}

	const TConstArrayView<FStateTreeSharedEvent> EventsView = Item.InstanceData.GetEventQueue().GetEventsView();
	Events.SetNum(EventsView.Num());

	for (int32 EventIndex = 0; EventIndex < EventsView.Num(); EventIndex++)
	{
		checkf(EventsView[EventIndex].IsValid(), TEXT("Invalid state tree event"));
		Events[EventIndex] = *EventsView[EventIndex].Get();
	}

	TransitionRequests = Item.InstanceData.GetStorage().GetTransitionRequests();

	const FConstStructView GlobalParametersView = Item.InstanceData.GetStorage().GetGlobalParameters();

	FRecallInstancedPropertyBagWrapper InstancedPropertyBag;
	InstancedPropertyBag.GetMutableValue().InitializeAs(GlobalParametersView.GetScriptStruct(), GlobalParametersView.GetMemory());

	FMemoryWriter MemoryWriter(GlobalParametersMemory, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
	InstancedPropertyBag.Serialize(Ar);
}

//----------------------------------------------------------------------//
// FRecallStateTreeInstanceDataSnapshot
//----------------------------------------------------------------------//
constexpr int32 RecallStateTreeInstancePerThread = 20;

void FRecallStateTreeInstanceDataSnapshot::GetInstanceDataArray(UObject& InOwner, TArray<FRecallStateTreeInstanceDataItem, TFixedAllocator<RECALL_STATE_TREE_INSTANCE_MAX>>& InstanceDataArray) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("FRecallStateTreeInstanceDataSnapshot::GetInstanceDataArray"));
	
	InstanceDataArray.SetNumUninitialized(ItemSnapshots.Num());

	const int32 ThreadCount = (InstanceDataArray.Num() / RecallStateTreeInstancePerThread) + 1;
	
	ParallelFor(ThreadCount, [&](int32 ThreadIndex)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("FRecallStateTreeInstanceDataSnapshot::GetInstanceDataArray ParallelFor"));

			const int32 StartIndex = RecallStateTreeInstancePerThread * ThreadIndex;
			const int32 EndIndex = FMath::Min(StartIndex + RecallStateTreeInstancePerThread, InstanceDataArray.Num());
		
			for (int32 Index = StartIndex; Index < EndIndex; Index++)
			{
				if (!InstanceDataFreelist.Contains(Index))
				{
					ItemSnapshots[Index].Restore(InOwner, InstanceDataArray[Index]);
				}
				else
				{
					InstanceDataArray[Index] = FRecallStateTreeInstanceDataItem();
				}
			}
		}
	);
}

void FRecallStateTreeInstanceDataSnapshot::SetInstanceDataArray(const TArray<FRecallStateTreeInstanceDataItem, TFixedAllocator<RECALL_STATE_TREE_INSTANCE_MAX>>& InstanceDataArray)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("FRecallStateTreeInstanceDataSnapshot::SetInstanceDataArray"));
	
	ItemSnapshots.SetNumUninitialized(InstanceDataArray.Num());

	const int32 ThreadCount = (InstanceDataArray.Num() / RecallStateTreeInstancePerThread) + 1;

	ParallelFor(ThreadCount, [&](int32 ThreadIndex)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("FRecallStateTreeInstanceDataSnapshot::SetInstanceDataArray ParallelFor"));

			const int32 StartIndex = RecallStateTreeInstancePerThread * ThreadIndex;
			const int32 EndIndex = FMath::Min(StartIndex + RecallStateTreeInstancePerThread, InstanceDataArray.Num());
			const int32 InstanceCount = EndIndex - StartIndex;
		
			void* Address = (void*)((uint8*)ItemSnapshots.GetAllocatorInstance().GetAllocation() + StartIndex * sizeof(FRecallStateTreeInstanceDataArraySnapshot));
			DefaultConstructItems<FRecallStateTreeInstanceDataArraySnapshot>(Address, InstanceCount);

			for (int32 Index = StartIndex; Index < EndIndex; Index++)
			{
				if (!InstanceDataFreelist.Contains(Index))
				{
					ItemSnapshots[Index].Save(InstanceDataArray[Index]);
				}
			}
		}
	);
}
