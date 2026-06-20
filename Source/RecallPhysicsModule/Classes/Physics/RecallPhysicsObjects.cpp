// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsObjects.h"

#include "DrawDebugHelpers.h"
#include "RecallPhysicsTypes.h"
#include "Jolt/RecallPhysicsJoltTypes.h"
#include "Utility/Math/RecallMathUtils.h"

#if WITH_JOLT_PHYSICS
// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

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

DEFINE_LOG_CATEGORY(LogRecallPhysicsObject);

#if WITH_JOLT_PHYSICS
void FRecallPhysicsBody::SetupBodyCreationSettings(BodyCreationSettings& body_creation_settings, const FRecallPhysicsBodyParameters& Params)
{
	body_creation_settings.mGravityFactor = Params.GravityFactor;
	body_creation_settings.mMotionQuality = static_cast<EMotionQuality>(Params.MotionQuality);
	body_creation_settings.mEnhancedInternalEdgeRemoval = Params.EnhancedInternalEdgeRemoval;
	body_creation_settings.mFriction = Params.Friction;
	body_creation_settings.mAllowDynamicOrKinematic = Params.bAllowDynamicOrKinematic;
	body_creation_settings.mIsSensor = Params.bIsSensor;
	body_creation_settings.mRestitution = Params.Restitution;
	body_creation_settings.mMaxAngularVelocity = DegreesToRadians(Params.MaxAngularVelocityDegrees);
	body_creation_settings.mAllowSleeping = Params.bAllowSleeping;
	body_creation_settings.mAllowedDOFs = static_cast<EAllowedDOFs>(Params.AllowedDOFs);
	body_creation_settings.mOverrideMassProperties = static_cast<EOverrideMassProperties>(Params.OverrideMassProperties);
	body_creation_settings.mInertiaMultiplier = Params.InertiaMultiplier;
	body_creation_settings.mMassPropertiesOverride.mMass = Params.MassPropertiesOverride.Mass;
	body_creation_settings.mMassPropertiesOverride.mInertia = Mat44::sScale(Params.MassPropertiesOverride.Inertia);
	body_creation_settings.mNumVelocityStepsOverride = Params.NumVelocityStepsOverride;
	body_creation_settings.mNumPositionStepsOverride = Params.NumPositionStepsOverride;
}
#endif // WITH_JOLT_PHYSICS

void FRecallPhysicsBody::SetPosition(const FVector& Position) const
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const FVector PhysicsPos = UnrealToJoltPhysics(Position);
		const RVec3 Pos(PhysicsPos.X, PhysicsPos.Y, PhysicsPos.Z);

		GetBodyInterface().SetPosition(*body_id.Get(), Pos, EActivation::DontActivate);
	}
#endif // WITH_JOLT_PHYSICS
}

void FRecallPhysicsBody::SetPositionAndRotation(const FVector& Position, const FQuat& Rotation) const
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const FVector PhysicsPos = UnrealToJoltPhysics(Position);
		const RVec3 Pos(PhysicsPos.X, PhysicsPos.Y, PhysicsPos.Z);
		const FQuat Rot = UnrealToJoltPhysics(Rotation);

		GetBodyInterface().SetPositionAndRotation(*body_id.Get(), Pos, Quat(Rot.X, Rot.Y, Rot.Z, Rot.W), EActivation::DontActivate);
	}
#endif // WITH_JOLT_PHYSICS
}

void FRecallPhysicsBody::GetPosition(FVector& OutPosition) const
{
	FQuat Rotation = FQuat::Identity;
	GetPositionAndRotation(OutPosition, Rotation);
}

void FRecallPhysicsBody::GetPositionAndRotation(FVector& OutPosition, FQuat& OutRotation) const
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		RVec3 Position = RVec3::sZero();
		Quat Rotation = Quat::sIdentity();
		GetBodyInterface().GetPositionAndRotation(*body_id.Get(), Position, Rotation);
		
		OutPosition = JoltPhysicsToUnreal(FVector(Position.GetX(), Position.GetY(), Position.GetZ()));
		OutRotation = JoltPhysicsToUnreal(FQuat(Rotation.GetX(), Rotation.GetY(), Rotation.GetZ(), Rotation.GetW()));
	}
#endif // WITH_JOLT_PHYSICS
}

FTransform FRecallPhysicsBody::GetTransform() const
{
	FVector Position = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	GetPositionAndRotation(Position, Rotation);

	return FTransform(Rotation, Position);
}

void FRecallPhysicsBody::SetRotation(const FQuat& Rotation) const
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const FQuat Rot = UnrealToJoltPhysics(Rotation);
		GetBodyInterface().SetRotation(*body_id.Get(), Quat(Rot.X, Rot.Y, Rot.Z, Rot.W), EActivation::DontActivate);
	}
#endif // WITH_JOLT_PHYSICS
}

void FRecallPhysicsBody::SetRotation(const FRotator& Rotation) const
{
	SetRotation(Rotation.Quaternion());
}

void FRecallPhysicsBody::GetRotation(FQuat& OutRotation) const
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const Quat Rotation = GetBodyInterface().GetRotation(*body_id.Get());
		OutRotation = JoltPhysicsToUnreal(FQuat(Rotation.GetX(), Rotation.GetY(), Rotation.GetZ(), Rotation.GetW()));
	}
#endif // WITH_JOLT_PHYSICS
}

bool FRecallPhysicsBody::CollideShape(const FVector& Position, uint32& OutContactBodyID, FVector& OutContactPosition,
	FVector& OutContactNormal) const
{
#if WITH_JOLT_PHYSICS
	if (!body_id.IsValid())
	{
		return false;
	}
	
	RefConst<Shape> shape = GetBodyInterface().GetShape(*body_id.Get());

	const FVector inJoltRayOrigin = UnrealToJoltPhysics(Position);
	const RVec3 inOrigin(inJoltRayOrigin.X, inJoltRayOrigin.Y, inJoltRayOrigin.Z);
	
	const ObjectLayer object_layer = GetBodyInterface().GetObjectLayer(*body_id.Get());
	const DefaultBroadPhaseLayerFilter default_broadphase_layer_filter = GetPhysicsSystem().GetDefaultBroadPhaseLayerFilter(object_layer);
	const BroadPhaseLayerFilter& broadphase_layer_filter = default_broadphase_layer_filter;
	
	const DefaultObjectLayerFilter default_object_layer_filter = GetPhysicsSystem().GetDefaultLayerFilter(object_layer);
	const ObjectLayerFilter &object_layer_filter = default_object_layer_filter;

	const IgnoreSingleBodyFilter default_body_filter(*body_id.Get());
	const BodyFilter &body_filter = default_body_filter;
	
	RMat44 start_position = GetBodyInterface().GetWorldTransform(*body_id.Get());
	start_position.SetTranslation(inOrigin);
	
	class MyCollector : public CollideShapeCollector
	{
	public:
		MyCollector(PhysicsSystem &inPhysicsSystem) :
			mPhysicsSystem(inPhysicsSystem)
		{
		}

		virtual void		AddHit(const CollideShapeResult &inResult) override
		{
			// Test if this collision is closer than the previous one
			if (inResult.GetEarlyOutFraction() < GetEarlyOutFraction())
			{
				// Lock the body
				BodyLockRead lock(mPhysicsSystem.GetBodyLockInterfaceNoLock(), inResult.mBodyID2);
				JPH_ASSERT(lock.Succeeded()); // When this runs all bodies are locked so this should not fail
				const Body *body = &lock.GetBody();

				if (body->IsSensor())
					return;

				// Test that we're not hitting a vertical wall
				RVec3 contact_pos = inResult.mContactPointOn1;
				Vec3 normal = body->GetWorldSpaceSurfaceNormal(inResult.mSubShapeID2, contact_pos);
				
				// Update early out fraction to this hit
				UpdateEarlyOutFraction(inResult.GetEarlyOutFraction());

				// Get the contact properties
				mBody = body;
				mContactPosition = contact_pos;
				mContactNormal = normal;
			}
		}

		// Configuration
		PhysicsSystem &		mPhysicsSystem;

		// Resulting closest collision
		const Body *		mBody = nullptr;
		RVec3				mContactPosition = RVec3::sZero();
		Vec3				mContactNormal = Vec3::sZero();
	};

	CollideShapeSettings settings;

	MyCollector collector(GetPhysicsSystem());
	GetPhysicsSystem().GetNarrowPhaseQueryNoLock().CollideShape(shape, Vec3(1.0f, 1.0f, 1.0f), start_position,
		settings, shape->GetCenterOfMass(), collector, broadphase_layer_filter, object_layer_filter, body_filter);
	if (collector.mBody == nullptr)
	{
		return false;		
	}

	OutContactBodyID = collector.mBody->GetID().GetIndexAndSequenceNumber();
	OutContactPosition = JoltPhysicsToUnreal(
		FVector(collector.mContactPosition.GetX(), collector.mContactPosition.GetY(), collector.mContactPosition.GetZ()));
	OutContactNormal = JoltPhysicsToUnrealDirection(
		FVector(collector.mContactNormal.GetX(), collector.mContactNormal.GetY(), collector.mContactNormal.GetZ()));

	return true;
#else // WITH_JOLT_PHYSICS
	return false;
#endif
}

bool FRecallPhysicsBody::ShapeCast(const FVector& Position, const FVector& Direction, float Distance,
                                     uint32& OutContactBodyID, FVector& OutContactPosition, FVector& OutContactNormal) const
{
#if WITH_JOLT_PHYSICS
	if (!body_id.IsValid())
	{
		return false;
	}
	
	RefConst<Shape> shape = GetBodyInterface().GetShape(*body_id.Get());

	const FVector inJoltRayOrigin = UnrealToJoltPhysics(Position);
	const RVec3 inOrigin(inJoltRayOrigin.X, inJoltRayOrigin.Y, inJoltRayOrigin.Z);
	
	const FVector inJoltRayDirection = UnrealToJoltPhysicsDirection(Direction);
	const Vec3 inDirection(inJoltRayDirection.X, inJoltRayDirection.Y, inJoltRayDirection.Z);

	const float ray_length = Distance * UnrealToJoltPhysicsUnitScale;

	const ObjectLayer object_layer = GetBodyInterface().GetObjectLayer(*body_id.Get());
	const DefaultBroadPhaseLayerFilter default_broadphase_layer_filter = GetPhysicsSystem().GetDefaultBroadPhaseLayerFilter(object_layer);
	const BroadPhaseLayerFilter& broadphase_layer_filter = default_broadphase_layer_filter;
	
	const DefaultObjectLayerFilter default_object_layer_filter = GetPhysicsSystem().GetDefaultLayerFilter(object_layer);
	const ObjectLayerFilter &object_layer_filter = default_object_layer_filter;

	const IgnoreSingleBodyFilter default_body_filter(*body_id.Get());
	// const BodyFilter &body_filter = mBodyFilter != nullptr? *mBodyFilter : default_body_filter;
	const BodyFilter &body_filter = default_body_filter;

	RMat44 start_position = GetBodyInterface().GetWorldTransform(*body_id.Get());
	start_position.SetTranslation(inOrigin);
	
	RShapeCast shape_cast(shape, Vec3::sReplicate(1.0f),
		start_position, inDirection * ray_length);

	class MyCollector : public CastShapeCollector
	{
	public:
		MyCollector(PhysicsSystem &inPhysicsSystem, const RShapeCast &inShape) :
			mPhysicsSystem(inPhysicsSystem),
			mShape(inShape)
		{
		}

		virtual void		AddHit(const ShapeCastResult &inResult) override
		{
			// Test if this collision is closer than the previous one
			if (inResult.mFraction < GetEarlyOutFraction())
			{
				// Lock the body
				BodyLockRead lock(mPhysicsSystem.GetBodyLockInterfaceNoLock(), inResult.mBodyID2);
				JPH_ASSERT(lock.Succeeded()); // When this runs all bodies are locked so this should not fail
				const Body *body = &lock.GetBody();

				if (body->IsSensor())
					return;

				// Test that we're not hitting a vertical wall
				RVec3 contact_pos = mShape.GetPointOnRay(inResult.mFraction);
				Vec3 normal = body->GetWorldSpaceSurfaceNormal(inResult.mSubShapeID2, contact_pos);
				
				// Update early out fraction to this hit
				UpdateEarlyOutFraction(inResult.mFraction);

				// Get the contact properties
				mBody = body;
				mContactPosition = contact_pos;
				mContactNormal = normal;
			}
		}

		// Configuration
		PhysicsSystem &		mPhysicsSystem;
		RShapeCast			mShape;

		// Resulting closest collision
		const Body *		mBody = nullptr;
		RVec3				mContactPosition = RVec3::sZero();
		Vec3				mContactNormal = Vec3::sZero();
	};

	ShapeCastSettings settings;

	MyCollector collector(GetPhysicsSystem(), shape_cast);
	GetPhysicsSystem().GetNarrowPhaseQueryNoLock().CastShape(shape_cast, settings, shape->GetCenterOfMass(),
		collector, broadphase_layer_filter, object_layer_filter, body_filter);
	if (collector.mBody == nullptr)
	{
		return false;		
	}
	
	OutContactBodyID = collector.mBody->GetID().GetIndexAndSequenceNumber();
	OutContactPosition = JoltPhysicsToUnreal(
		FVector(collector.mContactPosition.GetX(), collector.mContactPosition.GetY(), collector.mContactPosition.GetZ()));
	OutContactNormal = JoltPhysicsToUnrealDirection(
		FVector(collector.mContactNormal.GetX(), collector.mContactNormal.GetY(), collector.mContactNormal.GetZ()));

	return true;
#else // WITH_JOLT_PHYSICS
	return false;
#endif
}

float FRecallPhysicsBody::GetMass() const
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		BodyLockRead lock(GetPhysicsSystem().GetBodyLockInterfaceNoLock(), *body_id.Get());
		JPH_ASSERT(lock.Succeeded()); // When this runs all bodies are locked so this should not fail
		const Body *body = &lock.GetBody();
		return body->GetBodyCreationSettings().GetMassProperties().mMass;
	}
#endif // WITH_JOLT_PHYSICS
	return 0.0f;
}

void FRecallPhysicsBody::AddLinearVelocity(const FVector& LinearVelocity)
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const FVector LinearVelocityPerSecond = Recall::Math::Utils::UnitsPerFrameToPerSecond(LinearVelocity);
		const FVector PhysicsLinearVelocity = UnrealToJoltPhysics(LinearVelocityPerSecond);

		Vec3 LinearVel;
		LinearVel.SetX(PhysicsLinearVelocity.X);
		LinearVel.SetY(PhysicsLinearVelocity.Y);
		LinearVel.SetZ(PhysicsLinearVelocity.Z);

		GetBodyInterface().AddLinearVelocity(*body_id.Get(), LinearVel);
	}
#endif // WITH_JOLT_PHYSICS
}

void FRecallPhysicsBody::AddLinearAndAngularVelocity(const FVector& LinearVelocity, const FVector& AngularVelocity)
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const FVector LinearVelocityPerSecond = Recall::Math::Utils::UnitsPerFrameToPerSecond(LinearVelocity);
		const FVector PhysicsLinearVelocity = UnrealToJoltPhysics(LinearVelocityPerSecond);
		const Vec3 LinearVel(PhysicsLinearVelocity.X, PhysicsLinearVelocity.Y, PhysicsLinearVelocity.Z);

		const FVector AngularVelocityPerSecond = Recall::Math::Utils::UnitsPerFrameToPerSecond(AngularVelocity);
		const FVector PhysicsAngularVelocity = FMath::DegreesToRadians(UnrealToJoltPhysics(AngularVelocityPerSecond));
		const Vec3 AngularVel(PhysicsAngularVelocity.X, PhysicsAngularVelocity.Y, PhysicsAngularVelocity.Z);

		GetBodyInterface().AddLinearAndAngularVelocity(*body_id.Get(), LinearVel, AngularVel);
	}
#endif // WITH_JOLT_PHYSICS
}

void FRecallPhysicsBody::SetLinearAndAngularVelocity(const FVector& LinearVelocity, const FVector& AngularVelocity)
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const FVector LinearVelocityPerSecond = Recall::Math::Utils::UnitsPerFrameToPerSecond(LinearVelocity);
		const FVector PhysicsLinearVelocity = UnrealToJoltPhysics(LinearVelocityPerSecond);
		const Vec3 LinearVel(PhysicsLinearVelocity.X, PhysicsLinearVelocity.Y, PhysicsLinearVelocity.Z);

		const FVector AngularVelocityPerSecond = Recall::Math::Utils::UnitsPerFrameToPerSecond(AngularVelocity);
		const FVector PhysicsAngularVelocity = FMath::DegreesToRadians(UnrealToJoltPhysics(AngularVelocityPerSecond));
		const Vec3 AngularVel(PhysicsAngularVelocity.X, PhysicsAngularVelocity.Y, PhysicsAngularVelocity.Z);

		GetBodyInterface().SetLinearAndAngularVelocity(*body_id.Get(), LinearVel, AngularVel);
	}
#endif // WITH_JOLT_PHYSICS
}

void FRecallPhysicsBody::AddImpulse(const FVector& ForceNewton, bool bIgnoreMass)
{
	const double Mass = bIgnoreMass ? 1.0f : GetMass();
	const FVector Velocity = Recall::Math::Utils::UnitsPerSecondToPerFrame(ForceNewton) * 100.0 / Mass;

	AddLinearVelocity(Velocity);
}

void FRecallPhysicsBody::SetLinearVelocityFromImpulse(const FVector& ForceNewton, bool bIgnoreMass)
{
	const double Mass = bIgnoreMass ? 1.0f : GetMass();
	const FVector Velocity = Recall::Math::Utils::UnitsPerSecondToPerFrame(ForceNewton) * 100.0 / Mass;

	SetLinearVelocity(Velocity);
}

void FRecallPhysicsBody::SetLinearVelocity2DFromImpulse(const FVector& ForceNewton, bool bIgnoreMass)
{
	const double Mass = bIgnoreMass ? 1.0f : GetMass();
	const FVector Velocity = Recall::Math::Utils::UnitsPerSecondToPerFrame(ForceNewton) * 100.0 / Mass;

	SetLinearVelocity2D(FVector2D(Velocity.X, Velocity.Y));
}

void FRecallPhysicsBody::SetLinearVelocity2D(const FVector2D& LinearVelocity)
{
	FVector WorldVelocity = GetLinearVelocity();
	WorldVelocity.X = LinearVelocity.X;
	WorldVelocity.Y = LinearVelocity.Y;

	SetLinearVelocity(WorldVelocity);
}

void FRecallPhysicsBody::SetLinearZVelocity(float ZVelocity)
{
	FVector WorldVelocity = GetLinearVelocity();
	WorldVelocity.Z = ZVelocity;

	SetLinearVelocity(WorldVelocity);
}

void FRecallPhysicsBody::SetLinearVelocity(const FVector& LinearVelocity)
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const FVector LinearVelocityPerSecond = Recall::Math::Utils::UnitsPerFrameToPerSecond(LinearVelocity);
		const FVector PhysicsVelocity = UnrealToJoltPhysics(LinearVelocityPerSecond);
		const Vec3 Vel(PhysicsVelocity.X, PhysicsVelocity.Y, PhysicsVelocity.Z);

		GetBodyInterface().SetLinearVelocity(*body_id.Get(), Vel);
	}
#endif // WITH_JOLT_PHYSICS
}

FVector FRecallPhysicsBody::GetLinearVelocity() const
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const Vec3 PhysicsVelocity = GetBodyInterface().GetLinearVelocity(*body_id.Get());
		const FVector WorldVelocity = JoltPhysicsToUnreal(FVector(PhysicsVelocity.GetX(), PhysicsVelocity.GetY(), PhysicsVelocity.GetZ()));

		return Recall::Math::Utils::UnitsPerSecondToPerFrame(WorldVelocity);
	}
#endif // WITH_JOLT_PHYSICS
	return FVector::ZeroVector;
}

FVector2D FRecallPhysicsBody::GetLinearVelocity2D() const
{
	const FVector Velocity = GetLinearVelocity();
	return static_cast<FVector2D>(Velocity);
}

void FRecallPhysicsBody::SetAngularVelocity(const FVector& AngularVelocity)
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const FVector AngularVelocityPerSecond = Recall::Math::Utils::UnitsPerFrameToPerSecond(AngularVelocity);
		const FVector PhysicsVelocity = FMath::DegreesToRadians(UnrealToJoltPhysics(AngularVelocityPerSecond));
		const Vec3 Vel(PhysicsVelocity.X, PhysicsVelocity.Y, PhysicsVelocity.Z);

		GetBodyInterface().SetAngularVelocity(*body_id.Get(), Vel);
	}
#endif // WITH_JOLT_PHYSICS
}

FVector FRecallPhysicsBody::GetAngularVelocity() const
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		const Vec3 PhysicsAngularVelocity = GetBodyInterface().GetAngularVelocity(*body_id.Get());
		const FVector WorldAngularVelocity = FMath::RadiansToDegrees(JoltPhysicsToUnreal(FVector(PhysicsAngularVelocity.GetX(), PhysicsAngularVelocity.GetY(), PhysicsAngularVelocity.GetZ())));

		return Recall::Math::Utils::UnitsPerSecondToPerFrame(WorldAngularVelocity);
	}
#endif // WITH_JOLT_PHYSICS
	return FVector::ZeroVector;
}

void FRecallPhysicsBody::SetActive(bool bActive)
{
	if (bActive)
	{
		Activate();
	}
	else
	{
		Desactivate();
	}
}

void FRecallPhysicsBody::Activate()
{
	if (bEnabled) return;

	bEnabled = true;

#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		GetBodyInterface().AddBody(*body_id.Get(), EActivation::Activate);
	}
#endif // WITH_JOLT_PHYSICS
}

void FRecallPhysicsBody::Desactivate()
{
	if (!bEnabled) return;

	bEnabled = false;

#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		GetBodyInterface().RemoveBody(*body_id.Get());
	}
#endif // WITH_JOLT_PHYSICS
}

FVector FRecallPhysicsBody::GetMovementForwardVector() const
{
	const FVector WorldVelocity = GetLinearVelocity();
	if (!WorldVelocity.IsNearlyZero())
	{
		return WorldVelocity.GetSafeNormal();
	}
	
	return GetForwardVector();
}

FVector FRecallPhysicsBody::GetForwardVector() const
{
	FQuat Rotation = FQuat::Identity;
	GetRotation(Rotation);
	return Rotation.GetForwardVector();
}

FVector FRecallPhysicsBody::GetRightVector() const
{
	FQuat Rotation = FQuat::Identity;
	GetRotation(Rotation);
	return Rotation.GetRightVector();
}

#if WITH_JOLT_PHYSICS
Body* FRecallPhysicsBody::CreateAndSetBody(const BodyCreationSettings& body_settings, uint32 InBodyID)
{
	const BodyID NewBodyID(InBodyID);

	Body* body = GetBodyInterface().CreateBodyWithID(NewBodyID, body_settings);

	SetBodyID(NewBodyID);

	return body;
}

void FRecallPhysicsBody::SetBodyID(const BodyID& InBodyID)
{
	body_id = TSharedPtr<BodyID>(new BodyID(InBodyID));
}

void FRecallPhysicsBody::SaveVector(JPH::StateRecorder& State, const FVector& Vec)
{
	State.Write(DVec3(Vec.X, Vec.Y, Vec.Z));
}

void FRecallPhysicsBody::RestoreVector(JPH::StateRecorder& State, FVector& Vec)
{
	DVec3 Src = DVec3::sZero();
	State.Read(Src);
	Vec = FVector(Src.GetX(), Src.GetY(), Src.GetZ());
}

BodyInterface& FRecallPhysicsBody::GetBodyInterface() const
{
	check(physics_system.IsValid());
	return physics_system.Pin()->GetBodyInterface();
}

PhysicsSystem& FRecallPhysicsBody::GetPhysicsSystem() const
{
	check(physics_system.IsValid());
	return *physics_system.Pin();
}

JPH::TempAllocator& FRecallPhysicsBody::GetTempAllocator() const
{
	check(temp_allocator.IsValid());
	return *temp_allocator.Pin();
}
#endif // WITH_JOLT_PHYSICS

void FRecallPhysicsBody::ReleasePhysicsObject()
{
	Desactivate();

#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		GetBodyInterface().DestroyBody(*body_id.Get());

		body_id.Reset();
	}
#endif // WITH_JOLT_PHYSICS
}

uint32 FRecallPhysicsBody::GetBodyID() const
{
#if WITH_JOLT_PHYSICS
	return body_id.IsValid() ? body_id->GetIndexAndSequenceNumber() : JPH::BodyID::cInvalidBodyID;
#else // WITH_JOLT_PHYSICS
	return 0;
#endif
}

#if !UE_BUILD_SHIPPING
void FRecallPhysicsBody::DumpObject() const
{
#if WITH_JOLT_PHYSICS
	if (body_id.IsValid())
	{
		RVec3 PhysicsPosition = RVec3::sZero();
		Quat PhysicsRotation = Quat::sIdentity();
		GetBodyInterface().GetPositionAndRotation(*body_id.Get(), PhysicsPosition, PhysicsRotation);

		const FVector WorldPosition = JoltPhysicsToUnreal(FVector(PhysicsPosition.GetX(), PhysicsPosition.GetY(), PhysicsPosition.GetZ()));
		const FQuat WorldRotation = JoltPhysicsToUnreal(FQuat(PhysicsRotation.GetX(), PhysicsRotation.GetY(), PhysicsRotation.GetZ(), PhysicsRotation.GetW()));

		const Vec3 PhysicsVelocity = GetBodyInterface().GetLinearVelocity(*body_id.Get());
		const FVector WorldVelocity = JoltPhysicsToUnreal(FVector(PhysicsVelocity.GetX(), PhysicsVelocity.GetY(), PhysicsVelocity.GetZ()));

		FString DumpString;
		DumpString += FString::Printf(TEXT("[BodyID: %d]\n"), GetBodyID());

		DumpString += FString::Printf(TEXT("PhysicsVelocity: %s\n"), *FString::Printf(TEXT("X=%3.3f Y=%3.3f Z=%3.3f"), PhysicsVelocity.GetX(), PhysicsVelocity.GetY(), PhysicsVelocity.GetZ()));
		DumpString += FString::Printf(TEXT("WorldVelocity: %s\n"), *WorldVelocity.ToString());

		DumpString += FString::Printf(TEXT("PhysicsPosition: %s\n"), *FString::Printf(TEXT("X=%3.3f Y=%3.3f Z=%3.3f"), PhysicsPosition.GetX(), PhysicsPosition.GetY(), PhysicsPosition.GetZ()));
		DumpString += FString::Printf(TEXT("WorldPosition: %s\n"), *WorldPosition.ToString());

		UE_LOG(LogRecallPhysicsObject, Log, TEXT("%s"), *DumpString);
	}
#endif // WITH_JOLT_PHYSICS
}
#endif // !UE_BUILD_SHIPPING
// FRecallPhysicsBody End
