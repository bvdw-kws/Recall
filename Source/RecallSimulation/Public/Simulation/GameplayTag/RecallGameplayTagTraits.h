// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "GameplayTagContainer.h"
#include "MassEntityTraitBase.h"

#include "RecallGameplayTagTraits.generated.h"

/*
* Trait to define an entity that can have a faction
*/
UCLASS(meta=(DisplayName="MS Tags"))
class RECALLSIMULATION_API URecallGameplayTagsTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

protected:
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer OwnedTags;
		
	UPROPERTY(EditAnywhere)
	TArray<FName> NameTags;
};
