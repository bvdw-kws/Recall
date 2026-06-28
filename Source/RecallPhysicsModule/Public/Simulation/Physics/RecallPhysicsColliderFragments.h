// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Mass/EntityElementTypes.h"
#include "Physics/RecallPhysicsTypes.h"
#include "Physics/Common/RecallPhysicsCommonShapeTypes.h"

#include "RecallPhysicsColliderFragments.generated.h"

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsBoxShapeFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsBoxShape Shape;
	
	UPROPERTY(VisibleAnywhere)
	FJPRPhysicsBodyParameters Params;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsSphereShapeFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsSphereShape Shape;
	
	UPROPERTY(VisibleAnywhere)
	FJPRPhysicsBodyParameters Params;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsCapsuleFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsCapsuleShape Shape;
	
	UPROPERTY(VisibleAnywhere)
	FJPRPhysicsBodyParameters Params;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsMeshFragment : public FMassFragment
{
	GENERATED_BODY()
		
	UPROPERTY(VisibleAnywhere)
	FVector Scale = FVector::OneVector;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsMeshConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FVector3f> Vertices;

	UPROPERTY(VisibleAnywhere)
	TArray<int32> Triangles;
	
	UPROPERTY(VisibleAnywhere)
	FJPRPhysicsBodyParameters Params;

	UPROPERTY(VisibleAnywhere)
	bool bConvexHull = true;
	
	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsMeshShapeSettings MeshShapeSettings;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsHeightFieldConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsHeightFieldShape Shape;
	
	UPROPERTY(VisibleAnywhere)
	FVector Location = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere)
	FQuat Rotation = FQuat::Identity;
	
	UPROPERTY(VisibleAnywhere)
	FJPRPhysicsBodyParameters Params;
};
