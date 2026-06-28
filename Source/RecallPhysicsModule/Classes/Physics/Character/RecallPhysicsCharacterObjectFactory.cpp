// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsCharacterObjectFactory.h"

#include "Physics/JPRPhysicsLayerDataAsset.h"
#include "RecallPhysicsCharacterObject.h"
#include "RecallPhysicsCharacterShapeTypes.h"
#include "RecallPhysicsCharacterVirtualObject.h"
#include "Physics/RecallPhysicsTypes.h"

TSharedPtr<FJPRPhysicsBody> URecallPhysicsCharacterObjectFactory::BuildPhysicsObject(UObject* Outer,
		uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsCharacter& CharacterShape = Shape.Get<FRecallPhysicsCharacter>();
	const TSharedPtr<FRecallPhysicsCharacterBody> CharacterBody = MakeShared<FRecallPhysicsCharacterBody>();
	
	SetupPhysicsObject(Outer, CharacterBody);
	
	const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	CharacterBody->InitCharacter(CharacterShape, Params, BodyID, Layer);
	
	return CharacterBody;
}

TSharedPtr<FJPRPhysicsBody> URecallPhysicsCharacterVirtualObjectFactory::BuildPhysicsObject(UObject* Outer,
	uint32 BodyID, const FInstancedStruct& Shape, const FJPRPhysicsBodyParameters& Params) const
{
	const FRecallPhysicsCharacterVirtual& CharacterVirtualShape = Shape.Get<FRecallPhysicsCharacterVirtual>();
	const TSharedPtr<FRecallPhysicsCharacterVirtualBody> CharacterBody = MakeShared<FRecallPhysicsCharacterVirtualBody>();
	
	SetupPhysicsObject(Outer, CharacterBody);
	
	const int32 Layer = UJPRPhysicsLayerDataAsset::GetLayerIndex(Params.Layer);
	CharacterBody->InitCharacterVirtual(CharacterVirtualShape, Params, BodyID, Layer);
	
	return CharacterBody;
}
