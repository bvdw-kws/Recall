// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

struct FInstancedStruct;
struct FRecallActorHandle;
class URecallObjectPoolContainer;

class RECALLSIMULATION_API IRecallObjectPoolInterface
{
public:
	virtual ~IRecallObjectPoolInterface() = default;

public:
	virtual void SetPoolContainer(const TWeakObjectPtr<URecallObjectPoolContainer>& InContainer) = 0;
	virtual URecallObjectPoolContainer* GetPoolContainer() const = 0;
	virtual void FlushCommands() = 0;

	virtual FRecallActorHandle RequestObject(const FInstancedStruct& Desc, const FRecallActorHandle* RestoreHandle = nullptr) = 0;
	
	virtual void ReleaseObject(const FRecallActorHandle& Handle) = 0;
	virtual void ReleaseObjectAtIndex(int32 Index) = 0;
	virtual void ReleaseAllObjects() = 0;

	virtual TWeakObjectPtr<AActor> GetObject(const FRecallActorHandle& Handle) const = 0;
	
	virtual TArray<int32> GetActiveIndices() const = 0;
	virtual FRecallActorHandle GetOwnerHandleAtIndex(int32 Index) const = 0;
};
