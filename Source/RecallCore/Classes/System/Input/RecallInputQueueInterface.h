// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "System/Input/RecallInputQueueTypes.h"

#include "RecallInputQueueInterface.generated.h"

/**
* Interface to queue inputs inside the simulation
*/
UINTERFACE()
class RECALLCORE_API URecallInputQueueInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallInputQueueInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual bool GetFrameInput(uint32 Frame, const FString& PlayerId, FRecallInput& OutInput, bool bAllowNone = true) const = 0;
	virtual TArray<FRecallPlayerInput> GetFramePlayerInputs(uint32 Frame) const = 0;

	virtual void SetInputQueue(const FRecallInputQueue& InInputQueue) = 0;
	virtual const FRecallInputQueue& GetInputQueue() const = 0;

	virtual void ClearInputQueuePastFrame(uint32 Frame) = 0;

	virtual bool HasFrameInput(uint32 Frame, const FString& PlayerId) const = 0;

	virtual void PushInput(uint32 Frame, const FString& PlayerId, const FRecallInput& Input) = 0;
	virtual void PushFrameInput(const FString& PlayerId, const FRecallFrameInput& FrameInput) = 0;

};
