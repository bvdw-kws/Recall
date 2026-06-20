// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Synchronization/RecallRollbackFrameManager.h"

#include "System/Simulation/RecallMultiSimSubsystem.h"
#include "System/Snapshot/RecallMultiSimSnapshotSubsystem.h"
#include "System/Snapshot/RecallMultiSimSnapshotTypes.h"
#include "Utility/Rollback/RecallDebugRollbackManager.h"
#include "Utility/Rollback/RecallRollbackConfig.h"
#include "Utility/Rollback/RecallRollbackUtils.h"

FRecallRollbackFrameManager::FRecallRollbackFrameManager()
{
}

void FRecallRollbackFrameManager::AddFrame(const TFunction<void(FRecallRollbackFrame&)>& Delegate)
{
	FRecallRollbackFrame& BufferFrame = FrameBuffer.Add_GetRef();
	Delegate(BufferFrame);
	Recall::Rollback::Utils::ValidateFrameBuffer(FrameBuffer, BufferFrame.Comparator.Frame);

	checkf(!BufferFrame.bValidSnapshot || BufferFrame.Comparator.Frame == BufferFrame.Snapshot->Frame,
		TEXT("%hs Comparator and snapshot should be on the same frame"), __FUNCTION__);
}

void FRecallRollbackFrameManager::CleanupFrameBuffer(int32 LastSyncedFrameIndex)
{
	if (LastSyncedFrameIndex <= 0)
	{
		return;
	}

	const int32 SyncedFrameCount = LastSyncedFrameIndex;
	const uint32 RollbackFrame = FrameBuffer.Num() > LastSyncedFrameIndex ? 
		FrameBuffer[LastSyncedFrameIndex].Comparator.Frame : 0;

	UE_LOG(LogRecallRollback, VeryVerbose,
		TEXT("%hs PopFront %d frames until frame %d (Size: %d)"), __FUNCTION__, SyncedFrameCount,
		RollbackFrame, FrameBuffer.Num());
	
	FrameBuffer.PopFront(SyncedFrameCount);
}

bool FRecallRollbackFrameManager::ProcessFrameBuffer(const FRecallRollbackFrameContext& Context, uint32 Frame)
{
	if (!Context.IsValid())
	{
		UE_LOG(LogRecallRollback, Error, TEXT("%hs Invalid context provided"), __FUNCTION__);
		return false;
	}

	const uint32 CurrentFrame = Frame;
	const uint32 ConfirmFrame = Context.Config->GetConfirmFrame();
	
	// Track if rollback was executed
	bool bRollbackExecuted = false;
	const int32 LastSyncedFrameIndex = ProcessFrameBufferLoop(Context, CurrentFrame, ConfirmFrame, bRollbackExecuted);

	CleanupFrameBuffer(LastSyncedFrameIndex);
	return bRollbackExecuted;
}

int32 FRecallRollbackFrameManager::ProcessFrameBufferLoop(const FRecallRollbackFrameContext& Context,
	uint32 CurrentFrame, uint32 ConfirmFrame, bool& bRollbackExecuted)
{
	int32 LastSyncedFrameIndex = 0;
	bRollbackExecuted = false;

	for (int32 FrameIndex = 0; FrameIndex < FrameBuffer.Num(); ++FrameIndex)
	{
		const FRecallRollbackFrame& SyncData = FrameBuffer[FrameIndex];
		
		bool bFrameRollbackExecuted = false;
		if (!ProcessSingleFrame(Context, SyncData, CurrentFrame, ConfirmFrame, FrameIndex,
			LastSyncedFrameIndex, bFrameRollbackExecuted))
		{
			bRollbackExecuted = bFrameRollbackExecuted;
			break;
		}
	}

	return LastSyncedFrameIndex;
}

bool FRecallRollbackFrameManager::ProcessSingleFrame(const FRecallRollbackFrameContext& Context, 
	const FRecallRollbackFrame& SyncData, uint32 CurrentFrame, uint32 ConfirmFrame, 
	int32 FrameIndex, int32& LastSyncedFrameIndex, bool& bRollbackExecuted)
{
	const int32 ForceRollbackFrameCount = Context.Config->GetForceRollbackFrameCount();
	const uint32 LastSyncedFrame = *Context.LastSyncedFrame;

	// Validate frame and determine processing action
	const Recall::Rollback::Utils::EFrameProcessingAction Action = 
		Recall::Rollback::Utils::ValidateFrameForProcessing(
			SyncData, CurrentFrame, LastSyncedFrame, ConfirmFrame, ForceRollbackFrameCount,
			[Context](uint32 FrameToValidate)
		{
			return Recall::Rollback::Utils::CreateFrameComparator(Context.World, FrameToValidate);
		}
	);

	if (Action == Recall::Rollback::Utils::EFrameProcessingAction::Continue)
	{
		return true; // Continue processing
	}
	if (Action == Recall::Rollback::Utils::EFrameProcessingAction::Break)
	{
		return false; // Stop processing
	}

	// Process frame sync or rollback
	const uint32 SyncFrame = SyncData.Comparator.Frame;
	const FRecallSynchronizationFrameComparator CurrentFrameComparator = 
		Recall::Rollback::Utils::CreateFrameComparator(Context.World, SyncFrame);
	const int32 NumRollbackFrames = Recall::Rollback::Utils::CalculateRollbackFrames(CurrentFrame, SyncFrame);
	
	bool bIsFrameSynced = Recall::Rollback::Utils::EvaluateFrameSync(SyncData, CurrentFrameComparator);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	// Apply debug overrides for random rollback simulation
	Context.DebugManager->ApplyDebugOverrides(bIsFrameSynced);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	if (Recall::Rollback::Utils::CanAdvanceLastSyncedFrame(bIsFrameSynced, SyncFrame, ConfirmFrame, SyncData,
			ForceRollbackFrameCount, NumRollbackFrames))
	{
		return ProcessFrameSync(Context, SyncData, SyncFrame, FrameIndex, LastSyncedFrameIndex);
	}
	else
	{
		bRollbackExecuted = true;
		return ProcessFrameRollback(Context, CurrentFrame, SyncFrame, FrameIndex, LastSyncedFrameIndex,
			ForceRollbackFrameCount, NumRollbackFrames);
	}
}

bool FRecallRollbackFrameManager::ProcessFrameSync(const FRecallRollbackFrameContext& Context, 
	const FRecallRollbackFrame& SyncData, uint32 SyncFrame, int32 FrameIndex, int32& LastSyncedFrameIndex)
{
	Recall::Rollback::Utils::LogFrameSyncProcessing(SyncFrame);
	
	// We need a valid snapshot to advance
	if (SyncData.bValidSnapshot)
	{
		Recall::Rollback::Utils::DebugDumpSyncedFrame(SyncData.Comparator);
		Recall::Rollback::Utils::LogSyncFrameAdvancement(SyncFrame, FrameIndex);
		
		*Context.LastSyncedFrame = SyncFrame;
		LastSyncedFrameIndex = FrameIndex;
	}
	
	return true; // Continue processing
}

bool FRecallRollbackFrameManager::ProcessFrameRollback(const FRecallRollbackFrameContext& Context, 
	uint32 CurrentFrame, uint32 SyncFrame, int32 FrameIndex, int32& LastSyncedFrameIndex, 
	int32 ForceRollbackFrameCount, int32 NumRollbackFrames)
{
	checkf(CurrentFrame > *Context.LastSyncedFrame,
		TEXT("%hs Can only rollback past the LastSyncedFrame"), __FUNCTION__);
	
	Recall::Rollback::Utils::LogFrameRollbackProcessing(SyncFrame);
	
	// Check if we should execute rollback based on force rollback settings
	if (!Recall::Rollback::Utils::ShouldExecuteRollback(false, ForceRollbackFrameCount, NumRollbackFrames))
	{
		return true; // Continue processing
	}

	// Log rollback frame count warning if needed
	const int32 RollbackFrameLimit = Context.Config->GetRollbackFrameCount();
	Recall::Rollback::Utils::LogRollbackFrameCountWarning(NumRollbackFrames, RollbackFrameLimit);

	// Execute rollback
	ExecuteRollback(Context, CurrentFrame, FrameIndex, LastSyncedFrameIndex);
	return false; // Stop processing after rollback
}

void FRecallRollbackFrameManager::ExecuteRollback(const FRecallRollbackFrameContext& Context, 
	uint32 CurrentFrame, int32 SyncFrameIndex, int32& LastSyncedFrameIndex)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("FRecallRollbackFrameManager::ExecuteRollback"));

	const FRecallRollbackFrame& SyncData = FrameBuffer[SyncFrameIndex];
	const uint32 SyncFrame = SyncData.Comparator.Frame;

	// Determine which frame to rollback to (use last valid snapshot if current frame lacks one)
	const int32 RollbackFrameIndex = SyncData.bValidSnapshot ? SyncFrameIndex : LastSyncedFrameIndex;
	const FRecallRollbackFrame& RollbackData = FrameBuffer[RollbackFrameIndex];
	const TSharedPtr<FRecallMultiSimSnapshot> Snapshot = RollbackData.Snapshot;
	
	if (!Snapshot.IsValid())
	{
		UE_LOG(LogRecallRollback, Error, 
			TEXT("%hs Unable to find snapshot for rollback to frame %d"), __FUNCTION__, SyncFrame);
		return;
	}

	checkf(Recall::Rollback::Utils::IsValidFrameForRollback(Snapshot->Frame, *Context.LastSyncedFrame), 
		TEXT("%hs Can not rollback behind last synced frame"), __FUNCTION__);
	checkf(CurrentFrame > Snapshot->Frame, TEXT("%hs Should be more recent than rollback frame"), __FUNCTION__);

	// Get current frame from the snapshot system instead
	const int32 NumStepFrames = static_cast<int32>(CurrentFrame - Snapshot->Frame);
	
	// Log rollback execution
	Recall::Rollback::Utils::LogRollbackExecution(
		Context.World, SyncFrame, CurrentFrame, *Snapshot,
		Context.Config->GetConfirmFrame(), *Context.LastSyncedFrame, NumStepFrames);

	// Pop frames past the rollback frame (CRITICAL: must happen before rollback execution)
	const int32 OutOfSyncFrameCount = FrameBuffer.Num() - RollbackFrameIndex;
	Recall::Rollback::Utils::LogFrameBufferCleanup(OutOfSyncFrameCount, FrameBuffer.Num(), 
		SyncData.bValidSnapshot ? SyncFrameIndex : LastSyncedFrameIndex, SyncFrameIndex, LastSyncedFrameIndex);
	FrameBuffer.Pop(OutOfSyncFrameCount);

	// Execute rollback
	if (Context.MultiSimSnapshotSystem.IsValid())
	{
		Context.MultiSimSnapshotSystem->LoadQuickSnapshot(*Snapshot, Recall::SnapshotReason::Rollback);
	}
	if (Context.MultiSimSystem.IsValid())
	{
		Context.MultiSimSystem->Rollback_Step(NumStepFrames);
	}
}
