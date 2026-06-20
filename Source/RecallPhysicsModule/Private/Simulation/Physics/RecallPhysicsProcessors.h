// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassProcessor.h"

#include "RecallPhysicsProcessors.generated.h"

UCLASS()
class URecallPhysicsInitializerProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	URecallPhysicsInitializerProcessor();

	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

	TSharedPtr<struct FRecallPhysicsInitializerCacheManager> CacheManager;
};

UCLASS()
class URecallPhysicsUpdateProcessor : public UMassProcessor
{
	GENERATED_BODY()

	URecallPhysicsUpdateProcessor();

public:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override final;

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void ExecuteDumpPhysicsObject(FMassEntityManager& EntityManager, FMassExecutionContext& Context, const FString& ContextString);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

};

UCLASS()
class URecallPhysicsCopyLocationProcessor : public UMassProcessor
{
	GENERATED_BODY()

	URecallPhysicsCopyLocationProcessor();

public:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;
};

UCLASS()
class URecallPhysicsGeneratesHitEventProcessor : public UMassProcessor
{
	GENERATED_BODY()

	URecallPhysicsGeneratesHitEventProcessor();

public:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override final;

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

	TSharedPtr<struct FRecallPhysicsGeneratesHitEventCacheManager> CacheManager;
};

UCLASS()
class URecallPhysicsResetHitEventsProcessor : public UMassProcessor
{
	GENERATED_BODY()

	URecallPhysicsResetHitEventsProcessor();

public:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override final;

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

};

UCLASS()
class URecallPhysicsRepresentationProcessor : public UMassProcessor
{
	GENERATED_BODY()

	URecallPhysicsRepresentationProcessor();

protected:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;

private:
	FMassEntityQuery EntityQuery;

};
