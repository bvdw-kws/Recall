// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "MassProcessor.h"

#include "RecallGameRuleBaseProcessors.generated.h"

/**
 * Tick the game rule match time.
 */
UCLASS()
class URecallGameRuleBaseProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	URecallGameRuleBaseProcessor();

	void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager) override final;
	bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override final;
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override final;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override final;
};
