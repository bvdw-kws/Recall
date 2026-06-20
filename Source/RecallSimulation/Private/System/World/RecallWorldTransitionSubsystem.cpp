// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/World/RecallWorldTransitionSubsystem.h"

#include "RecallWorldTransitionSnapshot.h"

URecallWorldTransitionSubsystem::URecallWorldTransitionSubsystem()
	: Super()
{
}

void URecallWorldTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void URecallWorldTransitionSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URecallWorldTransitionSubsystem::Reset()
{
	FieldTransitionData = FRecallWorldTransitionData();
}

void URecallWorldTransitionSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallWorldTransitionSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_FieldTransition_Save);

	OutSnapshot.InitializeAs<FRecallWorldTransitionSnapshot>();

	FRecallWorldTransitionSnapshot& NewSnapshot = OutSnapshot.GetMutable<FRecallWorldTransitionSnapshot>();
	NewSnapshot.Data = FieldTransitionData;
}

void URecallWorldTransitionSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallWorldTransitionSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_FieldTransition_Restore);

	if (const FRecallWorldTransitionSnapshot* InData = InSnapshot.GetPtr<FRecallWorldTransitionSnapshot>())
	{
		FieldTransitionData = InData->Data;
	}
}

void URecallWorldTransitionSubsystem::Start(const FRecallSimulationStartParams& Params)
{
}

void URecallWorldTransitionSubsystem::SendEvent(uint32 Frame, const FString& PlayerId, ERecallWorldTransitionEventType EventType)
{
	FRecallWorldTransitionEvent NewEvent;
	NewEvent.Frame = Frame;
	NewEvent.PlayerId = PlayerId;
	NewEvent.Type = EventType;

	FieldTransitionData.Events.Add(NewEvent);
}

void URecallWorldTransitionSubsystem::RollbackFrameForPlayerArray(uint32 Frame, TArray<FString>& PlayerArray) const
{
	for (int32 EventIndex = FieldTransitionData.Events.Num() - 1; EventIndex >= 0; EventIndex--)
	{
		const auto& Event = FieldTransitionData.Events[EventIndex];

		if (Event.Frame < Frame)
		{
			break;
		}
		else if (Event.Frame == Frame)
		{
			if (Event.IsEnterEvent())
			{
				PlayerArray.Remove(Event.PlayerId);
			}
			else if (Event.IsLeaveEvent())
			{
				PlayerArray.Add(Event.PlayerId);
			}
		}
	}
}
