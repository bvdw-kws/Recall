// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

struct FRecallPhysicsBodyParameters;

#ifndef WITH_JOLT_PHYSICS
#define WITH_JOLT_PHYSICS 0
#endif

#if WITH_JOLT_PHYSICS
namespace JPH
{
	class Body;
	class BodyID;
	class BodyInterface;
	class PhysicsSystem;
	class StateRecorder;
	class SphereShape;
	class BodyCreationSettings;
	class TempAllocator;
} // namespace JPH
#endif // WITH_JOLT_PHYSICS

DECLARE_LOG_CATEGORY_EXTERN(LogRecallPhysicsObject, Log, All);

/**
* These Objects help us to keep track of Jolt Physics shape reference.
* They also give us some interface to work with Jolt Physics without including all the source files.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsBody
{
public:
	FRecallPhysicsBody() {}
	virtual ~FRecallPhysicsBody() {}

public:
	bool IsEnabled() const { return bEnabled; }
	bool DoesTriggerHitEvents() const { return bTriggerHitEvents; }

	uint32 GetBodyID() const;

	/* Velocity */

	/**
	 * Adds linear and angular velocity to this body's existing velocities.
	 * @param LinearVelocity Linear velocity to add (cm/s)
	 * @param AngularVelocity Angular velocity to add (radians/s)
	 */
	void AddLinearAndAngularVelocity(const FVector& LinearVelocity, const FVector& AngularVelocity);

	/**
	 * Sets this body's linear and angular velocities, replacing any existing velocities.
	 * @param LinearVelocity New linear velocity (cm/s)
	 * @param AngularVelocity New angular velocity (radians/s)
	 */
	void SetLinearAndAngularVelocity(const FVector& LinearVelocity, const FVector& AngularVelocity);

	/**
	 * Applies an impulse (instantaneous force) to this body.
	 * @param ForceNewton Impulse force in Newtons
	 * @param bIgnoreMass If true, impulse is applied directly without accounting for mass
	 */
	void AddImpulse(const FVector& ForceNewton, bool bIgnoreMass = true);

	/**
	 * Sets linear velocity based on an impulse force.
	 * @param ForceNewton Impulse force in Newtons
	 * @param bIgnoreMass If true, applies force directly without mass scaling
	 */
	void SetLinearVelocityFromImpulse(const FVector& ForceNewton, bool bIgnoreMass = true);

	/**
	 * Sets 2D linear velocity (XY plane only) based on an impulse force.
	 * @param ForceNewton Impulse force in Newtons
	 * @param bIgnoreMass If true, applies force directly without mass scaling
	 */
	void SetLinearVelocity2DFromImpulse(const FVector& ForceNewton, bool bIgnoreMass = true);

	/**
	 * Sets linear velocity in the XY plane only, preserving Z velocity.
	 * @param LinearVelocity 2D velocity vector (cm/s)
	 */
	void SetLinearVelocity2D(const FVector2D& LinearVelocity);

	/**
	 * Sets only the Z-component of linear velocity.
	 * @param ZVelocity Vertical velocity (cm/s)
	 */
	void SetLinearZVelocity(float ZVelocity);

	/**
	 * Gets the linear velocity in the XY plane only.
	 * @return 2D velocity vector (cm/s)
	 */
	FVector2D GetLinearVelocity2D() const;

	/**
	 * Sets this body's rotation velocity.
	 * @param AngularVelocity Rotation rate in radians per second
	 */
	void SetAngularVelocity(const FVector& AngularVelocity);

	/**
	 * Gets this body's current rotation velocity.
	 * @return Angular velocity in radians per second
	 */
	FVector GetAngularVelocity() const;

	/* Transform */

	/**
	 * Gets the current world position of this body.
	 * @param OutPosition The world position output
	 */
	void GetPosition(FVector& OutPosition) const;

	/**
	 * Gets the complete transform (position and rotation) of this body.
	 * @return The world space transform
	 */
	FTransform GetTransform() const;

	/**
	 * Sets the rotation of this body without moving its position.
	 * @param Rotation New rotation to apply
	 */
	void SetRotation(const FRotator& Rotation) const;

	/**
	 * Gets the current rotation of this body.
	 * @param OutRotation The quaternion rotation output
	 */
	void GetRotation(FQuat& OutRotation) const;

	/**
	 * Gets the forward vector based on this body's current rotation.
	 * @return Forward direction (X-axis in local space)
	 */
	FVector GetForwardVector() const;

	/**
	 * Gets the right vector based on this body's current rotation.
	 * @return Right direction (Y-axis in local space)
	 */
	FVector GetRightVector() const;

	/**
	 * Tests if this body's shape overlaps any objects at the given world position.
	 *
	 * Use Case: Determine what objects this body would collide with if positioned at a specific location.
	 * Common Uses: Ground detection, overlap checks for placement validation, collision testing at a point.
	 *
	 * @param Position World position to test the collision at
	 * @param OutContactBodyID ID of the body this shape collides with (if any)
	 * @param OutContactPosition The contact point(s) - may represent penetration center
	 * @param OutContactNormal Normal vector at the contact point
	 * @return true if the shape overlaps any objects at this position
	 *
	 * @note This does NOT move the body - it only tests collision at a position.
	 * @note For finding the nearest point ON a surface, use ShapeCast() instead.
	 */
	bool CollideShape(const FVector& Position,
		uint32& OutContactBodyID, FVector& OutContactPosition, FVector& OutContactNormal) const;

	/**
	 * Sweeps (casts) this body's shape from a starting position in a direction to find collisions.
	 *
	 * Use Case: Find where an object would hit if moving in a direction (raycasting with shape volume).
	 * Common Uses: Wall detection for climbing, movement prediction, finding grip points on surfaces.
	 *
	 * @param Position Starting world position for the sweep
	 * @param Direction Direction to sweep the shape (will be normalized internally)
	 * @param Distance How far to sweep in the given direction
	 * @param OutContactBodyID ID of the body that was hit (if any)
	 * @param OutContactPosition The exact point where the shape first made contact with a surface
	 * @param OutContactNormal Surface normal at the contact point (accurately calculated via GetWorldSpaceSurfaceNormal)
	 * @return true if the sweep hit any object within the distance
	 *
	 * @note Returns the FIRST contact point along the sweep direction.
	 * @note The contact normal is deterministic and accurate for surface interactions.
	 * @note Prefer this over CollideShape when you need surface normals or nearest point detection.
	 */
	bool ShapeCast(const FVector& Position, const FVector& Direction, float Distance,
		uint32& OutContactBodyID, FVector& OutContactPosition, FVector& OutContactNormal) const;

	float GetMass() const;

	void SetActive(bool bActive);
	
#if !UE_BUILD_SHIPPING
	virtual void DumpObject() const;
#endif // !UE_BUILD_SHIPPING

	/**
	 * Activates this body for physics simulation.
	 * @note Inactive bodies are not updated by the physics engine.
	 */
	virtual void Activate();

	/**
	 * Deactivates this body - it will no longer participate in physics simulation.
	 * @note Use this for performance optimization when bodies shouldn't be active.
	 */
	virtual void Desactivate();

	/**
	 * Adds to the linear velocity of this body.
	 * @param LinearVelocity Velocity to add (cm/s)
	 * @note This is a virtual override of AddLinearAndAngularVelocity.
	 */
	virtual void AddLinearVelocity(const FVector& LinearVelocity);

	/**
	 * Sets the linear velocity of this body, replacing any existing velocity.
	 * @param LinearVelocity New velocity (cm/s)
	 * @note This is a virtual override of SetLinearAndAngularVelocity.
	 */
	virtual void SetLinearVelocity(const FVector& LinearVelocity);

	/**
	 * Gets the current linear velocity of this body.
	 * @return Current velocity (cm/s)
	 * @note Virtual implementation may differ from the non-virtual variant.
	 */
	virtual FVector GetLinearVelocity() const;

	/**
	 * Sets the world position of this body.
	 * @param Position New world position
	 * @note This is a virtual override of the non-virtual GetPosition variant.
	 */
	virtual void SetPosition(const FVector& Position) const;

	/**
	 * Sets the rotation of this body using a quaternion.
	 * @param Rotation New rotation as quaternion
	 * @note Provides an alternative to the FRotator-based SetRotation.
	 */
	virtual void SetRotation(const FQuat& Rotation) const;

	/**
	 * Gets both position and rotation in a single call.
	 * @param OutPosition The world position output
	 * @param OutRotation The quaternion rotation output
	 * @note More efficient than calling GetPosition and GetRotation separately.
	 */
	virtual void GetPositionAndRotation(FVector& OutPosition, FQuat& OutRotation) const;

	/**
	 * Sets both position and rotation in a single call.
	 * @param Position New world position
	 * @param Rotation New rotation as quaternion
	 * @note More efficient than calling SetPosition and SetRotation separately.
	 */
	virtual void SetPositionAndRotation(const FVector& Position, const FQuat& Rotation) const;

	/**
	 * Gets the forward direction vector, accounting for any movement-specific rotation.
	 * @return Forward direction
	 * @note May differ from GetForwardVector in physics objects with special movement handling.
	 */
	virtual FVector GetMovementForwardVector() const;

	/**
	 * Draws a debug visualization of this body's shape in the world.
	 * @param World The world context for debug drawing
	 * @param Color Color to draw in
	 * @note Only available in non-shipping builds.
	 */
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const {}

	/**
	 * Draws debug information about this body (velocities, forces, etc).
	 * @param World The world context for debug drawing
	 * @param Color Color to draw in
	 * @note Only available in non-shipping builds.
	 */
	virtual void DrawDebugInfo(const UWorld* World, const FColor& Color) const {}

public:
	// This will be called before physics system is destroyed
	virtual void ReleasePhysicsObject();

public:
	void SetWorldContextObject(UObject* Object) { world_context_object = Object; }
	UObject* GetWorldContextObject() const { return world_context_object.Get(); }

protected:
	bool bEnabled{ false };
	bool bTriggerHitEvents{ true };

private:
	TWeakObjectPtr<UObject> world_context_object;

#if WITH_JOLT_PHYSICS
public:
	virtual void SaveState(JPH::StateRecorder& State) {}
	virtual void RestoreState(JPH::StateRecorder& State) {}
	
	/**
	 * Helper method to save a FVector to the state recorder.
	 */
	static void SaveVector(JPH::StateRecorder& State, const FVector& Vec);
	
	/**
	 * Helper method to restore a FVector from the state recorder.
	 */
	static void RestoreVector(JPH::StateRecorder& State, FVector& Vec);
	
public:
	void SetPhysicsSystem(const TWeakPtr<JPH::PhysicsSystem>& System) { physics_system = System; }
	void SetTempAllocator(const TWeakPtr<JPH::TempAllocator>& TempAllocator) { temp_allocator = TempAllocator; }

protected:
	JPH::Body* CreateAndSetBody(const JPH::BodyCreationSettings& body_settings, uint32 InBodyID);
	void SetBodyID(const JPH::BodyID& InBodyID);

	JPH::BodyInterface& GetBodyInterface() const;
	JPH::PhysicsSystem& GetPhysicsSystem() const;
	JPH::TempAllocator& GetTempAllocator() const;
	
	static void SetupBodyCreationSettings(JPH::BodyCreationSettings& body_creation_settings, const FRecallPhysicsBodyParameters& Params);
	
protected:
	TSharedPtr<JPH::BodyID> body_id;

private:
	TWeakPtr<JPH::TempAllocator> temp_allocator;
	TWeakPtr<JPH::PhysicsSystem> physics_system;
#endif // WITH_JOLT_PHYSICS
};
