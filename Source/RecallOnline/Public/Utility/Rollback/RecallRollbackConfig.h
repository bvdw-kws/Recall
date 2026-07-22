// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

class URecallSynchronizationContainerSubsystem;
class UWorld;

/**
 * Configuration helper for rollback system settings and state management
 */
struct RECALLONLINE_API FRecallRollbackConfig
{
private:
	TWeakObjectPtr<URecallSynchronizationContainerSubsystem> ContainerSystem;

public:
	/** Sets the container system (should only be called during initialization) */
	void SetContainerSystem(const TWeakObjectPtr<URecallSynchronizationContainerSubsystem>& InContainerSystem);
	
	/** Gets the configured rollback frame count */
	int32 GetRollbackFrameCount() const;
	
	/** Sets the rollback frame count */
	void SetRollbackFrameCount(int32 FrameCount);
	
	/** Gets the force rollback frame count from debug settings */
	int32 GetForceRollbackFrameCount() const;
	
	/** Gets the current confirm frame */
	uint32 GetConfirmFrame() const;
	
	/** Sets the confirm frame */
	void SetConfirmFrame(uint32 Frame);
	
	/** Sets the rollback state */
	void SetRollback(bool bInRollback);
	
	/** Sets the net pause state using frame-based logic */
	void SetNetPause(uint32 CurrentFrame, uint32 LastSyncedFrame);

	/** Determines if the network should be paused based on how far CurrentFrame has run ahead of
	 * ConfirmFrame - kept ConfirmFrame-based (not LastSyncedFrame-based) so the check stays
	 * recoverable: ConfirmFrame is updated externally by network delivery independent of the tick
	 * loop, whereas LastSyncedFrame only advances from inside the very pipeline this pause halts,
	 * so gating on it directly would freeze both terms forever once paused. Tightened by a capped
	 * margin when LastSyncedFrame is observed lagging ConfirmFrame, without ever fully cancelling
	 * ConfirmFrame's contribution. */
	bool ShouldNetPause(uint32 CurrentFrame, uint32 LastSyncedFrame) const;
};