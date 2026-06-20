// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Controller/RecallControllerTraits.h"

#include "MassEntityTemplateRegistry.h"
#include "Simulation/Controller/RecallControllerFragments.h"
#include "Simulation/Player/Input/RecallPlayerInputFragments.h"
#include "Simulation/Representation/RecallActorRepresentationFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"

//----------------------------------------------------------------------//
// URecallPlayerControllerTrait
//----------------------------------------------------------------------//
void URecallPlayerControllerTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.RequireFragment<FRecallTransformFragment>();
	BuildContext.RequireFragment<FRecallActorRepresentationFragment>();

	BuildContext.AddFragment<FRecallControllerFragment>();
	
	BuildContext.AddFragment<FRecallPlayerInputFragment>();
	BuildContext.AddTag<FRecallPlayerControllerTag>();

	FRecallPlayerControllerSharedFragment SharedFragment;
	SharedFragment.CameraEntityConfig = PlayerCameraEntityConfig;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(SharedFragment));
}

//----------------------------------------------------------------------//
// URecallAIControllerTrait
//----------------------------------------------------------------------//
void URecallAIControllerTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.AddFragment<FRecallControllerFragment>();

	FRecallPlayerControllerSharedFragment SharedFragment;
	SharedFragment.CameraEntityConfig = CameraEntityConfig;

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(SharedFragment));
}
