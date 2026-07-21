// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Rollback/RecallRollbackConfig.h"

#include "Kismet/KismetStringLibrary.h"
#include "System/Synchronization/RecallSynchronizationContainerSubsystem.h"
#include "Utility/Rollback/RecallRollbackUtils.h"

void FRecallRollbackConfig::SetContainerSystem(const TWeakObjectPtr<URecallSynchronizationContainerSubsystem>& InContainerSystem)
{
	ContainerSystem = InContainerSystem;
}

int32 FRecallRollbackConfig::GetRollbackFrameCount() const
{
	check(ContainerSystem.IsValid());
	return ContainerSystem->RollbackFrameCount;
}

void FRecallRollbackConfig::SetRollbackFrameCount(int32 FrameCount)
{
	check(ContainerSystem.IsValid());
	ContainerSystem->RollbackFrameCount = FrameCount;
}

int32 FRecallRollbackConfig::GetForceRollbackFrameCount() const
{
	return Recall::Rollback::Utils::GetForceRollbackFrameCount();
}

uint32 FRecallRollbackConfig::GetConfirmFrame() const
{
	return ContainerSystem->ConfirmFrame;
}

void FRecallRollbackConfig::SetConfirmFrame(uint32 Frame)
{
	check(ContainerSystem.IsValid());
	ContainerSystem->ConfirmFrame = Frame;
}

void FRecallRollbackConfig::SetRollback(bool bInRollback)
{
	check(ContainerSystem.IsValid());
	ContainerSystem->Rollback = bInRollback;
}

void FRecallRollbackConfig::SetNetPause(uint32 CurrentFrame, uint32 LastSyncedFrame)
{
	check(ContainerSystem.IsValid());
	ContainerSystem->NetPause = ShouldNetPause(CurrentFrame, LastSyncedFrame);
	UE_LOG(LogRecallRollback, VeryVerbose, TEXT("NetPause %s (CurrentFrame: %d, LastSyncedFrame: %d, ConfirmFrame: %d)"), 
		*UKismetStringLibrary::Conv_BoolToString(ContainerSystem->NetPause), CurrentFrame, LastSyncedFrame, GetConfirmFrame());
}

bool FRecallRollbackConfig::ShouldNetPause(uint32 CurrentFrame, uint32 LastSyncedFrame) const
{
	const int32 RollbackFrameCount = GetRollbackFrameCount();
	const uint32 ConfirmFrame = GetConfirmFrame();
	if (RollbackFrameCount > 0 && CurrentFrame > ConfirmFrame)
	{
		const int32 ConfirmFrameDelta = static_cast<int32>(CurrentFrame - ConfirmFrame) + 1;

		// A rollback's fallback target is always LastSyncedFrame's snapshot, so the real budget we
		// must not exceed is CurrentFrame - LastSyncedFrame <= RollbackFrameCount + 1 (MaxStepCount).
		// When LastSyncedFrame is still behind ConfirmFrame, that gap eats into the margin, so pause
		// one frame earlier (relative to ConfirmFrame) to leave it room to catch up; once
		// LastSyncedFrame == ConfirmFrame there's no such gap and the full +1 margin is safe again.
		const int32 RollbackFrameMargin = LastSyncedFrame < ConfirmFrame ? 0 : 1;
		return ConfirmFrameDelta > RollbackFrameCount + RollbackFrameMargin;
	}
	else
	{
		return false;
	}
}
