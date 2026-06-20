// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "System/Player/RecallPlayerQueueTypes.h"

#include "RecallPlayerQueueInterface.generated.h"

UINTERFACE()
class RECALLCORE_API URecallPlayerQueueInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallPlayerQueueInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual void RequestAddPlayer(uint32 Frame, const FString& PlayerId, const FRecallPlayerSpawnParameters& SpawnParams) = 0;
	virtual void RequestRemovePlayer(uint32 Frame, const FString& PlayerId) = 0;

	virtual const FRecallPlayerQueue& GetPlayerQueue() const = 0;
	virtual void SetPlayerQueue(const FRecallPlayerQueue& InQueue) = 0;

	virtual void ClearPlayerQueuePastFrame(uint32 Frame) = 0;

};
