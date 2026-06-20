// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

#include "RecallPhysicsShapeTypes.generated.h"

class URecallPhysicsObjectFactory;

/**
 * Base struct of a physics shape that can be used to build a physics object.
 */
USTRUCT()
struct RECALLPHYSICSMODULE_API FRecallPhysicsShape
{
	GENERATED_BODY()
	
	FRecallPhysicsShape() = default;
	FRecallPhysicsShape(const TSubclassOf<URecallPhysicsObjectFactory>& InFactoryClass);

	UPROPERTY(VisibleAnywhere)
	TSubclassOf<URecallPhysicsObjectFactory> FactoryClass;
};
