// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallPlayerControllerInterface.generated.h"

/**
* Interface to access our player ID
*/
UINTERFACE()
class RECALLCORE_API URecallPlayerControllerInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallPlayerControllerInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	/** Set Options for the next input, should be performed from outside the simulation */
	virtual void SetInputOptions(const FString& Options, bool bOverride = false) = 0;
	virtual const FString& GetInputOptions() const = 0;

};
