// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallActorPoolTypes.h"

#include "Animation/AnimInstance.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DecalActor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Representation/Actor/RecallActorMeshRepresentationTypes.h"
#include "Representation/Actor/Pool/RecallObjectPoolTypes.h"
#include "StructUtils/InstancedStruct.h"

//----------------------------------------------------------------------//
// FRecallActorPool
//----------------------------------------------------------------------//
template<typename ActorType>
static ActorType* SpawnPoolActor(const TWeakObjectPtr<UWorld>& World, const TSubclassOf<ActorType>& Class)
{
	if (World.IsValid())
	{
		ActorType* NewActor = World->SpawnActorDeferred<ActorType>(Class, FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		NewActor->bOnlyRelevantToOwner = true;
		NewActor->FinishSpawning(FTransform::Identity);
		return NewActor;
	}

	return nullptr;
}

AActor* FRecallActorPool::CreateObject(const FInstancedStruct& Desc)
{
	const FActorRepresentationDesc& ActorDesc = Desc.Get<FActorRepresentationDesc>();
	return SpawnPoolActor<AActor>(GetContainer().GetWorld(), GetActorClass<AActor>());
}

void FRecallActorPool::InitObject(AActor* Actor, const FInstancedStruct& Desc)
{
	const FActorRepresentationDesc& ActorDesc = Desc.Get<FActorRepresentationDesc>();
	if (IsValid(Actor))
	{
		if (ActorDesc.bResetMaterialsOnInit)
		{
			Actor->ForEachComponent<UDecalComponent>(false, [](UDecalComponent* Component)
			{
				if (IsValid(Component))
				{
					if (const UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Component->GetDecalMaterial()))
					{
						Component->SetDecalMaterial(DynamicMaterial->Parent);
					}
					Component->SetDecalColor(FColor::White);
				}
			});
		
			Actor->ForEachComponent<UMeshComponent>(false, [](UMeshComponent* Component)
			{
				if (IsValid(Component))
				{
					Component->EmptyOverrideMaterials();
				}
			});
		}
			
		Actor->ForEachComponent<UStaticMeshComponent>(false, [](UStaticMeshComponent* Component)
		{
			if (IsValid(Component))
			{
				Component->SetVisibility(true, false);
				// Component->SetSimulatePhysics(false);
				// Component->SetMobility(EComponentMobility::Movable);
			}
		});
		
		Actor->ForEachComponent<USkeletalMeshComponent>(false, [](USkeletalMeshComponent* Component)
		{
			if (IsValid(Component))
			{
				Component->SetVisibility(true, false);
				// Component->SetSimulatePhysics(false);

				// Give us direct control of the tick.
				Component->EnableExternalTickRateControl(true);
			}
		});
	}
}

void FRecallActorPool::EnableObject(AActor* Actor)
{
	SetActorActive(Actor, true);
}

void FRecallActorPool::DisableObject(AActor* Actor)
{
	SetActorActive(Actor, false);
}

//----------------------------------------------------------------------//
// FRecallStaticMeshActorPool
//----------------------------------------------------------------------//
AActor* FRecallStaticMeshActorPool::CreateObject(const FInstancedStruct& Desc)
{
	const FStaticMeshRepresentationMeshDesc& MeshDesc = Desc.Get<FStaticMeshRepresentationMeshDesc>();
	return SpawnPoolActor<AStaticMeshActor>(GetContainer().GetWorld(), GetActorClass<AStaticMeshActor>());
}

void FRecallStaticMeshActorPool::InitObject(AActor* Actor, const FInstancedStruct& Desc)
{	
	if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor))
	{
		const FStaticMeshRepresentationMeshDesc& MeshDesc = Desc.Get<FStaticMeshRepresentationMeshDesc>();
		
		if (UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent())
		{
			// Set up basic properties that don't require asset loading
			StaticMeshComponent->SetVisibility(true, true);
			StaticMeshComponent->SetSimulatePhysics(false);
			StaticMeshComponent->SetMobility(EComponentMobility::Movable);
			StaticMeshComponent->SetCustomDepthStencilValue(MeshDesc.CustomDepthStencilValue);
			StaticMeshComponent->EmptyOverrideMaterials();

			// Collect assets that need loading
			TMap<FName, FSoftObjectPath> AssetsToLoad;
			if (!MeshDesc.Mesh.IsNull())
			{
				AssetsToLoad.Add(RecallAssetKeys::MESH, MeshDesc.Mesh);
			}
			if (!MeshDesc.OverlayMaterial.IsNull())
			{
				AssetsToLoad.Add(RecallAssetKeys::OVERLAY_MATERIAL, MeshDesc.OverlayMaterial);
			}
			
			// Add material overrides using slot names instead of indices
			CollectMaterialOverrideAssets(MeshDesc.MaterialOverrides, AssetsToLoad);

			// Request async asset loading with callback
			URecallObjectPoolContainer& PoolContainer = GetPoolContainerChecked();
			PoolContainer.RequestActorAssets(Actor, AssetsToLoad, 
				[this, MeshDesc](AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets)
				{
					OnStaticMeshAssetsLoaded(LoadedActor, LoadedAssets, MeshDesc);
				});
		}
	}
}

void FRecallStaticMeshActorPool::EnableObject(AActor* Actor)
{
	SetActorActive(Actor, true);
}

void FRecallStaticMeshActorPool::DisableObject(AActor* Actor)
{
	SetActorActive(Actor, false);
}

void FRecallStaticMeshActorPool::OnStaticMeshAssetsLoaded(AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets, const FStaticMeshRepresentationMeshDesc& MeshDesc)
{
	if (AStaticMeshActor* LoadedStaticMeshActor = Cast<AStaticMeshActor>(LoadedActor))
	{
		if (UStaticMeshComponent* LoadedComponent = LoadedStaticMeshActor->GetStaticMeshComponent())
		{
			// Apply loaded assets
			if (UStaticMesh* LoadedMesh = Cast<UStaticMesh>(LoadedAssets.FindRef(RecallAssetKeys::MESH)))
			{
				LoadedComponent->SetStaticMesh(LoadedMesh);
			}
			
			if (UMaterialInterface* LoadedOverlayMaterial = Cast<UMaterialInterface>(LoadedAssets.FindRef(RecallAssetKeys::OVERLAY_MATERIAL)))
			{
				LoadedComponent->SetOverlayMaterial(LoadedOverlayMaterial);
			}

			// Apply material overrides using slot names
			for (const auto& MaterialOverride : MeshDesc.MaterialOverrides)
			{
				const FName AssetKey = CreateMaterialOverrideKey(MaterialOverride.MaterialSlotName);
				if (UMaterialInterface* LoadedMaterial = Cast<UMaterialInterface>(LoadedAssets.FindRef(AssetKey)))
				{
					LoadedComponent->SetMaterialByName(MaterialOverride.MaterialSlotName, LoadedMaterial);
				}
			}
		}
	}
}

//----------------------------------------------------------------------//
// FRecallSkeletalMeshActorPool
//----------------------------------------------------------------------//
AActor* FRecallSkeletalMeshActorPool::CreateObject(const FInstancedStruct& Desc)
{
	const FSkeletalMeshRepresentationMeshDesc& MeshDesc = Desc.Get<FSkeletalMeshRepresentationMeshDesc>();
	return SpawnPoolActor<ASkeletalMeshActor>(GetContainer().GetWorld(), GetActorClass<ASkeletalMeshActor>());
}

void FRecallSkeletalMeshActorPool::InitObject(AActor* Actor, const FInstancedStruct& Desc)
{
	if (ASkeletalMeshActor* SkeletalMeshActor = Cast<ASkeletalMeshActor>(Actor))
	{
		const FSkeletalMeshRepresentationMeshDesc& MeshDesc = Desc.Get<FSkeletalMeshRepresentationMeshDesc>();
		
		if (USkeletalMeshComponent* SkeletalMeshComponent = SkeletalMeshActor->GetSkeletalMeshComponent())
		{
			// Set up basic properties that don't require asset loading
			SkeletalMeshComponent->SetVisibility(true, true);
			SkeletalMeshComponent->SetSimulatePhysics(false);
			SkeletalMeshComponent->SetCustomDepthStencilValue(MeshDesc.CustomDepthStencilValue);
			SkeletalMeshComponent->EnableExternalTickRateControl(true);
			SkeletalMeshComponent->EmptyOverrideMaterials();

			// Collect assets that need loading
			TMap<FName, FSoftObjectPath> AssetsToLoad;
			if (!MeshDesc.Mesh.IsNull())
			{
				AssetsToLoad.Add(RecallAssetKeys::MESH, MeshDesc.Mesh);
			}
			if (!MeshDesc.AnimBP.IsNull())
			{
				AssetsToLoad.Add(RecallAssetKeys::ANIM_BP, MeshDesc.AnimBP);
			}
			if (!MeshDesc.OverlayMaterial.IsNull())
			{
				AssetsToLoad.Add(RecallAssetKeys::OVERLAY_MATERIAL, MeshDesc.OverlayMaterial);
			}
			
			// Add material overrides using slot names instead of indices
			CollectMaterialOverrideAssets(MeshDesc.MaterialOverrides, AssetsToLoad);

			// Request async asset loading with callback
			URecallObjectPoolContainer& PoolContainer = GetPoolContainerChecked();
			PoolContainer.RequestActorAssets(Actor, AssetsToLoad, 
				[this, MeshDesc](AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets)
				{
					OnSkeletalMeshAssetsLoaded(LoadedActor, LoadedAssets, MeshDesc);
				});
		}
	}
}

void FRecallSkeletalMeshActorPool::EnableObject(AActor* Actor)
{
	SetActorActive(Actor, true);
}

void FRecallSkeletalMeshActorPool::DisableObject(AActor* Actor)
{
	SetActorActive(Actor, false);
}

void FRecallSkeletalMeshActorPool::OnSkeletalMeshAssetsLoaded(AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets, const FSkeletalMeshRepresentationMeshDesc& MeshDesc)
{
	if (ASkeletalMeshActor* LoadedSkeletalMeshActor = Cast<ASkeletalMeshActor>(LoadedActor))
	{
		if (USkeletalMeshComponent* LoadedComponent = LoadedSkeletalMeshActor->GetSkeletalMeshComponent())
		{
			// Apply loaded assets
			if (USkeletalMesh* LoadedMesh = Cast<USkeletalMesh>(LoadedAssets.FindRef(RecallAssetKeys::MESH)))
			{
				LoadedComponent->SetSkeletalMesh(LoadedMesh);
			}
			
			if (UClass* LoadedAnimBP = Cast<UClass>(LoadedAssets.FindRef(RecallAssetKeys::ANIM_BP)))
			{
				LoadedComponent->SetAnimInstanceClass(LoadedAnimBP);
			}
			
			if (UMaterialInterface* LoadedOverlayMaterial = Cast<UMaterialInterface>(LoadedAssets.FindRef(RecallAssetKeys::OVERLAY_MATERIAL)))
			{
				LoadedComponent->SetOverlayMaterial(LoadedOverlayMaterial);
			}

			// Apply material overrides using slot names
			for (const auto& MaterialOverride : MeshDesc.MaterialOverrides)
			{
				const FName AssetKey = CreateMaterialOverrideKey(MaterialOverride.MaterialSlotName);
				if (UMaterialInterface* LoadedMaterial = Cast<UMaterialInterface>(LoadedAssets.FindRef(AssetKey)))
				{
					LoadedComponent->SetMaterialByName(MaterialOverride.MaterialSlotName, LoadedMaterial);
				}
			}
		}
	}
}

//----------------------------------------------------------------------//
// FRecallLevelSequenceActorPool
//----------------------------------------------------------------------//
AActor* FRecallLevelSequenceActorPool::CreateObject(const FInstancedStruct& Desc)
{
	const FLevelSequenceActorRepresentationDesc& LevelSequenceDesc = Desc.Get<FLevelSequenceActorRepresentationDesc>();
	return SpawnPoolActor<ALevelSequenceActor>(GetContainer().GetWorld(), GetActorClass<ALevelSequenceActor>());
}

void FRecallLevelSequenceActorPool::InitObject(AActor* Actor, const FInstancedStruct& Desc)
{
	const FLevelSequenceActorRepresentationDesc& LevelSequenceDesc = Desc.Get<FLevelSequenceActorRepresentationDesc>();
	
	if (ALevelSequenceActor* LevelSequenceActor = Cast<ALevelSequenceActor>(Actor))
	{
		// Collect assets that need loading
		TMap<FName, FSoftObjectPath> AssetsToLoad;
		if (!LevelSequenceDesc.LevelSequenceAsset.IsNull())
		{
			AssetsToLoad.Add(RecallAssetKeys::LEVEL_SEQUENCE, LevelSequenceDesc.LevelSequenceAsset);
		}

		// Request async asset loading with callback
		URecallObjectPoolContainer& PoolContainer = GetPoolContainerChecked();
		PoolContainer.RequestActorAssets(Actor, AssetsToLoad, 
			[this](AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets)
			{
				OnLevelSequenceAssetsLoaded(LoadedActor, LoadedAssets);
			});
	}
}

void FRecallLevelSequenceActorPool::EnableObject(AActor* Actor)
{
	SetActorActive(Actor, true);
}

void FRecallLevelSequenceActorPool::DisableObject(AActor* Actor)
{
	if (ALevelSequenceActor* LevelSequenceActor = Cast<ALevelSequenceActor>(Actor))
	{
		if (LevelSequenceActor->GetSequencePlayer())
		{
			if (LevelSequenceActor->GetSequencePlayer()->IsPlaying())
			{
				LevelSequenceActor->GetSequencePlayer()->Stop();
			}

			LevelSequenceActor->SetSequence(nullptr);
		}
	}

	SetActorActive(Actor, false);
}

void FRecallLevelSequenceActorPool::OnLevelSequenceAssetsLoaded(AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets)
{
	if (ALevelSequenceActor* LoadedLevelSequenceActor = Cast<ALevelSequenceActor>(LoadedActor))
	{
		// Apply loaded level sequence
		if (ULevelSequence* LoadedSequence = Cast<ULevelSequence>(LoadedAssets.FindRef(RecallAssetKeys::LEVEL_SEQUENCE)))
		{
			LoadedLevelSequenceActor->SetSequence(LoadedSequence);
		}
	}
}

//----------------------------------------------------------------------//
// FRecallDecalActorPool
//----------------------------------------------------------------------//
AActor* FRecallDecalActorPool::CreateObject(const FInstancedStruct& Desc)
{
	const FRecallDecalActorRepresentationDesc& DecalDesc = Desc.Get<FRecallDecalActorRepresentationDesc>();
	return SpawnPoolActor<ADecalActor>(GetContainer().GetWorld(), GetActorClass<ADecalActor>());
}

void FRecallDecalActorPool::InitObject(AActor* Actor, const FInstancedStruct& Desc)
{
	const FRecallDecalActorRepresentationDesc& DecalDesc = Desc.Get<FRecallDecalActorRepresentationDesc>();
	
	if (ADecalActor* DecalActor = Cast<ADecalActor>(Actor))
	{
		// Set up basic properties that don't require asset loading
		if (UDecalComponent* Decal = DecalActor->GetDecal())
		{
			Decal->SetDecalColor(DecalDesc.DecalColor);
			Decal->DecalSize = DecalDesc.DecalSize;
			Decal->SetSortOrder(DecalDesc.DecalSortOrder);
			Decal->SetFadeIn(DecalDesc.DecalFadeInStartDelay, DecalDesc.DecalFadeInDuration);
		}

		// Collect assets that need loading
		TMap<FName, FSoftObjectPath> AssetsToLoad;
		if (!DecalDesc.Material.IsNull())
		{
			AssetsToLoad.Add(RecallAssetKeys::MATERIAL, DecalDesc.Material);
		}

		// Request async asset loading with callback
		URecallObjectPoolContainer& PoolContainer = GetPoolContainerChecked();
		PoolContainer.RequestActorAssets(Actor, AssetsToLoad, 
			[this](AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets)
			{
				OnDecalAssetsLoaded(LoadedActor, LoadedAssets);
			});
	}
}

void FRecallDecalActorPool::EnableObject(AActor* Actor)
{
	SetActorActive(Actor, true);
}

void FRecallDecalActorPool::DisableObject(AActor* Actor)
{
	SetActorActive(Actor, false);
}

void FRecallDecalActorPool::OnDecalAssetsLoaded(AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets)
{
	if (ADecalActor* LoadedDecalActor = Cast<ADecalActor>(LoadedActor))
	{
		// Apply loaded material
		if (UMaterialInterface* LoadedMaterial = Cast<UMaterialInterface>(LoadedAssets.FindRef(RecallAssetKeys::MATERIAL)))
		{
			LoadedDecalActor->SetDecalMaterial(LoadedMaterial);
		}
	}
}
