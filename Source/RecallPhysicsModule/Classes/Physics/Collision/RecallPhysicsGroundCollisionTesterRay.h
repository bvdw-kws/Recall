// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Physics/RecallPhysicsTypes.h"

#if WITH_JOLT_PHYSICS
namespace JPH
{
	class PhysicsSystem;
} // namespace JPH
#endif // WITH_JOLT_PHYSICS

/// Class that does collision detection with ground
class RECALLPHYSICSMODULE_API FRecallPhysicsGroundCollisionTesterRay
{
public:
	/// Constructor
	/// @param inObjectLayer Object layer to test collision with
	/// @param inUp World space up vector, used to avoid colliding with vertical walls.
	/// @param inMaxSlopeAngle Max angle (degrees) that is considered for colliding wheels. This is to avoid colliding with vertical walls.
	FRecallPhysicsGroundCollisionTesterRay(int32 inObjectLayer, FVector inUp = FVector::UpVector, float inMaxSlopeAngle = 80.0f);

#if WITH_JOLT_PHYSICS
	bool Collide(JPH::PhysicsSystem& inPhysicsSystem, uint32 inBodyID, const FVector& inRayOrigin, const FVector& inRayDirection, float inRayLength,
		uint32& outContactBodyID, FVector& outContactPosition, FVector& outContactNormal) const;
#endif // WITH_JOLT_PHYSICS
	
private:
	int32							mObjectLayer;
	FVector							mUp;
	float							mCosMaxSlopeAngle;
};
