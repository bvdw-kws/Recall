// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "RecallGameEditorGameModeComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Online/RecallPlayerCameraManager.h"
#include "World/RecallWorldSettings.h"

#ifdef WITH_GAME_EDITOR_RUNTIME
#include "Data/GameEditorMapAsset.h"
#include "System/GameEditorWidgetSubsystem.h"
#include "Utility/GameEditorFunctionLibrary.h"
#endif // WITH_GAME_EDITOR_RUNTIME

URecallGameEditorGameModeComponent::URecallGameEditorGameModeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void URecallGameEditorGameModeComponent::OnStartPlay()
{
	if (ShouldOpenGameEditor())
	{
		EnterGameEditorMode();
	}
}

void URecallGameEditorGameModeComponent::EnterGameEditorMode()
{
	if (IsInGameEditorMode())
	{
		return;
	}
	
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!IsValid(PlayerController))
	{
		return;
	}
	
	bIsInGameEditorMode = true;

#ifdef WITH_GAME_EDITOR_RUNTIME
	UGameEditorWidgetSubsystem::GetRef(GetWorld()).OpenGameEditor(PlayerController);
#endif // WITH_GAME_EDITOR_RUNTIME

	if (ARecallPlayerCameraManager* CameraManager = Cast<ARecallPlayerCameraManager>(PlayerController->PlayerCameraManager))
	{
		CameraManager->OnEnterGameEditor();
	}
}

bool URecallGameEditorGameModeComponent::CanStartMatch() const
{
	return !ShouldOpenGameEditor();
}

bool URecallGameEditorGameModeComponent::ShouldOpenGameEditor() const
{
#ifdef WITH_GAME_EDITOR_RUNTIME
	const ARecallWorldSettings* WorldSettings = Cast<ARecallWorldSettings>(GetWorld()->GetWorldSettings());
	if (WorldSettings && WorldSettings->bStartInGameEditor)
	{
		return true;
	}
#endif // WITH_GAME_EDITOR_RUNTIME

	return false;
}

#ifdef WITH_GAME_EDITOR_RUNTIME
UGameEditorMapAsset* URecallGameEditorGameModeComponent::GetGameEditorMapAsset() const
{
	const ARecallWorldSettings* WorldSettings = Cast<ARecallWorldSettings>(GetWorld()->GetWorldSettings());
	return WorldSettings ? Cast<UGameEditorMapAsset>(WorldSettings->GameEditorMapAsset) : nullptr;
}
#endif // WITH_GAME_EDITOR_RUNTIME
