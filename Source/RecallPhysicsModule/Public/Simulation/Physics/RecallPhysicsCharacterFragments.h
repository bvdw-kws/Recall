// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityElementTypes.h"
#include "Physics/Character/RecallPhysicsCharacterShapeTypes.h"
#include "Physics/Character/RecallPhysicsCharacterTypes.h"
#include "Physics/RecallPhysicsTypes.h"

#include "RecallPhysicsCharacterFragments.generated.h"

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsCharacterFragment :
	public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	bool bIsSupported = true;

	UPROPERTY(VisibleAnywhere)
	ERecallPhysicsCharacterGroundState GroundState = ERecallPhysicsCharacterGroundState::OnGround;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsCharacterShapeConstSharedFragment :
	public FMassConstSharedFragment
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsCharacter Shape;
	
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsBodyParameters Params;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsCharacterVirtualShapeConstSharedFragment :
	public FMassConstSharedFragment
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsCharacterVirtual Shape;
	
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsBodyParameters Params;
};
