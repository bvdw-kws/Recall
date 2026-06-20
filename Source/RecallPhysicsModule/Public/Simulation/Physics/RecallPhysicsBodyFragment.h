// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityElementTypes.h"
#include "Physics/RecallPhysicsBodyHandle.h"
#include "Physics/RecallPhysicsTypes.h"

#include "RecallPhysicsBodyFragment.generated.h"

// Tag to know if physics body has been initialized
USTRUCT() struct RECALLPHYSICSMODULE_API FRecallPhysicsBodyInitializedTransformTag : public FMassTag { GENERATED_BODY() };

// Tag to identify static colliders
USTRUCT() struct RECALLPHYSICSMODULE_API FRecallPhysicsStaticColliderTag : public FMassTag { GENERATED_BODY() };

// Tag to identify entities that should generate hit events
USTRUCT() struct RECALLPHYSICSMODULE_API FRecallPhysicsGeneratesHitEventTag : public FMassTag { GENERATED_BODY() };

/**
 * Physics body start parameters.
 */
UENUM(meta=(BitFlags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class ERecallPhysicsBodyStartParameters : uint8
{
	None					= 0,
	GeneratesHitEvent		= 1 << 0,
	StartsEnabled			= 1 << 1,
};
ENUM_CLASS_FLAGS(ERecallPhysicsBodyStartParameters)

constexpr bool EnumHasAnyFlags(uint8 Flags, ERecallPhysicsBodyStartParameters Contains) { return (Flags & static_cast<uint8>(Contains)) != 0; }
inline uint8& operator|=(uint8& Lhs, ERecallPhysicsBodyStartParameters Rhs) { return Lhs |= static_cast<uint8>(Rhs); }

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsBodyFragment : public FMassFragment
{
	GENERATED_BODY()

	/**
	 * Handle to reference the physics body stored in the physics system.
	 */
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsBodyHandle BodyHandle;

	/**
	 * Extents of our collider
	 */
	UPROPERTY(VisibleAnywhere)
	FVector Extents = FVector::ZeroVector;

	/**
	 * Allow us to set the entity initial velocity during creation.
	 */
	UPROPERTY(VisibleAnywhere)
	FVector StartVelocity = FVector::ZeroVector;

	/**
	 * Parameters to be applied when the physics body is initialized.
	 */
	UPROPERTY(VisibleAnywhere, meta=(Bitmask, BitmaskEnum="/Script/RecallPhysicsModule.ERecallPhysicsBodyStartParameters"))
	uint8 StartParameters = 0;
	
	UPROPERTY(VisibleAnywhere, meta=(Bitmask, BitmaskEnum="/Script/RecallPhysicsModule.ERecallPhysicsTransformCopyParameters"))
	uint8 TransformCopy = static_cast<uint8>(ERecallPhysicsTransformCopyParameters::All);	

	FORCEINLINE bool ShouldStartEnabled() const
	{
		return EnumHasAnyFlags(StartParameters, ERecallPhysicsBodyStartParameters::StartsEnabled);
	}

	FORCEINLINE bool ShouldGenerateHitEvent() const
	{
		return EnumHasAnyFlags(StartParameters, ERecallPhysicsBodyStartParameters::GeneratesHitEvent);
	}
	
	FORCEINLINE bool ShouldCopyPositionAndRotation() const
	{
		return EnumHasAnyFlags(TransformCopy, ERecallPhysicsTransformCopyParameters::CopyLocationAndRotation);
	}
	
	FORCEINLINE bool ShouldCopyPosition() const
	{
		return EnumHasAnyFlags(TransformCopy, ERecallPhysicsTransformCopyParameters::CopyLocation);
	}
	
	FORCEINLINE bool ShouldCopyRotation() const
	{
		return EnumHasAnyFlags(TransformCopy, ERecallPhysicsTransformCopyParameters::CopyRotation);
	}
};
