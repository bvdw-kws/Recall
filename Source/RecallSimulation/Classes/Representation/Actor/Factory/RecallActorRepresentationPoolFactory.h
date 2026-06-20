// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

#include "RecallActorRepresentationPoolFactory.generated.h"

class IRecallObjectPoolInterface;
struct FInstancedStruct;

/**
 * Base class for actor pool factory.
 * This class allows us to decouple pool creation code from the actor system.
 * This way we can expand it from other modules to add new actor pools.
 */
UCLASS(Abstract, Within=RecallActorSubsystem)
class RECALLSIMULATION_API URecallActorRepresentationPoolFactory : public UObject
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<IRecallObjectPoolInterface> BuildActorPool() const;
	virtual FName GetActorPoolName(const FInstancedStruct& Desc) const;
};
