// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Sensor/RecallSensorTraits.h"

#include "MassEntityTemplateRegistry.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h"
#include "Simulation/Sensor/RecallSensorFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"

void URecallSensorTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.RequireFragment<FRecallTransformFragment>();
	BuildContext.RequireFragment<FRecallPhysicsBodyFragment>();
	BuildContext.RequireFragment<FRecallPhysicsSensorFragment>();

	BuildContext.AddFragment<FRecallSensorFragment>();

	TArray<FName> SensorNames;
	Sensors.GenerateKeyArray(SensorNames);

	FRecallSensorConstSharedFragment SharedFragment;
	SharedFragment.InstanceParameters.Reserve(Sensors.Num());

	for (int32 SensorIndex = 0; SensorIndex < Sensors.Num(); SensorIndex++)
	{
		const FName& SensorName = SensorNames[SensorIndex];
		if (!ensureMsgf(!SensorName.IsNone(), TEXT("Sensor must have a valid name")))
		{
			continue;
		}

		const FRecallSensorSettings& Settings = Sensors[SensorName];

		FJPRPhysicsBodyParameters Params;
		Params.MotionType = EJPRPhysicsMotionType::Dynamic;
		Params.Layer = Settings.SensorLayer;
		Params.bIsSensor = true;
		Params.GravityFactor = 0.0f;
		Params.OverrideMassProperties = EJPRPhysicsOverrideMassProperties::MassAndInertiaProvided;
		Params.MassPropertiesOverride.Mass = 0.001f;

		FRecallSensorInstanceParameters& Instance = SharedFragment.InstanceParameters.AddDefaulted_GetRef();
		Instance.SensorName = SensorName;
		Instance.Shape = FRecallPhysicsSphereShape{ Settings.SensorRange };
		Instance.Params = Params;
		Instance.Offset = Settings.CenterOffset;
		Instance.DebugActivatedColor = Settings.DebugActivatedColor;
		Instance.DebugOverlappingColor = Settings.DebugOverlappingColor;
		Instance.DebugDeactivatedColor = Settings.DebugDeactivatedColor;
	}

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(SharedFragment));
}

TArray<FName> URecallSensorTrait::GetSensorNames() const
{
	TArray<FName> Result;
	Sensors.GetKeys(Result);
	
	return Result;
}
