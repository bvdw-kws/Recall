// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "RecallObjectPoolInterface.h"
#include "System/Actor/RecallActorTypes.h"

#include "RecallObjectPoolTypes.generated.h"

USTRUCT()
struct RECALLSIMULATION_API FRecallObjectPoolItem
{
	GENERATED_BODY()

public:
	/* Keep track of the handle owning this actor. */
	UPROPERTY(Transient)
	FRecallActorHandle OwnerHandle;

	/* Actors stored in our pool. */
	UPROPERTY(Transient)
	TObjectPtr<UObject> Object;
};

/**
 * Tracks asset loading state for a specific actor
 */
USTRUCT()
struct RECALLSIMULATION_API FActorAssetLoadingState
{
	GENERATED_BODY()

	/** The actor waiting for assets */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> Actor;

	/** Map of asset names to their soft object paths */
	UPROPERTY(Transient)
	TMap<FName, FSoftObjectPath> RequestedAssets;

	/** Active streamable handles for each asset */
	TMap<FName, TSharedPtr<FStreamableHandle>> ActiveHandles;

	/** Completion callback to execute when all assets are loaded */
	TFunction<void(AActor*, const TMap<FName, UObject*>&)> CompletionCallback;

	FActorAssetLoadingState() = default;
};

UCLASS()
class RECALLSIMULATION_API URecallObjectPoolContainer : public UObject
{
	GENERATED_BODY()

public:
	void InitContainerAsync(const FName& InClassName, const FSoftClassPath& InSoftClassPath, const TWeakPtr<int32>& InSerialNumberGenerator);
	
	/** Check if class loading is complete */
	bool IsClassLoaded() const;
	
	/** Get the loaded actor class (only valid after IsClassLoaded() returns true) */
	UClass* GetLoadedClass() const;
	
	/** Request asset loading for an actor with completion callback */
	void RequestActorAssets(AActor* Actor, 
							const TMap<FName, FSoftObjectPath>& Assets,
							TFunction<void(AActor*, const TMap<FName, UObject*>&)> CompletionCallback);
	
	/** Cancel asset loading for a specific actor */
	void CancelAssetLoadingForActor(AActor* Actor);
	
	/** Check if all actor asset loads are complete */
	bool AreAllActorAssetsLoaded() const;
	

#if !UE_BUILD_SHIPPING
	void StartTrackObject(UObject* Object);
	void StopTrackObject(UObject* Object);
	void StopTrackAllObjects();
#endif // !UE_BUILD_SHIPPING

public:
	const FName& GetClassName() const;
	int32& GetSerialNumberGeneratorChecked() const;

public:
	UPROPERTY(Transient)
	TArray<int32> FreeItemIndices;

	UPROPERTY(Transient)
	TArray<FRecallObjectPoolItem> Items;

protected:
	UFUNCTION()
	void OnDestroyed(AActor* DestroyedActor);
	
	/** Start async loading of the actor class */
	void StartAsyncClassLoading();
	
private:
	FName ClassName = NAME_None;
	TWeakPtr<int32> SerialNumberGenerator;
	
	/** Async loading state */
	FSoftClassPath SoftClassPath;
	TSharedPtr<FStreamableHandle> StreamableHandle;
	bool bIsAsyncLoading = false;
	bool bClassLoaded = false;
	
	/** Actor asset loading tracking */
	TArray<FActorAssetLoadingState> PendingActorLoads;
	
	/** Callback for when async loading completes */
	void OnAsyncLoadingComplete();
	
	/** Callback for when actor asset loading completes */
	void OnActorAssetLoadComplete();
};

class RECALLSIMULATION_API FRecallObjectPoolBase
{
public:
	static void SetActorActive(AActor* Actor, bool bIsActive);

protected:
	const URecallObjectPoolContainer& GetContainer() const;
	URecallObjectPoolContainer& GetMutableContainer();

	const TArray<FRecallObjectPoolItem>& GetItems() const;
	TArray<FRecallObjectPoolItem>& GetMutableItems();

	TArray<int32>& GetMutableFreeItemIndices();
	const TArray<int32>& GetFreeItemIndices() const;

	const FName& GetClassName() const;
	int32& GetMutableSerialNumberGenerator();
	
	URecallObjectPoolContainer* GetPoolContainer_Internal() const;
	void SetPoolContainer_Internal(const TWeakObjectPtr<URecallObjectPoolContainer>& InContainer);

private:
	TWeakObjectPtr<URecallObjectPoolContainer> Container;
};

using FRecallObjectPoolCommandFunc = TFunction<void()>;

struct RECALLSIMULATION_API FRecallObjectPoolCommand
{
	FRecallObjectPoolCommandFunc Callback;
};

/**
* Template pool to re-use the same objects.
* It gives us an actor handle to reference the object.
* We can pass a description object to initialize our object.
* We can restore an object from a handle.
*/
/**
 * Asset key constants to avoid magic strings
 */
namespace RecallAssetKeys
{
	static constexpr const TCHAR* MESH = TEXT("Mesh");
	static constexpr const TCHAR* ANIM_BP = TEXT("AnimBP");
	static constexpr const TCHAR* OVERLAY_MATERIAL = TEXT("OverlayMaterial");
	static constexpr const TCHAR* MATERIAL = TEXT("Material");
	static constexpr const TCHAR* LEVEL_SEQUENCE = TEXT("LevelSequence");
	static constexpr const TCHAR* MATERIAL_OVERRIDE_PREFIX = TEXT("Material_");
}

class RECALLSIMULATION_API FRecallObjectPool :
	public FRecallObjectPoolBase, public IRecallObjectPoolInterface
{
	// IRecallObjectPoolInterface BEGIN
public:
	virtual void SetPoolContainer(const TWeakObjectPtr<URecallObjectPoolContainer>& InContainer) override final;
	virtual URecallObjectPoolContainer* GetPoolContainer() const override final;
	virtual void FlushCommands() override final;

	virtual void ReleaseObject(const FRecallActorHandle& Handle) override final;
	virtual void ReleaseObjectAtIndex(int32 Index) override final;
	virtual void ReleaseAllObjects() override final;

	virtual TArray<int32> GetActiveIndices() const override final;

	virtual FRecallActorHandle GetOwnerHandleAtIndex(int32 Index) const override final;

	virtual TWeakObjectPtr<AActor> GetObject(const FRecallActorHandle& Handle) const override;

	virtual FRecallActorHandle RequestObject(const FInstancedStruct& Desc, const FRecallActorHandle* RestoreHandle = nullptr) override;
	// IRecallObjectPoolInterface END

	bool IsEmpty() const { return GetItems().IsEmpty(); }

public:
	void ReserveObjects(const FInstancedStruct& Desc, int32 Count = 1, bool bReleaseToPool = true);

protected:
	virtual AActor* CreateObject(const FInstancedStruct& Desc) = 0;
	virtual void InitObject(AActor* Object, const FInstancedStruct& Desc) = 0;

	virtual void EnableObject(AActor* Object) = 0;
	virtual void DisableObject(AActor* Object) = 0;

	UClass* GetActorClass() const;

	template<typename T>
	TSubclassOf<T> GetActorClass()
	{
		if (UClass* ActorClass = GetActorClass())
		{
			return ActorClass;
		}
		return T::StaticClass();
	}

	URecallObjectPoolContainer& GetPoolContainerChecked() const;

	/** Helper to create material override asset keys with slot names instead of indices */
	static FName CreateMaterialOverrideKey(const FName& SlotName);

	/** Helper to collect common material override assets */
	template<typename TMaterialOverrideArray>
	static void CollectMaterialOverrideAssets(const TMaterialOverrideArray& MaterialOverrides, TMap<FName, FSoftObjectPath>& OutAssetsToLoad);

private:
	FCriticalSection CommandGuard;
	TArray<FRecallObjectPoolCommand> Commands;

	void PushCommand(FRecallObjectPoolCommandFunc NewCommand);
	bool CanFlushCommands() const;
};
