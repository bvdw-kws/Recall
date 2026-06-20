// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallInputTokenPawnComponent.h"

#include "Settings/RecallSimulationSettings.h"

URecallInputTokenPawnComponent::URecallInputTokenPawnComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URecallInputTokenPawnComponent::BeginPlay()
{
	Super::BeginPlay();

	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	InputTokenCount = SimulationSettings->InputTokenLimit;
}

void URecallInputTokenPawnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void URecallInputTokenPawnComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	constexpr float RegenerateInputTokenFrequency = 0.0333333333333333f;

	RegenerateInputTokenTimer += DeltaTime;
	if (RegenerateInputTokenTimer >= RegenerateInputTokenFrequency)
	{
		RegenerateInputToken();
	}
}

bool URecallInputTokenPawnComponent::ConsumeInputToken()
{
	if (InputTokenCount == 0)
	{
		return false;
	}

	InputTokenCount--;
	return true;
}

void URecallInputTokenPawnComponent::RegenerateInputToken()
{
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	InputTokenCount = FMath::Min(InputTokenCount + 1, SimulationSettings->InputTokenLimit);
	RegenerateInputTokenTimer = 0.0f;
}
