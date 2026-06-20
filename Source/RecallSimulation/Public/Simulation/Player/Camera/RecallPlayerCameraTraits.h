// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "RecallPlayerCameraTypes.h"

#include "RecallPlayerCameraTraits.generated.h"

UCLASS(meta=(DisplayName="MS Player Camera"))
class RECALLSIMULATION_API URecallPlayerCameraTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

protected:
	UPROPERTY(EditAnywhere, meta=(AllowedClasses="/Script/Engine.Actor"))
	FSoftClassPath CameraActorClass;

	UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties))
	FRecallPlayerCameraSettings CameraSettings;
};
