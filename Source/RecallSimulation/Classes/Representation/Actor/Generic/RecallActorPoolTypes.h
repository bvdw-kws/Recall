// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Representation/Actor/Pool/RecallObjectPoolTypes.h"

class FRecallActorPool : public FRecallObjectPool
{
protected:
	virtual AActor* CreateObject(const FInstancedStruct& Desc) override final;
	virtual void InitObject(AActor* Actor, const FInstancedStruct& Desc) override final;

	virtual void EnableObject(AActor* Actor) override final;
	virtual void DisableObject(AActor* Actor) override final;
};

class FRecallStaticMeshActorPool : public FRecallObjectPool
{
protected:
	virtual AActor* CreateObject(const FInstancedStruct& Desc) override final;
	virtual void InitObject(AActor* Actor, const FInstancedStruct& Desc) override final;

	virtual void EnableObject(AActor* Actor) override final;
	virtual void DisableObject(AActor* Actor) override final;

private:
	/** Callback for when static mesh assets are loaded */
	void OnStaticMeshAssetsLoaded(AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets, const struct FStaticMeshRepresentationMeshDesc& MeshDesc);
};

class FRecallSkeletalMeshActorPool : public FRecallObjectPool
{
protected:
	virtual AActor* CreateObject(const FInstancedStruct& Desc) override final;
	virtual void InitObject(AActor* Actor, const FInstancedStruct& Desc) override final;

	virtual void EnableObject(AActor* Actor) override final;
	virtual void DisableObject(AActor* Actor) override final;

private:
	/** Callback for when skeletal mesh assets are loaded */
	void OnSkeletalMeshAssetsLoaded(AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets, const struct FSkeletalMeshRepresentationMeshDesc& MeshDesc);
};

class FRecallLevelSequenceActorPool : public FRecallObjectPool
{
protected:
	virtual AActor* CreateObject(const FInstancedStruct& Desc) override final;
	virtual void InitObject(AActor* Actor, const FInstancedStruct& Desc) override final;

	virtual void EnableObject(AActor* Actor) override final;
	virtual void DisableObject(AActor* Actor) override final;

private:
	/** Callback for when level sequence assets are loaded */
	void OnLevelSequenceAssetsLoaded(AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets);
};

class FRecallDecalActorPool : public FRecallObjectPool
{
protected:
	virtual AActor* CreateObject(const FInstancedStruct& Desc) override final;
	virtual void InitObject(AActor* Actor, const FInstancedStruct& Desc) override final;

	virtual void EnableObject(AActor* Actor) override final;
	virtual void DisableObject(AActor* Actor) override final;

private:
	/** Callback for when decal assets are loaded */
	void OnDecalAssetsLoaded(AActor* LoadedActor, const TMap<FName, UObject*>& LoadedAssets);
};
