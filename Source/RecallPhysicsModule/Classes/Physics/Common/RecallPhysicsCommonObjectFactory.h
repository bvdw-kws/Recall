// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Physics/Factory/RecallPhysicsObjectFactory.h"

#include "RecallPhysicsCommonObjectFactory.generated.h"

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsBoxObjectFactory : public URecallPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsSphereObjectFactory : public URecallPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsCapsuleObjectFactory : public URecallPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsMeshObjectFactory : public URecallPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsConvexHullObjectFactory : public URecallPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsHeightFieldObjectFactory : public URecallPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsStaticCompoundObjectFactory : public URecallPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const override;
};
