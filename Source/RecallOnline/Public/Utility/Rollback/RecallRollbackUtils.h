// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "System/Synchronization/RecallRollbackTypes.h"

class URecallRollbackSubsystem;
class URecallSynchronizationContainerSubsystem;
class FRecallRollbackFrame;
struct FRecallSynchronizationFrameComparator;
struct FRecallMultiSimSnapshot;

namespace Recall::Rollback::Utils
{
	/**
	 * Frame processing decision
	 */
	enum class EFrameProcessingAction : uint8
	{
		Continue,		// Continue to next frame
		Break,			// Stop processing frames
		Process			// Process this frame
	};

	/**
	 * Frame validation utilities
	 */
	
	/** Checks if a frame is valid for rollback processing */
	RECALLONLINE_API bool IsValidFrameForRollback(uint32 Frame, uint32 LastSyncedFrame);
	
	/** Determines if a sync frame should be processed based on current and last synced frames */
	RECALLONLINE_API bool ShouldProcessFrame(uint32 SyncFrame, uint32 CurrentFrame, uint32 LastSyncedFrame);
	
	/** Checks if sync can proceed past the confirm frame threshold */
	RECALLONLINE_API bool CanSyncPastConfirmFrame(uint32 SyncFrame, uint32 ConfirmFrame, int32 ForceRollbackFrameCount);

	/** Checks if LastSyncedFrame has fallen dangerously close to the max safe rollback replay distance */
	RECALLONLINE_API bool ShouldForceRollbackForLag(uint32 SyncFrame, uint32 CurrentFrame, uint32 LastSyncedFrame, uint32 ConfirmFrame, int32 RollbackFrameCount);

	/** Validates frame for processing and returns appropriate action */
	RECALLONLINE_API EFrameProcessingAction ValidateFrameForProcessing(
		const FRecallRollbackFrame& SyncData,
		uint32 CurrentFrame,
		uint32 LastSyncedFrame,
		uint32 ConfirmFrame,
		int32 ForceRollbackFrameCount,
		int32 RollbackFrameCount,
		TFunction<FRecallSynchronizationFrameComparator(uint32)> CreateFrameComparator
	);

	/**
	 * Sync state evaluation
	 */
	
	/** Evaluates if frame is synchronized with optional debug overrides */
	RECALLONLINE_API bool EvaluateFrameSync(
		const FRecallRollbackFrame& SyncData,
		const FRecallSynchronizationFrameComparator& CurrentFrameComparator
	);

	/** Determines if LastSyncedFrame can be advanced based on sync state and conditions */
	RECALLONLINE_API bool CanAdvanceLastSyncedFrame(
		bool bIsFrameSynced,
		uint32 SyncFrame,
		uint32 ConfirmFrame,
		uint32 CurrentFrame,
		uint32 LastSyncedFrame,
		int32 RollbackFrameCount,
		int32 ForceRollbackFrameCount,
		int32 NumRollbackFrames
	);

	/**
	 * Buffer management utilities
	 */
	
	/** Validates frame buffer integrity and ordering */
	RECALLONLINE_API void ValidateFrameBuffer(const FRecallRollbackFrameBuffer& FrameBuffer, uint32 CurrentFrame);

	/**
	 * Debug and logging utilities
	 */
	
	/** Validates last synced frame consistency in debug builds */
	RECALLONLINE_API void ValidateLastSyncedFrame(
		const FRecallRollbackFrame& SyncData,
		const FRecallSynchronizationFrameComparator& LastSyncedFrameComparator
	);

	/** Debug dumps synced frame information when debug flag is enabled */
	RECALLONLINE_API void DebugDumpSyncedFrame(const FRecallSynchronizationFrameComparator& Comparator);
	
	/** Logs rollback execution details with World context */
	RECALLONLINE_API void LogRollbackExecution(
		const UWorld* World,
		uint32 SyncFrame,
		uint32 CurrentFrame,
		const FRecallMultiSimSnapshot& Snapshot,
		uint32 ConfirmFrame,
		uint32 LastSyncedFrame,
		int32 NumStepFrames
	);

	/** Logs frame buffer cleanup operations */
	RECALLONLINE_API void LogFrameBufferCleanup(
		int32 OutOfSyncFrameCount,
		int32 BufferSize,
		int32 PopStartIndex,
		int32 FrameIndex,
		int32 LastSyncedFrameIndex
	);

	/** Logs sync frame advancement */
	RECALLONLINE_API void LogSyncFrameAdvancement(uint32 SyncFrame, int32 LastSyncedFrameIndex);

	/** Logs frame sync processing status */
	RECALLONLINE_API void LogFrameSyncProcessing(uint32 SyncFrame);

	/** Logs frame rollback processing status */
	RECALLONLINE_API void LogFrameRollbackProcessing(uint32 SyncFrame);

	/** Logs warning when rollback frame count exceeds limits */
	RECALLONLINE_API void LogRollbackFrameCountWarning(int32 NumRollbackFrames, int32 RollbackFrameLimit);

	/**
	 * Rollback execution helpers
	 */
	
	/** Determines if rollback should be executed based on sync state and force settings */
	RECALLONLINE_API bool ShouldExecuteRollback(
		bool bIsFrameSynced,
		int32 ForceRollbackFrameCount,
		int32 NumRollbackFrames
	);

	/** Calculates number of rollback frames between current and sync frames */
	RECALLONLINE_API int32 CalculateRollbackFrames(uint32 CurrentFrame, uint32 SyncFrame);

	/** Debug console variable accessors */
	RECALLONLINE_API extern bool DebugIsPauseRollback();
	RECALLONLINE_API extern int32 DebugGetForceRollbackFrames();
	RECALLONLINE_API extern bool DebugUseRollbackComparator();
	RECALLONLINE_API extern bool DebugUseRandomRollback();
	RECALLONLINE_API extern float DebugRandomRollbackTimer();

	/**
	 * Gets the force rollback frame count with proper bounds checking
	 */
	RECALLONLINE_API int32 GetForceRollbackFrameCount();

	/**
	 * Frame comparator creation utilities
	 */
	
	/** Creates frame comparator for synchronization validation */
	RECALLONLINE_API FRecallSynchronizationFrameComparator CreateFrameComparator(const UWorld* World, uint32 Frame);
}