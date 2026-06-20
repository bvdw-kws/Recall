// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Representation/Actor/Factory/RecallActorRepresentationPoolFactory.h"

#include "RecallActorPoolFactoryTypes.generated.h"

UCLASS()
class URecallActorPoolFactory : public URecallActorRepresentationPoolFactory
{
	GENERATED_BODY()
	
	// URecallActorRepresentationPoolFactory implementation Begin
public:
	virtual TSharedPtr<IRecallObjectPoolInterface> BuildActorPool() const override;
	virtual FName GetActorPoolName(const FInstancedStruct& Desc) const override;
	// URecallActorRepresentationPoolFactory implementation End
};

UCLASS()
class URecallStaticMeshActorPoolFactory : public URecallActorRepresentationPoolFactory
{
	GENERATED_BODY()
	
	// URecallActorRepresentationPoolFactory implementation Begin
public:
	virtual TSharedPtr<IRecallObjectPoolInterface> BuildActorPool() const override;
	virtual FName GetActorPoolName(const FInstancedStruct& Desc) const override;
	// URecallActorRepresentationPoolFactory implementation End	
};

UCLASS()
class URecallSkeletalMeshActorPoolFactory : public URecallActorRepresentationPoolFactory
{
	GENERATED_BODY()
	
	// URecallActorRepresentationPoolFactory implementation Begin
public:
	virtual TSharedPtr<IRecallObjectPoolInterface> BuildActorPool() const override;
	virtual FName GetActorPoolName(const FInstancedStruct& Desc) const override;
	// URecallActorRepresentationPoolFactory implementation End
};

UCLASS()
class URecallLevelSequenceActorPoolFactory : public URecallActorRepresentationPoolFactory
{
	GENERATED_BODY()
	
	// URecallActorRepresentationPoolFactory implementation Begin
public:
	virtual TSharedPtr<IRecallObjectPoolInterface> BuildActorPool() const override;
	virtual FName GetActorPoolName(const FInstancedStruct& Desc) const override;
	// URecallActorRepresentationPoolFactory implementation End
};

UCLASS()
class URecallDecalActorPoolFactory : public URecallActorRepresentationPoolFactory
{
	GENERATED_BODY()
	
	// URecallActorRepresentationPoolFactory implementation Begin
public:
	virtual TSharedPtr<IRecallObjectPoolInterface> BuildActorPool() const override;
	virtual FName GetActorPoolName(const FInstancedStruct& Desc) const override;
	// URecallActorRepresentationPoolFactory implementation End
};
