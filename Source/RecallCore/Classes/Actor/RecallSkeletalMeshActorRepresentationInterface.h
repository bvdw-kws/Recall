// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallSkeletalMeshActorRepresentationInterface.generated.h"

class USkeletalMeshComponent;

/**
* Interface for actors that holds a skeletal mesh.
*/
UINTERFACE()
class RECALLCORE_API URecallSkeletalMeshActorRepresentationInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallSkeletalMeshActorRepresentationInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	USkeletalMeshComponent* GetSkeletalMeshComponent() const;
};
