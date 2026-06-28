// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Physics/RecallPhysicsVehicleTraits.h"

#include "MassEntityTemplateRegistry.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h"
#include "Simulation/Physics/RecallPhysicsVehicleFragments.h"

//----------------------------------------------------------------------//
// URecallVehicleCollisionTrait
//----------------------------------------------------------------------//
URecallVehicleCollisionTrait::URecallVehicleCollisionTrait()
	: Super()
{	
	Params.OverrideMassProperties = EJPRPhysicsOverrideMassProperties::CalculateInertia;
	Params.MassPropertiesOverride.Mass = 1500.0f;	
}

void URecallVehicleCollisionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	Super::BuildTemplate(BuildContext, World);

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.AddFragment<FRecallPhysicsVehicleFragment>();
		
	FRecallPhysicsVehicleConstSharedFragment VehicleConstSharedFragment;
	VehicleConstSharedFragment.Shape = Vehicle;
	VehicleConstSharedFragment.Params = Params;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(VehicleConstSharedFragment));
}
