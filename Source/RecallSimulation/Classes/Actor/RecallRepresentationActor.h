// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "GameFramework/Actor.h"

#include "RecallRepresentationActor.generated.h"

/**
 * Base class for an actor representation.
 */
UCLASS(Abstract, Blueprintable, DisplayName="MS Actor Representation", ComponentWrapperClass)
class RECALLSIMULATION_API ARecallRepresentationActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category=Representation)
	void SetControlRotationRepresentation(const FQuat& ControlRotation);
};
