// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "RecallRollbackTypes.h"

class URecallMultiSimSubsystem;
class URecallMultiSimSnapshotSubsystem;
struct FRecallDebugRollbackManager;
struct FRecallRollbackConfig;

/**
 * Context structure containing dependencies needed by the frame manager
 */
struct RECALLONLINE_API FRecallRollbackFrameContext
{
	TWeakObjectPtr<URecallMultiSimSubsystem> MultiSimSystem;
	TWeakObjectPtr<URecallMultiSimSnapshotSubsystem> MultiSimSnapshotSystem;
	const UWorld* World = nullptr;
	uint32 ConfirmFrame = 0;
	uint32 RollbackFrameCount = 0;
	uint32 ForceRollbackFrameCount = 0;
	uint32* LastSyncedFrame = nullptr;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	const FRecallDebugRollbackManager* DebugManager = nullptr;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
};

/**
 * Manages frame processing and rollback execution for the rollback subsystem.
 * Handles frame buffer management, frame validation, sync/rollback decisions,
 * and rollback execution while keeping the subsystem focused on high-level coordination.
 */
class RECALLONLINE_API FRecallRollbackFrameManager
{
public:
	FRecallRollbackFrameManager();
	~FRecallRollbackFrameManager() = default;

	// Non-copyable
	FRecallRollbackFrameManager(const FRecallRollbackFrameManager&) = delete;
	FRecallRollbackFrameManager& operator=(const FRecallRollbackFrameManager&) = delete;

	/**
	 * Frame buffer management
	 */
	
	/** Adds a new frame to the buffer */
	void AddFrame(const TFunction<void(FRecallRollbackFrame&)>& Delegate);
	
	/** Gets the current frame buffer size */
	int32 GetFrameBufferSize() const { return FrameBuffer.Num(); }
	
	/** Gets direct access to the frame buffer for legacy operations */
	FRecallRollbackFrameBuffer& GetFrameBuffer() { return FrameBuffer; }
	const FRecallRollbackFrameBuffer& GetFrameBuffer() const { return FrameBuffer; }
	
	/** Cleans up processed frames from the buffer */
	void CleanupFrameBuffer(int32 LastSyncedFrameIndex);

	/**
	 * Frame processing
	 */
	
	/** Processes the frame buffer for a given frame, returns true if rollback was executed */
	bool ProcessFrameBuffer(const FRecallRollbackFrameContext& Context, uint32 Frame);
	
	/** Processes frames in the buffer up to the current frame */
	int32 ProcessFrameBufferLoop(const FRecallRollbackFrameContext& Context, uint32 CurrentFrame, uint32 ConfirmFrame, bool& bRollbackExecuted);
	
	/** Processes a single frame from the buffer */
	bool ProcessSingleFrame(const FRecallRollbackFrameContext& Context, const FRecallRollbackFrame& SyncData, 
		uint32 CurrentFrame, uint32 ConfirmFrame, int32 FrameIndex, int32& LastSyncedFrameIndex, bool& bRollbackExecuted);

private:
	/** Frame buffer containing sync data for rollback processing */
	FRecallRollbackFrameBuffer FrameBuffer;
	
	/** Constants */
	static constexpr int32 MaxFrameBufferSizeWarning = 100;

	/**
	 * Frame processing implementation
	 */
	
	/** Processes a frame that is synchronized */
	bool ProcessFrameSync(const FRecallRollbackFrameContext& Context, uint32 CurrentFrame, uint32 ConfirmFrame,
		const FRecallRollbackFrame& SyncData, uint32 SyncFrame, int32 FrameIndex, int32& LastSyncedFrameIndex);
	
	/** Processes a frame that requires rollback */
	bool ProcessFrameRollback(const FRecallRollbackFrameContext& Context, uint32 CurrentFrame, uint32 SyncFrame, 
		int32 FrameIndex, int32& LastSyncedFrameIndex, int32 ForceRollbackFrameCount, int32 NumRollbackFrames);
	
	/** Executes rollback to a specific frame */
	void ExecuteRollback(const FRecallRollbackFrameContext& Context, uint32 CurrentFrame,
		int32 SyncFrameIndex, int32& LastSyncedFrameIndex);
};