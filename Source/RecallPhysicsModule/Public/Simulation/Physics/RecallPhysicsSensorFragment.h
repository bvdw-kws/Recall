// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Mass/EntityElementTypes.h"
#include "Physics/RecallPhysicsBodyHandle.h"

#include "RecallPhysicsSensorFragment.generated.h"

// Tag to identify entities that are overlapping with another entity
USTRUCT() struct FRecallPhysicsSensorOverlapTag : public FMassTag { GENERATED_BODY() };

// Tag to identify entities that can sense other entities
USTRUCT() struct RECALLPHYSICSMODULE_API FRecallPhysicsSensorTag : public FMassTag { GENERATED_BODY() };

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsSensorInstance
{
	GENERATED_BODY()

	// Handle for the interaction volume
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsBodyHandle BodyHandle;

	UPROPERTY(VisibleAnywhere)
	FName SensorName = NAME_None;

	UPROPERTY(VisibleAnywhere)
	TArray<FMassEntityHandle> OverlappingEntities;

	FORCEINLINE bool IsOverlapping() const
	{
		return OverlappingEntities.Num() > 0;
	}
};

/**
* Fragment to register sensors, all the physics bodies have this fragment (even bodies without sensor)
*/
USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsSensorFragment : public FMassFragment
{
	GENERATED_BODY()

	FORCEINLINE bool IsSensor() const
	{
		return SensorInstances.Num() > 0;
	}

	FORCEINLINE TArray<FMassEntityHandle> GetOverlappingEntities(const FName& SensorName = NAME_None) const
	{
		TArray<FMassEntityHandle> Results;

		for (const FRecallPhysicsSensorInstance& SensorInstance : SensorInstances)
		{
			if (SensorName.IsNone() || SensorName == SensorInstance.SensorName)
			{
				Results.Append(SensorInstance.OverlappingEntities);
			}
		}

		return Results;
	}

	FORCEINLINE bool IsSensorOverlapping(const FName& SensorName) const
	{
		for (const FRecallPhysicsSensorInstance& SensorInstance : SensorInstances)
		{
			if ((SensorName.IsNone() || SensorName == SensorInstance.SensorName) && SensorInstance.IsOverlapping())
			{
				return true;
			}
		}
		return false;
	}

	FORCEINLINE bool IsSensorOverlapping(const FRecallPhysicsBodyHandle& BodyHandle) const
	{
		if (BodyHandle.IsValid())
		{
			for (const FRecallPhysicsSensorInstance& SensorInstance : SensorInstances)
			{
				if (SensorInstance.BodyHandle == BodyHandle && SensorInstance.IsOverlapping())
				{
					return true;
				}
			}
		}
		return false;
	}

	FORCEINLINE bool IsAnySensorOverlapping() const
	{
		return IsSensorOverlapping(NAME_None);
	}

	FORCEINLINE TArray<FRecallPhysicsBodyHandle> GetSensorBodyHandles() const
	{
		TArray<FRecallPhysicsBodyHandle> Results;
		Results.Reserve(SensorInstances.Num());

		for (const FRecallPhysicsSensorInstance& SensorInstance : SensorInstances)
		{
			Results.Add(SensorInstance.BodyHandle);
		}

		return Results;
	}

	FORCEINLINE void AddSensor(const FRecallPhysicsBodyHandle& BodyHandle, FName SensorName = NAME_None)
	{
		FRecallPhysicsSensorInstance& NewInstance = SensorInstances.AddDefaulted_GetRef();
		NewInstance.BodyHandle = BodyHandle;
		NewInstance.SensorName = SensorName;
	}

	FORCEINLINE void RemoveSensor(const FRecallPhysicsBodyHandle& BodyHandle)
	{
		for (int32 SensorIndex = 0; SensorIndex < SensorInstances.Num(); SensorIndex++)
		{
			if (SensorInstances[SensorIndex].BodyHandle == BodyHandle)
			{
				SensorInstances.RemoveAt(SensorIndex);
				break;
			}
		}
	}

	UPROPERTY(VisibleAnywhere)
	TArray<FRecallPhysicsSensorInstance> SensorInstances;
};

template <>
struct TMassFragmentTraits<FRecallPhysicsSensorFragment> final
{
	enum { AuthorAcceptsItsNotTriviallyCopyable = true };
};
