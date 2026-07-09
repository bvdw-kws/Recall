// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "RecallEntityTypes.h"

#include "RecallEntitySubsystem.generated.h"

class URecallEntityComponent;

struct FMassEntityConfig;

/*
* Manage entities.
* Dynamic entities are spawned at runtime, while static entities are spawned at the start of our level (from our EntityComponent).
*/
UCLASS()
class RECALLSIMULATION_API URecallEntitySubsystem : public UWorldSubsystem, public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	URecallEntitySubsystem();

	// Entity components
	void RegisterEntityComponent(URecallEntityComponent* EntityComponent);
	void ShutdownEntityComponent(URecallEntityComponent* EntityComponent);

	// Entity creation
	void CreateEntities(const TObjectPtr<const UMassEntityConfigAsset>& EntityConfigAsset, int32 Count, TArray<FMassEntityHandle>& OutEntities);
	void CreateControllerEntity(const TObjectPtr<const UMassEntityConfigAsset>& EntityConfigAsset, FMassEntityHandle& OutEntity, const FRecallControllerEntitySpawnParameters& InParams);

	// Controller
	void RegisterControllerEntity(const FString& ControllerID, const FMassEntityHandle& Entity, const TObjectPtr<const UMassEntityConfigAsset>& EntityConfigAsset);
	void UnregisterControllerEntity(const FString& ControllerID);

	bool HasControllerOwnedEntity(const FString& ControllerID) const;
	bool FindOwnerControllerId(const FMassEntityHandle& Entity, FString& OutControllerID) const;
	bool FindControllerOwnedEntity(const FString& ControllerID, FMassEntityHandle& OutEntity) const;

	TArray<FString> GetControllerIDs() const;
	TArray<FMassEntityHandle> GetControllerEntities() const;
	int32 GetControllerCount() const;

	const FRecallControllerEntityCreationContext& PeekControllerEntityCreationContext() const;
	FRecallControllerEntityData GetControllerEntityData(const FString& ControllerID) const;

	FORCEINLINE bool IsStaticEntity(const FMassEntityHandle& Entity) const { return Entity.SerialNumber <= LastStaticEntitySerialNumber; }
	FORCEINLINE int32 GetStaticEntityCount() const { return LastStaticEntitySerialNumber; }
	
	// Streaming support
	/** Get all registered entity components for streaming system access */
	const TArray<URecallEntityComponent*>& GetEntityComponents() const { return EntityComponents; }
	
	/** Create streaming entity from component with streaming fragment */
	FMassEntityHandle CreateStreamingEntity(const URecallEntityComponent* EntityComponent);

protected:
	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	virtual void Start(const FRecallSimulationStartParams& Params) override final;
	virtual int32 GetStartOrderPriority() const override final { return Recall::SimReactSystem::StartOrder::MediumPriority; }
	virtual void Reset() override final;
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	virtual void PreRestore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
public:
	void SetDebugEmptyLevel(bool bEmpty) { bDebugEmptyLevel = bEmpty; }

private:
	bool bDebugEmptyLevel = false;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

protected:
	UPROPERTY(VisibleAnywhere, Transient)
	FRecallEntityRegistry EntityRegistry;
	UPROPERTY(VisibleAnywhere, Transient)
	TArray<URecallEntityComponent*> EntityComponents;
	UPROPERTY(VisibleAnywhere, Transient)
	int32 LastStaticEntitySerialNumber{ 0 };

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<const UMassEntityConfigAsset>> EntityConfigAssetCache;
	
	/* Controller entity creation context, valid only while CreateControllerEntity is creating the entity. */
	TUniquePtr<FRecallControllerEntityCreationContext> ControllerEntityCreationContext;

	// Cache manager
	TSharedPtr<struct FRecallEntityRestoreSaveCacheManager> SaveCacheManager;
	TSharedPtr<struct FRecallEntityRestoreCacheManager> RestoreCacheManager;

	void CreateStaticEntities_Internal();
	bool CreateEntities_Internal(const TObjectPtr<const UMassEntityConfigAsset>& EntityConfigAsset, int32 Count, TArray<FMassEntityHandle>& OutEntities);
	void CreateEntities_Internal(const FMassEntityConfig& EntityConfig, int32 Count, TArray<FMassEntityHandle>& OutEntities);

	/** Refactored method to expose entity component spawn logic for reuse */
	FMassEntityHandle BuildEntityFromComponent(const URecallEntityComponent* EntityComponent);
	
	/** Apply object fragment initializers to entity */
	void ApplyObjectFragmentInitializers(const URecallEntityComponent* EntityComponent, const FMassEntityHandle& Entity);
};

template<>
struct TMassExternalSubsystemTraits<URecallEntitySubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
