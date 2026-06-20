// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Game/RecallGameUtils.h"

#include "Data/Game/RecallGameRuleBaseAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Observer/Game/RecallGameReactInterface.h"
#include "System/Game/RecallGameRuleSubsystem.h"
#include "System/Representation/RecallRepresentationEventSubsystem.h"

namespace Recall::Game::Utils
{

void EndGame(URecallRepresentationEventSubsystem& RepresentationEventSystem, const FString& Reason)
{
	RepresentationEventSystem.PushObserverEvent<IRecallGameReactInterface>(
		[Reason](auto& Observer)
	{
		Observer.Interface.OnGameEnd(Reason);
	});
}

void ControllerDefeat(URecallRepresentationEventSubsystem& RepresentationEventSystem,
	const FString& ControllerID, const FString& Reason)
{
	RepresentationEventSystem.PushObserverEvent<IRecallGameReactInterface>(
		[ControllerID, Reason](auto& Observer)
	{
		Observer.Interface.OnControllerDefeat(ControllerID, Reason);
	});
}

TArray<FName> GetGameRuleNames()
{
	TArray<FName> Results;
	
	Results.Add(NAME_None);
	
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		TArray<FSoftObjectPath> GameRuleAssetPaths;
		AssetManager->GetPrimaryAssetPathList(URecallGameRuleBaseAsset::AssetType, GameRuleAssetPaths);

		for (const FSoftObjectPath& GameRuleAssetPath : GameRuleAssetPaths)
		{
			Results.Add(*GameRuleAssetPath.GetAssetName());
		}
	}

	return Results;
}

double GetMatchTimeSeconds(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const URecallGameRuleSubsystem* GameRuleSystem = UWorld::GetSubsystem<URecallGameRuleSubsystem>(World);
	if (IsValid(GameRuleSystem))
	{
		return GameRuleSystem->GetMatchTimeSeconds();
	}
	else
	{		
		return 0.0;
	}
}
	
} // namespace Recall::Game::Utils
