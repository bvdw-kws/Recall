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
	FORCEINLINE bool IsRollback() const { return Rollback; }
	FORCEINLINE bool IsNetPause() const { return NetPause; }
	FORCEINLINE uint32 GetConfirmFrame() const { return ConfirmFrame; }
	FORCEINLINE int32 GetRollbackFrameCount() const { return RollbackFrameCount; }

protected:
	friend struct FRecallRollbackConfig;

	/* True when a rollback is being executed. */
	bool Rollback = false;

	/* True when simulation is currently net pause */
	bool NetPause = false;

	/* Last shared confirm frame for all the players */
	uint32 ConfirmFrame = 0;

	/* How many frames can be rollback at most */
	int32 RollbackFrameCount = 0;

};
