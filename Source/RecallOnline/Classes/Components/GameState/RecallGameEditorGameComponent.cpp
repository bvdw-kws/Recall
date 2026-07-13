// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "RecallGameEditorGameComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Online/RecallPlayerCameraManager.h"
#include "World/RecallWorldSettings.h"

#ifdef WITH_GAME_EDITOR_RUNTIME
#include "Data/GameEditorMapAsset.h"
#include "System/GameEditorWidgetSubsystem.h"
#include "Utility/GameEditorFunctionLibrary.h"
#endif // WITH_GAME_EDITOR_RUNTIME

URecallGameEditorGameComponent::URecallGameEditorGameComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void URecallGameEditorGameComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bIsInGameEditorMode);
}

void URecallGameEditorGameComponent::OnStartPlay()
{
	if (ShouldOpenGameEditor())
	{
		EnterGameEditorMode();
	}
}

void URecallGameEditorGameComponent::EnterGameEditorMode()
{
#if WITH_SERVER_CODE
	if (!HasAuthority() || IsInGameEditorMode())
	{
		return;
	}

	bIsInGameEditorMode = true;
	OnRep_IsInGameEditorMode();
#endif // WITH_SERVER_CODE
}

void URecallGameEditorGameComponent::OnRep_IsInGameEditorMode()
{
	if (!IsInGameEditorMode())
	{
		return;
	}

	OnEnterGameEditor();
}

void URecallGameEditorGameComponent::OnEnterGameEditor()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!IsValid(PlayerController))
	{
		return;
	}

#ifdef WITH_GAME_EDITOR_RUNTIME
	// Entity Actors are hidden by default so we need to display them.
	const TArray<AActor*> DeviceActors = UGameEditorFunctionLibrary::GetAllDeviceActors(this);
	for (AActor* DeviceActor : DeviceActors)
	{
		DeviceActor->SetActorHiddenInGame(false);
	}

	UGameEditorWidgetSubsystem::GetRef(GetWorld()).OpenGameEditor(PlayerController);
#endif // WITH_GAME_EDITOR_RUNTIME

	if (ARecallPlayerCameraManager* CameraManager = Cast<ARecallPlayerCameraManager>(PlayerController->PlayerCameraManager))
	{
		CameraManager->OnEnterGameEditor();
	}
}

AActor* URecallGameEditorGameComponent::GetGameEditorViewTarget() const
{
#ifdef WITH_GAME_EDITOR_RUNTIME
	if (!IsInGameEditorMode())
	{
		return nullptr;
	}

	return UGameEditorWidgetSubsystem::GetRef(GetWorld()).GetGameEditorCameraActor();
#else // WITH_GAME_EDITOR_RUNTIME
	return nullptr;
#endif
}

bool URecallGameEditorGameComponent::CanStartMatch() const
{
	return !ShouldOpenGameEditor();
}

bool URecallGameEditorGameComponent::ShouldOpenGameEditor() const
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
UGameEditorMapAsset* URecallGameEditorGameComponent::GetGameEditorMapAsset() const
{
	const ARecallWorldSettings* WorldSettings = Cast<ARecallWorldSettings>(GetWorld()->GetWorldSettings());
	return WorldSettings ? Cast<UGameEditorMapAsset>(WorldSettings->GameEditorMapAsset) : nullptr;
}
#endif // WITH_GAME_EDITOR_RUNTIME
