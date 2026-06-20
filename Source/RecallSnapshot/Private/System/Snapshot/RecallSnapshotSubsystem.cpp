// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Snapshot/RecallSnapshotSubsystem.h"

#include "Async/ParallelFor.h"
#include "Engine/World.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "System/Simulation/RecallSimulationSubsystem.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

URecallSnapshotSubsystem::URecallSnapshotSubsystem()
	: Super()
{
}

void URecallSnapshotSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<URecallSimulationSubsystem>();

	StepSimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(GetWorld());
}

void URecallSnapshotSubsystem::Deinitialize()
{
	Super::Deinitialize();

	StepSimulationSystem.Reset();
}

void URecallSnapshotSubsystem::TakeSnapshot(FRecallSimulationSnapshot& OutSnapshot, const FName& Reason /*= Recall::SnapshotReason::Snapshot*/) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallSnapshotSubsystem::TakeSnapshot")));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_SnapshotSystem_TakeSnapshot);

	const uint32 Frame = Recall::Simulation::Utils::GetFrame(GetWorld());

	OutSnapshot.Frame = Frame;
	OutSnapshot.TimeSeconds = Recall::Simulation::Utils::GetTimeSeconds(GetWorld());

	const TArray<TScriptInterface<IRecallSimulationReactSystemInterface>> SimNotifications = URecallSimulationReactSystemInterface::GetSimulationReactSystems(GetWorld());

	OutSnapshot.DataSnapshots.SetNum(SimNotifications.Num());

	FRecallSnapshotContext Context;
	Context.Frame = Frame;
	Context.Reason = Reason;

	ParallelFor(SimNotifications.Num(), [&SimNotifications, &Context, &OutSnapshot](int32 Index)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("TakeSnapshot")));
			SimNotifications[Index]->Save(Context, OutSnapshot.DataSnapshots[Index]);
		}
	);
}

void URecallSnapshotSubsystem::LoadSnapshot(const FRecallSimulationSnapshot& Snapshot,
	const FName& Reason /*= Recall::SnapshotReason::Snapshot*/, bool bGameThread /*= true*/) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallSnapshotSubsystem::LoadSnapshot")));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_SnapshotSystem_LoadSnapshot);

	const TArray<TScriptInterface<IRecallSimulationReactSystemInterface>> SimNotifications = URecallSimulationReactSystemInterface::GetSimulationReactSystems(GetWorld());

	checkf(Snapshot.DataSnapshots.Num() == SimNotifications.Num(),
		TEXT("%hs Should have data for each system"), __FUNCTION__)
	check(!bGameThread || IsInGameThread());

	if (bGameThread)
	{
		PreLoadSnapshot(Snapshot, Reason);
	}
	
	FRecallSnapshotContext Context;
	Context.Frame = Snapshot.Frame;
	Context.Reason = Reason;

	ParallelFor(SimNotifications.Num(), [&SimNotifications, &Context, &Snapshot](int32 Index)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("LoadSnapshot")));
			SimNotifications[Index]->Restore(Context, Snapshot.DataSnapshots[Index]);
		}
	);

	if (bGameThread)
	{
		FinishLoadSnapshot(Snapshot, Reason);
	}
}

void URecallSnapshotSubsystem::PreLoadSnapshot(const FRecallSimulationSnapshot& Snapshot, const FName& Reason) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallSnapshotSubsystem::LoadSnapshot")));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_SnapshotSystem_LoadSnapshot);

	const TArray<TScriptInterface<IRecallSimulationReactSystemInterface>> SimNotifications = URecallSimulationReactSystemInterface::GetSimulationReactSystems(
		GetWorld());

	checkf(Snapshot.DataSnapshots.Num() == SimNotifications.Num(),
		TEXT("%hs Should have data for each system"), __FUNCTION__)

	FRecallSnapshotContext Context;
	Context.Frame = Snapshot.Frame;
	Context.Reason = Reason;

	for (int32 Index = 0; Index < SimNotifications.Num(); Index++)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("PreRestore")));
		SimNotifications[Index]->PreRestore(Context, Snapshot.DataSnapshots[Index]);
	}
}

void URecallSnapshotSubsystem::FinishLoadSnapshot(const FRecallSimulationSnapshot& Snapshot, const FName& Reason /*= Recall::SnapshotReason::Snapshot*/) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallSnapshotSubsystem::FinishLoadSnapshot")));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_SnapshotSystem_FinishLoadSnapshot);

	TArray<TScriptInterface<IRecallSimulationReactSystemInterface>> SimNotifications = URecallSimulationReactSystemInterface::GetSimulationReactSystems(GetWorld());

	checkf(Snapshot.DataSnapshots.Num() == SimNotifications.Num(),
		TEXT("%hs Should have data for each system"), __FUNCTION__)
	
	SimNotifications.StableSort([](const TScriptInterface<IRecallSimulationReactSystemInterface>& lhs, const TScriptInterface<IRecallSimulationReactSystemInterface>& rhs)
		{
			return lhs->GetPostRestoreOrderPriority() >= rhs->GetPostRestoreOrderPriority();
		}
	);

	for (const TScriptInterface<IRecallSimulationReactSystemInterface>& SimNotif : SimNotifications)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("FinishLoadSnapshot")));
		SimNotif->PostRestore();
	}

	if (Reason == Recall::SnapshotReason::Rollback ||
		Reason == Recall::SnapshotReason::Snapshot)
	{
		if (StepSimulationSystem.IsValid())
		{
			StepSimulationSystem->OnLoadSnapshot(Snapshot.Frame, Snapshot.TimeSeconds);
		}

		OnLoadSnapshot.Broadcast(Snapshot.Frame, Snapshot.TimeSeconds);
	}
}
