// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsCommonObjectFactory.h"

#include "Physics/JPRPhysicsLayerDataAsset.h"
#include "RecallPhysicsCommonObjects.h"
#include "RecallPhysicsCommonShapeTypes.h"
#include "Physics/RecallPhysicsTypes.h"

TSharedPtr<FRecallPhysicsBody> URecallPhysicsBoxObjectFactory::BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsBoxShape& BoxShape = Shape.Get<FRecallPhysicsBoxShape>();
	const TSharedPtr<FRecallPhysicsBoxBody> BoxBody = MakeShared<FRecallPhysicsBoxBody>();
	
	SetupPhysicsObject(BoxBody);
	
	const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	BoxBody->InitBox(BoxShape.WorldExtents, Params, BodyID, Layer);
	
	return BoxBody;
}

TSharedPtr<FRecallPhysicsBody> URecallPhysicsSphereObjectFactory::BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsSphereShape& SphereShape = Shape.Get<FRecallPhysicsSphereShape>();
	const TSharedPtr<FRecallPhysicsSphereBody> SphereBody = MakeShared<FRecallPhysicsSphereBody>();
	
	SetupPhysicsObject(SphereBody);
	
	const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	SphereBody->InitSphere(SphereShape.RadiusCentimeters, Params, BodyID, Layer);
	
	return SphereBody;
}

TSharedPtr<FRecallPhysicsBody> URecallPhysicsCapsuleObjectFactory::BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsCapsuleShape& CapsuleShape = Shape.Get<FRecallPhysicsCapsuleShape>();
	const TSharedPtr<FRecallPhysicsCapsuleBody> CapsuleBody = MakeShared<FRecallPhysicsCapsuleBody>();
	
	SetupPhysicsObject(CapsuleBody);
	
	const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	CapsuleBody->InitCapsule(CapsuleShape.RadiusCentimeters, CapsuleShape.HalfHeightCentimeters, Params, BodyID, Layer);
	
	return CapsuleBody;
}

TSharedPtr<FRecallPhysicsBody> URecallPhysicsMeshObjectFactory::BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsMeshShape& MeshShape = Shape.Get<FRecallPhysicsMeshShape>();
	const TSharedPtr<FRecallPhysicsMeshBody> MeshBody = MakeShared<FRecallPhysicsMeshBody>();
	
	SetupPhysicsObject(MeshBody);
	
	const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	MeshBody->InitMesh(MeshShape, Params, BodyID, Layer);
	
	return MeshBody;
}

TSharedPtr<FRecallPhysicsBody> URecallPhysicsConvexHullObjectFactory::BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsConvexHullShape& ConvexHullShape = Shape.Get<FRecallPhysicsConvexHullShape>();
	const TSharedPtr<FRecallPhysicsConvexHullBody> ConvexHullBody = MakeShared<FRecallPhysicsConvexHullBody>();
	
	SetupPhysicsObject(ConvexHullBody);
	
	const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	ConvexHullBody->InitConvexHull(ConvexHullShape.Vertices, Params, BodyID, Layer);
	
	return ConvexHullBody;
}

TSharedPtr<FRecallPhysicsBody> URecallPhysicsHeightFieldObjectFactory::BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsHeightFieldShape& HeightFieldShape = Shape.Get<FRecallPhysicsHeightFieldShape>();
	const TSharedPtr<FRecallPhysicsHeightFieldBody> HeightFieldBody = MakeShared<FRecallPhysicsHeightFieldBody>();
	
	SetupPhysicsObject(HeightFieldBody);
	
	const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	HeightFieldBody->InitHeightField(HeightFieldShape.SizeX, HeightFieldShape.SizeY, HeightFieldShape.Heights,
		HeightFieldShape.Scale, Params, BodyID, Layer);
	
	return HeightFieldBody;
}

TSharedPtr<FRecallPhysicsBody> URecallPhysicsStaticCompoundObjectFactory::BuildPhysicsObject(uint32 BodyID,
	const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsStaticCompoundShape& StaticCompoundShape = Shape.Get<FRecallPhysicsStaticCompoundShape>();
	const TSharedPtr<FRecallPhysicsStaticCompoundBody> StaticCompoundBody = MakeShared<FRecallPhysicsStaticCompoundBody>();
	
	SetupPhysicsObject(StaticCompoundBody);
	
	const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	StaticCompoundBody->InitStaticCompound(StaticCompoundShape, Params, BodyID, Layer);
	
	return StaticCompoundBody;
}
