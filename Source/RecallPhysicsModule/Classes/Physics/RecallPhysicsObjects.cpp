// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsObjects.h"

#include "Physics/JPRPhysicsBody.h"
#include "Utility/Math/RecallMathUtils.h"

DEFINE_LOG_CATEGORY(LogRecallPhysicsObject);

namespace
{
FVector ToPerSecond(const FVector& Value)
{
	return Recall::Math::Utils::UnitsPerFrameToPerSecond(Value);
}

FVector ToPerFrame(const FVector& Value)
{
	return Recall::Math::Utils::UnitsPerSecondToPerFrame(Value);
}
}

//----------------------------------------------------------------------//
// FConstRecallPhysicsBodyView
//----------------------------------------------------------------------//
FConstRecallPhysicsBodyView::FConstRecallPhysicsBodyView(const TWeakPtr<FJPRPhysicsBody>& InBody) : Body(InBody)
{
}

bool FConstRecallPhysicsBodyView::IsValid() const
{
	return Body.IsValid();
}

TSharedPtr<const FJPRPhysicsBody> FConstRecallPhysicsBodyView::Pin() const
{
	return Body.Pin();
}

FVector FConstRecallPhysicsBodyView::GetLinearVelocity() const
{
	return ToPerFrame(Pin()->GetLinearVelocityPerSecond());
}

FVector FConstRecallPhysicsBodyView::GetAngularVelocity() const
{
	return ToPerFrame(Pin()->GetAngularVelocityPerSecond());
}

FVector2D FConstRecallPhysicsBodyView::GetLinearVelocity2D() const
{
	return static_cast<FVector2D>(GetLinearVelocity());
}

FVector FConstRecallPhysicsBodyView::GetMovementForwardVector() const
{
	const FVector Velocity = GetLinearVelocity();
	return Velocity.IsNearlyZero() ? Pin()->GetForwardVector() : Velocity.GetSafeNormal();
}

void FConstRecallPhysicsBodyView::GetPosition(FVector& OutPosition) const
{
	Pin()->GetPosition(OutPosition);
}

void FConstRecallPhysicsBodyView::GetRotation(FQuat& OutRotation) const
{
	Pin()->GetRotation(OutRotation);
}

void FConstRecallPhysicsBodyView::GetPositionAndRotation(FVector& OutPosition, FQuat& OutRotation) const
{
	Pin()->GetPositionAndRotation(OutPosition, OutRotation);
}

//----------------------------------------------------------------------//
// FRecallPhysicsBodyView
//----------------------------------------------------------------------//
FRecallPhysicsBodyView::FRecallPhysicsBodyView(const TWeakPtr<FJPRPhysicsBody>& InBody) : FConstRecallPhysicsBodyView(InBody)
{
}

TSharedPtr<FJPRPhysicsBody> FRecallPhysicsBodyView::Pin() const
{
	return ConstCastSharedPtr<FJPRPhysicsBody>(Body.Pin());
}

void FRecallPhysicsBodyView::AddLinearVelocity(const FVector& Velocity) const
{
	Pin()->AddLinearVelocityPerSecond(ToPerSecond(Velocity));
}

void FRecallPhysicsBodyView::AddLinearAndAngularVelocity(const FVector& Linear, const FVector& Angular) const
{
	Pin()->AddLinearAndAngularVelocityPerSecond(ToPerSecond(Linear), ToPerSecond(Angular));
}

void FRecallPhysicsBodyView::SetLinearAndAngularVelocity(const FVector& Linear, const FVector& Angular) const
{
	Pin()->SetLinearAndAngularVelocityPerSecond(ToPerSecond(Linear), ToPerSecond(Angular));
}

void FRecallPhysicsBodyView::SetLinearVelocity(const FVector& Velocity) const
{
	Pin()->SetLinearVelocityPerSecond(ToPerSecond(Velocity));
}

void FRecallPhysicsBodyView::SetAngularVelocity(const FVector& Velocity) const
{
	Pin()->SetAngularVelocityPerSecond(ToPerSecond(Velocity));
}

void FRecallPhysicsBodyView::AddImpulse(const FVector& ForceNewton, bool bIgnoreMass) const
{
	const double Mass = bIgnoreMass ? 1.0 : Pin()->GetMass();
	AddLinearVelocity(ToPerFrame(ForceNewton) * 100.0 / Mass);
}

void FRecallPhysicsBodyView::SetLinearVelocityFromImpulse(const FVector& ForceNewton, bool bIgnoreMass) const
{
	const double Mass = bIgnoreMass ? 1.0 : Pin()->GetMass();
	SetLinearVelocity(ToPerFrame(ForceNewton) * 100.0 / Mass);
}

void FRecallPhysicsBodyView::SetLinearVelocity2DFromImpulse(const FVector& ForceNewton, bool bIgnoreMass) const
{
	const double Mass = bIgnoreMass ? 1.0 : Pin()->GetMass();
	const FVector Velocity = ToPerFrame(ForceNewton) * 100.0 / Mass;
	SetLinearVelocity2D(FVector2D(Velocity.X, Velocity.Y));
}

void FRecallPhysicsBodyView::SetLinearVelocity2D(const FVector2D& Velocity) const
{
	FVector Current = GetLinearVelocity();
	Current.X = Velocity.X;
	Current.Y = Velocity.Y;
	SetLinearVelocity(Current);
}

void FRecallPhysicsBodyView::SetLinearZVelocity(float Velocity) const
{
	FVector Current = GetLinearVelocity();
	Current.Z = Velocity;
	SetLinearVelocity(Current);
}

void FRecallPhysicsBodyView::SetPosition(const FVector& Position) const
{
	Pin()->SetPosition(Position);
}

void FRecallPhysicsBodyView::SetRotation(const FQuat& Rotation) const
{
	Pin()->SetRotation(Rotation);
}

void FRecallPhysicsBodyView::SetRotation(const FRotator& Rotation) const
{
	Pin()->SetRotation(Rotation);
}

void FRecallPhysicsBodyView::SetPositionAndRotation(const FVector& Position, const FQuat& Rotation) const
{
	Pin()->SetPositionAndRotation(Position, Rotation);
}
