// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallGameReactInterface.generated.h"

/**
* Observer interface to react to game events.
*/
UINTERFACE()
class RECALLCORE_API URecallGameReactInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallGameReactInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	/**
	 * Callback when the game simulation should end.
	 */
	virtual void OnGameEnd(const FString& Reason) {}
	
	/**
	 * Callback when a controller is defeated.
	 */
	virtual void OnControllerDefeat(const FString& ControllerID, const FString& Reason) {}

};
