// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "RecallReplayGameComponent.h"

#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallPlayerState_InGame.h"
#include "RecallClientRestoreComponent.h"
#include "RecallGameSimulationComponent.h"
#include "System/Replay/RecallReplaySubsystem.h"
#include "System/Restore/RecallRestoreSubsystem.h"

URecallReplayGameComponent::URecallReplayGameComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void URecallReplayGameComponent::BeginPlay()
{
	Super::BeginPlay();

	ReplaySystem = UGameInstance::GetSubsystem<URecallReplaySubsystem>(
	GetGameInstance<UGameInstance>());
	RestoreSystem = UWorld::GetSubsystem<URecallRestoreSubsystem>(GetWorld());
}

void URecallReplayGameComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ReplaySystem.Reset();
	RestoreSystem.Reset();
}

void URecallReplayGameComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, bPlayingReplay);
}

bool URecallReplayGameComponent::IsPlayingReplay() const
{
	if (bPlayingReplay)
	{
		return true;
	}
	
	if (ReplaySystem.IsValid())
	{
		return ReplaySystem->IsPlayReplay();
	}
	
	return false;
}

void URecallReplayGameComponent::StartReplay()
{
#if WITH_SERVER_CODE
	if (!HasAuthority() || IsPlayingReplay())
	{
		return;
	}
	
	bPlayingReplay = true;
	NetMulticast_StartReplay();

	ReplaySystem->SaveCacheReplay();
	ReplaySystem->LoadCacheReplay();
	ReplaySystem->ResumeReplay();
	
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (IsValid(PlayerController))
		{
			ARecallPlayerState_InGame* PlayerState = PlayerController->GetPlayerState<ARecallPlayerState_InGame>();
			if (IsValid(PlayerState))
			{
				PlayerState->SetIsSpectator(true);
				PlayerState->SetJoinedGame(false);
			}
			
			PlayerController->Reset();
			PlayerController->ClientReset();
		}
	}
#endif // WITH_SERVER_CODE
}

void URecallReplayGameComponent::ServerRestoreAllRemoteClients() const
{
#if WITH_SERVER_CODE
	if (!ensure(HasAuthority()) || !RestoreSystem.IsValid())
	{
		return;
	}
	
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ARecallPlayerState_InGame* InGamePlayerState = Cast<ARecallPlayerState_InGame>(PlayerState);
			
		if (IsValid(InGamePlayerState) && !InGamePlayerState->HasLocalNetOwner())
		{
			if (RestoreSystem->IsRestoreClient(*InGamePlayerState))
			{
				RestoreSystem->CloseRestoreClient(*InGamePlayerState);
			}
					
			RestoreSystem->OpenRestoreClient(*InGamePlayerState);
		}
	}
#endif // WITH_SERVER_CODE
}

void URecallReplayGameComponent::ClientStartRestoringGameSimulation() const
{
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	URecallClientRestoreComponent* ClientRestoreComponent = GameState->GetClientRestoreComponentChecked();

	if (ClientRestoreComponent->IsRestoringGameSimulation())
	{
		ClientRestoreComponent->StopRestoreGameSimulation();
	}

	const URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	GameSimulationComponent->ResetSimulation(false, false);
	
	ClientRestoreComponent->RestoreGameSimulation(GameSimulationComponent->GetGameSimStartParams(), false);
}

#pragma region RPC
void URecallReplayGameComponent::NetMulticast_StartReplay_Implementation()
{
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerRestoreAllRemoteClients();
	}
	else
#endif // WITH_SERVER_CODE
	{
		ClientStartRestoringGameSimulation();
	}
}
#pragma endregion RPC
