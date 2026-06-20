// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallObjectPoolTypes.h"

#include "Actor/RecallActorPoolInterface.h"
#include "System/Actor/RecallActorTypes.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

//----------------------------------------------------------------------//
// URecallObjectPoolContainer
//----------------------------------------------------------------------//
void URecallObjectPoolContainer::InitContainerAsync(const FName& InClassName,
	const FSoftClassPath& InSoftClassPath, const TWeakPtr<int32>& InSerialNumberGenerator)
{
	checkf(!InSoftClassPath.IsNull(), TEXT("%hs Cannot initialize container with null SoftClassPath"), __FUNCTION__);
	
	SoftClassPath = InSoftClassPath;
	SerialNumberGenerator = InSerialNumberGenerator;
	ClassName = InClassName;
	bClassLoaded = false;
	bIsAsyncLoading = false;

	StartAsyncClassLoading();
}

#if !UE_BUILD_SHIPPING
void URecallObjectPoolContainer::StartTrackObject(UObject* Object)
{
	if (AActor* Actor = Cast<AActor>(Object))
	{
		Actor->OnDestroyed.AddDynamic(this, &ThisClass::OnDestroyed);
	}
}

void URecallObjectPoolContainer::StopTrackObject(UObject* Object)
{
	if (AActor* Actor = Cast<AActor>(Object))
	{
		Actor->OnDestroyed.RemoveDynamic(this, &ThisClass::OnDestroyed);
	}
}

void URecallObjectPoolContainer::StopTrackAllObjects()
{
	for (const FRecallObjectPoolItem& Item : Items)
	{
		StopTrackObject(Item.Object);
	}
}
#endif // !UE_BUILD_SHIPPING

void URecallObjectPoolContainer::OnDestroyed(AActor* DestroyedActor)
{
	ensureMsgf(false, TEXT("Do not destroy pooled actor at runtime"));
}

const FName& URecallObjectPoolContainer::GetClassName() const
{
	return ClassName; 
}

int32& URecallObjectPoolContainer::GetSerialNumberGeneratorChecked() const
{
	check(SerialNumberGenerator.IsValid());
	return *SerialNumberGenerator.Pin().Get();
}

void URecallObjectPoolContainer::StartAsyncClassLoading()
{
	if (bClassLoaded || bIsAsyncLoading)
	{
		return; // Already loaded, loading, or no path to load
	}
	
	if (SoftClassPath.IsNull())
	{
		bClassLoaded = true;
		return;
	}
	
	bIsAsyncLoading = true;

	TWeakObjectPtr<URecallObjectPoolContainer> WeakSelf = this;
	AsyncTask(ENamedThreads::GameThread, [WeakSelf]() mutable {
		if (WeakSelf.IsValid())
		{
			UAssetManager& AssetManager = UAssetManager::Get();
			FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();
	
			WeakSelf->StreamableHandle = StreamableManager.RequestAsyncLoad(
				WeakSelf->SoftClassPath,
				FStreamableDelegate::CreateUObject(WeakSelf.Get(), &ThisClass::OnAsyncLoadingComplete)
			);
		}
	});
}

bool URecallObjectPoolContainer::IsClassLoaded() const
{
	return bClassLoaded;
}

UClass* URecallObjectPoolContainer::GetLoadedClass() const
{
	if (!bClassLoaded)
	{
		return nullptr;
	}
	
	if (!SoftClassPath.IsNull())
	{
		return SoftClassPath.TryLoadClass<UObject>();
	}
	
	// Fallback for immediate mode - try to find class by name
	return Cast<UClass>(StaticFindObject(UClass::StaticClass(), nullptr, *ClassName.ToString()));
}


void URecallObjectPoolContainer::RequestActorAssets(AActor* Actor, 
	const TMap<FName, FSoftObjectPath>& Assets,
	TFunction<void(AActor*, const TMap<FName, UObject*>&)> CompletionCallback)
{
	if (!Actor)
	{
		return;
	}

	// Create new loading state for this actor
	FActorAssetLoadingState& LoadingState = PendingActorLoads.AddDefaulted_GetRef();
	LoadingState.Actor = Actor;
	LoadingState.RequestedAssets = Assets;
	LoadingState.CompletionCallback = CompletionCallback;

	// Start async loading for each asset
	UAssetManager& AssetManager = UAssetManager::Get();
	FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();

	for (const auto& AssetPair : Assets)
	{
		if (!AssetPair.Value.IsNull())
		{
			TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(
				AssetPair.Value,
				FStreamableDelegate::CreateUObject(this, &URecallObjectPoolContainer::OnActorAssetLoadComplete)
			);
			LoadingState.ActiveHandles.Add(AssetPair.Key, Handle);
		}
	}

	// Check if already completed (for cases where assets are already loaded)
	OnActorAssetLoadComplete();
}

void URecallObjectPoolContainer::CancelAssetLoadingForActor(AActor* Actor)
{
	for (int32 i = PendingActorLoads.Num() - 1; i >= 0; i--)
	{
		if (PendingActorLoads[i].Actor == Actor)
		{
			// Cancel all active handles
			for (auto& HandlePair : PendingActorLoads[i].ActiveHandles)
			{
				if (HandlePair.Value.IsValid())
				{
					HandlePair.Value->CancelHandle();
				}
			}
			
			PendingActorLoads.RemoveAt(i);
			break;
		}
	}
}

bool URecallObjectPoolContainer::AreAllActorAssetsLoaded() const
{
	return PendingActorLoads.IsEmpty();
}

void URecallObjectPoolContainer::OnActorAssetLoadComplete()
{
	// Check all pending actor loads for completion
	for (int32 i = PendingActorLoads.Num() - 1; i >= 0; i--)
	{
		FActorAssetLoadingState& LoadingState = PendingActorLoads[i];
		
		// Skip if actor is no longer valid
		if (!LoadingState.Actor.IsValid())
		{
			PendingActorLoads.RemoveAt(i);
			continue;
		}
		
		// Check if all assets for this actor are loaded
		bool bAllLoaded = true;
		TMap<FName, UObject*> LoadedAssets;
		
		for (const auto& HandlePair : LoadingState.ActiveHandles)
		{
			if (HandlePair.Value.IsValid() && HandlePair.Value->HasLoadCompleted())
			{
				if (UObject* LoadedAsset = HandlePair.Value->GetLoadedAsset())
				{
					LoadedAssets.Add(HandlePair.Key, LoadedAsset);
				}
				else
				{
					// Asset failed to load
					UE_LOG(LogRecallActor, Warning, TEXT("Failed to load asset: %s for actor: %s"), 
						*HandlePair.Key.ToString(), *LoadingState.Actor->GetName());
					bAllLoaded = false;
					break;
				}
			}
			else
			{
				// Still loading
				bAllLoaded = false;
				break;
			}
		}
		
		// If all assets loaded, execute callback and remove from pending
		if (bAllLoaded)
		{
			LoadingState.CompletionCallback(LoadingState.Actor.Get(), LoadedAssets);
			PendingActorLoads.RemoveAt(i);
		}
	}
}

void URecallObjectPoolContainer::OnAsyncLoadingComplete()
{
	bIsAsyncLoading = false;
	bClassLoaded = true;
	StreamableHandle.Reset();
	
	UE_LOG(LogRecallActor, Log, TEXT("Async loading complete for class: %s"), *SoftClassPath.ToString());
	
	// Notify the actor subsystem that this container's class is now loaded
	// The subsystem will process pending commands for this container
}

//----------------------------------------------------------------------//
// FRecallObjectPoolBase
//----------------------------------------------------------------------//
void FRecallObjectPoolBase::SetActorActive(AActor* Actor, bool bIsActive)
{
	if (IsValid(Actor))
	{
		if (!bIsActive)
		{
			Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}

		Actor->SetActorHiddenInGame(!bIsActive);
		Actor->SetActorTickEnabled(bIsActive);
		Actor->SetActorEnableCollision(bIsActive);

		for (UActorComponent* ActorComponent : Actor->GetComponents())
		{
			if (ActorComponent->IsA<UNiagaraComponent>())
			{
				CastChecked<UNiagaraComponent>(ActorComponent)->SetPaused(!bIsActive);
			}
			else
			{
				ActorComponent->SetComponentTickEnabled(bIsActive);
			}
		}
	}
}

void FRecallObjectPoolBase::SetPoolContainer_Internal(const TWeakObjectPtr<URecallObjectPoolContainer>& InContainer)
{
	Container = InContainer;
}

URecallObjectPoolContainer& FRecallObjectPoolBase::GetMutableContainer()
{
	check(Container.IsValid());
	return *Container.Get();
}

const URecallObjectPoolContainer& FRecallObjectPoolBase::GetContainer() const
{
	check(Container.IsValid());
	return *Container.Get();
}

const FName& FRecallObjectPoolBase::GetClassName() const
{
	return GetContainer().GetClassName();
}

URecallObjectPoolContainer* FRecallObjectPoolBase::GetPoolContainer_Internal() const
{
	return Container.Get();
}

TArray<FRecallObjectPoolItem>& FRecallObjectPoolBase::GetMutableItems()
{
	return GetMutableContainer().Items;
}

const TArray<FRecallObjectPoolItem>& FRecallObjectPoolBase::GetItems() const
{
	return GetContainer().Items;
}

TArray<int32>& FRecallObjectPoolBase::GetMutableFreeItemIndices()
{
	return GetMutableContainer().FreeItemIndices;
}

const TArray<int32>& FRecallObjectPoolBase::GetFreeItemIndices() const
{
	return GetContainer().FreeItemIndices;
}

int32& FRecallObjectPoolBase::GetMutableSerialNumberGenerator()
{
	return GetMutableContainer().GetSerialNumberGeneratorChecked();
}

//----------------------------------------------------------------------//
// FRecallObjectPool
//----------------------------------------------------------------------//
void FRecallObjectPool::SetPoolContainer(const TWeakObjectPtr<URecallObjectPoolContainer>& InContainer)
{
	SetPoolContainer_Internal(InContainer);
}

URecallObjectPoolContainer* FRecallObjectPool::GetPoolContainer() const
{
	return GetPoolContainer_Internal();
}

void FRecallObjectPool::FlushCommands()
{
	if (!CanFlushCommands())
	{
		return;
	}

	FScopeLock CommandLock(&CommandGuard);
	for (const FRecallObjectPoolCommand& Command : Commands)
	{
		Command.Callback();
	}
	Commands.Reset();
}

void FRecallObjectPool::ReleaseObject(const FRecallActorHandle& Handle)
{
	check(Handle.ClassName == GetClassName());

	const int32 Index = Handle.Index;

	check(GetItems().IsValidIndex(Index));
	check(Handle == GetItems()[Index].OwnerHandle);

	ReleaseObjectAtIndex(Index);
}

void FRecallObjectPool::ReleaseObjectAtIndex(int32 Index)
{
	if (ensureMsgf(GetFreeItemIndices().Contains(Index) == false, TEXT("This object has already been released.")) &&
		ensure(GetItems().IsValidIndex(Index)))
	{
		PushCommand([this, Index]()
			{
				FRecallObjectPoolItem& NewItem = GetMutableItems()[Index];
				AActor* Actor = CastChecked<AActor>(NewItem.Object);
				
				// Cancel any pending asset loading for this actor
				GetPoolContainerChecked().CancelAssetLoadingForActor(Actor);
				
				DisableObject(Actor);
			
				if (Actor->GetClass()->ImplementsInterface(URecallActorPoolInterface::StaticClass()))
				{
					IRecallActorPoolInterface::Execute_OnPoolDisable(Actor);
				}
			}
		);
		GetMutableFreeItemIndices().Add(Index);
		GetMutableItems()[Index].OwnerHandle.Invalidate();
	}
}

void FRecallObjectPool::ReleaseAllObjects()
{
	const TArray<int32> ActiveIndices = GetActiveIndices();

	for (const int32 Index : ActiveIndices)
	{
		ReleaseObjectAtIndex(Index);
	}
}

TArray<int32> FRecallObjectPool::GetActiveIndices() const
{
	TArray<int32> ActiveIndices;
	ActiveIndices.Reserve(GetItems().Num() - GetFreeItemIndices().Num());

	for (int32 Index = 0; Index < GetItems().Num(); Index++)
	{
		if (GetFreeItemIndices().Contains(Index))
		{
			continue;
		}

		ActiveIndices.Add(Index);
	}

	return ActiveIndices;
}

FRecallActorHandle FRecallObjectPool::GetOwnerHandleAtIndex(int32 Index) const
{
	if (GetItems().IsValidIndex(Index))
	{
		check(!GetFreeItemIndices().Contains(Index) || !GetItems()[Index].OwnerHandle.IsValid());
		return GetItems()[Index].OwnerHandle;
	}
	else
	{
		return FRecallActorHandle();
	}
}

TWeakObjectPtr<AActor> FRecallObjectPool::GetObject(const FRecallActorHandle& Handle) const
{
	check(GetClassName() == Handle.ClassName);
	check(GetItems().IsValidIndex(Handle.Index));
	if (GetItems()[Handle.Index].OwnerHandle == Handle)
	{
		return Cast<AActor>(GetItems()[Handle.Index].Object);
	}

	return nullptr;
}

FRecallActorHandle FRecallObjectPool::RequestObject(const FInstancedStruct& Desc,
	const FRecallActorHandle* RestoreHandle)
{
	int32 Index = INDEX_NONE;

	if (RestoreHandle != nullptr)
	{
		// Additional diagnostics before the main check
		checkf(GetPoolContainer() != nullptr, TEXT("%hs PoolContainer is invalid during restore"), __FUNCTION__);
		checkf(RestoreHandle->ClassName != NAME_None, 
			TEXT("%hs RestoreHandle has invalid ClassName='%s'"), __FUNCTION__, *RestoreHandle->ClassName.ToString());
		
		// Now we use the soft class path for consistent naming, so this should match
		checkf(GetClassName() == RestoreHandle->ClassName, 
			TEXT("Pool class name mismatch during restore: Pool='%s' vs Handle='%s' for container %p"), 
			*GetClassName().ToString(), *RestoreHandle->ClassName.ToString(), &GetContainer());
		Index = RestoreHandle->Index;
			
		if (GetFreeItemIndices().Contains(Index))
		{
			GetMutableFreeItemIndices().Remove(Index);
		}
		else
		{
			check(Index >= GetItems().Num());
			const int32 FillItemCount = Index - GetItems().Num();
			ReserveObjects(Desc, FillItemCount, true);
			ReserveObjects(Desc, 1, false);
		}
	}
	else
	{
		if (GetFreeItemIndices().Num() > 0)
		{
			Index = GetMutableFreeItemIndices().Pop();
		}
		else
		{
			Index = GetItems().Num();
			ReserveObjects(Desc, 1, false);
		}
	}

	PushCommand([this, Index, Desc]()
		{
			const FRecallObjectPoolItem& Item = GetItems()[Index];
			AActor* Actor = CastChecked<AActor>(Item.Object);

			InitObject(Actor, Desc);
			EnableObject(Actor);

			if (Actor->GetClass()->ImplementsInterface(URecallActorPoolInterface::StaticClass()))
			{
				IRecallActorPoolInterface::Execute_OnPoolEnable(Actor);
			}
		}
	);

	FRecallActorHandle NewHandle;

	if (RestoreHandle != nullptr)
	{
		NewHandle = (*RestoreHandle);
	}
	else
	{
		NewHandle = FRecallActorHandle::Make(GetClassName(), Index, ++GetMutableSerialNumberGenerator());
	}

	GetMutableItems()[Index].OwnerHandle = NewHandle;
	return NewHandle;
}

void FRecallObjectPool::ReserveObjects(const FInstancedStruct& Desc, int32 Count, bool bReleaseToPool)
{
	for (int32 Index = 0; Index < Count; Index++)
	{
		const int32 NewItemIndex = GetItems().Num();

		FRecallObjectPoolItem& NewItem = GetMutableItems().AddDefaulted_GetRef();

		PushCommand([this, Desc, NewItemIndex]()
			{
				FRecallObjectPoolItem& NewItem = GetMutableItems()[NewItemIndex];
				NewItem.Object = CreateObject(Desc);

#if !UE_BUILD_SHIPPING
				GetMutableContainer().StartTrackObject(NewItem.Object);
#endif // !UE_BUILD_SHIPPING
			}
		);

		if (bReleaseToPool)
		{
			ReleaseObjectAtIndex(NewItemIndex);
		}
	}
}

UClass* FRecallObjectPool::GetActorClass() const
{
	// Check if class loading is complete before processing commands
	URecallObjectPoolContainer* PoolContainer = GetPoolContainer();
	check(PoolContainer && PoolContainer->IsClassLoaded());
	return PoolContainer->GetLoadedClass();
}

URecallObjectPoolContainer& FRecallObjectPool::GetPoolContainerChecked() const
{
	URecallObjectPoolContainer* PoolContainer = GetPoolContainer();
	check(PoolContainer);
	return *PoolContainer;
}

void FRecallObjectPool::PushCommand(FRecallObjectPoolCommandFunc NewCommand)
{
	if (IsInGameThread() && CanFlushCommands())
	{
		NewCommand();
	}
	else
	{
		FScopeLock CommandLock(&CommandGuard);
		FRecallObjectPoolCommand& Command = Commands.AddDefaulted_GetRef();
		Command.Callback = NewCommand;
	}
}

bool FRecallObjectPool::CanFlushCommands() const
{
	// Check if both class loading and actor asset loading are complete
	URecallObjectPoolContainer* PoolContainer = GetPoolContainer();
	if (!ensure(PoolContainer) || !PoolContainer->IsClassLoaded() || !PoolContainer->AreAllActorAssetsLoaded())
	{
		// Commands will be flushed later when all loading completes
		return false;
	}

	return true;
}

FName FRecallObjectPool::CreateMaterialOverrideKey(const FName& SlotName)
{
	return FName(*FString::Printf(TEXT("%s%s"), RecallAssetKeys::MATERIAL_OVERRIDE_PREFIX, *SlotName.ToString()));
}

template<typename TMaterialOverrideArray>
void FRecallObjectPool::CollectMaterialOverrideAssets(const TMaterialOverrideArray& MaterialOverrides, TMap<FName, FSoftObjectPath>& OutAssetsToLoad)
{
	for (const auto& MaterialOverride : MaterialOverrides)
	{
		if (!MaterialOverride.MaterialInterface.IsNull())
		{
			const FName AssetKey = CreateMaterialOverrideKey(MaterialOverride.MaterialSlotName);
			OutAssetsToLoad.Add(AssetKey, MaterialOverride.MaterialInterface);
		}
	}
}
