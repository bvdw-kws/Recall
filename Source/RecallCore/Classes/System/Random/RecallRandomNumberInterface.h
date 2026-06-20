// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallRandomNumberInterface.generated.h"

UINTERFACE()
class RECALLCORE_API URecallRandomNumberInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallRandomNumberInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual int32 GetSeed() const = 0;
	virtual void SetSeed(int32 InSeed) = 0;

};
