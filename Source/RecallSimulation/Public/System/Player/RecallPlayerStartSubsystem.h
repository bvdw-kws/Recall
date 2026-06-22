// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "Mass/ExternalSubsystemTraits.h"

#include "RecallPlayerStartSubsystem.generated.h"

class APlayerStart;

UCLASS()
class RECALLSIMULATION_API URecallPlayerStartSubsystem :
	public UWorldSubsystem,
	public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

	URecallPlayerStartSubsystem();

public:
	bool FindPlayerStart(const FRandomStream& RandomStream,
		FVector& OutLocation, FQuat& OutRotation, FName PlayerStartTag = NAME_None, bool bAllowStartPIE = true) const;

protected:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	void Start(const FRecallSimulationStartParams& Params) override final;
	// IRecallSimulationReactSystemInterface implementation End

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerStart>> PlayerStarts;
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TObjectPtr<APlayerStart> PlayerStartPIE;
#endif // WITH_EDITORONLY_DATA

	FTransform FindDefaultPlayerStart(const FRandomStream& RandomStream,
		const FName& PlayerStartTag, bool bAllowStartPIE) const;
};

template<>
struct TMassExternalSubsystemTraits<URecallPlayerStartSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
