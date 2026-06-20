// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsCommonShapeTypes.h"

#include "Physics/Common/RecallPhysicsCommonObjectFactory.h"

FRecallPhysicsBoxShape::FRecallPhysicsBoxShape()
	: Super(URecallPhysicsBoxObjectFactory::StaticClass())
{
}

FRecallPhysicsBoxShape::FRecallPhysicsBoxShape(const FVector& InExtents)
	: Super(URecallPhysicsBoxObjectFactory::StaticClass())
	, WorldExtents(InExtents)
{
}

FRecallPhysicsSphereShape::FRecallPhysicsSphereShape()
	: Super(URecallPhysicsSphereObjectFactory::StaticClass())
{
}

FRecallPhysicsSphereShape::FRecallPhysicsSphereShape(float Radius)
	: Super(URecallPhysicsSphereObjectFactory::StaticClass())
	, RadiusCentimeters(Radius)
{
}

FRecallPhysicsCapsuleShape::FRecallPhysicsCapsuleShape()
	: Super(URecallPhysicsCapsuleObjectFactory::StaticClass())
{
}

FRecallPhysicsCapsuleShape::FRecallPhysicsCapsuleShape(float Radius, float HalfHeight)
	: Super(URecallPhysicsCapsuleObjectFactory::StaticClass())
	, RadiusCentimeters(Radius)
	, HalfHeightCentimeters(HalfHeight)
{
}

FRecallPhysicsMeshShape::FRecallPhysicsMeshShape()
	: Super(URecallPhysicsMeshObjectFactory::StaticClass())
{
}

FRecallPhysicsMeshShape::FRecallPhysicsMeshShape(const TArray<FVector3f>& InVertices, const TArray<int32>& InTriangles)
	: Super(URecallPhysicsMeshObjectFactory::StaticClass())
	, Vertices(InVertices)
	, Triangles(InTriangles)
{
}

FRecallPhysicsConvexHullShape::FRecallPhysicsConvexHullShape()
	: Super(URecallPhysicsConvexHullObjectFactory::StaticClass())
{
}

FRecallPhysicsConvexHullShape::FRecallPhysicsConvexHullShape(const TArray<FVector3f>& InVertices)
	: Super(URecallPhysicsConvexHullObjectFactory::StaticClass())
	, Vertices(InVertices)
{
}

FRecallPhysicsHeightFieldShape::FRecallPhysicsHeightFieldShape()
	: Super(URecallPhysicsHeightFieldObjectFactory::StaticClass())
{
}

FRecallPhysicsHeightFieldShape::FRecallPhysicsHeightFieldShape(int32 InSizeX, int32 InSizeY,
	const TArray<float>& InHeights, const FVector& InScale)
	: Super(URecallPhysicsHeightFieldObjectFactory::StaticClass())
	, SizeX(InSizeX)
	, SizeY(InSizeY)
	, Heights(InHeights)
	, Scale(InScale)
{
}

FRecallPhysicsStaticCompoundShape::FRecallPhysicsStaticCompoundShape()
	: Super(URecallPhysicsStaticCompoundObjectFactory::StaticClass())
{
}
