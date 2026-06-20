// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "StateTreeInstanceData.h"

#include "RecallStateTreeInstanceTypes.generated.h"

/**
 * A handle pointing to a StateTree instance data in URecallStateTreeSubsystem.
 */
USTRUCT()
struct RECALLSIMULATION_API FRecallStateTreeInstanceHandle
{
	GENERATED_BODY()

	FRecallStateTreeInstanceHandle() = default;

	/** Initializes new handle based on an index */
	static FRecallStateTreeInstanceHandle Make(int32 InIndex, int32 InGeneration) { return FRecallStateTreeInstanceHandle(InIndex, InGeneration); }

	/** @returns index the handle points to */
	int32 GetIndex() const { return Index; }

	/** @returns generation of the handle, used to identify recycled indices. */
	int32 GetGeneration() const { return Generation; }

	/** @returns true if the handle is valid. */
	bool IsValid() const { return Index != INDEX_NONE; }

	/** Invalidate the handle */
	void Invalidate() { Index = INDEX_NONE; Generation = 0; }

	FString DebugGetDescription() const
	{
		return FString::Printf(TEXT("i: %d g: %d"), Index, Generation);
	}

protected:
	FRecallStateTreeInstanceHandle(int32 InIndex, int32 InGeneration) : Index(InIndex), Generation(InGeneration) {}

	UPROPERTY(VisibleAnywhere)
	int32 Index = INDEX_NONE;

	UPROPERTY(VisibleAnywhere)
	int32 Generation = 0;

public:
	bool operator==(const FRecallStateTreeInstanceHandle Other) const
	{
		return Index == Other.Index && Generation == Other.Generation;
	}

	bool operator!=(const FRecallStateTreeInstanceHandle Other) const
	{
		return !operator==(Other);
	}

	friend uint32 GetTypeHash(const FRecallStateTreeInstanceHandle Entity)
	{
		return HashCombine(Entity.Index, Entity.Generation);
	}
};

#define RECALL_STATE_TREE_INSTANCE_MAX 8192

/*
* State tree instance data that can be reused.
*/
struct FRecallStateTreeInstanceDataItem
{
	FStateTreeInstanceData InstanceData;
	int32 Generation = 0;
};
