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

		// IMPORTANT: this must stay gated on ConfirmFrame, not on LastSyncedFrame directly, even
		// though CurrentFrame - LastSyncedFrame is the quantity that actually determines rollback
		// safety (see ShouldForceRollbackForLag). LastSyncedFrame is only ever advanced from inside
		// the tick/step pipeline (URecallRollbackSubsystem::OnFrameSync -> SyncFrame ->
		// ProcessFrameBuffer), which itself only runs while NetPause is false (see
		// URecallMultiSimSubsystem::Tick gating StepCount on !IsNetPause). If we paused based on
		// CurrentFrame - LastSyncedFrame, both terms would freeze the instant NetPause engages and
		// the check would return the exact same "stay paused" result forever - a permanent deadlock,
		// since nothing would ever be able to advance LastSyncedFrame again to clear it.
		// ConfirmFrame, by contrast, is updated externally by incoming network confirmations
		// (UpdateConfirmFrameState) independent of the tick loop, so CurrentFrame - ConfirmFrame
		// keeps shrinking while paused and NetPause can clear once the network catches up; the next
		// real step then lets ProcessFrameBufferLoop flush LastSyncedFrame forward across the whole
		// confirmed backlog in one shot.
		//
		// Since LastSyncedFrame can lag ConfirmFrame by a few frames even in the steady state (frame
		// buffer promotion happens one step at a time), pause a bit earlier when that observed gap is
		// non-trivial - capped, so this margin can never fully substitute for ConfirmFrame's progress
		// and reintroduce the deadlock via cancellation.
		constexpr int32 MaxSyncLagMargin = 2;
		const int32 ObservedSyncLag = LastSyncedFrame < ConfirmFrame
			? FMath::Min(MaxSyncLagMargin, static_cast<int32>(ConfirmFrame - LastSyncedFrame))
			: 0;
		return ConfirmFrameDelta > RollbackFrameCount - ObservedSyncLag;
	}
	else
	{
		return false;
	}
}
