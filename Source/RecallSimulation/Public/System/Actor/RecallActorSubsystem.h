// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "MassExternalSubsystemTraits.h"
#include "RecallActorRegistryTypes.h"
#include "Representation/Actor/RecallActorRepresentationTypes.h"

#include "RecallActorSubsystem.generated.h"

class URecallObjectPoolContainer;

#define RECALL_OBJECT_POOL_MAX 64

/*
* Manage actors used by the view of our simulation in Unreal.
*/
UCLASS()
class RECALLSIMULATION_API URecallActorSubsystem :
	public UWorldSubsystem,
	public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	URecallActorSubsystem();

	template<typename T=FRecallActorRepresentationDesc>
	FRecallActorHandle CreateActor(const T& Desc)
	{
		return CreateActor_Internal(FInstancedStruct::Make(Desc));
	}
	
	FRecallActorHandle CreateActor(const FInstancedStruct& Desc)
	{
		return CreateActor_Internal(Desc);
	}

	void ReleaseActor(FRecallActorHandle& ActorHandle);
	
	template<typename T=AActor>
	TWeakObjectPtr<T> GetActor(const FRecallActorHandle& Handle) const
	{
		return Cast<T>(GetActor_Internal(Handle));
	}

	TWeakObjectPtr<class ASkeletalMeshActor> GetSkeletalMeshActor(const FRecallActorHandle& Handle) const;
	TWeakObjectPtr<class AStaticMeshActor> GetStaticMeshActor(const FRecallActorHandle& Handle) const;
	TWeakObjectPtr<class ADecalActor> GetDecalActor(const FRecallActorHandle& Handle) const;
	TWeakObjectPtr<class ALevelSequenceActor> GetLevelSequenceActor(const FRecallActorHandle& Handle) const;
	
protected:
	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	virtual void Reset() override final;
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	virtual void PreRestore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	virtual int32 GetPostRestoreOrderPriority() const override final { return Recall::SimReactSystem::RestoreOrder::Actor; }
	// IRecallSimulationReactSystemInterface implementation End

protected:
	/** Keep track of actors being used */
	UPROPERTY(VisibleAnywhere)
	FRecallActorRegistry ActorRegistry;
	
	/** Keep track of how many actors have been created. */
	TSharedPtr<int32> SerialNumberGenerator{ new int32(0) };

private:
	/** Keep track of our pool containers so we know if an object is GC */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<URecallObjectPoolContainer>> PoolContainerMap;
	/** Pre-allocate some pool objects on the gamethread */
	UPROPERTY(Transient)
	TArray<TObjectPtr<URecallObjectPoolContainer>> PoolQueue;
	
	/** Index of next free pool container (instead of Pop/Push) */
	UPROPERTY(Transient)
	int32 NextFreePoolIndex = 0;

	/** Actor pool per class name. */
	TMap<FName, TSharedPtr<class IRecallObjectPoolInterface>> ClassNameToActorPoolMap;

	FCriticalSection DataGuard;

	void OnPreRender();
	void FlushActorPoolCommands();

	/** Update our actor handles to only keep handles for which Actor does not exist. */
	void CleanupActorsFromPool(TArray<FRecallActorHandle>& OutHandles);
	void RestoreActorsFromPool(const TArray<FRecallActorHandle>& ActorHandles);

	void ReleaseActorFromPool(const FRecallActorHandle& ActorHandle);

	TSharedPtr<IRecallObjectPoolInterface> GetOrCreatePool(const FInstancedStruct& Desc);

	template<typename T>
	void StoreEntryInstance_Internal(const FRecallActorHandle& ActorHandle, const T& Instance)
	{
		StoreEntryInstance_Internal(ActorHandle, FInstancedStruct::Make(Instance));
	}
	
	void StoreEntryInstance_Internal(const FRecallActorHandle& ActorHandle, const FInstancedStruct& Instance);

	FRecallActorHandle CreateActor_Internal(const FInstancedStruct& Desc);
	void CreateOrRestoreActor_Internal(const FInstancedStruct& Desc, FRecallActorHandle& OutHandle, const FRecallActorHandle* RestoreHandle);

	TWeakObjectPtr<AActor> GetActor_Internal(const FRecallActorHandle& Handle) const;
};

template<>
struct TMassExternalSubsystemTraits<URecallActorSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
