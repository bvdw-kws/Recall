// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Mass/EntityElementTypes.h"
#include "Physics/Common/RecallPhysicsCommonShapeTypes.h"
#include "Physics/RecallPhysicsBodyHandle.h"
#include "Physics/RecallPhysicsTypes.h"

#include "RecallSensorFragments.generated.h"

/**
* Fragment to keep track of the bodies of our sensors
*/
USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallSensorFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FRecallPhysicsBodyHandle> BodyHandles;
};

template <>
struct TMassFragmentTraits<FRecallSensorFragment> final
{
	enum { AuthorAcceptsItsNotTriviallyCopyable = true };
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallSensorInstanceParameters
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FName SensorName = NAME_None;

	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsSphereShape Shape;

	UPROPERTY(VisibleAnywhere)
	FJPRPhysicsBodyParameters Params;

	UPROPERTY(VisibleAnywhere)
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere)
	FColor DebugActivatedColor = FColor::Green;

	UPROPERTY(VisibleAnywhere)
	FColor DebugOverlappingColor = FColor::Purple;

	UPROPERTY(VisibleAnywhere)
	FColor DebugDeactivatedColor = FColor::Red;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallSensorConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FRecallSensorInstanceParameters> InstanceParameters;
};
