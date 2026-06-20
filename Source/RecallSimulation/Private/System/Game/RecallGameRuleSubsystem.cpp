// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Game/RecallGameRuleSubsystem.h"

#include "Data/Game/RecallGameRuleBaseAsset.h"
#include "Engine/AssetManager.h"
#include "System/Simulation/RecallSimulationControllerTypes.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

URecallGameRuleSubsystem::URecallGameRuleSubsystem()
	: Super()
{
}

void URecallGameRuleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (const UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		AssetManager->GetPrimaryAssetPathList(URecallGameRuleBaseAsset::AssetType, GameRuleAssetPaths);
	}
}

void URecallGameRuleSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URecallGameRuleSubsystem::Start(const FRecallSimulationStartParams& Params)
{
	GameRule.GameRuleName = Params.GameRuleName;

	if (Params.GameRuleName.IsNone())
	{
		GameRule.MatchState = Recall::Game::State::InProgress;
	}
	
	ReloadGameRuleAsset();
}

void URecallGameRuleSubsystem::Reset()
{
	GameRule = FRecallGameRule();
	GameRuleAsset = nullptr;
}

void URecallGameRuleSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallGameRuleSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_GameRule_Save);

	OutSnapshot.InitializeAs<FRecallGameRule>(GameRule);
}

void URecallGameRuleSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallGameRuleSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_GameRule_Restore);

	if (const FRecallGameRule* DataPtr = InSnapshot.GetPtr<FRecallGameRule>())
	{
		GameRule = *DataPtr;
	}

	if (Context.IsSnapshot())
	{
		bReloadGameRuleAssetForSnapshot = true;
	}
}

void URecallGameRuleSubsystem::PostRestore()
{
	if (bReloadGameRuleAssetForSnapshot)
	{
		ReloadGameRuleAsset();
		bReloadGameRuleAssetForSnapshot = false;
	}
}

TObjectPtr<const URecallGameRuleBaseAsset> URecallGameRuleSubsystem::GetGameRuleAsset() const
{
	return GameRuleAsset;
}

void URecallGameRuleSubsystem::ReloadGameRuleAsset()
{
	GameRuleAsset = nullptr;
	
	if (GameRule.GameRuleName.IsNone())
	{
		return;
	}
	
	const FSoftObjectPath* AssetPathPtr = GameRuleAssetPaths.FindByPredicate([this](const FSoftObjectPath& Path)
	{
		return Path.GetAssetName() == GameRule.GameRuleName;
	});
	if (AssetPathPtr == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs Fail to find GameRule: %s"), __FUNCTION__, *GameRule.GameRuleName.ToString());
		return;
	}

	if (ensureAlwaysMsgf(IsInGameThread(),
		TEXT("%hs Can only reload game rule asset from gamethread"), __FUNCTION__))
	{
		GameRuleAsset = CastChecked<URecallGameRuleBaseAsset>(AssetPathPtr->TryLoad());		
	}
}

void URecallGameRuleSubsystem::TickMatchTime(float DeltaTime)
{
	if (IsInProgress())
	{
		FScopeLock Lock(&DataGuard);
		GameRule.MatchTimeSeconds += DeltaTime;
	}
}

double URecallGameRuleSubsystem::GetMatchTimeSeconds() const
{
	FScopeLock Lock(&DataGuard);
	return GameRule.MatchTimeSeconds;
}

void URecallGameRuleSubsystem::SetMatchState(const FName& State)
{
	check(Recall::Simulation::Utils::IsSimulationProcessingPhase(this));
	
	FScopeLock Lock(&DataGuard);
	GameRule.MatchState = State;
}

const FName& URecallGameRuleSubsystem::GetMatchState() const
{
	FScopeLock Lock(&DataGuard);
	return GameRule.MatchState;
}

bool URecallGameRuleSubsystem::IsWaitingToStart() const
{
	return GetMatchState() == Recall::Game::State::WaitingToStart;
}

bool URecallGameRuleSubsystem::IsInProgress() const
{
	return GetMatchState() == Recall::Game::State::InProgress;
}
