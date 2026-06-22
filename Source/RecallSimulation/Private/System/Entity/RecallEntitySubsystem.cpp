// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Entity/RecallEntitySubsystem.h"

#include "Async/ParallelFor.h"
#include "Component/RecallEntityComponent.h"
#include "Desync/RecallDesyncLog.h"
#include "Engine/World.h"
#include "MassEntityManager.h"
#include "MassEntityUtils.h"
#include "MassEntityView.h"
#include "MassObserverNotificationTypes.h"
#include "Settings/RecallSimulationSettings.h"
#include "Snapshot/RecallEntitySnapshotCacheTypes.h"
#include "Snapshot/RecallEntitySnapshotTypes.h"
#include "Snapshot/RecallEntitySnapshotRestoreUtils.h"
#include "Snapshot/RecallEntitySnapshotSaveUtils.h"

URecallEntitySubsystem::URecallEntitySubsystem()
	: Super()
{
}

void URecallEntitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveCacheManager = MakeShared<FRecallEntityRestoreSaveCacheManager>();
	RestoreCacheManager = MakeShared<FRecallEntityRestoreCacheManager>();
}

void URecallEntitySubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URecallEntitySubsystem::Start(const FRecallSimulationStartParams& Params)
{
	CreateStaticEntities_Internal();
}

void URecallEntitySubsystem::Reset()
{
	const UWorld* World = GetWorld();
	if (ensureMsgf(IsValid(World), TEXT("Invalid world")))
	{
		FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);

		EntityManager.DestroyAllEntities();
		EntityManager.ResetAllSharedFragments();
		EntityManager.SetSerialNumberGenerator(1);
	}

	EntityRegistry = FRecallEntityRegistry();
	LastStaticEntitySerialNumber = 0;
}

void URecallEntitySubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Entity_Save);
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallEntitySubsystem::Save"));

	const UWorld* World = GetWorld();
	if (!ensureMsgf(World,
		TEXT("%hs Invalid world"), __FUNCTION__))
	{
		return;
	}
	
	OutSnapshot.InitializeAs<FRecallEntityManagerSnapshot>();

	FRecallEntityManagerSnapshot& NewSnapshot = OutSnapshot.GetMutable<FRecallEntityManagerSnapshot>();

	{		
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Save EntityRegistry")));
		NewSnapshot.EntityRegistry = EntityRegistry;
	}
	
	check(SaveCacheManager.IsValid());
	SaveCacheManager->ResetCache();

	TMap<FMassArchetypeHandle, FRecallArchetypeSaveCache>& ArchetypeCacheMap = SaveCacheManager->ArchetypeCacheMap;
	TArray<FMassArchetypeHandle>& ArchetypeHandlesCache = SaveCacheManager->ArchetypeHandles;
	
	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);

	Recall::EntitySnapshot::Save::Utils::SaveEntityManager(EntityManager, LastStaticEntitySerialNumber,
		NewSnapshot, ArchetypeCacheMap, ArchetypeHandlesCache);
}

void URecallEntitySubsystem::PreRestore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallEntitySubsystem::PreRestore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Entity_PreRestore);

	const FRecallEntityManagerSnapshot* InData = InSnapshot.GetPtr<FRecallEntityManagerSnapshot>();
	if (InData == nullptr)
	{
		return;
	}
	
	// Generate our archetypes to be up to date (and have all the required const shared fragments ready)
	if (IsInGameThread())
	{
		EntityConfigAssetCache.Reset(InData->EntityRegistry.BuiltEntityConfigs.Num());
		for (const FSoftObjectPath& BuiltEntityConfig : InData->EntityRegistry.BuiltEntityConfigs)
		{
			const UMassEntityConfigAsset* EntityConfigAsset = CastChecked<UMassEntityConfigAsset>(BuiltEntityConfig.TryLoad());
			const FMassEntityTemplate& EntityTemplate = EntityConfigAsset->GetConfig().GetOrCreateEntityTemplate(*GetWorld());
			EntityConfigAssetCache.Add(EntityConfigAsset);
		}
	}
}

void URecallEntitySubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallEntitySubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Entity_Restore);

	const UWorld* World = GetWorld();
	const FRecallEntityManagerSnapshot* InData = InSnapshot.GetPtr<FRecallEntityManagerSnapshot>();
	if (InData == nullptr || !ensureMsgf(World,
		TEXT("%hs Invalid world"), __FUNCTION__))
	{
		return;
	}

	check(RestoreCacheManager.IsValid());
	RestoreCacheManager->ResetCache();

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);

	Recall::EntitySnapshot::Restore::Utils::RestoreEntityManager(EntityManager, *InData, LastStaticEntitySerialNumber,
		RestoreCacheManager->ReservedEntities, RestoreCacheManager->ArchetypeRestoreMap, RestoreCacheManager->ArchetypeRestoreCaches);

	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(*FString::Printf(TEXT("URecallEntitySubsystem::Restore EntityRegistry")));
		EntityRegistry = InData->EntityRegistry;
	}
}

void URecallEntitySubsystem::RegisterEntityComponent(URecallEntityComponent* EntityComponent)
{
	EntityComponents.Add(EntityComponent);
}

void URecallEntitySubsystem::ShutdownEntityComponent(URecallEntityComponent* EntityComponent)
{
	EntityComponents.Remove(EntityComponent);
}

void URecallEntitySubsystem::CreateStaticEntities_Internal()
{
	UWorld* World = GetWorld();
	if (World == NULL)
	{
		return;
	}

	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);

	// Entities created beforehand are considered as static by default
	LastStaticEntitySerialNumber = EntityManager.GetSerialNumberGenerator() - 1;
	checkf(LastStaticEntitySerialNumber == 0,
		TEXT("%hs Static entities should be created first."), __FUNCTION__);
	
	TArray<URecallEntityComponent*> OrderedEntityComponent = EntityComponents;
	OrderedEntityComponent.StableSort([](const URecallEntityComponent& lEntityComponent, const URecallEntityComponent& rEntityComponent)
	{
		// Create our non-mutable entities first so they can be skipped during rollback
		if (lEntityComponent.IsMutableEntity() != rEntityComponent.IsMutableEntity())
		{
			return !lEntityComponent.IsMutableEntity();
		}

		return lEntityComponent.GetEntityAssetName() < rEntityComponent.GetEntityAssetName();
	});

	for (const URecallEntityComponent* EntityComponent : OrderedEntityComponent)
	{
		if (EntityComponent == nullptr) 
		{
			continue;
		}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (bDebugEmptyLevel && !EntityComponent->ShouldSpawnInEmptyLevel())
		{
			continue;
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

		const bool bMutable = EntityComponent->IsMutableEntity();
		
		// Check if this entity should be skipped for streaming
		const URecallSimulationSettings* SimSettings = GetDefault<URecallSimulationSettings>();
		const bool bGlobalStreamingEnabled = SimSettings && SimSettings->StreamingConfig.bEnableEntityStreaming;
		
		if (bMutable && bGlobalStreamingEnabled && EntityComponent->IsStreamingEnabled())
		{
			// Skip entity creation - streaming subsystem will handle spawning during Start()
			continue;
		}
		
		if (!bMutable)
		{
			LastStaticEntitySerialNumber++;
		}

		const FMassEntityHandle CreatedEntity = BuildEntityFromComponent(EntityComponent);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (!bMutable && CreatedEntity.IsValid())
		{
			checkf(LastStaticEntitySerialNumber == CreatedEntity.SerialNumber,
				TEXT("%hs Static entities should be properly registered"), __FUNCTION__);
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}
}

void URecallEntitySubsystem::CreateEntities(const TObjectPtr<const UMassEntityConfigAsset>& EntityConfigAsset, int32 Count, TArray<FMassEntityHandle>& OutEntities)
{
	CreateEntities_Internal(EntityConfigAsset, Count, OutEntities);
}

bool URecallEntitySubsystem::CreateEntities_Internal(const TObjectPtr<const UMassEntityConfigAsset>& EntityConfigAsset, int32 Count, TArray<FMassEntityHandle>& OutEntities)
{
	if (IsValid(EntityConfigAsset))
	{
		EntityRegistry.BuiltEntityConfigs.AddUnique(FSoftObjectPath(EntityConfigAsset));
		EntityConfigAssetCache.Add(EntityConfigAsset);
		CreateEntities_Internal(EntityConfigAsset->GetConfig(), Count, OutEntities);
		return true;
	}
	return false;
}

void URecallEntitySubsystem::CreateControllerEntity(const TObjectPtr<const UMassEntityConfigAsset>& EntityConfigAsset,
	FMassEntityHandle& OutEntity, const FRecallControllerEntitySpawnParameters& InParams)
{
	const UWorld* World = GetWorld();
	if (!ensureMsgf(IsValid(World), TEXT("Invalid world")) || !IsValid(EntityConfigAsset))
	{
		return;
	}

	if (!InParams.OwnerControllerId.IsEmpty())
	{
		ControllerEntityCreationContext = MakeUnique<FRecallControllerEntityCreationContext>(FRecallControllerEntityCreationContext{
			InParams.OwnerControllerId, EntityConfigAsset, InParams.SpawnParameters });
	}

	TArray<FMassEntityHandle> Entities;
	CreateEntities_Internal(EntityConfigAsset, 1, Entities);

	check(Entities.Num() == 1);
	OutEntity = Entities[0];

#if RECALL_DESYNC_LOG
	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	const FMassArchetypeHandle ArchetypeHandle = EntityManager.GetArchetypeForEntity(OutEntity);
	int32 AbsoluteIndex, ChunkIndex;
	EntityManager.GetArchetypeInternalIndexForEntity(OutEntity, ArchetypeHandle, AbsoluteIndex, ChunkIndex);
	RECALL_DESYNC_LOG_STR(World, ControllerEntityCreated,
		FString::Printf(TEXT("ControllerID: %s, Entity: %s, AbsoluteIndex: %d, ChunkIndex: %d"),
			*InParams.OwnerControllerId, *OutEntity.DebugGetDescription(), AbsoluteIndex, ChunkIndex));
#endif // RECALL_DESYNC_LOG

	// ControllerEntityCreationContext.Reset();
}

void URecallEntitySubsystem::CreateEntities_Internal(const FMassEntityConfig& EntityConfig, int32 Count, TArray<FMassEntityHandle>& OutEntities)
{
	const UWorld* World = GetWorld();
	if (ensureMsgf(IsValid(World), TEXT("Invalid world")))
	{
		const FMassEntityTemplate& EntityTemplate = EntityConfig.GetOrCreateEntityTemplate(*World);
		if (EntityTemplate.IsValid())
		{
			FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
			const TSharedRef<FMassEntityManager::FEntityCreationContext> CreationContext = EntityManager.BatchCreateEntities(EntityTemplate.GetArchetype(), EntityTemplate.GetSharedFragmentValues(), Count, OutEntities);

			const TConstArrayView<FInstancedStruct> FragmentInstances = EntityTemplate.GetInitialFragmentValues();
			for (const FMassArchetypeEntityCollection& EntityCollection : CreationContext->GetEntityCollections(EntityManager))
			{
				EntityManager.BatchSetEntityFragmentValues(EntityCollection, FragmentInstances);
			}
		}
	}
}

FRecallControllerEntityData URecallEntitySubsystem::GetControllerEntityData(const FString& ControllerID) const
{
	const FRecallControllerEntityEntry* ControllerEntryPtr = EntityRegistry.ControllerEntries.FindByPredicate([&ControllerID](const FRecallControllerEntityEntry& ControllerEntry)
		{
			return ControllerEntry.ControllerID == ControllerID;
		});

	checkf(ControllerEntryPtr != nullptr, TEXT("Controller should exist"));

	const UWorld* World = GetWorld();
	checkf(IsValid(World), TEXT("Invalid world"));

	FRecallControllerEntityData EntityData;
	EntityData.EntityConfig = ControllerEntryPtr->EntityConfig;

	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	EntityManager.GetEntityData(ControllerEntryPtr->EntityHandle, EntityData.FragmentInstanceList);

	return EntityData;
}

bool URecallEntitySubsystem::FindOwnerControllerId(const FMassEntityHandle& Entity, FString& OutControllerID) const
{
	const FRecallControllerEntityEntry* ControllerEntryPtr = EntityRegistry.ControllerEntries.FindByPredicate([&Entity](const FRecallControllerEntityEntry& ControllerEntry)
		{
			return ControllerEntry.EntityHandle == Entity;
		});

	if (ControllerEntryPtr != nullptr)
	{
		OutControllerID = ControllerEntryPtr->ControllerID;
		return true;
	}

	return false;
}

bool URecallEntitySubsystem::FindControllerOwnedEntity(const FString& ControllerID, FMassEntityHandle& OutEntity) const
{
	const FRecallControllerEntityEntry* ControllerEntryPtr = EntityRegistry.ControllerEntries.FindByPredicate([&ControllerID](const FRecallControllerEntityEntry& ControllerEntry)
		{
			return ControllerEntry.ControllerID == ControllerID;
		});

	if (ControllerEntryPtr != nullptr)
	{
		OutEntity = ControllerEntryPtr->EntityHandle;
		return true;
	}

	return false;
}

void URecallEntitySubsystem::RegisterControllerEntity(const FString& ControllerID, const FMassEntityHandle& Entity, const TObjectPtr<const UMassEntityConfigAsset>& EntityConfigAsset)
{
	if (ensureAlwaysMsgf(!ControllerID.IsEmpty(), TEXT("ControllerID should be valid")) &&
		ensureAlwaysMsgf(!HasControllerOwnedEntity(ControllerID),
			TEXT("%hs We already have an entity registered for Controller: %s"), __FUNCTION__, *ControllerID))
	{
		EntityRegistry.ControllerEntries.Add(FRecallControllerEntityEntry{ ControllerID, Entity, EntityConfigAsset });
	}
}

void URecallEntitySubsystem::UnregisterControllerEntity(const FString& ControllerID)
{
	for (int32 ControllerIndex = EntityRegistry.ControllerEntries.Num() - 1; ControllerIndex >= 0; ControllerIndex--)
	{
		const FRecallControllerEntityEntry& ControllerEntry = EntityRegistry.ControllerEntries[ControllerIndex];

		if (ControllerEntry.ControllerID == ControllerID)
		{
			EntityRegistry.ControllerEntries.RemoveAt(ControllerIndex);
		}
	}
}

bool URecallEntitySubsystem::HasControllerOwnedEntity(const FString& ControllerID) const
{
	return EntityRegistry.ControllerEntries.ContainsByPredicate([&ControllerID](const FRecallControllerEntityEntry& Entry) { return Entry.ControllerID == ControllerID; });
}

TArray<FString> URecallEntitySubsystem::GetControllerIDs() const
{
	TArray<FString> ControllerIDs;
	ControllerIDs.Reserve(EntityRegistry.ControllerEntries.Num());

	Algo::Transform(EntityRegistry.ControllerEntries, ControllerIDs,
		[](const FRecallControllerEntityEntry& Entry) { return Entry.ControllerID; });

	ControllerIDs.Sort();

	return ControllerIDs;
}

TArray<FMassEntityHandle> URecallEntitySubsystem::GetControllerEntities() const
{
	const TArray<FString> ControllerIDs = GetControllerIDs();
	
	TArray<FMassEntityHandle> ControllerEntities;
	ControllerEntities.Reserve(ControllerIDs.Num());
	
	Algo::Transform(ControllerIDs, ControllerEntities,
		[this](const FString& ControllerID)
	{
		FMassEntityHandle ControllerEntity;
		FindControllerOwnedEntity(ControllerID, ControllerEntity);
		return ControllerEntity;
	});

	return ControllerEntities;
}

int32 URecallEntitySubsystem::GetControllerCount() const
{
	return EntityRegistry.ControllerEntries.Num();
}

FRecallControllerEntityCreationContext URecallEntitySubsystem::PopControllerEntityCreationContext()
{
	// Make sure we have a player entity currently being created.
	check(ControllerEntityCreationContext.IsValid());
	const FRecallControllerEntityCreationContext CreationContext = MoveTemp(*ControllerEntityCreationContext.Get());
	ControllerEntityCreationContext.Reset();
	return CreationContext;
}

FMassEntityHandle URecallEntitySubsystem::CreateStreamingEntity(const URecallEntityComponent* EntityComponent)
{
	return BuildEntityFromComponent(EntityComponent);
}

FMassEntityHandle URecallEntitySubsystem::BuildEntityFromComponent(const URecallEntityComponent* EntityComponent)
{
	if (!EntityComponent)
	{
		return FMassEntityHandle();
	}

	// Create our entities
	const FMassEntityConfig& EntityConfig = EntityComponent->GetEntityConfig();
	
	TArray<FMassEntityHandle> Entities;
	CreateEntities_Internal(EntityConfig, 1, Entities);

	checkf(Entities.Num() == 1,
		TEXT("%hs One entity should be created for each entity component"), __FUNCTION__);

	// Apply object fragment initializers if available
	if (Entities.Num() > 0)
	{
		ApplyObjectFragmentInitializers(EntityComponent, Entities[0]);
		return Entities[0];
	}

	return FMassEntityHandle();
}

void URecallEntitySubsystem::ApplyObjectFragmentInitializers(const URecallEntityComponent* EntityComponent, const FMassEntityHandle& Entity)
{
	if (!EntityComponent)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);

	// Initialize our static entities from the Actor.
	const FMassEntityConfig& EntityConfig = EntityComponent->GetEntityConfig();
	const FMassEntityTemplate& EntityTemplate = EntityConfig.GetOrCreateEntityTemplate(*World);
	
	if (EntityTemplate.IsValid() && EntityTemplate.GetObjectFragmentInitializers().Num())
	{
		const TConstArrayView<FMassEntityTemplateData::FObjectFragmentInitializerFunction> ObjectFragmentInitializers = EntityTemplate.GetObjectFragmentInitializers();

		FMassEntityView EntityView(EntityManager, Entity);

		AActor* Owner = EntityComponent->GetOwner();
		check(Owner);
		
		for (const FMassEntityTemplateData::FObjectFragmentInitializerFunction& Initializer : ObjectFragmentInitializers)
		{
			Initializer(*Owner, EntityView, EMassTranslationDirection::ActorToMass);
		}
	}
}
