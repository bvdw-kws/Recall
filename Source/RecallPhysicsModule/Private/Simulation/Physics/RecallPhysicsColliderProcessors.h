// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassObserverProcessor.h"

#include "RecallPhysicsColliderProcessors.generated.h"

UCLASS()
class URecallBodyFragmentDestructor : public UMassObserverProcessor
{
	GENERATED_BODY()

	URecallBodyFragmentDestructor();

protected:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

};

UCLASS()
class URecallBoxShapeFragmentConstructor : public UMassObserverProcessor
{
	GENERATED_BODY()

	URecallBoxShapeFragmentConstructor();

protected:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

};

UCLASS()
class URecallSphereShapeFragmentConstructor : public UMassObserverProcessor
{
	GENERATED_BODY()

	URecallSphereShapeFragmentConstructor();

protected:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

};

UCLASS()
class URecallCapsuleShapeFragmentConstructor : public UMassObserverProcessor
{
	GENERATED_BODY()

	URecallCapsuleShapeFragmentConstructor();

protected:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)  override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

};

UCLASS()
class URecallMeshShapeFragmentProcessor : public UMassProcessor
{
	GENERATED_BODY()

	URecallMeshShapeFragmentProcessor();

public:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)  override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

};

UCLASS()
class URecallHeightFieldShapeFragmentProcessor : public UMassProcessor
{
	GENERATED_BODY()

	URecallHeightFieldShapeFragmentProcessor();

public:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)  override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

};
