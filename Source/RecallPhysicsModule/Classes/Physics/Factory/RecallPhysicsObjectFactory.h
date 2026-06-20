// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "RecallPhysicsObjectFactory.generated.h"

class FRecallPhysicsBody;
struct FRecallPhysicsBodyParameters;

/**
 * Base class for physics object factory.
 * This class allows us to decouple physics object creation code from the physics system.
 * This way we can expand it from other modules to add new physics objects.
 */
UCLASS(Abstract, Within=RecallPhysicsSubsystem)
class RECALLPHYSICSMODULE_API URecallPhysicsObjectFactory : public UObject
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FRecallPhysicsBody> BuildPhysicsObject(
		uint32 BodyID, const FInstancedStruct& Shape, const FRecallPhysicsBodyParameters& Params) const;

protected:
	/**
	 * Initial setup of the physics body.
	 */
	void SetupPhysicsObject(const TSharedPtr<FRecallPhysicsBody>& Body) const;
};
