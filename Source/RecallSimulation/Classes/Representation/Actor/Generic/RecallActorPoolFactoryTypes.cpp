// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallActorPoolFactoryTypes.h"

#include "Animation/SkeletalMeshActor.h"
#include "Engine/DecalActor.h"
#include "Engine/StaticMeshActor.h"
#include "LevelSequenceActor.h"
#include "RecallActorPoolTypes.h"
#include "Representation/Actor/RecallActorMeshRepresentationTypes.h"
#include "StructUtils/InstancedStruct.h"

//----------------------------------------------------------------------//
// URecallActorPoolFactory
//----------------------------------------------------------------------//
TSharedPtr<IRecallObjectPoolInterface> URecallActorPoolFactory::BuildActorPool() const
{
	return MakeShared<FRecallActorPool>();
}

FName URecallActorPoolFactory::GetActorPoolName(const FInstancedStruct& Desc) const
{
	const FActorRepresentationDesc& ActorDesc = Desc.Get<FActorRepresentationDesc>();
	const FSoftClassPath SoftClassPath = ActorDesc.GetSoftClassPath();
	if (!SoftClassPath.IsNull())
	{
		return *SoftClassPath.GetAssetName();
	}
	else
	{
		return AActor::StaticClass()->GetFName();
	}
}

//----------------------------------------------------------------------//
// URecallStaticMeshActorPoolFactory
//----------------------------------------------------------------------//
TSharedPtr<IRecallObjectPoolInterface> URecallStaticMeshActorPoolFactory::BuildActorPool() const
{
	return MakeShared<FRecallStaticMeshActorPool>();
}

FName URecallStaticMeshActorPoolFactory::GetActorPoolName(const FInstancedStruct& Desc) const
{
	const FStaticMeshRepresentationMeshDesc& MeshDesc = Desc.Get<FStaticMeshRepresentationMeshDesc>();
	const FSoftClassPath SoftClassPath = MeshDesc.GetSoftClassPath();
	if (!SoftClassPath.IsNull())
	{
		return *SoftClassPath.GetAssetName();
	}
	else
	{
		return AStaticMeshActor::StaticClass()->GetFName();
	}
}

//----------------------------------------------------------------------//
// URecallSkeletalMeshActorPoolFactory
//----------------------------------------------------------------------//
TSharedPtr<IRecallObjectPoolInterface> URecallSkeletalMeshActorPoolFactory::BuildActorPool() const
{
	return MakeShared<FRecallSkeletalMeshActorPool>();
}

FName URecallSkeletalMeshActorPoolFactory::GetActorPoolName(const FInstancedStruct& Desc) const
{
	const FSkeletalMeshRepresentationMeshDesc& MeshDesc = Desc.Get<FSkeletalMeshRepresentationMeshDesc>();
	const FSoftClassPath SoftClassPath = MeshDesc.GetSoftClassPath();
	if (!SoftClassPath.IsNull())
	{
		return *SoftClassPath.GetAssetName();
	}
	else
	{
		return ASkeletalMeshActor::StaticClass()->GetFName();
	}
}

//----------------------------------------------------------------------//
// URecallLevelSequenceActorPoolFactory
//----------------------------------------------------------------------//
TSharedPtr<IRecallObjectPoolInterface> URecallLevelSequenceActorPoolFactory::BuildActorPool() const
{
	return MakeShared<FRecallLevelSequenceActorPool>();
}

FName URecallLevelSequenceActorPoolFactory::GetActorPoolName(const FInstancedStruct& Desc) const
{
	const FLevelSequenceActorRepresentationDesc& LevelSequenceDesc = Desc.Get<FLevelSequenceActorRepresentationDesc>();
	const FSoftClassPath SoftClassPath = LevelSequenceDesc.GetSoftClassPath();
	if (!SoftClassPath.IsNull())
	{
		return *SoftClassPath.GetAssetName();
	}
	else
	{
		return ALevelSequenceActor::StaticClass()->GetFName();
	}
}

//----------------------------------------------------------------------//
// URecallDecalActorPoolFactory
//----------------------------------------------------------------------//
TSharedPtr<IRecallObjectPoolInterface> URecallDecalActorPoolFactory::BuildActorPool() const
{
	return MakeShared<FRecallDecalActorPool>();
}

FName URecallDecalActorPoolFactory::GetActorPoolName(const FInstancedStruct& Desc) const
{
	const FRecallDecalActorRepresentationDesc& DecalDesc = Desc.Get<FRecallDecalActorRepresentationDesc>();
	const FSoftClassPath SoftClassPath = DecalDesc.GetSoftClassPath();
	if (!SoftClassPath.IsNull())
	{
		return *SoftClassPath.GetAssetName();
	}
	else
	{
		return ADecalActor::StaticClass()->GetFName();
	}
}
