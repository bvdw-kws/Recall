// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Snapshot/RecallMultiSimSnapshotSubsystem.h"

#include "Async/ParallelFor.h"
#include "Engine/World.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "System/Simulation/RecallMultiSimSubsystem.h"
#include "System/Snapshot/RecallSnapshotSubsystem.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"
#include "Subsystems/SubsystemCollection.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
#include "HAL/IConsoleManager.h"

static bool bDebugLogSnapshotMemoryFootprint = false;
static FAutoConsoleVariableRef CVarFRecallLogSnapshotMemoryFootprint(
	TEXT("recall.snapshot.LogMemoryFootprint"),
	bDebugLogSnapshotMemoryFootprint,
	TEXT("Log Snapshot Memory Footprint")
);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

URecallMultiSimSnapshotSubsystem::URecallMultiSimSnapshotSubsystem()
	: Super()
{
}

void URecallMultiSimSnapshotSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Recall::MultiWorld::Utils::InitializeMultiWorldDependency(Collection);
	Collection.InitializeDependency<URecallMultiSimSubsystem>();

	MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(GetWorld());
}

void URecallMultiSimSnapshotSubsystem::Deinitialize()
{
	Super::Deinitialize();

	MultiSimSystem.Reset();
}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
static void LogSnapshotMemoryFootprint(const FRecallMultiSimSnapshot& Snasphot, int32 ByteSize)
{
	const float TotalKilobytesSize = ByteSize * 0.001;

	FString DebugLog;

	DebugLog += TEXT("\n");
	DebugLog += TEXT("//----------------------------------------------------------------------//\n");
	DebugLog += FString::Printf(TEXT("// Snapshot (Size: %.2f kb\n"), TotalKilobytesSize);
	DebugLog += TEXT("//----------------------------------------------------------------------//\n");

	DebugLog += FString::Printf(TEXT("Frame: %d\n"), Snasphot.Frame);
	DebugLog += FString::Printf(TEXT("Time: %.2f\n"), Snasphot.Time);

	int32 WorldNumber = 1;

	for (const FRecallSimulationSnapshot& SimSnapshot : Snasphot.SimSnapshots)
	{
		if (Snasphot.SimSnapshots.Num() > 1)
		{
			DebugLog += FString::Printf(TEXT("Field: %d\n"), WorldNumber++);
		}

		for (const FInstancedStruct& DataSnapshot : SimSnapshot.DataSnapshots)
		{
			if (!DataSnapshot.IsValid())
			{
				continue;
			}

			TArray<uint8> Memory;
			FMemoryWriter MemoryWriter(Memory, true);
			FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);

			const_cast<FInstancedStruct&>(DataSnapshot).Serialize(Ar);

			const float KilobytesSize = Memory.Num() * 0.001;
			DebugLog += FString::Printf(TEXT("* %s: %.2f kb\n"), *DataSnapshot.GetScriptStruct()->GetName(), KilobytesSize);
		}
	}

	DebugLog += TEXT("//----------------------------------------------------------------------//\n");

	UE_LOG(LogRecallSnapshot, Log, TEXT("%s"), *DebugLog);
}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

bool URecallMultiSimSnapshotSubsystem::TakeSnapshot(TArray<uint8>& OutSnapshot) const
{
	FRecallMultiSimSnapshot QuickSnapshot;
	if (ensureMsgf(TakeQuickSnapshot(QuickSnapshot), TEXT("Invalid snapshot")))
	{
		OutSnapshot.Reset();

		SerializeSnapshot(QuickSnapshot, OutSnapshot);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (bDebugLogSnapshotMemoryFootprint)
		{
			LogSnapshotMemoryFootprint(QuickSnapshot, OutSnapshot.Num());
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

		return true;
	}
	return false;
}

void URecallMultiSimSnapshotSubsystem::SerializeSnapshot(const FRecallMultiSimSnapshot& Snapshot,
	TArray<uint8>& OutMemory)
{
	FMemoryWriter MemoryWriter(OutMemory, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
	FRecallMultiSimSnapshot::StaticStruct()->SerializeBin(Ar, const_cast<FRecallMultiSimSnapshot*>(&Snapshot));
}

bool URecallMultiSimSnapshotSubsystem::LoadSnapshot(const TArray<uint8>& Snapshot)
{
	FMemoryReader MemoryReader(Snapshot, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
	FRecallMultiSimSnapshot QuickSnapshot;
	FRecallMultiSimSnapshot::StaticStruct()->SerializeBin(Ar, &QuickSnapshot);

	return LoadQuickSnapshot(QuickSnapshot);
}

void URecallMultiSimSnapshotSubsystem::SaveQuick()
{
	TakeSnapshot(QuickSnapshotMemory);
}

void URecallMultiSimSnapshotSubsystem::LoadQuick()
{
	if (!QuickSnapshotMemory.IsEmpty())
	{
		LoadSnapshot(QuickSnapshotMemory);
	}
}

bool URecallMultiSimSnapshotSubsystem::TakeQuickSnapshot(FRecallMultiSimSnapshot& OutSnapshot, const FName& Reason /*= Recall::SnapshotReason::Snapshot*/) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(
		TEXT("URecallMultiSimSnapshotSubsystem::TakeQuickSnapshot")));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSnapshotSystem_TakeQuickSnapshot);

	if (!ensureMsgf(MultiSimSystem.IsValid() && MultiSimSystem->HasSimulationStarted(),
		TEXT("Only take snapshot when game sim is running")))
	{
		return false;
	}

	if (Reason == Recall::SnapshotReason::Snapshot)
	{
		MultiSimSystem->WaitForStepThread();
	}

	OutSnapshot.Frame = MultiSimSystem->GetElapsedFrame();
	OutSnapshot.Time = MultiSimSystem->GetElapsedTime();

	const TArray<const UWorld*> NestedWorlds = Recall::MultiWorld::Utils::GetMultiWorlds(this);

	OutSnapshot.SimSnapshots.SetNum(NestedWorlds.Num());

	ParallelFor(NestedWorlds.Num(), [&NestedWorlds, &OutSnapshot, Reason](int32 Index)
		{
			if (const URecallSnapshotSubsystem* SnapshotSystem = UWorld::GetSubsystem<URecallSnapshotSubsystem>(NestedWorlds[Index]))
			{
				SnapshotSystem->TakeSnapshot(OutSnapshot.SimSnapshots[Index], Reason);
			}
		}
	);

	return true;
}

bool URecallMultiSimSnapshotSubsystem::LoadQuickSnapshot(const FRecallMultiSimSnapshot& Snapshot, const FName& Reason /*= Recall::SnapshotReason::Snapshot*/)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallMultiSimSnapshotSubsystem::LoadQuickSnapshot")));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSnapshotSystem_LoadQuickSnapshot);

	if (!ensureMsgf(MultiSimSystem.IsValid() && MultiSimSystem->HasSimulationStarted(), TEXT("Only load snapshot when game sim is running")))
	{
		return false;
	}

	const TArray<const UWorld*> NestedWorlds = Recall::MultiWorld::Utils::GetMultiWorlds(this);
	if (!ensureMsgf(NestedWorlds.Num() == Snapshot.SimSnapshots.Num(),
		TEXT("%hs Snapshot world count does not match."), __FUNCTION__))
	{
		return false;
	}

	if (Reason == Recall::SnapshotReason::Snapshot)
	{
		MultiSimSystem->WaitForStepThread();
	}

	for (int32 WorldIndex = 0; WorldIndex < NestedWorlds.Num(); WorldIndex++)
	{
		const UWorld* NestedWorld = NestedWorlds[WorldIndex];
		if (const URecallSnapshotSubsystem* SnapshotSystem = UWorld::GetSubsystem<URecallSnapshotSubsystem>(NestedWorld))
		{
			const FRecallSimulationSnapshot& WorldSnapshot = Snapshot.SimSnapshots[WorldIndex];
			SnapshotSystem->PreLoadSnapshot(WorldSnapshot, Reason);
		}
	}
	
	ParallelFor(NestedWorlds.Num(), [&NestedWorlds, &Snapshot, Reason](int32 WorldIndex)
		{
			const UWorld* NestedWorld = NestedWorlds[WorldIndex];
			if (const URecallSnapshotSubsystem* SnapshotSystem = UWorld::GetSubsystem<URecallSnapshotSubsystem>(NestedWorld))
			{
				const FRecallSimulationSnapshot& WorldSnapshot = Snapshot.SimSnapshots[WorldIndex];
				SnapshotSystem->LoadSnapshot(WorldSnapshot, Reason, false);
			}
		}
	);

	for (int32 WorldIndex = 0; WorldIndex < NestedWorlds.Num(); WorldIndex++)
	{
		const UWorld* NestedWorld = NestedWorlds[WorldIndex];
		if (const URecallSnapshotSubsystem* SnapshotSystem = UWorld::GetSubsystem<URecallSnapshotSubsystem>(NestedWorld))
		{
			const FRecallSimulationSnapshot& WorldSnapshot = Snapshot.SimSnapshots[WorldIndex];
			SnapshotSystem->FinishLoadSnapshot(WorldSnapshot, Reason);
		}
	}

	if (Reason == Recall::SnapshotReason::Rollback || 
		Reason == Recall::SnapshotReason::Snapshot)
	{
		const bool bIsRollback = Reason == Recall::SnapshotReason::Rollback;
		MultiSimSystem->OnLoadSnapshot(Snapshot.Frame, Snapshot.Time, bIsRollback);

		GetOnLoadSnapshotEvent().Broadcast(Snapshot.Frame, Snapshot.Time, bIsRollback);
	}

	return true;
}
