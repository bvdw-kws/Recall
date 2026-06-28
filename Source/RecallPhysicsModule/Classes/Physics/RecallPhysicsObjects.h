// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Physics/JPRPhysicsBody.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRecallPhysicsObject, Log, All);

/**
* These Objects help us to keep track of Jolt Physics shape reference.
* They also give us some interface to work with Jolt Physics without including all the source files.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsBody : public FJPRPhysicsBody
{
public:
	FRecallPhysicsBody() = default;
	virtual ~FRecallPhysicsBody() override = default;

public:
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
	 * Gets the forward direction vector, accounting for any movement-specific rotation.
	 * @return Forward direction
	 * @note May differ from GetForwardVector in physics objects with special movement handling.
	 */
	virtual FVector GetMovementForwardVector() const;
};
