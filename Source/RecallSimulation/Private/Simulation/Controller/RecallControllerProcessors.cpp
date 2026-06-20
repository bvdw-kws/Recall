// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallControllerProcessors.h"

#include "Data/Game/RecallGameRuleBaseAsset.h"
#include "Desync/RecallDesyncLog.h"
#include "MassEntityConfigAsset.h"
#include "MassExecutionContext.h"
#include "MassEntityView.h"
#include "Simulation/Controller/RecallControllerFragments.h"
#include "Simulation/GameplayTag/RecallGameplayTagFragments.h"
#include "Simulation/Player/RecallPlayerProcessorGroupTypes.h"
#include "Simulation/Player/Camera/RecallPlayerCameraFragments.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "System/Entity/RecallEntitySubsystem.h"
#include "System/Game/RecallGameRuleSubsystem.h"
#include "System/Player/RecallPlayerStartSubsystem.h"
#include "System/Player/RecallPlayerQueueSubsystem.h"
#include "System/Player/RecallPlayerTypes.h"
#include "System/Random/RecallRandomNumberSubsystem.h"
#include "Utility/Camera/RecallCameraUtils.h"
#include "Utility/Player/RecallPlayerUtils.h"

//----------------------------------------------------------------------//
// URecallControllerInitializer
//----------------------------------------------------------------------//
URecallControllerInitializer::URecallControllerInitializer()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedType = FRecallControllerFragment::StaticStruct();
	Operation = EMassObservedOperation::Add;
}

void URecallControllerInitializer::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallControllerInitializer::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRecallControllerFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRecallGameplayTagFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	EntityQuery.AddConstSharedRequirement<FRecallPlayerControllerSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallPlayerStartSubsystem>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<URecallRandomNumberSubsystem>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<URecallEntitySubsystem>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<URecallGameRuleSubsystem>(EMassFragmentAccess::ReadOnly);
}

static void DeferSpawnPlayerCamera(FMassExecutionContext& Context,
	const FMassEntityHandle PlayerEntity, const TObjectPtr<const UMassEntityConfigAsset>& PlayerCameraEntityConfig)
{
	URecallEntitySubsystem& EntitySystem = Context.GetMutableSubsystemChecked<URecallEntitySubsystem>();
	
	Context.Defer().PushCommand<FMassDeferredCreateCommand>(
		[PlayerEntity, &EntitySystem, PlayerCameraEntityConfig](FMassEntityManager& System)
	{
		if (!System.IsEntityValid(PlayerEntity) || !PlayerCameraEntityConfig)
		{
			return;
		}

		const FMassEntityView PlayerView(System, PlayerEntity);
		const FRecallTransformFragment& PlayerTransformFragment = PlayerView.GetFragmentData<FRecallTransformFragment>();
		FRecallControllerFragment& PlayerControllerFragment = PlayerView.GetFragmentData<FRecallControllerFragment>();

		TArray<FMassEntityHandle> PlayerCameraEntities;
		EntitySystem.CreateEntities(PlayerCameraEntityConfig, 1, PlayerCameraEntities);

		PlayerControllerFragment.CameraEntity = PlayerCameraEntities[0];
		PlayerControllerFragment.ViewTargetEntity = PlayerControllerFragment.CameraEntity;

		const FMassEntityView PlayerCameraView(System, PlayerControllerFragment.CameraEntity);

		FRecallPlayerCameraFragment* PlayerCameraFragmentPtr = PlayerCameraView.GetFragmentDataPtr<FRecallPlayerCameraFragment>();
		if (PlayerCameraFragmentPtr != nullptr)
		{
			PlayerCameraFragmentPtr->TargetPlayerID = PlayerControllerFragment.ControllerID;
		}
			
		if (FRecallTransformFragment* CameraTransformFragmentPtr = PlayerCameraView.GetFragmentDataPtr<FRecallTransformFragment>())
		{
			if (PlayerCameraFragmentPtr != nullptr)
			{
				const auto& CameraSharedFragment = PlayerCameraView.GetConstSharedFragmentData<FRecallPlayerCameraSharedFragment>();
				Recall::Camera::Utils::GetPlayerCameraTargetPosition(EntitySystem, System,
					CameraSharedFragment, *PlayerCameraFragmentPtr,
					CameraTransformFragmentPtr->Position, CameraTransformFragmentPtr->Rotation);
			}
			else
			{				
				CameraTransformFragmentPtr->Position = PlayerTransformFragment.Position;
				CameraTransformFragmentPtr->Rotation = PlayerTransformFragment.Rotation;
			}
		}
	});
}

void DeferSetPlayerSpawnTransform(FMassExecutionContext& Context,
	FRecallTransformFragment& TransformFragment, const FString& ControllerID, const FRecallPlayerSpawnParameters& SpawnParameters)
{
	const URecallGameRuleSubsystem& GameRuleSystem = Context.GetSubsystemChecked<URecallGameRuleSubsystem>();
	const TObjectPtr<const URecallGameRuleBaseAsset> GameRuleAsset = GameRuleSystem.GetGameRuleAsset();
	
	const URecallPlayerStartSubsystem& PlayerStartSystem = Context.GetSubsystemChecked<URecallPlayerStartSubsystem>();
	URecallRandomNumberSubsystem& RandomNumberSystem = Context.GetMutableSubsystemChecked<URecallRandomNumberSubsystem>();

	const FRandomStream& RandomStream = RandomNumberSystem.GetRandomStream();
	const bool bPlayerController = Context.DoesArchetypeHaveTag<FRecallPlayerControllerTag>();
	
	switch (SpawnParameters.SpawnLocation)
	{
	case ERecallPlayerSpawnLocation::Default:
		{
			const FName PlayerStart = GameRuleAsset ? GameRuleAsset->GetPlayerStart(
				Context.GetWorld(), ControllerID, SpawnParameters.CustomParams) : NAME_None;
			
			PlayerStartSystem.FindPlayerStart(RandomStream, TransformFragment.Position, TransformFragment.Rotation,
				PlayerStart, bPlayerController);
		}
		break;
				
	case ERecallPlayerSpawnLocation::PlayerStart:
		{
			PlayerStartSystem.FindPlayerStart(RandomStream, TransformFragment.Position, TransformFragment.Rotation,
				SpawnParameters.PlayerStart, bPlayerController);
		}
		break;
		
	case ERecallPlayerSpawnLocation::Transform:
		{
			TransformFragment.Position = SpawnParameters.Transform.GetLocation();
			TransformFragment.Rotation = SpawnParameters.Transform.GetRotation();
		}
		break;

	default:
		unimplemented();
		break;
	}
}

void URecallControllerInitializer::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallEntitySubsystem& EntitySystem = Context.GetMutableSubsystemChecked<URecallEntitySubsystem>();

		const FRecallPlayerControllerSharedFragment& PlayerControllerSharedFragment = Context.GetConstSharedFragment<FRecallPlayerControllerSharedFragment>();

		const TArrayView<FRecallTransformFragment> TransformList = Context.GetMutableFragmentView<FRecallTransformFragment>();
		const TArrayView<FRecallControllerFragment> ControllerList = Context.GetMutableFragmentView<FRecallControllerFragment>();
		const TArrayView<FRecallGameplayTagFragment> GameplayTagList = Context.GetMutableFragmentView<FRecallGameplayTagFragment>();

		const bool bPlayerController = Context.DoesArchetypeHaveTag<FRecallPlayerControllerTag>();
		
		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);

			FRecallTransformFragment& TransformFragment = TransformList[EntityIndex];
			FRecallControllerFragment& ControllerFragment = ControllerList[EntityIndex];
			FRecallGameplayTagFragment* const GameplayTagFragmentPtr = GameplayTagList.IsValidIndex(EntityIndex) ? &GameplayTagList[EntityIndex] : nullptr;

			const FRecallControllerEntityCreationContext& CreationContext = EntitySystem.GetControllerEntityCreationContext();

			ControllerFragment.ControllerID = CreationContext.OwnerControllerId;

			if (bPlayerController && GameplayTagFragmentPtr != nullptr)
			{
				const int32 PlayerIndex = Recall::Player::Utils::GetPlayerIndexFromId(CreationContext.OwnerControllerId);
				check(PlayerIndex != INDEX_NONE);
				GameplayTagFragmentPtr->GameplayTagCountMap.AddTag(Recall::Player::Utils::GetPlayerControllerTagByIndex(PlayerIndex));
			}

			EntitySystem.RegisterControllerEntity(CreationContext.OwnerControllerId, Entity, CreationContext.EntityConfig);

			DeferSetPlayerSpawnTransform(Context, TransformFragment,
				CreationContext.OwnerControllerId, CreationContext.SpawnParameters);
			DeferSpawnPlayerCamera(Context, Entity, PlayerControllerSharedFragment.CameraEntityConfig);
			
			ControllerFragment.ControlRotation = TransformFragment.Rotation.Rotator();

#if RECALL_DESYNC_LOG
			RECALL_DESYNC_LOG_STR(Context.GetWorld(), ControllerInit,
				FString::Printf(TEXT("Entity: %s, ControllerID: %s, Position: %s, Rotation: %s"),
					*Entity.DebugGetDescription(), *ControllerFragment.ControllerID,
					*TransformFragment.Position.ToString(), *TransformFragment.Rotation.ToString()));
#endif // RECALL_DESYNC_LOG
		}
	});
}

//----------------------------------------------------------------------//
// URecallControllerDeinitializer
//----------------------------------------------------------------------//
URecallControllerDeinitializer::URecallControllerDeinitializer()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ObservedType = FRecallControllerFragment::StaticStruct();
	Operation = EMassObservedOperation::Remove;
}

void URecallControllerDeinitializer::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallControllerDeinitializer::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallControllerFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddConstSharedRequirement<FRecallPlayerControllerSharedFragment>();
	EntityQuery.AddSubsystemRequirement<URecallEntitySubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallControllerDeinitializer::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		URecallEntitySubsystem& EntitySystem = Context.GetMutableSubsystemChecked<URecallEntitySubsystem>();

		const FRecallPlayerControllerSharedFragment& PlayerControllerSharedFragment = Context.GetConstSharedFragment<FRecallPlayerControllerSharedFragment>();

		const TConstArrayView<FRecallControllerFragment> PlayerControllerList = Context.GetFragmentView<FRecallControllerFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallControllerFragment& PlayerControllerFragment = PlayerControllerList[EntityIndex];

			EntitySystem.UnregisterControllerEntity(PlayerControllerFragment.ControllerID);

			Context.Defer().DestroyEntity(PlayerControllerFragment.CameraEntity);
		}
	});
}

//----------------------------------------------------------------------//
// URecallControllerSpawnProcessor
//----------------------------------------------------------------------//
URecallControllerSpawnProcessor::URecallControllerSpawnProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteBefore.Add(Recall::Player::ProcessorGroupNames::Input);
}

struct FRecallPlayerCacheManager
{
	FRecallPlayerEvent PlayerEvent;
	FMassEntityHandle DestroyedEntity;

	void ResetCache()
	{
		PlayerEvent = FRecallPlayerEvent();
		DestroyedEntity.Reset();
	}
};

void URecallControllerSpawnProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);

	CacheManager = MakeShared<FRecallPlayerCacheManager>();
}

bool URecallControllerSpawnProcessor::ShouldAllowQueryBasedPruning(const bool bRuntimeMode /*= true*/) const
{
	return false;
}

void URecallControllerSpawnProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<URecallEntitySubsystem>(EMassFragmentAccess::ReadWrite);
	ProcessorRequirements.AddSubsystemRequirement<URecallPlayerQueueSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallControllerSpawnProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_PlayerSpawn_Execute);

	check(CacheManager.IsValid());
	CacheManager->ResetCache();

	URecallPlayerQueueSubsystem& PlayerQueueSystem = Context.GetMutableSubsystemChecked<URecallPlayerQueueSubsystem>();

	FRecallPlayerEvent& PlayerEvent = CacheManager->PlayerEvent;
	while (PlayerQueueSystem.PopNextPlayerEvent(PlayerEvent))
	{
		URecallEntitySubsystem& EntitySystem = Context.GetMutableSubsystemChecked<URecallEntitySubsystem>();

		if (PlayerEvent.bAddEvent)
		{
			Context.Defer().PushCommand<FMassDeferredCreateCommand>(
				[&EntitySystem, PlayerID = PlayerEvent.PlayerId,
					SpawnParameters = PlayerEvent.SpawnParameters, EntityConfig = PlayerEvent.EntityConfig](FMassEntityManager& System)
				{
					if (ensureMsgf(EntityConfig,
						TEXT("%hs Player's Entity Config was not an EntityConfig."), __FUNCTION__) == false)
					{
						return;
					}

					FRecallControllerEntitySpawnParameters EntitySpawnParams;
					EntitySpawnParams.OwnerControllerId = PlayerID;
					EntitySpawnParams.SpawnParameters = SpawnParameters;

					FMassEntityHandle PlayerEntity;
					EntitySystem.CreateControllerEntity(EntityConfig, PlayerEntity, EntitySpawnParams);

#if RECALL_DESYNC_LOG
					RECALL_DESYNC_LOG_STR(System.GetWorld(), ControllerSpawn,
						FString::Printf(TEXT("PlayerEntity: %s, PlayerID: %s"),
							*PlayerEntity.DebugGetDescription(), *PlayerID));
#endif // RECALL_DESYNC_LOG
				}
			);
		}
		else
		{
			FMassEntityHandle& DestroyedEntity = CacheManager->DestroyedEntity;
			if (ensureMsgf(EntitySystem.FindControllerOwnedEntity(PlayerEvent.PlayerId, DestroyedEntity), 
				TEXT("%hs We dot not have an Entity for this player."), __FUNCTION__))
			{
				Context.Defer().DestroyEntity(DestroyedEntity);
			}
		}
	}
}
