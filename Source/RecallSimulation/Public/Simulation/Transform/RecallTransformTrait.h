// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityTraitBase.h"

#include "RecallTransformTrait.generated.h"

/*
* Trait to define a transform that can be set from a static object.
*/
UCLASS(meta=(DisplayName="MS Transform"))
class RECALLSIMULATION_API URecallTransformTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

	URecallTransformTrait();

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override final;

protected:
	UPROPERTY(EditAnywhere)
	bool bUseActorRotation = true;
};
