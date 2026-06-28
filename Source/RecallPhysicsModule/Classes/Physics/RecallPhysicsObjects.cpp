// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsObjects.h"

#include "Utility/Math/RecallMathUtils.h"

DEFINE_LOG_CATEGORY(LogRecallPhysicsObject);

void FRecallPhysicsBody::AddLinearVelocity(const FVector& LinearVelocity)
{
	AddLinearVelocityPerSecond(Recall::Math::Utils::UnitsPerFrameToPerSecond(LinearVelocity));
}

void FRecallPhysicsBody::AddLinearAndAngularVelocity(const FVector& LinearVelocity, const FVector& AngularVelocity)
{
	AddLinearAndAngularVelocityPerSecond(
		Recall::Math::Utils::UnitsPerFrameToPerSecond(LinearVelocity),
		Recall::Math::Utils::UnitsPerFrameToPerSecond(AngularVelocity));
}

void FRecallPhysicsBody::SetLinearAndAngularVelocity(const FVector& LinearVelocity, const FVector& AngularVelocity)
{
	SetLinearAndAngularVelocityPerSecond(
		Recall::Math::Utils::UnitsPerFrameToPerSecond(LinearVelocity),
		Recall::Math::Utils::UnitsPerFrameToPerSecond(AngularVelocity));
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
	SetLinearVelocityPerSecond(Recall::Math::Utils::UnitsPerFrameToPerSecond(LinearVelocity));
}

FVector FRecallPhysicsBody::GetLinearVelocity() const
{
	return Recall::Math::Utils::UnitsPerSecondToPerFrame(GetLinearVelocityPerSecond());
}

FVector2D FRecallPhysicsBody::GetLinearVelocity2D() const
{
	const FVector Velocity = GetLinearVelocity();
	return static_cast<FVector2D>(Velocity);
}

void FRecallPhysicsBody::SetAngularVelocity(const FVector& AngularVelocity)
{
	SetAngularVelocityPerSecond(Recall::Math::Utils::UnitsPerFrameToPerSecond(AngularVelocity));
}

FVector FRecallPhysicsBody::GetAngularVelocity() const
{
	return Recall::Math::Utils::UnitsPerSecondToPerFrame(GetAngularVelocityPerSecond());
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
