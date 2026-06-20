// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

#include "RecallActorTypes.generated.h"

RECALLSIMULATION_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallActor, Log, All);

#define RECALL_ACTOR_INVALID_SERIAL_NUMBER 0

/*
* Handle used to reference our actor in the registry.
*/
USTRUCT()
struct RECALLSIMULATION_API FRecallActorHandle
{
	GENERATED_BODY()

	FRecallActorHandle() = default;

	/* Our actor class name to. */
	UPROPERTY(VisibleAnywhere)
	FName ClassName = NAME_None;

	/* Our actor index inside the pool. */
	UPROPERTY(VisibleAnywhere)
	int32 Index{ INDEX_NONE };

	/* Serial number of this actor entry inside the registry. */
	UPROPERTY(VisibleAnywhere)
	uint32 SerialNumber{ RECALL_ACTOR_INVALID_SERIAL_NUMBER };

	bool operator==(const FRecallActorHandle Other) const
	{
		return ClassName == Other.ClassName && Index == Other.Index && SerialNumber == Other.SerialNumber;
	}

	bool operator!=(const FRecallActorHandle Other) const
	{
		return !operator==(Other);
	}

	static FRecallActorHandle Make(const FName& InClassName, const int32 InIndex, const int32 InGeneration) { return FRecallActorHandle(InClassName, InIndex, InGeneration); }
	static FRecallActorHandle Invalid() { return FRecallActorHandle(); }

	FORCEINLINE void Invalidate()
	{
		ClassName = NAME_None;
		Index = INDEX_NONE;
		SerialNumber = RECALL_ACTOR_INVALID_SERIAL_NUMBER;
	}

	FORCEINLINE bool IsSet() const
	{
		return !ClassName.IsNone() && Index != INDEX_NONE && SerialNumber != RECALL_ACTOR_INVALID_SERIAL_NUMBER;
	}

	FORCEINLINE bool IsValid() const
	{
		return IsSet();
	}

	friend uint32 GetTypeHash(const FRecallActorHandle& Handle)
	{
		return HashCombine(Handle.Index, Handle.SerialNumber);
	}

private:
	FRecallActorHandle(const FName& InClassName, int32 InIndex, int32 InSerialNumber) : ClassName(InClassName), Index(InIndex), SerialNumber(InSerialNumber) {}

};
