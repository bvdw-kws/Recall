// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Rollback/RecallRollbackUtils.h"

#include "Engine/World.h"
#include "System/Input/RecallInputQueueSubsystem.h"
#include "System/Snapshot/RecallMultiSimSnapshotTypes.h"
#include "System/Synchronization/RecallRollbackSubsystem.h"
#include "System/Synchronization/RecallSynchronizationTypes.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"
#include "Utility/Player/RecallPlayerUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
static int32 DebugForceRollbackFrames = 0;
static FAutoConsoleVariableRef CVarRecallSetRollbackFrames(
	TEXT("recall.rollback.SetRollbackFrames"),
	DebugForceRollbackFrames,
	TEXT("Force rollback every frame")
);

static bool bDebugPauseRollback = false;
static FAutoConsoleVariableRef CVarRecallPauseRollback(
	TEXT("recall.rollback.Pause"),
	bDebugPauseRollback,
	TEXT("Pause Rollback")
);

static bool bDebugRandomRollback = false;
static FAutoConsoleVariableRef CVarRecallRandomRollback(
	TEXT("recall.rollback.RandomRollback"),
	bDebugRandomRollback,
	TEXT("Random Rollback")
);

static  float DebugRandomRollbackTimerMin = 1.0f;
static FAutoConsoleVariableRef CVarRecallRandomRollbackTimerMin(
	TEXT("recall.rollback.RandomRollbackTimerMin"),
	DebugRandomRollbackTimerMin,
	TEXT("Random Rollback Timer Min")
);

static  float DebugRandomRollbackTimerMax = 5.0f;
static FAutoConsoleVariableRef CVarRecallRandomRollbackTimerMax(
	TEXT("recall.rollback.RandomRollbackTimerMax"),
	DebugRandomRollbackTimerMax,
	TEXT("Random Rollback Timer Max")
);

static bool bDebugRollbackComparator = false;
static FAutoConsoleVariableRef CVarRecallDebugRollbackComparator(
	TEXT("recall.rollback.RollbackComparator"),
	bDebugRollbackComparator,
	TEXT("Rollback Comparator")
);

static bool bDebugDumpSyncedFrame = false;
static FAutoConsoleVariableRef CVarRecallDumpSyncedFrame(
	TEXT("recall.rollback.DumpSyncedFrame"),
	bDebugDumpSyncedFrame,
	TEXT("Dump Synced Frame")
);

static bool bDebugValidateLastSyncedFrame = false;
static FAutoConsoleVariableRef CVarRecallValidateLastSyncedFrame(
	TEXT("recall.rollback.ValidateLastSyncedFrame"),
	bDebugValidateLastSyncedFrame,
	TEXT("Validate Last Synced Frame")
);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

namespace Recall::Rollback::Utils
{
	bool IsValidFrameForRollback(uint32 Frame, uint32 LastSyncedFrame)
	{
		return Frame >= LastSyncedFrame;
	}

	bool ShouldProcessFrame(uint32 SyncFrame, uint32 CurrentFrame, uint32 LastSyncedFrame)
	{
		if (SyncFrame >= CurrentFrame)
		{
			UE_LOG(LogRecallRollback, Warning,
				TEXT("SyncFrame (%d) should be less than CurrentFrame (%d)"), SyncFrame, CurrentFrame);
			return false;
		}

		if (SyncFrame < LastSyncedFrame)
		{
			UE_LOG(LogRecallRollback, Warning,
				TEXT("SyncFrame (%d) should not be behind LastSyncedFrame (%d)"), SyncFrame, LastSyncedFrame);
			return false;
		}

		return true;
	}

	bool CanSyncPastConfirmFrame(uint32 SyncFrame, uint32 ConfirmFrame, int32 ForceRollbackFrameCount)
	{
		return SyncFrame <= ConfirmFrame || ForceRollbackFrameCount != 0;
	}

	bool ShouldForceRollbackForLag(uint32 CurrentFrame, uint32 LastSyncedFrame, int32 RollbackFrameCount)
	{
		if (RollbackFrameCount <= 0 || CurrentFrame < LastSyncedFrame)
		{
			return false;
		}

		const int32 SyncLag = static_cast<int32>(CurrentFrame - LastSyncedFrame);
		return SyncLag >= RollbackFrameCount;
	}

	EFrameProcessingAction ValidateFrameForProcessing(
		const FRecallRollbackFrame& SyncData,
		uint32 CurrentFrame,
		uint32 LastSyncedFrame,
		uint32 ConfirmFrame,
		int32 ForceRollbackFrameCount,
		int32 RollbackFrameCount,
		TFunction<FRecallSynchronizationFrameComparator(uint32)> CreateFrameComparator)
	{
		const uint32 SyncFrame = SyncData.Comparator.Frame;

		// Validate frame ordering and range
		if (!ShouldProcessFrame(SyncFrame, CurrentFrame, LastSyncedFrame))
		{
			return (SyncFrame >= CurrentFrame) ? EFrameProcessingAction::Break : EFrameProcessingAction::Continue;
		}

		// Skip frames already processed
		if (SyncFrame == LastSyncedFrame)
		{
			const FRecallSynchronizationFrameComparator LastSyncedFrameComparator = CreateFrameComparator(SyncFrame);
			ValidateLastSyncedFrame(SyncData, LastSyncedFrameComparator);
			return EFrameProcessingAction::Continue;
		}

		// Skip unconfirmed frames unless forced - but don't let an unconfirmed ConfirmFrame block us
		// from processing (and potentially forcing a resync on) a frame once we're already too far
		// behind LastSyncedFrame to safely wait for confirmation.
		if (!CanSyncPastConfirmFrame(SyncFrame, ConfirmFrame, ForceRollbackFrameCount) &&
			!ShouldForceRollbackForLag(CurrentFrame, LastSyncedFrame, RollbackFrameCount))
		{
			return EFrameProcessingAction::Break;
		}

		return EFrameProcessingAction::Process;
	}

	bool EvaluateFrameSync(
		const FRecallRollbackFrame& SyncData,
		const FRecallSynchronizationFrameComparator& CurrentFrameComparator)
	{
		bool bIsFrameSynced = (SyncData.Comparator == CurrentFrameComparator);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		// Debug logging to verify desync detection is working
		UE_LOG(LogRecallRollback, VeryVerbose, TEXT("%hs: Frame %d - IsFrameSynced: %s"), 
			__FUNCTION__, SyncData.Comparator.Frame, bIsFrameSynced ? TEXT("True") : TEXT("False"));
		
		if (!bIsFrameSynced)
		{
			UE_LOG(LogRecallRollback, Log, TEXT("%hs: DESYNC DETECTED at frame %d! Saved: %s, Current: %s"),
				__FUNCTION__, SyncData.Comparator.Frame, 
				*SyncData.Comparator.ToString(), *CurrentFrameComparator.ToString());
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

		// Note: Advanced debug overrides like random rollback simulation 
		// are handled at the subsystem level due to timer state management
		// This function focuses on frame comparison logic

		return bIsFrameSynced;
	}

	bool CanAdvanceLastSyncedFrame(
		bool bIsFrameSynced,
		uint32 SyncFrame,
		uint32 ConfirmFrame,
		uint32 CurrentFrame,
		uint32 LastSyncedFrame,
		int32 RollbackFrameCount,
		int32 ForceRollbackFrameCount,
		int32 NumRollbackFrames)
	{
		// Only synced frames should advance - desynced frames should trigger rollback
		bool bCanAdvanceSync = bIsFrameSynced;
		// No special case for confirmed frames - if frame is desynced, it should rollback

		// Proactively force a rollback/resync cycle once a rollback triggered right now would
		// already exceed the replay budget, so a real desync never has to replay more than that.
		if (bCanAdvanceSync && ForceRollbackFrameCount == 0 &&
			ShouldForceRollbackForLag(CurrentFrame, LastSyncedFrame, RollbackFrameCount))
		{
			bCanAdvanceSync = false;

			UE_LOG(LogRecallRollback, Log,
				TEXT("%hs: Forcing rollback due to sync lag - SyncFrame=%d, ConfirmFrame=%d, CurrentFrame=%d, LastSyncedFrame=%d, RollbackFrameCount=%d"),
				__FUNCTION__, SyncFrame, ConfirmFrame, CurrentFrame, LastSyncedFrame, RollbackFrameCount);
		}

		const bool bResult = bCanAdvanceSync && (ForceRollbackFrameCount == 0 || NumRollbackFrames > ForceRollbackFrameCount);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		UE_LOG(LogRecallRollback, VeryVerbose, TEXT("%hs: Frame %d - bIsFrameSynced=%s, SyncFrame=%d, ConfirmFrame=%d, bCanAdvanceSync=%s, Result=%s"),
			__FUNCTION__, SyncFrame, bIsFrameSynced ? TEXT("True") : TEXT("False"), SyncFrame, ConfirmFrame,
			bCanAdvanceSync ? TEXT("True") : TEXT("False"), bResult ? TEXT("True") : TEXT("False"));
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

		return bResult;
	}

	void ValidateFrameBuffer(const FRecallRollbackFrameBuffer& FrameBuffer, uint32 CurrentFrame)
	{
		constexpr int32 MaxFrameBufferSizeWarning = 100;
		checkf(FrameBuffer.Num() < MaxFrameBufferSizeWarning,
			TEXT("%hs Running out of sync, buffer size: %d"), __FUNCTION__, FrameBuffer.Num());
		
		// Additional validations could be added here
		for (int32 Index = 1; Index < FrameBuffer.Num(); Index++)
		{
			const uint32 PrevFrame = FrameBuffer[Index - 1].Comparator.Frame;
			const uint32 CurrFrame = FrameBuffer[Index].Comparator.Frame;
			checkf(PrevFrame < CurrFrame,
				TEXT("%hs Frame buffer should be ordered: %d >= %d"), __FUNCTION__, PrevFrame, CurrFrame);
		}
	}

	void LogRollbackExecution(
		const UWorld* World,
		uint32 SyncFrame,
		uint32 CurrentFrame,
		const FRecallMultiSimSnapshot& Snapshot,
		uint32 ConfirmFrame,
		uint32 LastSyncedFrame,
		int32 NumStepFrames)
	{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		const int32 MaxStepCount = Recall::Simulation::Utils::GetMaxStepCount(World);
		ensureAlwaysMsgf(NumStepFrames <= MaxStepCount,
			TEXT("Rollback %d frames exceeds max step count %d"), NumStepFrames, MaxStepCount);
		const FString LocalPlayerId = Recall::Player::Utils::GetLocalPlayerId(World);
		UE_LOG(LogRecallRollback, Log,
			TEXT("Rollback [%s] Frame %d is out of sync, replay %d frames from frame %d until frame %d (ConfirmFrame: %d, LastSyncedFrame: %d)"),
			*LocalPlayerId, SyncFrame, NumStepFrames + 1, Snapshot.Frame, CurrentFrame, ConfirmFrame, LastSyncedFrame);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}

	void LogFrameBufferCleanup(
		int32 OutOfSyncFrameCount,
		int32 BufferSize,
		int32 PopStartIndex,
		int32 FrameIndex,
		int32 LastSyncedFrameIndex)
	{
		UE_LOG(LogRecallRollback, Log,
			TEXT("Pop %d frames (Size: %d, PopStartIndex: %d, FrameIndex: %d, LastSyncedFrameIndex: %d)"),
			OutOfSyncFrameCount, BufferSize, PopStartIndex, FrameIndex, LastSyncedFrameIndex);
	}

	void LogSyncFrameAdvancement(uint32 SyncFrame, int32 LastSyncedFrameIndex)
	{
		UE_LOG(LogRecallRollback, VeryVerbose, TEXT("Frame %d is synced (Index: %d)"), SyncFrame, LastSyncedFrameIndex);
	}

	void LogFrameSyncProcessing(uint32 SyncFrame)
	{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		UE_LOG(LogRecallRollback, Verbose, TEXT("%hs: Frame %d - Processing as SYNCED"), __FUNCTION__, SyncFrame);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}

	void LogFrameRollbackProcessing(uint32 SyncFrame)
	{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		UE_LOG(LogRecallRollback, Verbose, TEXT("%hs: Frame %d - Processing as ROLLBACK"), __FUNCTION__, SyncFrame);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}

	bool ShouldExecuteRollback(
		bool bIsFrameSynced,
		int32 ForceRollbackFrameCount,
		int32 NumRollbackFrames)
	{
		return !bIsFrameSynced || (ForceRollbackFrameCount != 0 && NumRollbackFrames == ForceRollbackFrameCount);
	}

	int32 CalculateRollbackFrames(uint32 CurrentFrame, uint32 SyncFrame)
	{
		check(CurrentFrame > SyncFrame);
		return static_cast<int32>(CurrentFrame - SyncFrame);
	}

	bool DebugIsPauseRollback()
	{
		return bDebugPauseRollback;
	}

	int32 DebugGetForceRollbackFrames()
	{
		return DebugForceRollbackFrames;
	}

	bool DebugUseRollbackComparator()
	{
		return bDebugRollbackComparator;
	}

	bool DebugUseRandomRollback()
	{
		return bDebugRandomRollback;
	}

	float DebugRandomRollbackTimer()
	{
		return FMath::Min(DebugRandomRollbackTimerMin, DebugRandomRollbackTimerMax);
	}

	void LogRollbackFrameCountWarning(int32 NumRollbackFrames, int32 RollbackFrameLimit)
	{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		const int32 MaxFrameCount = RollbackFrameLimit + 1;
		if (NumRollbackFrames > MaxFrameCount)
		{
			UE_LOG(LogRecallRollback, Warning,
				TEXT("Rollback %d frames but limit is %d frames (RollbackFrameLimit: %d)"), NumRollbackFrames, MaxFrameCount, RollbackFrameLimit);
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}

	void ValidateLastSyncedFrame(
		const FRecallRollbackFrame& SyncData,
		const FRecallSynchronizationFrameComparator& LastSyncedFrameComparator)
	{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (bDebugValidateLastSyncedFrame)
		{
			ensureAlwaysMsgf(SyncData.Comparator == LastSyncedFrameComparator,
				TEXT("Synced frames should stay synced"));
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}

	void DebugDumpSyncedFrame(const FRecallSynchronizationFrameComparator& Comparator)
	{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (bDebugDumpSyncedFrame)
		{
			UE_LOG(LogRecallRollback, Log, TEXT("SyncedFrame: %s"), *Comparator.ToString());
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}

	int32 GetForceRollbackFrameCount()
	{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		const int32 ForceRollbackFrames = DebugGetForceRollbackFrames();
		return FMath::Max(0, ForceRollbackFrames);
#else // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		return 0;
#endif
	}

	FRecallSynchronizationFrameComparator CreateFrameComparator(const UWorld* World, uint32 Frame)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("Recall::Rollback::Utils::CreateFrameComparator"));
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Rollback_CreateFrameComparator);

		FRecallSynchronizationFrameComparator FrameComparator;
		FrameComparator.Frame = Frame;

		const TArray<const UWorld*> Worlds = Recall::MultiWorld::Utils::GetMultiWorlds(World);

		for (const UWorld* NestedWorld : Worlds)
		{
			FRecallSynchronizationWorldComparator WorldComparator;

			if (URecallInputQueueSubsystem* InputSystem = UWorld::GetSubsystem<URecallInputQueueSubsystem>(NestedWorld))
			{
				const TArray<FString> PlayerIds = Recall::Player::Utils::FindPlayersInWorldAtFrame(NestedWorld, Frame);
				WorldComparator.PlayerInputs = InputSystem->GetFramePlayerInputs(Frame, PlayerIds, false);
				WorldComparator.PlayerInputs.Sort(
					[](const FRecallPlayerInput& lPlayerInput, const FRecallPlayerInput& rPlayerInput)
					{
						return lPlayerInput.PlayerId < rPlayerInput.PlayerId;
					}
				);
			}

			FrameComparator.WorldComparators.Add(WorldComparator);
		}

		return FrameComparator;
	}
}