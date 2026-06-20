// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Synchronization/RecallRollbackSubsystem.h"

#include "Settings/RecallSimulationSettings.h"
#include "Subsystems/SubsystemCollection.h"
#include "System/Input/RecallInputQueueSubsystem.h"
#include "System/Simulation/Insight/RecallSimulationInsightSubsystem.h"
#include "System/Simulation/RecallMultiSimSubsystem.h"
#include "System/Snapshot/RecallMultiSimSnapshotSubsystem.h"
#include "System/Synchronization/RecallSynchronizationContainerSubsystem.h"
#include "System/Synchronization/RecallSynchronizationTypes.h"
#ifdef WITH_MULTI_WORLD
#include "System/MultiWorldSubsystem.h"
#include "Utility/MultiWorldUtils.h"
#endif // WITH_MULTI_WORLD
#include "TimerManager.h"
#include "Utility/Player/RecallPlayerUtils.h"
#include "Utility/Rollback/RecallRollbackUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

URecallRollbackSubsystem::URecallRollbackSubsystem()
	: Super()
{
}

void URecallRollbackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<URecallMultiSimSubsystem>();
	Collection.InitializeDependency<URecallMultiSimSnapshotSubsystem>();
	Collection.InitializeDependency<UMultiWorldSubsystem>();
	Collection.InitializeDependency<URecallSimulationInsightSubsystem>();
	Collection.InitializeDependency<URecallSynchronizationContainerSubsystem>();

	ContainerSystem = UWorld::GetSubsystem<URecallSynchronizationContainerSubsystem>(GetWorld());
	MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(GetWorld());
	MultiSimSnapshotSystem = UWorld::GetSubsystem<URecallMultiSimSnapshotSubsystem>(GetWorld());
	SimInsightSystem = UWorld::GetSubsystem<URecallSimulationInsightSubsystem>(GetWorld());

	// Initialize configuration
	if (ContainerSystem.IsValid())
	{
		Config.SetContainerSystem(ContainerSystem);
		const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
		Config.SetRollbackFrameCount(SimulationSettings->RollbackFrameCount);
	}

	if (MultiSimSystem.IsValid())
	{
		MultiSimSystem->GetOnFrameSyncEvent().AddUObject(this, &ThisClass::OnFrameSync);
	}
}

void URecallRollbackSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (MultiSimSystem.IsValid())
	{
		MultiSimSystem->GetOnFrameSyncEvent().RemoveAll(this);
	}
}

void URecallRollbackSubsystem::Reset()
{
	ResetSyncedFrame(0);
}

void URecallRollbackSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallRollbackSubsystem::Save"));
}

void URecallRollbackSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallRollbackSubsystem::Restore"));

#ifdef WITH_MULTI_WORLD
	if (!MultiWorld::Utils::IsMainWorld(this))
	{
		return;
	}
#endif // WITH_MULTI_WORLD
	
	if (Context.IsSnapshot())
	{
		ResetSyncedFrame(Context.Frame);
	}
}

uint32 URecallRollbackSubsystem::GetLastSyncedFrame() const
{
	if (!IsRollbackEnabled())
	{
		return Config.GetConfirmFrame();
	}
	
	return LastSyncedFrame;
}


void URecallRollbackSubsystem::HandleDebugRollbackComparison(uint32 Frame)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	DebugManager.HandleRollbackComparison(Frame, LastSyncedFrame, SimInsightSystem.Get());
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallRollbackSubsystem::HandleRollbackEnd(uint32 Frame)
{
	const uint32 CurrentFrame = Recall::Simulation::Utils::GetFrame(GetWorld());
	
	HandleDebugRollbackComparison(Frame);
	
	EndRollbackState(CurrentFrame);

	OnRollbackEnd.Broadcast();
}

void URecallRollbackSubsystem::OnFrameSync(uint32 Frame)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallRollbackSubsystem::OnFrameSync"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Rollback_OnFrameSync);

	if (IsPauseRollback())
	{
		return;
	}

	if (IsDuringRollback())
	{
		SaveFrame(Frame);

		// End rollback
		if (SyncingSimulationUntil == Frame)
		{
			HandleRollbackEnd(Frame);
		}
	}
	else
	{
		RollbackConfirmFrame = Config.GetConfirmFrame();

		if (SyncFrame(Frame) == false)
		{
			SaveFrame(Frame);
			Config.SetNetPause(GetWorld(), Frame);
		}
	}
}

void URecallRollbackSubsystem::SaveFrame(uint32 Frame)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallRollbackSubsystem::SaveFrame"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Rollback_SaveFrame);

	// No point to save more than one synced frame  
	if (!Recall::Rollback::Utils::IsValidFrameForRollback(Frame, LastSyncedFrame))
	{
		return;
	}

	const FString LocalPlayerId = Recall::Player::Utils::GetLocalPlayerId(GetWorld());
	UE_LOG(LogRecallRollback, VeryVerbose,
		TEXT("%hs [%s] Last snapshot frame: %d"), __FUNCTION__, *LocalPlayerId, Frame);

	if (MultiSimSnapshotSystem.IsValid())
	{
		FrameManager.AddFrame([this, Frame](FRecallRollbackFrame& SyncFrame)
		{
			SyncFrame.Comparator = Recall::Rollback::Utils::CreateFrameComparator(GetWorld(), Frame);
			SyncFrame.bValidSnapshot = !IsDuringRollback() || ShouldSaveRollbackSnapshot(Frame);

			if (SyncFrame.bValidSnapshot)
			{
				MultiSimSnapshotSystem->TakeQuickSnapshot(*SyncFrame.Snapshot.Get(), Recall::SnapshotReason::Rollback);
			}
		});
	}
}

bool URecallRollbackSubsystem::ShouldSaveRollbackSnapshot(uint32 Frame) const
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	const int32 ForceRollbackFrameCount = Recall::Rollback::Utils::GetForceRollbackFrameCount();
	if (ForceRollbackFrameCount != 0)
	{
		// Keep track of enough frames for the debug rollback to take effect
		check(SyncingSimulationUntil >= Frame);
		const int32 SyncingFrameLeft = SyncingSimulationUntil - Frame;
		
		if (ForceRollbackFrameCount > SyncingFrameLeft + 1)
		{
			return false;
		}
	}
	else
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	{
		if (Frame != RollbackConfirmFrame + 1)
		{
			return false;
		}
	}

	return true;
}

FRecallRollbackFrameContext URecallRollbackSubsystem::CreateFrameManagerContext()
{
	FRecallRollbackFrameContext Context;
	Context.MultiSimSystem = MultiSimSystem;
	Context.MultiSimSnapshotSystem = MultiSimSnapshotSystem;
	Context.World = GetWorld();
	Context.Config = &Config;
	Context.LastSyncedFrame = &LastSyncedFrame;
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	Context.DebugManager = &DebugManager;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	return Context;
}

bool URecallRollbackSubsystem::SyncFrame(uint32 Frame)
{
	checkf(SyncingSimulationUntil == 0, TEXT("Do not sync during rollback"));

	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallRollbackSubsystem::SyncFrame"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Rollback_SyncFrame);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	const float FixedDeltaTime = Recall::Simulation::Utils::GetFixedDeltaTime(this);
	DebugManager.UpdateTimer(FixedDeltaTime);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	const FRecallRollbackFrameContext Context = CreateFrameManagerContext();
	const bool bRollbackExecuted = FrameManager.ProcessFrameBuffer(Context, Frame);
	
	// Handle post-processing logic - if rollback was executed, begin rollback state
	if (bRollbackExecuted)
	{
		BeginRollbackState(Frame);
		
		// Rollback was executed, set up debug comparator if needed
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		DebugManager.SetupRollbackComparator(LastSyncedFrame, Frame, SimInsightSystem.Get());
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}

	return bRollbackExecuted;
}

bool URecallRollbackSubsystem::IsRollbackEnabled() const
{
	const int32 RollbackFrameCount = Config.GetRollbackFrameCount();
	const int32 ForceRollbackFrameCount = Config.GetForceRollbackFrameCount();
	if (RollbackFrameCount == 0 && ForceRollbackFrameCount == 0)
	{
		return false;
	}

	return true;
}

bool URecallRollbackSubsystem::IsPauseRollback() const
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (Recall::Rollback::Utils::DebugIsPauseRollback())
	{
		return true;
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	if (bPauseRollback)
	{
		return true;
	}

	if (!IsRollbackEnabled())
	{
		return true;
	}
	
	return false;
}

void URecallRollbackSubsystem::SetPauseRollback(bool bPause)
{ 
	bPauseRollback = bPause; 
}

void URecallRollbackSubsystem::ForceLastSyncedFrame(uint32 Frame)
{
	if (IsRollbackEnabled())
	{
		LastSyncedFrame = Frame;

		// We fake syncing so simulation events are skipped.
		SetSyncingSimulationUntil_Internal(Frame);
	}
}

void URecallRollbackSubsystem::ResetSyncedFrame(uint32 Frame)
{
	Config.SetConfirmFrame(Frame);

	if (IsRollbackEnabled())
	{
		SetSyncingSimulationUntil_Internal(0);
		LastSyncedFrame = Frame;
		FrameManager.GetFrameBuffer().Reset();
	}
}

void URecallRollbackSubsystem::SetConfirmFrame(uint32 Frame)
{
	UpdateConfirmFrameState(Frame);
}

uint32 URecallRollbackSubsystem::GetConfirmFrame() const
{
	return Config.GetConfirmFrame();
}

void URecallRollbackSubsystem::SetSyncingSimulationUntil_Internal(uint32 Frame)
{
	checkf(IsRollbackEnabled(),
		TEXT("%hs Can only sync if rollback is enabled"), __FUNCTION__)
	
	SyncingSimulationUntil = Frame;
}

bool URecallRollbackSubsystem::SaveLastSyncedSnapshot(TArray<uint8>& OutSnapshotMemory) const
{		
	const FRecallRollbackFrameBuffer& FrameBuffer = FrameManager.GetFrameBuffer();
	for (int32 FrameIndex = 0; FrameIndex < FrameBuffer.Num(); FrameIndex++)
	{
		const FRecallRollbackFrame& SyncData = FrameBuffer[FrameIndex];
		const uint32 SyncFrame = SyncData.Comparator.Frame;
		
		if (SyncFrame == LastSyncedFrame && ensure(SyncData.bValidSnapshot))
		{
			URecallMultiSimSnapshotSubsystem::SerializeSnapshot(*SyncData.Snapshot.Get(), OutSnapshotMemory);
			return true;
		}
	}

	return false;
}

void URecallRollbackSubsystem::LoadLastSyncedSnapshot(const TArray<uint8>& SnapshotMemory) const
{
	if (MultiSimSnapshotSystem.IsValid())
	{
		MultiSimSnapshotSystem->LoadSnapshot(SnapshotMemory);
	}
}

int32 URecallRollbackSubsystem::GetRollbackFrameCount() const
{
	return Config.GetRollbackFrameCount();
}

bool URecallRollbackSubsystem::ShouldNetPause() const
{
	const uint32 CurrentFrame = Recall::Simulation::Utils::GetFrame(GetWorld());
	return Config.ShouldNetPause(GetWorld(), CurrentFrame);
}

void URecallRollbackSubsystem::BeginRollbackState(uint32 Frame)
{
	SetSyncingSimulationUntil_Internal(Frame);
	Config.SetRollback(true);
}

void URecallRollbackSubsystem::EndRollbackState(uint32 CurrentFrame)
{
	SetSyncingSimulationUntil_Internal(0);
	Config.SetRollback(false);
	Config.SetNetPause(GetWorld(), CurrentFrame);
}

void URecallRollbackSubsystem::UpdateConfirmFrameState(uint32 Frame)
{
	const uint32 CurrentFrame = Recall::Simulation::Utils::GetFrame(GetWorld());
	Config.SetConfirmFrame(Frame);
	Config.SetNetPause(GetWorld(), CurrentFrame);
}
