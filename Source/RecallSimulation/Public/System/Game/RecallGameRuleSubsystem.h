// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "MassExternalSubsystemTraits.h"
#include "RecallGameRuleDataTypes.h"

#include "RecallGameRuleSubsystem.generated.h"

class URecallGameRuleBaseAsset;

/**
* Equivalent of the game mode but inside the game simulation (to avoid confusion).
* Contains a set of rules that determine the flow of the game.
*/
UCLASS()
class RECALLSIMULATION_API URecallGameRuleSubsystem :
	public UWorldSubsystem,
	public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()
	
public:
	URecallGameRuleSubsystem();

	TObjectPtr<const URecallGameRuleBaseAsset> GetGameRuleAsset() const;

	template<typename T=URecallGameRuleBaseAsset>
	TObjectPtr<const T> GetGameRuleAsset() const { return Cast<T>(GetGameRuleAsset()); }

	void TickMatchTime(float DeltaTime);
	double GetMatchTimeSeconds() const;
	
	void SetMatchState(const FName& State);
	const FName& GetMatchState() const;

	bool IsWaitingToStart() const;
	bool IsInProgress() const;
	
protected:
	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	virtual void Start(const FRecallSimulationStartParams& Params) override final;
	virtual void Reset() override final;
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	virtual void PostRestore() override final;
	// IRecallSimulationReactSystemInterface implementation End

private:
	UPROPERTY(Transient)
	FRecallGameRule GameRule;
	UPROPERTY(Transient)
	TArray<FSoftObjectPath> GameRuleAssetPaths;
	UPROPERTY(Transient)
	TObjectPtr<const URecallGameRuleBaseAsset> GameRuleAsset;
	UPROPERTY(Transient)
	bool bReloadGameRuleAssetForSnapshot = false;
	
	mutable FCriticalSection DataGuard;
	
	void ReloadGameRuleAsset();
};

template<>
struct TMassExternalSubsystemTraits<URecallGameRuleSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
