#pragma once

class RecallActorPoolInterface
{
public:
	
};
// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallActorPoolInterface.generated.h"

/**
* Interface for actors to react to the actor pooling system.
*/
UINTERFACE()
class RECALLCORE_API URecallActorPoolInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallActorPoolInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	/**
	 * Called when the pool enables this actor.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnPoolEnable();
	
	/**
	 * Called when the pool disables this actor.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnPoolDisable();
};
