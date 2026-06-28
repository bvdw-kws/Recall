// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRecallPhysicsObject, Log, All);

class FJPRPhysicsBody;

/** Non-owning, value-type Recall view over a const body owned by the physics subsystem. */
class RECALLPHYSICSMODULE_API FConstRecallPhysicsBodyView
{
public:
	FConstRecallPhysicsBodyView() = default;
	FConstRecallPhysicsBodyView(std::nullptr_t) {}
	explicit FConstRecallPhysicsBodyView(const TWeakPtr<FJPRPhysicsBody>& InBody);

	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }
	TSharedPtr<const FJPRPhysicsBody> Pin() const;

	template<typename TBody = FJPRPhysicsBody>
	TWeakPtr<const TBody> GetBody() const
	{
		return StaticCastWeakPtr<const TBody>(Body);
	}

	FVector GetLinearVelocity() const;
	FVector GetAngularVelocity() const;
	FVector2D GetLinearVelocity2D() const;
	FVector GetMovementForwardVector() const;
	void GetPosition(FVector& OutPosition) const;
	void GetRotation(FQuat& OutRotation) const;
	void GetPositionAndRotation(FVector& OutPosition, FQuat& OutRotation) const;

protected:
	TWeakPtr<FJPRPhysicsBody> Body;
};

/** Mutable extension of the const Recall body view. */
class RECALLPHYSICSMODULE_API FRecallPhysicsBodyView : public FConstRecallPhysicsBodyView
{
public:
	FRecallPhysicsBodyView() = default;
	FRecallPhysicsBodyView(std::nullptr_t) : FConstRecallPhysicsBodyView(nullptr) {}
	explicit FRecallPhysicsBodyView(const TWeakPtr<FJPRPhysicsBody>& InBody);

	TSharedPtr<FJPRPhysicsBody> Pin() const;

	template<typename TBody = FJPRPhysicsBody>
	TWeakPtr<TBody> GetBody() const
	{
		return StaticCastWeakPtr<TBody>(Body);
	}

	void AddLinearVelocity(const FVector& Velocity) const;
	void AddLinearAndAngularVelocity(const FVector& Linear, const FVector& Angular) const;
	void SetLinearAndAngularVelocity(const FVector& Linear, const FVector& Angular) const;
	void SetLinearVelocity(const FVector& Velocity) const;
	void SetAngularVelocity(const FVector& Velocity) const;
	void AddImpulse(const FVector& ForceNewton, bool bIgnoreMass = true) const;
	void SetLinearVelocityFromImpulse(const FVector& ForceNewton, bool bIgnoreMass = true) const;
	void SetLinearVelocity2DFromImpulse(const FVector& ForceNewton, bool bIgnoreMass = true) const;
	void SetLinearVelocity2D(const FVector2D& Velocity) const;
	void SetLinearZVelocity(float Velocity) const;
	void SetPosition(const FVector& Position) const;
	void SetRotation(const FQuat& Rotation) const;
	void SetRotation(const FRotator& Rotation) const;
	void SetPositionAndRotation(const FVector& Position, const FQuat& Rotation) const;
};
