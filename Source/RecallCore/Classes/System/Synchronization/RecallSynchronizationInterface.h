// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RecallSynchronizationTypes.h"

#include "RecallSynchronizationInterface.generated.h"

UINTERFACE()
class RECALLCORE_API URecallSynchronizationInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallSynchronizationInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual bool IsRollbackEnabled() const = 0;
	virtual uint32 GetLastSyncedFrame() const = 0;
	virtual int32 GetRollbackFrameCount() const = 0;
	virtual bool ShouldNetPause() const = 0;

	virtual void ForceLastSyncedFrame(uint32 Frame) = 0;
	virtual void ResetSyncedFrame(uint32 Frame) = 0;

	virtual void SetPauseRollback(bool bPause) = 0;
	virtual bool IsDuringRollback() const = 0;

	virtual void SetConfirmFrame(uint32 ConfirmFrame) = 0;
	virtual uint32 GetConfirmFrame() const = 0;

	virtual bool SaveLastSyncedSnapshot(TArray<uint8>& OutSnapshotMemory) const = 0;
	virtual void LoadLastSyncedSnapshot(const TArray<uint8>& SnapshotMemory) const = 0;

};
