// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallActorGameplayTagInterface.generated.h"

/**
* Interface for actors who want to react to their entity's gameplay tags.
* This is only called if the entity has a gameplay tag trait attached.
*/
UINTERFACE()
class RECALLCORE_API URecallActorGameplayTagInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallActorGameplayTagInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	/**
	 * Called at every update so gameplay tags can be used to update the actor.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnRenderGameplayTags(const TMap<FGameplayTag, int32>& GameplayTagCountMap);
};
