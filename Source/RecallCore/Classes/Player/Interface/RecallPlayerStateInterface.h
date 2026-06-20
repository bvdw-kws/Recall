// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallPlayerStateInterface.generated.h"

/**
* Interface to access our player ID
*/
UINTERFACE()
class RECALLCORE_API URecallPlayerStateInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallPlayerStateInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	/** Simulation Id to identify each player */
	virtual const FString& GetSimPlayerId() const = 0;

};
