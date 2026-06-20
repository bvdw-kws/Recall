// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Player/Camera/RecallPlayerCameraTraits.h"

#include "MassEntityTemplateRegistry.h"
#include "Simulation/Player/Camera/RecallPlayerCameraFragments.h"
#include "Simulation/Representation/RecallActorRepresentationFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"

void URecallPlayerCameraTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.RequireFragment<FRecallTransformFragment>();

	BuildContext.AddFragment<FRecallPlayerCameraFragment>();

	FRecallPlayerCameraSharedFragment SharedFragment;
	SharedFragment.CameraActorClass = CameraActorClass;
	SharedFragment.Settings = CameraSettings;

	BuildContext.AddFragment<FRecallActorRepresentationFragment>();

	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(SharedFragment));
}
