// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/AI/RecallStateTreeSubsystem.h"

#include "Engine/Engine.h"
#include "RecallStateTreeSnapshotTypes.h"

void URecallStateTreeSubsystem::Reset()
{
	InstanceDataFreelist.Reset();
	InstanceDataArray.Reset();
}

void URecallStateTreeSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallStateTreeSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_StateTree_Save);

	OutSnapshot.InitializeAs<FRecallStateTreeInstanceDataSnapshot>();

	FRecallStateTreeInstanceDataSnapshot& Snapshot = OutSnapshot.GetMutable<FRecallStateTreeInstanceDataSnapshot>();
	Snapshot.InstanceDataFreelist = InstanceDataFreelist;
	Snapshot.SetInstanceDataArray(InstanceDataArray);
}

void URecallStateTreeSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallStateTreeSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_StateTree_Restore);

	if (const FRecallStateTreeInstanceDataSnapshot* InData = InSnapshot.GetPtr<FRecallStateTreeInstanceDataSnapshot>())
	{
		InstanceDataFreelist = InData->InstanceDataFreelist;
		InData->GetInstanceDataArray(*this, InstanceDataArray);
	}
}

FRecallStateTreeInstanceHandle URecallStateTreeSubsystem::AllocateInstanceData()
{
	FRecallStateTreeInstanceHandle NewHandle;

	{
		FScopeLock Lock(&DataGuard);

		int32 Index = 0;
		if (InstanceDataFreelist.Num() > 0)
		{
			Index = InstanceDataFreelist.Pop();
		}
		else
		{
			Index = InstanceDataArray.Num();
			InstanceDataArray.AddDefaulted();
		}

		FRecallStateTreeInstanceDataItem& Item = InstanceDataArray[Index];
		Item.InstanceData.Reset();

		NewHandle = FRecallStateTreeInstanceHandle::Make(Index, Item.Generation);
	}

	return NewHandle;
}

void URecallStateTreeSubsystem::FreeInstanceData(FRecallStateTreeInstanceHandle& Handle)
{
	if (!IsValidHandle(Handle))
	{
		return;
	}

	{
		FScopeLock Lock(&DataGuard);

		FRecallStateTreeInstanceDataItem& Item = InstanceDataArray[Handle.GetIndex()];
		Item.InstanceData.Reset();
		Item.Generation++;

		InstanceDataFreelist.Add(Handle.GetIndex());
	}

	Handle.Invalidate();
}

FStateTreeInstanceData* URecallStateTreeSubsystem::GetInstanceData(const FRecallStateTreeInstanceHandle& Handle)
{
	return IsValidHandle(Handle) ? &InstanceDataArray[Handle.GetIndex()].InstanceData : nullptr;
}

bool URecallStateTreeSubsystem::IsValidHandle(const FRecallStateTreeInstanceHandle& Handle) const
{
	return InstanceDataArray.IsValidIndex(Handle.GetIndex()) && InstanceDataArray[Handle.GetIndex()].Generation == Handle.GetGeneration();
}

void URecallStateTreeSubsystem::SendStateTreeEvent(const FRecallStateTreeInstanceHandle& Handle, const FStateTreeEvent& Event)
{
	if (!Handle.IsValid())
	{
		return;
	}

	{		
		FScopeLock Lock(&DataGuard);
	
		FStateTreeInstanceData* InstanceDataPtr = GetInstanceData(Handle);
		if (InstanceDataPtr == nullptr)
		{
			return;
		}

		InstanceDataPtr->GetMutableEventQueue().SendEvent(this, Event.Tag, Event.Payload, Event.Origin);
	}
}

uint32 URecallStateTreeSubsystem::GetRandomStreamCombinedHash() const
{
	uint32 Hash = 0;

	for (int32 InstanceIndex = 0; InstanceIndex < InstanceDataArray.Num(); InstanceIndex++)
	{
		if (InstanceDataFreelist.Contains(InstanceIndex))
		{
			continue;
		}

		const FRecallStateTreeInstanceDataItem& InstanceDataItem = InstanceDataArray[InstanceIndex];

		if (const FStateTreeExecutionState* ExecState = InstanceDataItem.InstanceData.GetExecutionState())
		{
			Hash = HashCombine(Hash, ExecState->RandomStream.GetCurrentSeed());
		}
	}

	return Hash;
}
