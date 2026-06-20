// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Physics/RecallPhysicsShapeTypes.h"

#include "RecallPhysicsCharacterShapeTypes.generated.h"

USTRUCT(BlueprintType)
struct RECALLPHYSICSMODULE_API FRecallPhysicsCharacterCapsule
{
	GENERATED_BODY()
	
	FRecallPhysicsCharacterCapsule();
	FRecallPhysicsCharacterCapsule(float InRadius, float InHalfHeightOfCylinder);
	
	/**
	 * Radius of the capsule.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.0, Units=Centimeters))
	float Radius{ 40.0f };

	/**
	 * Capsule half-height minus the radius.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.0, Units=Centimeters), DisplayName="HalfHeightOfCylinder")
	float HalfHeightOfCylinder{ 52.0f };
};

USTRUCT(BlueprintType)
struct RECALLPHYSICSMODULE_API FRecallPhysicsCharacter : public FRecallPhysicsShape
{
	GENERATED_BODY()
	
	FRecallPhysicsCharacter();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRecallPhysicsCharacterCapsule Capsule;
	
	/**
	 * Scan for ground below the character to update its ground state, but can be performance heavy.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseGroundState = false;
	
	/**
	 * Max distance between the floor and the character to still consider the character standing on the floor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSeparationDistance = 10.0f;
};


USTRUCT(BlueprintType)
struct RECALLPHYSICSMODULE_API FRecallPhysicsCharacterVirtual : public FRecallPhysicsShape
{
	GENERATED_BODY()
	
	FRecallPhysicsCharacterVirtual();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRecallPhysicsCharacterCapsule StandingCapsule = { 40.0f, 52.0f };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRecallPhysicsCharacterCapsule CrouchingCapsule = { 40.0f, 17.0f };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.0, ClampMax=1.0))
	float InnerShapeFraction = 0.9f;
};
