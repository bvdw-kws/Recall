// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

#include "RecallActorRepresentationTypes.generated.h"

class URecallActorRepresentationPoolFactory;

/**
 * Base class to describe an actor representation.
 */
USTRUCT()
struct RECALLSIMULATION_API FRecallActorRepresentationDesc
{
	GENERATED_BODY()
	
	virtual ~FRecallActorRepresentationDesc() = default;
	FRecallActorRepresentationDesc() = default;
	FRecallActorRepresentationDesc(const TSubclassOf<URecallActorRepresentationPoolFactory>& InFactoryClass);
	
	UPROPERTY()
	TSubclassOf<URecallActorRepresentationPoolFactory> FactoryClass;

	/**
	 * Get the soft class path for async loading of the actor class.
	 * This ensures consistent pool naming across client/server even when classes aren't loaded yet.
	 */
	virtual FSoftClassPath GetSoftClassPath() const { return FSoftClassPath(); }
};
