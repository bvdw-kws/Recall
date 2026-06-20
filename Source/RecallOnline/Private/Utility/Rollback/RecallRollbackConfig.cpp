// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Rollback/RecallRollbackConfig.h"

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

void FRecallRollbackConfig::SetNetPause(const UWorld* World, uint32 CurrentFrame)
{
	check(ContainerSystem.IsValid());
	ContainerSystem->NetPause = ShouldNetPause(World, CurrentFrame);
}

bool FRecallRollbackConfig::ShouldNetPause(const UWorld* World, uint32 CurrentFrame) const
{
	if (CurrentFrame >= ContainerSystem->ConfirmFrame)
	{
		const int32 ConfirmFrameDelta = static_cast<int32>(CurrentFrame - ContainerSystem->ConfirmFrame);
		const int32 RollbackFrameCount = GetRollbackFrameCount();
		return ConfirmFrameDelta > RollbackFrameCount + 1;
	}
	else
	{
		return false;
	}
}
