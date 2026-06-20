// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallActorMeshRepresentationTypes.h"

#include "Animation/SkeletalMeshActor.h"
#include "Engine/DecalActor.h"
#include "Engine/StaticMeshActor.h"
#include "Generic/RecallActorPoolFactoryTypes.h"
#include "LevelSequenceActor.h"

//----------------------------------------------------------------------//
// FLevelSequenceActorRepresentationDesc
//----------------------------------------------------------------------//
FLevelSequenceActorRepresentationDesc::FLevelSequenceActorRepresentationDesc()
	: Super(URecallLevelSequenceActorPoolFactory::StaticClass())
{
}

FSoftClassPath FLevelSequenceActorRepresentationDesc::GetSoftClassPath() const
{
	return Blueprint.IsNull() ? ALevelSequenceActor::StaticClass() : Blueprint;
}

//----------------------------------------------------------------------//
// FActorRepresentationDesc
//----------------------------------------------------------------------//
FActorRepresentationDesc::FActorRepresentationDesc()
	: Super(URecallActorPoolFactory::StaticClass())
{
}

FSoftClassPath FActorRepresentationDesc::GetSoftClassPath() const
{
	return Blueprint.IsNull() ? AActor::StaticClass() : Blueprint;
}

//----------------------------------------------------------------------//
// FSkeletalMeshRepresentationMeshDesc
//----------------------------------------------------------------------//
FSkeletalMeshRepresentationMeshDesc::FSkeletalMeshRepresentationMeshDesc()
	: Super(URecallSkeletalMeshActorPoolFactory::StaticClass())
{
}

FSoftClassPath FSkeletalMeshRepresentationMeshDesc::GetSoftClassPath() const
{
	return Blueprint.IsNull() ? ASkeletalMeshActor::StaticClass() : Blueprint;
}

//----------------------------------------------------------------------//
// FStaticMeshRepresentationMeshDesc
//----------------------------------------------------------------------//
FStaticMeshRepresentationMeshDesc::FStaticMeshRepresentationMeshDesc()
	: Super(URecallStaticMeshActorPoolFactory::StaticClass())
{
}

FSoftClassPath FStaticMeshRepresentationMeshDesc::GetSoftClassPath() const
{
	return Blueprint.IsNull() ? AStaticMeshActor::StaticClass() : Blueprint;
}

//----------------------------------------------------------------------//
// FRecallDecalActorRepresentationDesc
//----------------------------------------------------------------------//
FRecallDecalActorRepresentationDesc::FRecallDecalActorRepresentationDesc()
	: Super(URecallDecalActorPoolFactory::StaticClass())
{
}

FSoftClassPath FRecallDecalActorRepresentationDesc::GetSoftClassPath() const
{
	return Blueprint.IsNull() ? ADecalActor::StaticClass() : Blueprint;
}
