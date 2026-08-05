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
#include "World/RecallWorldSettings.h"

bool URecallGameEditorFunctionLibrary::IsGameEditor(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

#ifdef WITH_GAME_EDITOR_RUNTIME
	// A GameEditor URL option, when present, overrides the World Settings value below
	// regardless of its value (e.g. the Puzzle Menu passes GameEditor=false so playing a
	// puzzle never opens the Game Editor even if left enabled on the map's World Settings).
	const FString GameEditorOption = World->URL.GetOption(TEXT("GameEditor="), TEXT(""));
	if (!GameEditorOption.IsEmpty())
	{
		return GameEditorOption.ToBool();
	}

	if (const ARecallWorldSettings* WorldSettings = Cast<ARecallWorldSettings>(World->GetWorldSettings()))
	{
		return WorldSettings->bStartInGameEditor;
	}
#endif // WITH_GAME_EDITOR_RUNTIME

	return false;
}

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
	return IsGameEditor(WorldContextObject);
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
