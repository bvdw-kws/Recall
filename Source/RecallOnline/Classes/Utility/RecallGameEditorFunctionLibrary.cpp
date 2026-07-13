// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "RecallGameEditorFunctionLibrary.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/GameState/RecallGameEditorGameComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Online/RecallGameMode.h"
#include "Online/RecallGameState_InGame.h"

void URecallGameEditorFunctionLibrary::PlayGame(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	ARecallGameState_InGame* GameState = World->GetGameState<ARecallGameState_InGame>();
	if (!IsValid(GameState))
	{
		return;
	}

	GameState->GetGameEditorComponentChecked()->MarkReadyToStartMatch();

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (IsValid(PlayerController) && PlayerController->IsLocalController() && IsValid(PlayerController->PlayerCameraManager))
		{
			PlayerController->PlayerCameraManager->SetManualCameraFade(1.0f, FColor::Black, true);
		}
	}
}

bool URecallGameEditorFunctionLibrary::CanExitPlayMode(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

	ARecallGameState_InGame* GameState = World->GetGameState<ARecallGameState_InGame>();
	if (!IsValid(GameState))
	{
		return false;
	}

	return GameState->GetGameEditorComponentChecked()->ShouldOpenGameEditor();
}

void URecallGameEditorFunctionLibrary::ExitPlayMode(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	ARecallGameMode* GameMode = World->GetAuthGameMode<ARecallGameMode>();
	if (!IsValid(GameMode))
	{
		return;
	}

	GameMode->ExitToWaitingToStart();
}
