// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassEntityTraitBase.h"

#include "RecallSensorTraits.generated.h"

USTRUCT()
struct FRecallSensorSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta=(Units="Centimeters"))
	float SensorRange = 100.0f;

	UPROPERTY(EditAnywhere, meta=(Units="Centimeters"))
	FVector CenterOffset = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, meta=(RowType="/Script/RecallCore.RecallPhysicsLayerTableRow"))
	FDataTableRowHandle SensorLayer;

	UPROPERTY(EditAnywhere)
	FColor DebugActivatedColor = FColor::Green;

	UPROPERTY(VisibleAnywhere)
	FColor DebugOverlappingColor = FColor::Purple;

	UPROPERTY(EditAnywhere)
	FColor DebugDeactivatedColor = FColor::Red;
};

/**
* Trait to attach a sensor collider to an entity
*/
UCLASS(meta=(DisplayName="MS Sensor"))
class RECALLPHYSICSMODULE_API URecallSensorTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

public:
	TArray<FName> GetSensorNames() const;
	
protected:
	UPROPERTY(EditAnywhere)
	TMap<FName, FRecallSensorSettings> Sensors;
};
