// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPhysicsProcessors.h"

#include "Async/ParallelFor.h"
#include "Desync/RecallDesyncLog.h"
#include "MassExecutionContext.h"
#include "RecallSignalSubsystem.h"
#include "Physics/JPRPhysicsBody.h"
#include "Physics/RecallPhysicsObjects.h"
#include "Simulation/Physics/RecallPhysicsBodyFragment.h"
#include "Simulation/Physics/RecallPhysicsProcessorGroupTypes.h"
#include "Simulation/Physics/RecallPhysicsSignalTypes.h"
#include "Simulation/Physics/RecallPhysicsSensorFragment.h"
#include "Simulation/Sensor/RecallSensorFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "System/Physics/RecallPhysicsSubsystem.h"
#include "Utility/Physics/RecallPhysicsUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
static void ExecuteDumpPhysicsObject(FMassEntityQuery& EntityQuery, FMassExecutionContext& Context, const FString& ContextString)
{
#if RECALL_DESYNC_LOG
	EntityQuery.ForEachEntityChunk(Context, ([&ContextString](FMassExecutionContext& Context)
	{
		const FMassEntityManager& EntityManager = Context.GetEntityManagerChecked();
		const URecallPhysicsSubsystem& PhysicsSystem = Context.GetSubsystemChecked<URecallPhysicsSubsystem>();

		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];

			const FConstRecallPhysicsBodyView PhysicsBody = PhysicsSystem.GetBody(BodyFragment.BodyHandle);
			if (ensureMsgf(PhysicsBody.IsValid(), TEXT("Body does not exist.")) == false)
			{
				continue;
			}
			
			if (PhysicsBody.IsEnabled() == false)
			{
				continue;
			}

			FVector Position = FVector::ZeroVector;
			FQuat Rotation = FQuat::Identity;
			PhysicsBody.GetPositionAndRotation(Position, Rotation);

			const FVector Velocity = PhysicsBody.GetLinearVelocity();
			
			const FMassArchetypeHandle ArchetypeHandle = EntityManager.GetArchetypeForEntity(Entity);
			int32 AbsoluteIndex = INDEX_NONE, ChunkIndex = INDEX_NONE;
			EntityManager.GetArchetypeInternalIndexForEntity(Entity, ArchetypeHandle, AbsoluteIndex, ChunkIndex);
			
			RECALL_DESYNC_LOG_STR(Context.GetWorld(), PhysicsObject,
				FString::Printf(TEXT("(%s) %s (%s) Position: %s, Rotation: %s, Velocity: %s (AbsoluteIndex: %d, ChunkIndex: %d)"),
				*ContextString, *Entity.DebugGetDescription(), *BodyFragment.BodyHandle.DebugGetDescription(),
				*Position.ToString(), *Rotation.ToString(), *Velocity.ToString(), AbsoluteIndex, ChunkIndex));
		}
	}));
#endif // RECALL_DESYNC_LOG
	
	if (!Recall::Physics::Utils::ShouldDebugDumpPhysicsObject())
	{
		return;
	}

	const uint32 Frame = Recall::Simulation::Utils::GetFrame(Context.GetWorld());

	UE_LOG(LogRecallPhysics, Log, TEXT("//----------------------------------------------------------------------//"));
	UE_LOG(LogRecallPhysics, Log, TEXT("//  [%d] %s"), Frame, *ContextString);
	UE_LOG(LogRecallPhysics, Log, TEXT("//----------------------------------------------------------------------//"));

	EntityQuery.ForEachEntityChunk(Context, ([](FMassExecutionContext& Context)
	{
		const URecallPhysicsSubsystem& PhysicsSystem = Context.GetSubsystemChecked<URecallPhysicsSubsystem>();

		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];

			const FConstRecallPhysicsBodyView PhysicsBody = PhysicsSystem.GetBody(BodyFragment.BodyHandle);
			if (ensureMsgf(PhysicsBody.IsValid(), TEXT("Body does not exist.")) == false)
			{
				continue;
			}

			if (PhysicsBody.IsEnabled() == false)
			{
				continue;
			}

			const TArray<FRecallPhysicsHitEvent> HitEvents = PhysicsSystem.GetHitEvents(BodyFragment.BodyHandle);
			UE_LOG(LogRecallPhysics, Log, TEXT("Hit Count: %d"), HitEvents.Num());

			PhysicsBody.Pin()->DumpObject();
		}
	}));
}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

//----------------------------------------------------------------------//
// URecallPhysicsInitializerProcessor
//----------------------------------------------------------------------//
URecallPhysicsInitializerProcessor::URecallPhysicsInitializerProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::StartPhysics;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::Initialize;
}

struct FRecallPhysicsInitializerCacheManager
{
	TArray<FMassEntityHandle> InitializedEntities;

	void ResetCache()
	{
		InitializedEntities.Reset();
	}
};

void URecallPhysicsInitializerProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);

	CacheManager = MakeShared<FRecallPhysicsInitializerCacheManager>();
}

void URecallPhysicsInitializerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	FMassTagBitSet InvalidTags;
	InvalidTags.Add(FRecallPhysicsBodyInitializedTransformTag::StaticStruct());

	EntityQuery.AddRequirement<FRecallTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirements<EMassFragmentPresence::None>(InvalidTags);
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPhysicsInitializerProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_Initializer);

	check(CacheManager.IsValid());
	CacheManager->ResetCache();

	TArray<FMassEntityHandle>& InitializedEntities = CacheManager->InitializedEntities;

	EntityQuery.ForEachEntityChunk(Context, [&InitializedEntities](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();

		const TConstArrayView<FRecallTransformFragment> TransformList = Context.GetFragmentView<FRecallTransformFragment>();
		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			const FRecallPhysicsBodyView PhysicsBody = PhysicsSystem.GetMutableBody(BodyFragment.BodyHandle);

			if (ensureMsgf(PhysicsBody.IsValid(), TEXT("Body does not exist.")) == false)
			{
				continue;
			}

			const FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];

			PhysicsBody.SetPositionAndRotation(TransformFragment.Position, TransformFragment.Rotation);
		}

		InitializedEntities.Append(Context.GetEntities());
	});

	if (InitializedEntities.Num() > 0)
	{
		Context.Defer().PushCommand<FMassCommandAddTag<FRecallPhysicsBodyInitializedTransformTag>>(InitializedEntities);
	}
}

//----------------------------------------------------------------------//
// URecallPhysicsStartSimulationProcessor
//----------------------------------------------------------------------//
URecallPhysicsStartSimulationProcessor::URecallPhysicsStartSimulationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	QueryBasedPruning = EMassQueryBasedPruning::Never;
	ProcessingPhase = EMassProcessingPhase::StartPhysics;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::StartSimulation;
	ExecutionOrder.ExecuteAfter.Add(Recall::Physics::ProcessorGroupNames::Initialize);
}

void URecallPhysicsStartSimulationProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPhysicsStartSimulationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
	
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadOnly);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallPhysicsStartSimulationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_StartSimulation);
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallPhysicsStartSimulationProcessor::Execute"));

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	static const FString PreUpdateContextString(TEXT("URecallPhysicsStartSimulationProcessor::Execute"));
	ExecuteDumpPhysicsObject(EntityQuery, Context, PreUpdateContextString);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
	PhysicsSystem.TickPhysics(Context.GetDeltaTimeSeconds());
}

//----------------------------------------------------------------------//
// URecallPhysicsEndSimulationProcessor
//----------------------------------------------------------------------//
URecallPhysicsEndSimulationProcessor::URecallPhysicsEndSimulationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	QueryBasedPruning = EMassQueryBasedPruning::Never;
	ProcessingPhase = EMassProcessingPhase::EndPhysics;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::EndSimulation;
}

void URecallPhysicsEndSimulationProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPhysicsEndSimulationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadOnly);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallPhysicsEndSimulationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_EndSimulation);
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallPhysicsEndSimulationProcessor::Execute"));

	URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
	PhysicsSystem.ForceEndPhysicsSimulation();

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	static const FString PostUpdateContextString(TEXT("URecallPhysicsEndSimulationProcessor::Execute"));
	ExecuteDumpPhysicsObject(EntityQuery, Context, PostUpdateContextString);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

#if RECALL_DESYNC_LOG
	RECALL_DESYNC_LOG_INT(Context.GetWorld(), NumContactEvents, PhysicsSystem.GetNumContactEvents());
#endif // RECALL_DESYNC_LOG
}

//----------------------------------------------------------------------//
// URecallPhysicsCopyLocationProcessor
//----------------------------------------------------------------------//
URecallPhysicsCopyLocationProcessor::URecallPhysicsCopyLocationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::EndPhysics;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::CopyLocation;
	ExecutionOrder.ExecuteAfter.Add(Recall::Physics::ProcessorGroupNames::EndSimulation);
	ExecutionOrder.ExecuteAfter.Add(Recall::Physics::ProcessorGroupNames::CharacterPostUpdate);
}

void URecallPhysicsCopyLocationProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPhysicsCopyLocationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	FMassTagBitSet InvalidTags;
	InvalidTags.Add(FRecallPhysicsStaticColliderTag::StaticStruct());

	FMassTagBitSet RequiredTags;
	RequiredTags.Add(FRecallPhysicsBodyInitializedTransformTag::StaticStruct());

	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRecallTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirements<EMassFragmentPresence::None>(InvalidTags);
	EntityQuery.AddTagRequirements<EMassFragmentPresence::All>(RequiredTags);
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPhysicsCopyLocationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_CopyLocation);
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallPhysicsCopyLocationProcessor::Execute")));

	EntityQuery.ParallelForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();

		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();
		const TArrayView<FRecallTransformFragment> TransformList = Context.GetMutableFragmentView<FRecallTransformFragment>();

		ParallelFor(Context.GetNumEntities(), [&](int32 EntityIndex)
		{
			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];

			const FRecallPhysicsBodyView PhysicsBody = PhysicsSystem.GetMutableBody(BodyFragment.BodyHandle);

			if (!ensureMsgf(PhysicsBody.IsValid(),
				TEXT("%hs Body does not exist."), __FUNCTION__))
			{
				return;
			}

			if (!PhysicsBody.IsEnabled())
			{
				return;
			}

			if (BodyFragment.ShouldCopyPositionAndRotation())
			{
				PhysicsBody.GetPositionAndRotation(TransformFragment.Position, TransformFragment.Rotation);
			}
			else if (BodyFragment.ShouldCopyPosition())
			{
				PhysicsBody.GetPosition(TransformFragment.Position);
			}
			else if (BodyFragment.ShouldCopyRotation())
			{
				PhysicsBody.GetRotation(TransformFragment.Rotation);
			}
		});
	});
		
#if RECALL_DESYNC_LOG
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();
		const TConstArrayView<FRecallTransformFragment> TransformList = Context.GetFragmentView<FRecallTransformFragment>();
		
		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			const FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];
		
			RECALL_DESYNC_LOG_CONTEXT(Context.GetWorld(), FString::Printf(TEXT("%s (%s): Position: %s, Rotation: %s"),
				*Entity.DebugGetDescription(), *BodyFragment.BodyHandle.DebugGetDescription(),
				*TransformFragment.Position.ToString(), *TransformFragment.Rotation.ToString()));
		}
	});
#endif // RECALL_DESYNC_LOG
}

//----------------------------------------------------------------------//
// URecallPhysicsGeneratesHitEventProcessor
//----------------------------------------------------------------------//
URecallPhysicsGeneratesHitEventProcessor::URecallPhysicsGeneratesHitEventProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	QueryBasedPruning = EMassQueryBasedPruning::Never;
	ProcessingPhase = EMassProcessingPhase::EndPhysics;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::GeneratesHitEvent;
	ExecutionOrder.ExecuteAfter.Add(Recall::Physics::ProcessorGroupNames::EndSimulation);
	ExecutionOrder.ExecuteAfter.Add(Recall::Physics::ProcessorGroupNames::CopyLocation);
}

struct FRecallPhysicsGeneratesHitEventCacheManager
{
	TSet<FRecallPhysicsBodyHandle> GeneratesEventBodyHandles;
	TMap<FRecallPhysicsBodyHandle, FMassEntityHandle> HitEventEntityMap;
	TArray<FMassEntityHandle> HitEntities;
	
	void ResetCache()
	{
		GeneratesEventBodyHandles.Reset();
		HitEventEntityMap.Reset();
		HitEntities.Reset();
	}
};

void URecallPhysicsGeneratesHitEventProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);

	CacheManager = MakeShared<FRecallPhysicsGeneratesHitEventCacheManager>();
}

void URecallPhysicsGeneratesHitEventProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	FMassTagBitSet RequiredTags;
	RequiredTags.Add(FRecallPhysicsBodyInitializedTransformTag::StaticStruct());

	FMassTagBitSet ValidTags;
	ValidTags.Add(FRecallPhysicsGeneratesHitEventTag::StaticStruct());
	ValidTags.Add(FRecallPhysicsSensorTag::StaticStruct());

	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRecallSensorFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	EntityQuery.AddTagRequirements<EMassFragmentPresence::All>(RequiredTags);
	EntityQuery.AddTagRequirements<EMassFragmentPresence::Any>(ValidTags);

	ProcessorRequirements.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
	ProcessorRequirements.AddSubsystemRequirement<URecallSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPhysicsGeneratesHitEventProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_GeneratesHitEvent);

	check(CacheManager.IsValid());
	CacheManager->ResetCache();

	TSet<FRecallPhysicsBodyHandle>& GeneratesEventBodyHandles = CacheManager->GeneratesEventBodyHandles;
	TMap<FRecallPhysicsBodyHandle, FMassEntityHandle>& HitEventEntityMap = CacheManager->HitEventEntityMap;

	EntityQuery.ForEachEntityChunk(Context,
		[&GeneratesEventBodyHandles, &HitEventEntityMap](FMassExecutionContext& Context)
	{
		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();
		const TConstArrayView<FRecallSensorFragment> SensorList = Context.GetFragmentView<FRecallSensorFragment>();

		const bool bGeneratesHitEvent = Context.DoesArchetypeHaveTag<FRecallPhysicsGeneratesHitEventTag>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];
			
			if (bGeneratesHitEvent && ensureMsgf(BodyFragment.BodyHandle.IsValid(), TEXT("Body handle is not valid.")))
			{
				GeneratesEventBodyHandles.Add(BodyFragment.BodyHandle);
				HitEventEntityMap.Add(BodyFragment.BodyHandle, Entity);
			}

			if (SensorList.IsValidIndex(EntityIndex))
			{
				GeneratesEventBodyHandles.Append(SensorList[EntityIndex].BodyHandles);
			}
		}
	});

#if RECALL_DESYNC_LOG
	RECALL_DESYNC_LOG_INT(Context.GetWorld(), GeneratesHitEventsBodyHandles, GeneratesEventBodyHandles.Num());
#endif // RECALL_DESYNC_LOG

	URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
	PhysicsSystem.GenerateHitEvents(GeneratesEventBodyHandles);

	// Gather entities that generated a hit.
	TArray<FMassEntityHandle>& HitEntities = CacheManager->HitEntities;		
	for (const TPair<FRecallPhysicsBodyHandle, FMassEntityHandle>& HitEventEntityTuple : HitEventEntityMap)
	{
		if (PhysicsSystem.HasHitEvent(HitEventEntityTuple.Key))
		{
			HitEntities.Add(HitEventEntityTuple.Value);
		}
	}

	if (HitEntities.Num())
	{
		URecallSignalSubsystem& SignalSystem = Context.GetMutableSubsystemChecked<URecallSignalSubsystem>();
		SignalSystem.SignalEntities(Recall::Physics::Signals::Hit, HitEntities);
	}
}

//----------------------------------------------------------------------//
// URecallPhysicsResetHitEventsProcessor
//----------------------------------------------------------------------//
URecallPhysicsResetHitEventsProcessor::URecallPhysicsResetHitEventsProcessor()
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	QueryBasedPruning = EMassQueryBasedPruning::Never;
	ProcessingPhase = EMassProcessingPhase::FrameEnd;
	ExecutionOrder.ExecuteInGroup = Recall::Physics::ProcessorGroupNames::ResetHitEvents;
}

void URecallPhysicsResetHitEventsProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPhysicsResetHitEventsProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) 
{
	ProcessorRequirements.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPhysicsResetHitEventsProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_ResetHitEvents);

	URecallPhysicsSubsystem& PhysicsSystem = Context.GetMutableSubsystemChecked<URecallPhysicsSubsystem>();
	PhysicsSystem.ResetHitEvents();
}

//----------------------------------------------------------------------//
// URecallPhysicsRepresentationProcessor
//----------------------------------------------------------------------//
URecallPhysicsRepresentationProcessor::URecallPhysicsRepresentationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::Render;
	bRequiresGameThreadExecution = true;
}

void URecallPhysicsRepresentationProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallPhysicsRepresentationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) 
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	EntityQuery.AddRequirement<FRecallPhysicsBodyFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRecallPhysicsSensorFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRecallSensorFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	EntityQuery.AddConstSharedRequirement<FRecallSensorConstSharedFragment>(EMassFragmentPresence::Optional);
	EntityQuery.AddSubsystemRequirement<URecallPhysicsSubsystem>(EMassFragmentAccess::ReadOnly);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallPhysicsRepresentationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Physics_Representation);

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const URecallPhysicsSubsystem& PhysicsSystem = Context.GetSubsystemChecked<URecallPhysicsSubsystem>();

		const FRecallSensorConstSharedFragment* SensorConstSharedFragmentPtr = Context.GetConstSharedFragmentPtr<FRecallSensorConstSharedFragment>();

		const TConstArrayView<FRecallPhysicsBodyFragment> BodyList = Context.GetFragmentView<FRecallPhysicsBodyFragment>();
		const TConstArrayView<FRecallPhysicsSensorFragment> PhysicsSensorList = Context.GetFragmentView<FRecallPhysicsSensorFragment>();
		const TConstArrayView<FRecallSensorFragment> SensorList = Context.GetFragmentView<FRecallSensorFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallPhysicsSensorFragment& PhysicsSensorFragment = PhysicsSensorList[EntityIndex];

			auto DrawDebugShape = [&Context, &PhysicsSystem, &PhysicsSensorFragment](
				const FRecallPhysicsBodyHandle& BodyHandle,
				FColor OverlappingColor = FColor::Purple,
				FColor ActivatedColor = FColor::Blue,
				FColor DeactivatedColor = FColor::Red)
			{
				const FConstRecallPhysicsBodyView PhysicsBody = PhysicsSystem.GetBody(BodyHandle);

				if (PhysicsBody.IsValid())
				{
					if (PhysicsBody.IsEnabled())
					{
						if (PhysicsSensorFragment.IsSensorOverlapping(BodyHandle))
						{
							PhysicsBody.Pin()->DrawDebugShape(Context.GetWorld(), OverlappingColor);
						}
						else
						{
							PhysicsBody.Pin()->DrawDebugShape(Context.GetWorld(), ActivatedColor);
						}
					}
					else
					{
						PhysicsBody.Pin()->DrawDebugShape(Context.GetWorld(), DeactivatedColor);
					}
				}
			};

			const FRecallPhysicsBodyFragment& BodyFragment = BodyList[EntityIndex];

			if (Recall::Physics::Utils::ShouldDebugShowColliders())
			{
				DrawDebugShape(BodyFragment.BodyHandle);

				if (SensorConstSharedFragmentPtr)
				{
					const FRecallSensorFragment& SensorFragment = SensorList[EntityIndex];

					for (int32 SensorIndex = 0; SensorIndex < SensorConstSharedFragmentPtr->InstanceParameters.Num(); SensorIndex++)
					{
						const FRecallSensorInstanceParameters& SensorParameters = SensorConstSharedFragmentPtr->InstanceParameters[SensorIndex];
						const FRecallPhysicsBodyHandle& SensorBodyHandle = SensorFragment.BodyHandles[SensorIndex];

						DrawDebugShape(SensorBodyHandle, SensorParameters.DebugOverlappingColor, SensorParameters.DebugActivatedColor, SensorParameters.DebugDeactivatedColor);
					}
				}
			}

			if (Recall::Physics::Utils::ShouldDebugShowInfos())
			{
				const FConstRecallPhysicsBodyView PhysicsBody = PhysicsSystem.GetBody(BodyFragment.BodyHandle);
				if (PhysicsBody.IsValid())
				{
					PhysicsBody.Pin()->DrawDebugInfo(Context.GetWorld(), FColor::Green);
				}
			}
		}
	});
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}
