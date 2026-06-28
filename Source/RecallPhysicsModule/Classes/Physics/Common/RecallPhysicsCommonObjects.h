// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Physics/RecallPhysicsObjects.h"

struct FRecallPhysicsCapsuleShape;

/**
* Wrapper Object for JPH::CapsuleShape.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsCapsuleBody : public FRecallPhysicsBody
{
public:
	void InitCapsule(float Radius, float HalfHeight, const FJPRPhysicsBodyParameters& Params, uint32 InBodyID, int32 Layer);
	void InitCapsule(const FRecallPhysicsCapsuleShape& CapsuleShape, const FJPRPhysicsBodyParameters& Params,
		uint32 InBodyID, int32 Layer);

public:
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const override;

};

/**
* Wrapper Object for JPH::SphereShape.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsSphereBody : public FRecallPhysicsBody
{
public:
	void InitSphere(float Radius, const FJPRPhysicsBodyParameters& Params, uint32 InBodyID, int32 Layer);

public:
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const override;

};

/**
* Wrapper Object for JPH::BoxShape.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsBoxBody : public FRecallPhysicsBody
{
public:
	void InitBox(const FVector& Extents, const FJPRPhysicsBodyParameters& Params, uint32 InBodyID, int32 Layer);

public:
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const override;

};

/**
* Wrapper Object for JPH::MeshShape.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsMeshBody : public FRecallPhysicsBody
{
public:
	void InitMesh(const struct FRecallPhysicsMeshShape& MeshShape, const FJPRPhysicsBodyParameters& Params,
		uint32 InBodyID, int32 Layer);

public:
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const override;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
private:
	TArray<FVector> world_vertices;
	TArray<int32> world_triangles;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
};

class RECALLPHYSICSMODULE_API FRecallPhysicsStaticCompoundBody : public FRecallPhysicsBody
{
public:
	void InitStaticCompound(const struct FRecallPhysicsStaticCompoundShape& StaticCompoundShape, const FJPRPhysicsBodyParameters& Params,
		uint32 InBodyID, int32 Layer);

public:
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const override;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
private:
	struct RecallPhysicsStaticCompoundMesh
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		FVector Position = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FBox BoundingBox;
	};
	TArray<RecallPhysicsStaticCompoundMesh> Meshes;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
};

/**
* Wrapper Object for JPH::ConvexHullShape.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsConvexHullBody : public FRecallPhysicsBody
{
public:
	void InitConvexHull(const TArray<FVector3f>& WorldVertices, const FJPRPhysicsBodyParameters& Params, uint32 InBodyID, int32 Layer);

public:
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const override;

};

/**
* Wrapper Object for JPH::HeightFieldShape.
*/
class RECALLPHYSICSMODULE_API FRecallPhysicsHeightFieldBody : public FRecallPhysicsBody
{
public:
	void InitHeightField(int32 SizeX, int32 SizeY, const TArray<float>& Heights, const FVector& Scale,
		const FJPRPhysicsBodyParameters& Params, uint32 InBodyID, int32 Layer);

public:
	virtual void DrawDebugShape(const UWorld* World, const FColor& Color) const override;

};
