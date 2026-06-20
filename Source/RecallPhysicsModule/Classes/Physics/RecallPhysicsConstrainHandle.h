// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "RecallPhysicsBodyHandle.h"

#include "RecallPhysicsConstrainHandle.generated.h"

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsConstrainHandle
{
	GENERATED_BODY()

	FRecallPhysicsConstrainHandle() = default;
	FRecallPhysicsConstrainHandle(const FRecallPhysicsBodyHandle& InBody1, const FRecallPhysicsBodyHandle& InBody2)
		: Body1(InBody1), Body2(InBody2) {}
	
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsBodyHandle Body1;

	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsBodyHandle Body2;
	
	bool operator!=(const FRecallPhysicsConstrainHandle& Other) const { return !(*this == Other); }
	bool operator==(const FRecallPhysicsConstrainHandle& Other) const
	{
		return Body1 == Other.Body1
			&& Body2 == Other.Body2;
	}

	friend uint32 GetTypeHash(const FRecallPhysicsConstrainHandle& Handle)
	{
		return HashCombine(GetTypeHash(Handle.Body1), GetTypeHash(Handle.Body2));
	}
};
