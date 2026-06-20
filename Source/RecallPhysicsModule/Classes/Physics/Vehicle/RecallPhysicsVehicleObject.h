// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "RecallPhysicsVehicleShapeTypes.h"
#include "Physics/RecallPhysicsObjects.h"
#include "RecallPhysicsVehicleTypes.h"

#if WITH_JOLT_PHYSICS
namespace JPH
{
	class VehicleConstraint;
} // namespace JPH
#endif // WITH_JOLT_PHYSICS

/**
* Wrapper Object for JPH::VehicleConstraintSettings.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsVehicleBody : public FRecallPhysicsBody
{
public:
	void InitVehicle(const FRecallPhysicsVehicleShape& VehicleShape, const FRecallPhysicsBodyParameters& Params, uint32 InBodyID, int32 Layer);
	
	void SetDriverInput(float Forward, float Right, float Brake, float HandBrake);
	void GetDriverInput(float& OutForward, float& OutRight, float& OutBrake, float& OutHandBrake) const;
	
	FVector GetSpeedKilometersPerHour(const UWorld* World) const;
	TArray<FRecallPhysicsVehicleWheelContact> GetWheelContacts() const;
	bool HasRearWheelsContact() const;
	
public:
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const override;
	virtual void DrawDebugInfo(const UWorld* World, const FColor& Color) const override;

protected:
	virtual void ReleasePhysicsObject() override;
	
#if WITH_JOLT_PHYSICS	
private:
	TSharedPtr<struct FRecallVehicleRef> vehicle_constrain;
#endif // WITH_JOLT_PHYSICS
};
