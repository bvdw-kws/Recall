// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallJoinGameComponent.h"

#include "Components/Controller/RecallMultiSimControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RecallGameSimulationComponent.h"
#include "RecallLatencyGameComponent.h"
#include "RecallSyncInputGameComponent.h"
#include "Online/RecallGameMode.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallPlayerState_InGame.h"
#include "Online/Base/RecallPlayerControllerBase.h"
#include "Utility/Player/RecallPlayerUtils.h"

URecallJoinGameComponent::URecallJoinGameComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void URecallJoinGameComponent::JoinGameAll()
{
#if WITH_SERVER_CODE
	ARecallGameMode* GameMode = GetGameMode<ARecallGameMode>();
	if (!IsValid(GameMode))
	{
		return;
	}
	
	// Start the game for all the clients
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			ARecallPlayerControllerBase* PlayerController = Cast<ARecallPlayerControllerBase>(Iterator->Get());
			if (!IsValid(PlayerController))
			{
				continue;
			}

			ARecallPlayerState_InGame* PlayerState = PlayerController->GetPlayerState<ARecallPlayerState_InGame>();
			if (!IsValid(PlayerState))
			{
				continue;
			}
			
			if (!PlayerState->IsSpectator() && GameMode->CanJoinGame(*PlayerController))
			{
				JoinGame(*PlayerController);
			}
			else if (PlayerController->IsSyncReady())
			{
				SpectateGame(*PlayerController, true);
			}
		}
	}
	
	// Bots join the game.
	if (const UWorld* World = GetWorld())
	{
		for (FConstControllerIterator Iterator = World->GetControllerIterator(); Iterator; ++Iterator)
		{
			AController* Controller = Iterator->Get();
			if (Controller->IsPlayerController() || !GameMode->CanNewPlayerJoinGame())
			{
				continue;
			}

			JoinGame(*Controller);
		}
	}
#endif // WITH_SERVER_CODE
}

void URecallJoinGameComponent::JoinGame(AController& Controller, bool bDebugJoin /*= false*/)
{
#if WITH_SERVER_CODE
	ARecallGameMode* GameMode = GetGameMode<ARecallGameMode>();
	if (!IsValid(GameMode))
	{
		return;
	}
	
	ARecallPlayerState_InGame* PlayerState = Controller.GetPlayerState<ARecallPlayerState_InGame>();
	if (!IsValid(PlayerState) || PlayerState->HasJoinedGame())
	{
		return;
	}
	
	const FSoftObjectPath PlayerEntityConfig = GameMode->GetPlayerEntityConfig(Controller);
	if (!ensureMsgf(!PlayerEntityConfig.IsNull(),
		TEXT("%hs Missing player entity config"), __FUNCTION__))
	{
		return;
	}

	bool bIsPlayerOne = false;

	ARecallPlayerControllerBase* PlayerController = Cast<ARecallPlayerControllerBase>(&Controller);
	if (IsValid(PlayerController))
	{
		if (!GameMode->CanJoinGame(*PlayerController))
		{
			return;
		}
		
		bIsPlayerOne = PlayerController->IsPlayerOne();
	}

	PlayerState->SetIsSpectator(false);

	const FString& PlayerId = PlayerState->GetSimPlayerId();
	const uint32 Frame = GetPlayerChangeSafeFrame();
	
	int32 WorldIndex = 0;

	if (!bIsPlayerOne)
	{
		// Spawn in the same world as Player One when join the game
		const int32 PlayerOneWorldIndex = Recall::Player::Utils::FindLocalPlayerWorldIndex(this, 0);
		if (PlayerOneWorldIndex != INDEX_NONE)
		{
			WorldIndex = PlayerOneWorldIndex;
		}
	}

	UE_LOG(LogRecallPlayer, Log,
		TEXT("%hs %s join world %d at frame %d"), __FUNCTION__, *PlayerId, WorldIndex, Frame);

	URecallGameSimulationComponent* GameSimulationComponent = GetGameStateChecked<ARecallGameState_InGame>()->GetGameSimulationComponentChecked();
	const FRecallPlayerSpawnParameters SpawnParams = GameMode->GetPlayerSpawnParameters(Controller);
	
	GameSimulationComponent->NetMulticast_AddPlayer(WorldIndex, Frame, PlayerId, SpawnParams);

	PlayerState->SetJoinedGame(true, bDebugJoin);
	
	NumJoinGameSimPlayers++;
	
	if (IsValid(PlayerController))
	{
		PlayerController->GetMultiSimComponentChecked()->Client_GoToWorld(WorldIndex);

		// Let the client knows that he joined the game.
		const bool bLaunchGameSimulation = Frame == 0;
		if (!PlayerController->IsLocalPlayerController() && bLaunchGameSimulation)
		{
			PlayerController->Client_LaunchGameSimulation();
		}
	}
	
	GameMode->RestartPlayer(&Controller);
#endif // WITH_SERVER_CODE
}

void URecallJoinGameComponent::LeaveGame(AController& Controller)
{
#if WITH_SERVER_CODE
	if (!HasAuthority())
	{
		return;
	}
	
	ARecallPlayerState_InGame* PlayerState = Controller.GetPlayerState<ARecallPlayerState_InGame>();
	if (!IsValid(PlayerState) || !PlayerState->HasJoinedGame())
	{
		return;
	}
	
	const uint32 Frame = PlayerState->IsABot() ? GetPlayerChangeSafeFrame() : PlayerState->GetLastReceivedInputFrame() + 1;
	const FString& PlayerId = PlayerState->GetSimPlayerId();
	const int32 WorldIndex = Recall::Player::Utils::GetPlayerWorldIndex(this, PlayerId);

	UE_LOG(LogGameState, Log,
		TEXT("%hs %s leave world %d at frame %d"), __FUNCTION__, *PlayerId, WorldIndex, Frame);
	
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	GameSimulationComponent->NetMulticast_RemovePlayer(WorldIndex, Frame, PlayerId);

	PlayerState->SetJoinedGame(false);

	if (!PlayerState->IsABot())
	{
		PlayerState->SetIsLeavingGame();
	}

	NumJoinGameSimPlayers--;

	ARecallPlayerControllerBase* PlayerController = Cast<ARecallPlayerControllerBase>(&Controller);
	if (IsValid(PlayerController))
	{
		PlayerController->PawnLeavingGame();
	}
	
	Controller.Reset();
#endif // WITH_SERVER_CODE
}

void URecallJoinGameComponent::SpectateGame(ARecallPlayerControllerBase& PlayerController, bool bLaunchGame)
{	
#if WITH_SERVER_CODE
	if (!HasAuthority())
	{
		return;
	}
	
	ARecallPlayerState_InGame* PlayerState = PlayerController.GetPlayerState<ARecallPlayerState_InGame>();
	if (!IsValid(PlayerState))
	{
		return;
	}

	if (PlayerState->HasJoinedGame())
	{
		LeaveGame(PlayerController);
	}
	
	PlayerState->SetIsSpectator(true);
				
	PlayerController.Reset();
	PlayerController.ClientReset();

	if (bLaunchGame && !PlayerController.IsLocalPlayerController())
	{
		PlayerController.Client_LaunchGameSimulation();
	}
#endif // WITH_SERVER_CODE
}


void URecallJoinGameComponent::RespawnPlayerEntities() const
{
#if WITH_SERVER_CODE
	ARecallGameMode* GameMode = GetGameMode<ARecallGameMode>();
	if (!IsValid(GameMode))
	{
		return;
	}

	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	GameSimulationComponent->ResetPlayerEvents();
	
	if (const UWorld* World = GetWorld())
	{
		for (FConstControllerIterator Iterator = World->GetControllerIterator(); Iterator; ++Iterator)
		{
			AController* Controller = Iterator->Get();
			if (!IsValid(Controller))
			{
				continue;
			}
			
			ARecallPlayerState_InGame* PlayerState = Controller->GetPlayerState<ARecallPlayerState_InGame>();
			if (!IsValid(PlayerState) || !PlayerState->HasJoinedGame())
			{
				continue;
			}
						
			const FString& PlayerId = PlayerState->GetSimPlayerId();
			const FRecallPlayerSpawnParameters SpawnParams = GameMode->GetPlayerSpawnParameters(*Controller);
			
			GameSimulationComponent->NetMulticast_AddPlayer(0, 0, PlayerId, SpawnParams);
		}
	}
#endif // WITH_SERVER_CODE
}

void URecallJoinGameComponent::SetLocalPlayerCharacter(const FName& CharacterName, int32 LocalPlayerIndex /*= 0*/) const
{
#if WITH_SERVER_CODE
	ARecallGameMode* GameMode = GetGameMode<ARecallGameMode>();
	if (!IsValid(GameMode))
	{
		return;
	}
	
	const ARecallPlayerControllerBase* PlayerController = Cast<ARecallPlayerControllerBase>(UGameplayStatics::GetPlayerController(this, LocalPlayerIndex));
	if (!IsValid(PlayerController) || !PlayerController->HasJoinedGame())
	{
		return;
	}
	
	const FString PlayerId = PlayerController->GetDefaultSimPlayerId();
	const int32 WorldIndex = Recall::Player::Utils::GetPlayerWorldIndex(this, PlayerId);
	const uint32 Frame = GetPlayerChangeSafeFrame();

	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	GameSimulationComponent->NetMulticast_RemovePlayer(WorldIndex, Frame, PlayerId);

	const FRecallPlayerSpawnParameters SpawnParams = GameMode->GetPlayerSpawnParameters(*PlayerController);

	GameSimulationComponent->NetMulticast_AddPlayer(WorldIndex, Frame + 1, PlayerId, SpawnParams);
#endif // WITH_SERVER_CODE
}

void URecallJoinGameComponent::FinishRestoring(ARecallPlayerControllerBase& PlayerController)
{
#if WITH_SERVER_CODE
	ARecallGameMode* GameMode = GetGameMode<ARecallGameMode>();
	if (!IsValid(GameMode))
	{
		return;
	}
	
	ARecallPlayerState_InGame* PlayerState = PlayerController.GetPlayerState<ARecallPlayerState_InGame>();
	if (!IsValid(PlayerState) || !ensure(!PlayerState->IsRestoring()))
	{
		return;
	}
	
	if (GameMode->CanJoinGame(PlayerController))
	{
		JoinGame(PlayerController);
	}
	else
	{
		SpectateGame(PlayerController);
	}
#endif // WITH_SERVER_CODE
}

uint32 URecallJoinGameComponent::GetPlayerChangeSafeFrame() const
{
	const ARecallGameState_InGame* GameState = GetGameStateChecked<ARecallGameState_InGame>();
	const URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
	
	const UWorld* World = GetWorld();
	const uint32 Frame = GameSimulationComponent->GetLocalSimulationFrame();
	if (!IsValid(World) || Frame == 0)
	{
		return 0;
	}
	
	// We add new players after the last input that was sent by the host.
	const URecallLatencyGameComponent* LatencyGameComponent = GameState->GetLatencyComponentChecked();
	const uint32 InputNetworkLatency = LatencyGameComponent->GetInputNetworkLatencySpikeInFrames();
	const URecallSyncInputGameComponent* SyncInputComponent = GameState->GetSyncInputComponentChecked();
	
	if (const ARecallPlayerControllerBase* LocalPC = Cast<ARecallPlayerControllerBase>(World->GetFirstPlayerController()))
	{
		if (const ARecallPlayerState_InGame* LocalPlayerState = LocalPC->GetPlayerState<ARecallPlayerState_InGame>())
		{
			return FMath::Max(LocalPlayerState->GetLastReceivedInputFrame(), SyncInputComponent->GetLocalConfirmFrame()) + 1 + InputNetworkLatency;
		}
	}

	return Frame;
}
