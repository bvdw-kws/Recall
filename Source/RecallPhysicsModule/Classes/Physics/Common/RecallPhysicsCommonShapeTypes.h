// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Physics/RecallPhysicsShapeTypes.h"

#include "RecallPhysicsCommonShapeTypes.generated.h"

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsBoxShape : public FRecallPhysicsShape
{
	GENERATED_BODY()

	FRecallPhysicsBoxShape();
	FRecallPhysicsBoxShape(const FVector& InExtents);

public:
	UPROPERTY(VisibleAnywhere)
	FVector WorldExtents = static_cast<FVector>(50.0);
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsSphereShape : public FRecallPhysicsShape
{
	GENERATED_BODY()
	
	FRecallPhysicsSphereShape();
	FRecallPhysicsSphereShape(float Radius);

public:
	UPROPERTY(VisibleAnywhere)
	float RadiusCentimeters = 100.0f;

	FORCEINLINE FVector GetSphereUnrealExtents() const { return FVector(RadiusCentimeters, RadiusCentimeters, RadiusCentimeters); }
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsCapsuleShape : public FRecallPhysicsShape
{
	GENERATED_BODY()
	
	FRecallPhysicsCapsuleShape();
	FRecallPhysicsCapsuleShape(float Radius, float HalfHeight);

public:
	UPROPERTY(VisibleAnywhere)
	float RadiusCentimeters{ 34.0f };

	UPROPERTY(VisibleAnywhere, DisplayName="HalfHeightOfCylinder")
	float HalfHeightCentimeters{ 88.0f };
};

USTRUCT(BlueprintType)
struct RECALLPHYSICSMODULE_API FRecallPhysicsMeshShapeSettings
{
	GENERATED_BODY()
	
	/**
	 * Maximum number of triangles in each leaf of the axis aligned box tree. This is a balance between memory and performance. Can be in the range [1, MeshShape::MaxTrianglesPerLeaf].
	 * Sensible values are between 4 (for better performance) and 8 (for less memory usage)
	 */
	UPROPERTY(EditAnywhere, meta=(ClampMin=1, ClampMax=8))
	int32 MaxTrianglesPerLeaf = 8;

	/**
	 * Threshold angle (if the angle between the two triangles is bigger than this, the edge is active, note that a concave edge is always inactive).
	 * Setting this value too small can cause ghost collisions with edges, setting it too big can cause depenetration artifacts (objects not depenetrating quickly).
	 * Valid ranges are between 0 degrees and 90 degrees. The default value is 5 degrees.
	 */
	UPROPERTY(EditAnywhere, meta=(Units=Degrees, ClampMin=0.0, ClampMax=90.0))
	float ActiveEdgeCosThresholdAngle = 5.0f;

	/**
	 * When true, we store the user data coming from Triangle::mUserData or IndexedTriangle::mUserData in the mesh shape.
	 * This can be used to store additional data like the original index of the triangle in the mesh.
	 * Can be retrieved using MeshShape::GetTriangleUserData.
	 * Turning this on increases the memory used by the MeshShape by roughly 25%.
	 */
	UPROPERTY(EditAnywhere)
	bool PerTriangleUserData = false;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsMeshShape : public FRecallPhysicsShape
{
	GENERATED_BODY()
	
	FRecallPhysicsMeshShape();
	FRecallPhysicsMeshShape(const TArray<FVector3f>& InVertices, const TArray<int32>& InTriangles);

public:
	UPROPERTY(VisibleAnywhere)
	TArray<FVector3f> Vertices;

	UPROPERTY(VisibleAnywhere)
	TArray<int32> Triangles;

	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsMeshShapeSettings MeshShapeSettings;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsConvexHullShape : public FRecallPhysicsShape
{
	GENERATED_BODY()

	FRecallPhysicsConvexHullShape();
	FRecallPhysicsConvexHullShape(const TArray<FVector3f>& InVertices);

public:
	UPROPERTY(VisibleAnywhere)
	TArray<FVector3f> Vertices;
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsHeightFieldShape : public FRecallPhysicsShape
{
	GENERATED_BODY()
	
	FRecallPhysicsHeightFieldShape();
	FRecallPhysicsHeightFieldShape(int32 InSizeX, int32 InSizeY, const TArray<float>& InHeights, const FVector& InScale);

public:
	UPROPERTY(VisibleAnywhere)
	int32 SizeX = 0;
	
	UPROPERTY(VisibleAnywhere)
	int32 SizeY = 0;
	
	UPROPERTY(VisibleAnywhere)
	TArray<float> Heights;
	
	UPROPERTY(VisibleAnywhere)
	FVector Scale = FVector::OneVector;

	FORCEINLINE bool IsValid() const { return Heights.Num() && SizeX > 0 && SizeY > 0 && Scale.SquaredLength() > 0.0f; }
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsStaticCompoundSubShape : public FRecallPhysicsShape
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FRecallPhysicsMeshShape MeshShape;
	
	UPROPERTY(VisibleAnywhere)
	FVector Position = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere)
	FQuat Rotation = FQuat::Identity;	
};

USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsStaticCompoundShape : public FRecallPhysicsShape
{
	GENERATED_BODY()

	FRecallPhysicsStaticCompoundShape();
	
	UPROPERTY(VisibleAnywhere)
	TArray<FRecallPhysicsStaticCompoundSubShape> SubShapes;
};
