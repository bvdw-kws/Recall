// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "RecallSynchronizationContainerSubsystem.generated.h"

UCLASS()
class RECALLCORE_API URecallSynchronizationContainerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FORCEINLINE bool IsRollback() const { return Rollback.Load(); }
	FORCEINLINE bool IsNetPause() const { return NetPause.Load(); }
	FORCEINLINE uint32 GetConfirmFrame() const { return ConfirmFrame.Load(); }
	FORCEINLINE int32 GetRollbackFrameCount() const { return RollbackFrameCount.Load(); }

protected:
	friend struct FRecallRollbackConfig;

	// These are written from the game thread (RPC/input processing) and read from the multi-sim step
	// thread (rollback/frame-sync processing), so they need to be atomic rather than plain fields.

	/* True when a rollback is being executed. */
	TAtomic<bool> Rollback = false;

	/* True when simulation is currently net pause */
	TAtomic<bool> NetPause = false;

	/* Last shared confirm frame for all the players */
	TAtomic<uint32> ConfirmFrame = 0;

	/* How many frames can be rollback at most */
	TAtomic<int32> RollbackFrameCount = 0;

};
