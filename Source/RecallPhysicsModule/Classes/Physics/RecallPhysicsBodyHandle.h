// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

#include "RecallPhysicsBodyHandle.generated.h"

#define INVALID_PHYSICS_BODY_SERIAL_NUMBER 0

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsBodyHandle
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	uint32 SerialNumber{ INVALID_PHYSICS_BODY_SERIAL_NUMBER };

	FORCEINLINE bool IsSet() const
	{
		return SerialNumber != INVALID_PHYSICS_BODY_SERIAL_NUMBER;
	}

	FORCEINLINE bool IsValid() const
	{
		return IsSet();
	}

	friend uint32 GetTypeHash(const FRecallPhysicsBodyHandle& Handle)
	{
		return Handle.SerialNumber;
	}

	static FRecallPhysicsBodyHandle Invalid() { return FRecallPhysicsBodyHandle(); }

	bool operator!=(const FRecallPhysicsBodyHandle& Other) const { return !(*this == Other); }
	bool operator==(const FRecallPhysicsBodyHandle& Other) const
	{
		return SerialNumber == Other.SerialNumber;
	}

	FString DebugGetDescription() const
	{
		return FString::Printf(TEXT("Body<%d>"), SerialNumber);
	}
};
