// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Physics/RecallPhysicsBodyHandle.h"
#include "Physics/RecallPhysicsConstrainHandle.h"
#include "Physics/RecallPhysicsTypes.h"
#include "StructUtils/InstancedStruct.h"

#include "RecallPhysicsSnapshot.generated.h"

#define INVALID_PHYSICS_BODY_ID 0xffffffff

USTRUCT()
struct FRecallPhysicsBodySnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	bool bEnabled{ true };

	UPROPERTY(VisibleAnywhere)
	FMassEntityHandle Entity;

	UPROPERTY(VisibleAnywhere)
	FInstancedStruct Shape;

	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsBodyParameters Params;

	UPROPERTY(VisibleAnywhere)
	bool bUseLayerOverride{ false };

	UPROPERTY(VisibleAnywhere)
	uint16 LayerOverride{ 0 };

	UPROPERTY(VisibleAnywhere)
	bool bUseMotionTypeOverride{ false };

	UPROPERTY(VisibleAnywhere)
	ERecallPhysicsMotionType MotionTypeOverride{ ERecallPhysicsMotionType::MAX };
};

USTRUCT()
struct FRecallPhysicsSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<uint8> Data;

	UPROPERTY(VisibleAnywhere)
	uint32 SerialNumberGenerator{ INVALID_PHYSICS_BODY_SERIAL_NUMBER };

	UPROPERTY(VisibleAnywhere)
	TMap<FRecallPhysicsBodyHandle, FRecallPhysicsBodySnapshot> Bodies;
	
	UPROPERTY(VisibleAnywhere)
	TSet<FRecallPhysicsConstrainHandle> Constrains;
};
