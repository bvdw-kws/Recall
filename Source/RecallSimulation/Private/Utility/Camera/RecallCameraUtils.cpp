// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Camera/RecallCameraUtils.h"

#include "MassEntityView.h"
#include "MassExecutionContext.h"
#include "Kismet/KismetMathLibrary.h"
#include "Simulation/Controller/RecallControllerFragments.h"
#include "Simulation/Player/Camera/RecallPlayerCameraFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "System/Entity/RecallEntitySubsystem.h"

namespace Recall::Camera::Utils
{
	
bool GetPlayerCameraTargetPosition(FMassExecutionContext& Context,
	const FRecallPlayerCameraSharedFragment& CameraSharedFragment, const FRecallPlayerCameraFragment& CameraFragment,
	FVector& OutPosition, FQuat& OutRotation)
{
	const URecallEntitySubsystem& EntitySystem = Context.GetSubsystemChecked<URecallEntitySubsystem>();
	const FMassEntityManager& EntityManager = Context.GetEntityManagerChecked();
	return GetPlayerCameraTargetPosition(EntitySystem, EntityManager, CameraSharedFragment, CameraFragment,
		OutPosition, OutRotation);
}
	
bool GetPlayerCameraTargetPosition(const URecallEntitySubsystem& EntitySystem, const FMassEntityManager& EntityManager,
	const FRecallPlayerCameraSharedFragment& CameraSharedFragment, const FRecallPlayerCameraFragment& CameraFragment,
	FVector& OutPosition, FQuat& OutRotation)
{
	FMassEntityHandle PlayerEntity;
	if (!EntitySystem.FindControllerOwnedEntity(CameraFragment.TargetPlayerID, PlayerEntity))
	{
		return false;
	}

	const FMassEntityView PlayerView(EntityManager, PlayerEntity);
	const FRecallTransformFragment& TransformFragment = PlayerView.GetFragmentData<FRecallTransformFragment>();

	FQuat WorldRotation = FQuat::Identity;

	if (CameraSharedFragment.Settings.bUseControlRotation)
	{
		const FRecallControllerFragment& ControllerFragment = PlayerView.GetFragmentData<FRecallControllerFragment>();
		WorldRotation = ControllerFragment.ControlRotation.Quaternion();
	}
	else if (CameraSharedFragment.Settings.bUsePawnRotation)
	{
		const FQuat TargetRotation = TransformFragment.Rotation;
		const FVector ForwardVector = TargetRotation.GetForwardVector();
		const FVector TargetRightVector = FVector::CrossProduct(TargetRotation.GetUpVector(), ForwardVector);
		
		WorldRotation = UKismetMathLibrary::MakeRotationFromAxes(ForwardVector, TargetRightVector, TargetRotation.GetUpVector()).Quaternion();
	}
	
	OutPosition = TransformFragment.GetTransform().TransformPosition(CameraSharedFragment.Settings.Offset.GetLocation());
	OutRotation = CameraSharedFragment.Settings.Offset.GetRotation() * WorldRotation;
	
	return true;
}
	
} // namespace Recall::Camera::Utils
