// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Physics/Factory/RecallPhysicsObjectFactory.h"

#include "RecallPhysicsCharacterObjectFactory.generated.h"

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsCharacterObjectFactory : public URecallPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const override;
};

UCLASS()
class RECALLPHYSICSMODULE_API URecallPhysicsCharacterVirtualObjectFactory : public URecallPhysicsObjectFactory
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const override;
};
