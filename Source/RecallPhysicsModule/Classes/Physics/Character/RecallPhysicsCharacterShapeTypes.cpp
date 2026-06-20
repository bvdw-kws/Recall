// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsCharacterShapeTypes.h"

#include "RecallPhysicsCharacterObjectFactory.h"

FRecallPhysicsCharacterCapsule::FRecallPhysicsCharacterCapsule()
{
}

FRecallPhysicsCharacterCapsule::FRecallPhysicsCharacterCapsule(float InRadius, float InHalfHeightOfCylinder)
	: Radius(InRadius)
	, HalfHeightOfCylinder(InHalfHeightOfCylinder)
{
}

FRecallPhysicsCharacter::FRecallPhysicsCharacter()
	: Super(URecallPhysicsCharacterObjectFactory::StaticClass())
{
}

FRecallPhysicsCharacterVirtual::FRecallPhysicsCharacterVirtual()
	: Super(URecallPhysicsCharacterVirtualObjectFactory::StaticClass())
{
}
