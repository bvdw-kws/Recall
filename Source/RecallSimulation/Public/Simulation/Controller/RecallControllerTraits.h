// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"

#include "RecallControllerTraits.generated.h"

class UMassEntityConfigAsset;

UCLASS(meta=(DisplayName="MS Player Controller"))
class RECALLSIMULATION_API URecallPlayerControllerTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

protected:
	UPROPERTY(EditAnywhere, DisplayName="Camera Entity Config")
	TObjectPtr<UMassEntityConfigAsset> PlayerCameraEntityConfig;
};

UCLASS(meta=(DisplayName="MS AI Controller"))
class RECALLSIMULATION_API URecallAIControllerTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

protected:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMassEntityConfigAsset> CameraEntityConfig;
};
