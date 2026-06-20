// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "RecallPhysicsBodyHandle.h"

#include "RecallPhysicsHitEvent.generated.h"

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsHitEvent
{
	GENERATED_BODY()

	/* Unique ID to identify a frame hit */
	UPROPERTY(VisibleAnywhere, Transient)
	uint32 HitID = 0;

	/* Body handle of the collider we hit. */
	UPROPERTY(VisibleAnywhere, Transient)
	FRecallPhysicsBodyHandle ColliderBodyHandle;

	/* Entity of the collider we hit. */
	UPROPERTY(VisibleAnywhere, Transient)
	FMassEntityHandle ColliderEntity;

	UPROPERTY(VisibleAnywhere, Transient)
	FVector ImpactWorldLocation{ FVector::ZeroVector };

	/* Normal of the impact on the hit surface. */
	UPROPERTY(VisibleAnywhere, Transient)
	FVector ImpactWorldNormal{ FVector::ZeroVector };

	UPROPERTY(VisibleAnywhere, Transient)
	FVector WorldVelocityAtImpact = FVector::ZeroVector;

	/**
	 * Hit a static body.
	 */
	UPROPERTY(VisibleAnywhere, Transient)
	bool bHitStatic{ false };

	/**
	 * Hit as sensor body.
	 */
	UPROPERTY(VisibleAnywhere, Transient)
	bool bHitSensor{ false };
};
