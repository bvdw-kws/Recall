// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPlayerCameraProcessors.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "MassExecutionContext.h"
#include "Representation/Actor/RecallActorMeshRepresentationTypes.h"
#include "Simulation/Player/Camera/RecallPlayerCameraFragments.h"
#include "Simulation/Representation/RecallActorRepresentationFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "System/Actor/RecallActorSubsystem.h"
#include "System/Entity/RecallEntitySubsystem.h"
#include "Utility/Camera/RecallCameraUtils.h"

//----------------------------------------------------------------------//
// URecallPlayerCameraConstructor
//----------------------------------------------------------------------//
URecallPlayerCameraConstructor::URecallPlayerCameraConstructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedType = FRecallPlayerCameraFragment::StaticStruct();
	Operation = EMassObservedOperation::Add;
}

void URecallPlayerCameraConstructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPlayerCameraConstructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{;
	EntityQuery.AddRequirement<FRecallActorRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FRecallPlayerCameraSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallActorSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPlayerCameraConstructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_PlayerCamera_Constructor);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallActorSubsystem& ActorSystem = Context.GetMutableSubsystemChecked<URecallActorSubsystem>();

		const FRecallPlayerCameraSharedFragment& CameraSharedFragment = Context.GetConstSharedFragment<FRecallPlayerCameraSharedFragment>();

		const TArrayView<FRecallActorRepresentationFragment> ActorList = Context.GetMutableFragmentView<FRecallActorRepresentationFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			FRecallActorRepresentationFragment& ActorFragment = ActorList[EntityIndex];

			FActorRepresentationDesc CameraActorDesc;
			CameraActorDesc.Blueprint = CameraSharedFragment.CameraActorClass;

			ActorFragment.ActorHandle = ActorSystem.CreateActor(CameraActorDesc);
		}
	});
}

//----------------------------------------------------------------------//
// URecallPlayerCameraDestructor
//----------------------------------------------------------------------//
URecallPlayerCameraDestructor::URecallPlayerCameraDestructor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedType = FRecallPlayerCameraFragment::StaticStruct();
	Operation = EMassObservedOperation::Remove;
}

void URecallPlayerCameraDestructor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPlayerCameraDestructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallActorRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FRecallPlayerCameraSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallActorSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPlayerCameraDestructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_PlayerCamera_Destructor);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallActorSubsystem& ActorSystem = Context.GetMutableSubsystemChecked<URecallActorSubsystem>();

		const FRecallPlayerCameraSharedFragment& CameraSharedFragment = Context.GetConstSharedFragment<FRecallPlayerCameraSharedFragment>();

		const TArrayView<FRecallActorRepresentationFragment> ActorList = Context.GetMutableFragmentView<FRecallActorRepresentationFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			FRecallActorRepresentationFragment& ActorFragment = ActorList[EntityIndex];

			ActorSystem.ReleaseActor(ActorFragment.ActorHandle);
		}
	});
}

//----------------------------------------------------------------------//
// URecallPlayerCameraFollowProcessor
//----------------------------------------------------------------------//
URecallPlayerCameraFollowProcessor::URecallPlayerCameraFollowProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::FrameEnd;
}

void URecallPlayerCameraFollowProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPlayerCameraFollowProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) 
{
	EntityQuery.AddRequirement<FRecallTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRecallPlayerCameraFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddConstSharedRequirement<FRecallPlayerCameraSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallEntitySubsystem>(EMassFragmentAccess::ReadOnly);
}

void URecallPlayerCameraFollowProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_PlayerCameraFollow_Execute);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const FRecallPlayerCameraSharedFragment& CameraSharedFragment = Context.GetConstSharedFragment<FRecallPlayerCameraSharedFragment>();

		const TConstArrayView<FRecallPlayerCameraFragment> CameraList = Context.GetFragmentView<FRecallPlayerCameraFragment>();
		const TArrayView<FRecallTransformFragment> TransformList = Context.GetMutableFragmentView<FRecallTransformFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallPlayerCameraFragment& CameraFragment = CameraList[EntityIndex];
			if (!CameraFragment.bFollowPlayer)
			{
				continue;
			}
			
			FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];

			FVector TargetLocation = TransformFragment.Position;
			FQuat TargetRotation = TransformFragment.Rotation;
			if (!Recall::Camera::Utils::GetPlayerCameraTargetPosition(Context, CameraSharedFragment, CameraFragment,
				TargetLocation, TargetRotation))
			{
				continue;
			}
			
			TransformFragment.Position = UKismetMathLibrary::VInterpTo(TransformFragment.Position,
				TargetLocation, Context.GetDeltaTimeSeconds(), CameraSharedFragment.Settings.FollowInterpSpeed);

			TransformFragment.Rotation = UKismetMathLibrary::RInterpTo(TransformFragment.Rotation.Rotator(), TargetRotation.Rotator(),
				Context.GetDeltaTimeSeconds(), CameraSharedFragment.Settings.RotateInterpSpeed).Quaternion();
		}
	});
}
