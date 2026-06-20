// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsGroundCollisionTesterRay.h"

#include "Physics/Jolt/RecallPhysicsJoltTypes.h"

#if WITH_JOLT_PHYSICS
// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

// We're also using STL classes in this example
using namespace std;
#endif // WITH_JOLT_PHYSICS

FRecallPhysicsGroundCollisionTesterRay::FRecallPhysicsGroundCollisionTesterRay(int32 inObjectLayer, FVector inUp, float inMaxSlopeAngle)
	: mObjectLayer(inObjectLayer)
	, mUp(UnrealToJoltPhysicsDirection(inUp))
	, mCosMaxSlopeAngle(Cos(DegreesToRadians(inMaxSlopeAngle)))
{	
}

#if WITH_JOLT_PHYSICS
bool FRecallPhysicsGroundCollisionTesterRay::Collide(PhysicsSystem& inPhysicsSystem, uint32 inBodyID,
                                          const FVector& inRayOrigin, const FVector& inRayDirection, float inRayLength,
                                          uint32& outContactBodyID, FVector& outContactPosition, FVector& outContactNormal) const
{
	const FVector inJoltRayOrigin = UnrealToJoltPhysics(inRayOrigin);
	const Vec3 inOrigin(inJoltRayOrigin.X, inJoltRayOrigin.Y, inJoltRayOrigin.Z);
	
	const FVector inJoltRayDirection = UnrealToJoltPhysicsDirection(inRayDirection);
	const Vec3 inDirection(inJoltRayDirection.X, inJoltRayDirection.Y, inJoltRayDirection.Z);

	const float ray_length = inRayLength * UnrealToJoltPhysicsUnitScale;

	const ObjectLayer object_layer(mObjectLayer);
	
	const DefaultBroadPhaseLayerFilter default_broadphase_layer_filter = inPhysicsSystem.GetDefaultBroadPhaseLayerFilter(object_layer);
	// const BroadPhaseLayerFilter &broadphase_layer_filter = mBroadPhaseLayerFilter != nullptr? *mBroadPhaseLayerFilter : default_broadphase_layer_filter;
	const BroadPhaseLayerFilter& broadphase_layer_filter = default_broadphase_layer_filter;
	
	const DefaultObjectLayerFilter default_object_layer_filter = inPhysicsSystem.GetDefaultLayerFilter(object_layer);
	// const ObjectLayerFilter &object_layer_filter = mObjectLayerFilter != nullptr? *mObjectLayerFilter : default_object_layer_filter;
	const ObjectLayerFilter &object_layer_filter = default_object_layer_filter;

	const BodyID body_id(inBodyID);
	const IgnoreSingleBodyFilter default_body_filter(body_id);
	// const BodyFilter &body_filter = mBodyFilter != nullptr? *mBodyFilter : default_body_filter;
	const BodyFilter &body_filter = default_body_filter;

	// const WheelSettings *wheel_settings = inVehicleConstraint.GetWheel(inWheelIndex)->GetSettings();
	// float wheel_radius = wheel_settings->mRadius;
	// float ray_length = wheel_settings->mSuspensionMaxLength + wheel_radius;
	RRayCast ray { inOrigin, ray_length * inDirection };

	class MyCollector : public CastRayCollector
	{
	public:
		MyCollector(PhysicsSystem &inPhysicsSystem, const RRayCast &inRay, Vec3Arg inUpDirection, float inCosMaxSlopeAngle) :
			mPhysicsSystem(inPhysicsSystem),
			mRay(inRay),
			mUpDirection(inUpDirection),
			mCosMaxSlopeAngle(inCosMaxSlopeAngle)
		{
		}

		virtual void		AddHit(const RayCastResult &inResult) override
		{
			// Test if this collision is closer than the previous one
			if (inResult.mFraction < GetEarlyOutFraction())
			{
				// Lock the body
				BodyLockRead lock(mPhysicsSystem.GetBodyLockInterfaceNoLock(), inResult.mBodyID);
				JPH_ASSERT(lock.Succeeded()); // When this runs all bodies are locked so this should not fail
				const Body *body = &lock.GetBody();

				if (body->IsSensor() || !body->IsStatic())
					return;

				// Test that we're not hitting a vertical wall
				RVec3 contact_pos = mRay.GetPointOnRay(inResult.mFraction);
				Vec3 normal = body->GetWorldSpaceSurfaceNormal(inResult.mSubShapeID2, contact_pos);
				if (normal.Dot(mUpDirection) > mCosMaxSlopeAngle)
				{
					// Update early out fraction to this hit
					UpdateEarlyOutFraction(inResult.mFraction);

					// Get the contact properties
					mBody = body;
					mSubShapeID2 = inResult.mSubShapeID2;
					mContactPosition = contact_pos;
					mContactNormal = normal;
				}
			}
		}

		// Configuration
		PhysicsSystem &		mPhysicsSystem;
		RRayCast			mRay;
		Vec3				mUpDirection;
		float				mCosMaxSlopeAngle;

		// Resulting closest collision
		const Body *		mBody = nullptr;
		SubShapeID			mSubShapeID2;
		RVec3				mContactPosition;
		Vec3				mContactNormal;
	};

	RayCastSettings settings;

	MyCollector collector(inPhysicsSystem, ray, Vec3(mUp.X, mUp.Y, mUp.Z), mCosMaxSlopeAngle);
	inPhysicsSystem.GetNarrowPhaseQueryNoLock().CastRay(ray, settings, collector, broadphase_layer_filter, object_layer_filter, body_filter);
	if (collector.mBody == nullptr)
	{
		return false;		
	}

	outContactBodyID = collector.mBody->GetID().GetIndexAndSequenceNumber();
	// outSubShapeID = collector.mSubShapeID2;
	outContactPosition = JoltPhysicsToUnreal(
		FVector(collector.mContactPosition.GetX(), collector.mContactPosition.GetY(), collector.mContactPosition.GetZ()));
	outContactNormal = JoltPhysicsToUnrealDirection(
		FVector(collector.mContactNormal.GetX(), collector.mContactNormal.GetY(), collector.mContactNormal.GetZ()));
	// outSuspensionLength = max(0.0f, ray_length * collector.GetEarlyOutFraction() - wheel_radius);

	return true;
}
#endif // WITH_JOLT_PHYSICS
