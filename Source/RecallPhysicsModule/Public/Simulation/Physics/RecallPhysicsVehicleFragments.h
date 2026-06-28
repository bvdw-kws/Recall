// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Mass/EntityElementTypes.h"
#include "Physics/Vehicle/RecallPhysicsVehicleShapeTypes.h"

#include "RecallPhysicsVehicleFragments.generated.h"

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsVehicleFragment : public FMassFragment
{
	GENERATED_BODY()

	/**
	 * Value between -1 and 1 for auto transmission and value between 0 and 1 indicating desired driving direction and amount the gas pedal is pressed
	 */
	UPROPERTY(VisibleAnywhere)
	float Forward = 0.0f;

	/**
	 * Value between -1 and 1 indicating desired steering angle (1 = right)
	 */
	UPROPERTY(VisibleAnywhere)
	float Right = 0.0f;

	/**
	 * Value between 0 and 1 indicating how strong the brake pedal is pressed
	 */
	UPROPERTY(VisibleAnywhere)
	float Brake = 0.0f;

	/**
	 * Value between 0 and 1 indicating how strong the hand brake is pulled
	 */
	UPROPERTY(VisibleAnywhere)
	float HandBrake = 0.0f;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsVehicleConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsVehicleShape Shape;

	UPROPERTY(VisibleAnywhere)
	FJPRPhysicsBodyParameters Params;
};
