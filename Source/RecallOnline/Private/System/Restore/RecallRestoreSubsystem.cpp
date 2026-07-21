// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Restore/RecallRestoreSubsystem.h"

#include "Components/GameState/RecallJoinGameComponent.h"
#include "DataTransfer/Subsystems/EasyDataTransferSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallPlayerController.h"
#include "Online/RecallPlayerState_InGame.h"
#include "Utility/Restore/RecallRestoreUtils.h"

void URecallRestoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const UWorld* World = GetWorld())
	{
		DataTransferSubsystem = UGameInstance::GetSubsystem<UEasyDataTransferSubsystem>(World->GetGameInstance());
	}
}

void URecallRestoreSubsystem::Deinitialize()
{
	// Cancel any active transfers before shutdown
	if (DataTransferSubsystem.IsValid())
	{
		for (TPair<FString, FRecallRestoreClientInfo>& ClientInfoPair : ClientRestoreInfos)
		{
			FRecallRestoreClientInfo& RestoreClientInfo = ClientInfoPair.Value;
			if (RestoreClientInfo.CombinedTransferHandle != 0)
			{
				UE_LOG(LogRecallRestore, Log,
					TEXT("%hs: Canceling EasyDataTransfer combined handle %d during shutdown"),
					__FUNCTION__, RestoreClientInfo.CombinedTransferHandle);
				
				// Cancel the transfer properly to free resources
				DataTransferSubsystem->CloseDataChannel(RestoreClientInfo.CombinedTransferHandle);
				RestoreClientInfo.CombinedTransferHandle = 0;
			}
		}

		DataTransferSubsystem.Reset();
	}
	
	Super::Deinitialize();
}

void URecallRestoreSubsystem::Tick(float DeltaTime)
{
#if WITH_SERVER_CODE
	ServerUpdateClientRestoreData();
#endif // WITH_SERVER_CODE
}

TStatId URecallRestoreSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URecallRestoreSubsystem, STATGROUP_Tickables);
}

void URecallRestoreSubsystem::OpenRestoreClient(ARecallPlayerState_InGame& PlayerState)
{
#if WITH_SERVER_CODE
	const FString& PlayerID = PlayerState.GetSimPlayerId();	
	UE_LOG(LogRecallRestore, Log, TEXT("%hs %s"), __FUNCTION__, *PlayerID);
	
	if (!ensureAlwaysMsgf(!PlayerID.IsEmpty(),
			TEXT("%hs Invalid PlayerID"), __FUNCTION__))
	{
		return;
	}
	
	FRecallRestoreClientInfo& RestoreClientInfo = ClientRestoreInfos.Add(PlayerID);	
	Recall::Restore::Utils::InitRestoreClientInfo(this, RestoreClientInfo);
	
	PlayerState.SetIsRestoring(true);
	PlayerState.SetRestoreTargetFrame(RestoreClientInfo.SnapshotFrame);
#endif // WITH_SERVER_CODE
}

void URecallRestoreSubsystem::CloseRestoreClient(ARecallPlayerState_InGame& PlayerState)
{
#if WITH_SERVER_CODE
	const FString& PlayerID = PlayerState.GetSimPlayerId();	
	UE_LOG(LogRecallRestore, Log, TEXT("%hs %s"), __FUNCTION__, *PlayerID);
	
	if (!ensureAlwaysMsgf(!PlayerID.IsEmpty(),
			TEXT("%hs Invalid PlayerID"), __FUNCTION__))
	{
		return;
	}
	
	if (!ensureAlwaysMsgf(ClientRestoreInfos.Contains(PlayerID),
			TEXT("%hs Player is not restoring: %s"), __FUNCTION__, *PlayerID))
	{
		return;
	}
	
	// Clean up any active EasyDataTransfer before removing the client info
	FRecallRestoreClientInfo* RestoreClientInfo = ClientRestoreInfos.Find(PlayerID);
	if (RestoreClientInfo && RestoreClientInfo->CombinedTransferHandle != 0 && DataTransferSubsystem.IsValid())
	{
		// Don't call CloseDataChannel during cleanup as it can cause infinite loops with multicast RPCs
		// Just clear our local handle - the EasyDataTransfer subsystem will clean up on its own
		UE_LOG(LogRecallRestore, Log,
			TEXT("%hs: Clearing EasyDataTransfer combined handle %d for player %s (no network notification)"),
			__FUNCTION__, RestoreClientInfo->CombinedTransferHandle, *PlayerID);
		DataTransferSubsystem->CloseDataChannel(RestoreClientInfo->CombinedTransferHandle);
		RestoreClientInfo->CombinedTransferHandle = 0;
	}
	
	ClientRestoreInfos.Remove(PlayerID);

	PlayerState.SetIsRestoring(false);
#endif // WITH_SERVER_CODE
}

bool URecallRestoreSubsystem::IsRestoreClient(ARecallPlayerState_InGame& PlayerState) const
{
	const FString& PlayerID = PlayerState.GetSimPlayerId();
	return ClientRestoreInfos.Contains(PlayerID);
}

void URecallRestoreSubsystem::ServerUpdateClientRestoreData()
{
#if WITH_SERVER_CODE
	if (ClientRestoreInfos.IsEmpty())
	{
		return;
	}
	
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	
	// Only the host has to update clients
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(
		UGameplayStatics::GetGameState(World));
	if (!IsValid(GameState) || !GameState->HasAuthority())
	{
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ARecallPlayerState_InGame* InGamePlayerState = Cast<ARecallPlayerState_InGame>(PlayerState);
		if (!IsValid(InGamePlayerState) || !InGamePlayerState->IsRestoring())
		{
			continue;
		}
		
		ServerSendClientRestoreData(*InGamePlayerState);
	}
#endif // WITH_SERVER_CODE
}

void URecallRestoreSubsystem::ServerSendClientRestoreData(ARecallPlayerState_InGame& PlayerState)
{
#if WITH_SERVER_CODE
	const FString& PlayerId = PlayerState.GetSimPlayerId();
	FRecallRestoreClientInfo* RestoreClientInfo = ClientRestoreInfos.Find(PlayerId);
	if (!ensureAlwaysMsgf(RestoreClientInfo != nullptr,
		TEXT("%hs Should own restore info for restoring client: %s"), __FUNCTION__, *PlayerId))
	{
		return;
	}
	
	// If no combined transfer is active, start one
	if (RestoreClientInfo->CombinedTransferHandle == 0)
	{
		if (StartCombinedRestoreTransfer(PlayerState, RestoreClientInfo))
		{
			return; // Stay in Load state while transfer is active
		}
		else
		{
			// If transfer fails, move directly to finish state
			UE_LOG(LogRecallRestore, Error,
				TEXT("%hs: Failed to start combined restore transfer"), __FUNCTION__);
		}
	}
	else
	{
		// Check if combined transfer is complete
		if (DataTransferSubsystem.IsValid())
		{
			const EDataTransferStatus Status = DataTransferSubsystem->GetTransferStatus(
				RestoreClientInfo->CombinedTransferHandle);
					
			if (Status == EDataTransferStatus::Completed)
			{
				UE_LOG(LogRecallRestore, Log,
					TEXT("%hs: Combined restore transfer completed: Handle=%d"), __FUNCTION__,
					RestoreClientInfo->CombinedTransferHandle);
				RestoreClientInfo->CombinedTransferHandle = 0;
				FinishClientRestore(PlayerState, RestoreClientInfo->NextInputBunchStartFrame);
			}
			else if (Status == EDataTransferStatus::Failed)
			{
				UE_LOG(LogRecallRestore, Error,
					TEXT("%hs: Combined restore transfer failed: Handle=%d"), __FUNCTION__,
					RestoreClientInfo->CombinedTransferHandle);
				RestoreClientInfo->CombinedTransferHandle = 0;
			}
			// else Status is InProgress, continue waiting
		}
	}
#endif // WITH_SERVER_CODE
}


void URecallRestoreSubsystem::FinishClientRestore(ARecallPlayerState_InGame& PlayerState,
	uint32 LastRestoreInputFrame)
{
#if WITH_SERVER_CODE
	CloseRestoreClient(PlayerState);
	
	if (const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(
		UGameplayStatics::GetGameState(this)))
	{
		if (ARecallPlayerController* PC = Cast<ARecallPlayerController>(PlayerState.GetPlayerController()))
		{
			GameState->GetJoinGameComponentChecked()->FinishRestoring(*PC);
		}
	}
#endif // WITH_SERVER_CODE
}

bool URecallRestoreSubsystem::StartCombinedRestoreTransfer(ARecallPlayerState_InGame& PlayerState,
	FRecallRestoreClientInfo* RestoreClientInfo)
{
#if WITH_SERVER_CODE
	if (!RestoreClientInfo || !DataTransferSubsystem.IsValid())
	{
		return false;
	}
	
	const FString& PlayerID = PlayerState.GetSimPlayerId();
	const uint32 TargetFrame = RestoreClientInfo->SnapshotFrame;
	
	// Track the last frame we're sending to the client
	RestoreClientInfo->NextInputBunchStartFrame = TargetFrame;
	
	// Prepare combined data using the utility function
	TArray<uint8> CombinedDataBuffer;
	Recall::Restore::Utils::PrepareCombinedData(RestoreClientInfo->SnapshotMemory, RestoreClientInfo->SnapshotFrame,
		RestoreClientInfo->SnapshotEventCount, CombinedDataBuffer);
	
	if (CombinedDataBuffer.Num() == 0)
	{
		UE_LOG(LogRecallRestore, Error,
			TEXT("%hs: No combined data to transfer for player %s"), __FUNCTION__, *PlayerID);
		return false;
	}
	
	// Get server PlayerState
	APlayerState* ServerPlayerState = UGameplayStatics::GetPlayerState(this, 0);
	if (!ensure(ServerPlayerState))
	{
		return false;
	}
	
	// Configure transfer options - use snapshot options for combined data
	const FEasyDataTransferOptions CombinedOptions = Recall::Restore::Utils::GetSnapshotTransferOptions();
	
	// Start the combined transfer
	const int32 TransferHandle = DataTransferSubsystem->OpenDataChannel(
		TEXT("RecallCombinedRestore"),
		ServerPlayerState,  // Sender (server)
		&PlayerState,       // Receiver (client)
		CombinedDataBuffer,
		CombinedOptions);
	
	if (TransferHandle != 0)
	{
		RestoreClientInfo->CombinedTransferHandle = TransferHandle;
		UE_LOG(LogRecallRestore, Log,
			TEXT("%hs: Started combined restore transfer for player %s - Handle: %d, Size: %d bytes"), 
			   __FUNCTION__, *PlayerID, TransferHandle, CombinedDataBuffer.Num());
		return true;
	}
	else
	{
		UE_LOG(LogRecallRestore, Error,
			TEXT("%hs: Failed to start combined restore transfer for player %s"), __FUNCTION__, *PlayerID);
		return false;
	}
#else
	return false;
#endif
}
