// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Representation/RecallRepresentationEventSubsystem.h"

#include "System/Observer/RecallObserverSubjectSubsystem.h"
#include "System/Simulation/RecallMultiSimSubsystem.h"
#include "System/Synchronization/RecallSynchronizationTypes.h"
#include "Utility/Simulation/RecallSimulationUtils.h"
#include "Utility/MultiWorldUtils.h"

void URecallRepresentationEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<URecallObserverSubjectSubsystem>();
	Collection.InitializeDependency<URecallMultiSimSubsystem>();

	const UWorld* MainWorld = MultiWorld::Utils::GetMainWorld(this);	
	if (URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld))
	{
		MultiSimSystem->GetOnTickStartEvent().AddUObject(this, &ThisClass::OnTickStart);
	}

	ObserverSubjectSystem = UWorld::GetSubsystem<URecallObserverSubjectSubsystem>(GetWorld());
}

void URecallRepresentationEventSubsystem::Deinitialize()
{
	Super::Deinitialize();

	const UWorld* MainWorld = MultiWorld::Utils::GetMainWorld(this);	
	if (URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld))
	{
		MultiSimSystem->GetOnTickStartEvent().RemoveAll(this);
	}

	ObserverSubjectSystem.Reset();
}

void URecallRepresentationEventSubsystem::Reset()
{
	FScopeLock Lock(&DataGuard);
	EventQueue.Reset();
}

void URecallRepresentationEventSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallRepresentationEventSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_RepresentationEvent_Save);
}

void URecallRepresentationEventSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallRepresentationEventSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_RepresentationEvent_Restore);

	FScopeLock Lock(&DataGuard);
	if (Context.IsSnapshot())
	{
		EventQueue.Reset();
	}
	else
	{
		while (EventQueue.Num() && EventQueue[0].Frame >= Context.Frame)
		{
			EventQueue.PopFrontNoCheck();
		}
	}
}

void URecallRepresentationEventSubsystem::PushEvent(FRecallRepresentationEventFunc Callback)
{
	Recall::Simulation::Utils::CheckSimulationProcessingPhase(this, TEXT("Events pushed outside of processing phase might be skipped"));

	FRecallRepresentationEvent NewEvent;
	NewEvent.Frame = Recall::Simulation::Utils::GetFrame(this);
	NewEvent.Callback = Callback;

	FScopeLock Lock(&DataGuard);
	if (ensureMsgf(EventQueue.Num() < RECALL_REPRESENTATION_EVENT_MAX,
		TEXT("%hs Please increase the size of the buffer"), __FUNCTION__))
	{
		EventQueue.Add(NewEvent);
	}
}

void URecallRepresentationEventSubsystem::OnTickStart(float DeltaTime)
{
	check(IsInGameThread());
	
	const uint32 ConfirmFrame = GetConfirmFrame(this);
	
	FScopeLock Lock(&DataGuard);
	while (EventQueue.Num() && ConfirmFrame >= EventQueue[0].Frame)
	{
		const FRecallRepresentationEvent& Event = EventQueue[0];
		Event.Callback();
		EventQueue.PopFrontNoCheck();
	}
}

TArray<UObject*> URecallRepresentationEventSubsystem::GetObserverObjects(UClass* Class) const
{
	if (ensure(ObserverSubjectSystem.IsValid()))
	{
		return ObserverSubjectSystem->GetObserverObjects(Class);
	}
	else
	{
		return {};
	}
}
