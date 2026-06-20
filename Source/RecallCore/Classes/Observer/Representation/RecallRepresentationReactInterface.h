// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallRepresentationReactInterface.generated.h"

/**
* Observer interface to react to game events.
*/
UINTERFACE()
class RECALLCORE_API URecallRepresentationReactInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallRepresentationReactInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	/**
	 * Callback when the representation of the game simulation is updated.
	 * Here it is safe to read the state of entities.
	 */
	virtual void OnRender() {}

};
