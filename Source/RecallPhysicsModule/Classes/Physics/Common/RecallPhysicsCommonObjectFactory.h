// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Physics/JPRPhysicsObjectFactory.h"

#include "RecallPhysicsCommonObjectFactory.generated.h"

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsBoxObjectFactory : public UJPRPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FJPRPhysicsBody> BuildPhysicsObject(UObject* Outer,
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsSphereObjectFactory : public UJPRPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FJPRPhysicsBody> BuildPhysicsObject(UObject* Outer,
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsCapsuleObjectFactory : public UJPRPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FJPRPhysicsBody> BuildPhysicsObject(UObject* Outer,
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsMeshObjectFactory : public UJPRPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FJPRPhysicsBody> BuildPhysicsObject(UObject* Outer,
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsConvexHullObjectFactory : public UJPRPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FJPRPhysicsBody> BuildPhysicsObject(UObject* Outer, 
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsHeightFieldObjectFactory : public UJPRPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FJPRPhysicsBody> BuildPhysicsObject(UObject* Outer, 
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsStaticCompoundObjectFactory : public UJPRPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FJPRPhysicsBody> BuildPhysicsObject(UObject* Outer, 
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const override;
};
