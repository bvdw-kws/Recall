// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Restore/RecallRestoreUtils.h"

#include "Components/GameState/RecallClientRestoreComponent.h"
#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallPlayerSyncGateComponent.h"
#include "Components/GameState/RecallReplayGameComponent.h"
#include "DataTransfer/EasyDataTransferTypes.h"
#include "Kismet/GameplayStatics.h"
#include "RecallFrontendUtils.h"
#include "Online/RecallGameState_InGame.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "System/Random/RecallRandomNumberInterface.h"
#include "System/Restore/RecallRestoreTypes.h"
#include "System/Snapshot/RecallSnapshotInterface.h"
#include "System/Synchronization/RecallSynchronizationInterface.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"
#include "Utility/Replay/RecallReplayUtils.h"
#include "Utility/Synchronization/RecallSynchronizationUtils.h"

namespace Recall::Restore::Utils
{
	
void InitRestoreClientInfo(const UObject* WorldContextObject, FRecallRestoreClientInfo& RestoreClientInfo)
{
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(WorldContextObject));
	if (!ensure(IsValid(GameState)))
	{
		return;
	}
	
	const URecallReplayGameComponent* ReplayComponent = GameState->GetReplayComponentChecked();
	if (ReplayComponent->IsPlayingReplay())
	{
		FRecallReplay Replay;
		Recall::Replay::Utils::GenerateReplay(WorldContextObject, Replay, {});
		Recall::Replay::Utils::SaveReplay(Replay, RestoreClientInfo.SnapshotMemory);
		RestoreClientInfo.SnapshotFrame = Replay.FrameCount;
	}
	else
	{
		IRecallSynchronizationInterface& Sync = Recall::Synchronization::Utils::GetSynchronizationRef(WorldContextObject);
		if (Sync.IsRollbackEnabled())
		{
			Sync.SaveLastSyncedSnapshot(RestoreClientInfo.SnapshotMemory);
			RestoreClientInfo.SnapshotFrame = Sync.GetLastSyncedFrame();
		}
		else
		{
			IRecallSnapshotInterface& Snapshot = Recall::Frontend::Utils::GetRef<IRecallSnapshotInterface>(WorldContextObject);
			Snapshot.TakeSnapshot(RestoreClientInfo.SnapshotMemory);
			RestoreClientInfo.SnapshotFrame = GameState->GetGameSimulationComponentChecked()->GetLocalSimulationFrame();
		}
	}	

	// Capture the event count in lockstep with the snapshot above, so it describes exactly the
	// player events (AddPlayer/RemovePlayer, join/leave flags) already reflected by this snapshot.
	// Any event issued after this point is still delivered normally to the restoring client via the
	// ordinary multicast RPC / OnRep path and must not be double-counted by this baseline.
	RestoreClientInfo.SnapshotEventCount = GameState->GetPlayerSyncGateComponentChecked()->GetReplicatedEventCount();
}

static void InitGameSimulationWorld(const UObject* WorldContextObject, int32 WorldIndex, bool bSeed)
{
	if (const UWorld* World = Recall::MultiWorld::Utils::GetWorldByIndex(WorldContextObject, WorldIndex))
	{
		const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(World));
		
		if (IsValid(GameState))
		{
			const URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
			const FRecallRestoreWorldArray& WorldStates = GameSimulationComponent->GetWorldStates();

			if (ensureMsgf(WorldStates.Items.IsValidIndex(WorldIndex), TEXT("Field not found")))
			{
				const FRecallWorldItemEntry& Item = WorldStates.Items[WorldIndex];

				if (bSeed)
				{
					Recall::Frontend::Utils::GetRefByWorld<IRecallRandomNumberInterface>(World).SetSeed(Item.Seed);
				}
			}
		}
	}
}

void InitGameSimulation(const UObject* WorldContextObject, bool bSeed /*= true*/)
{
	const int32 WorldCount = Recall::MultiWorld::Utils::GetWorldCount(WorldContextObject);

	for (int32 WorldIndex = 0; WorldIndex < WorldCount; WorldIndex++)
	{
		InitGameSimulationWorld(WorldContextObject, WorldIndex, bSeed);
	}
}

void LoadClientSnapshot(
	const UObject* WorldContextObject, const TArray<uint8>& SnapshotMemory)
{
	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(WorldContextObject));
	if (!ensure(IsValid(GameState)))
	{
		return;
	}

	// Do not sync past the first frame while playing a replay.
	const URecallReplayGameComponent* ReplayComponent = GameState->GetReplayComponentChecked();
	if (ReplayComponent->IsPlayingReplay())
	{
		FRecallReplay Replay;
		Recall::Replay::Utils::LoadReplay(SnapshotMemory, Replay);
		Recall::Replay::Utils::InitReplay(WorldContextObject, Replay);

		const URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
		GameState->GetClientRestoreComponentChecked()->StartGameSimulation(GameSimulationComponent->GetGameSimStartParams());
	}
	else
	{		
		// Load the game simulation snapshot, it will overwrite all the simulation data.
		IRecallSynchronizationInterface& Sync = Recall::Synchronization::Utils::GetSynchronizationRef(WorldContextObject);
		if (Sync.IsRollbackEnabled())
		{
			Sync.LoadLastSyncedSnapshot(SnapshotMemory);
		}
		else
		{			
			IRecallSnapshotInterface& Snapshot = Recall::Frontend::Utils::GetRef<IRecallSnapshotInterface>(WorldContextObject);
			Snapshot.LoadSnapshot(SnapshotMemory);
		}
	}
}

FEasyDataTransferOptions GetSnapshotTransferOptions()
{
	FEasyDataTransferOptions Options;
	Options.Priority = 10; // High priority for restore data
	Options.ChunkSize = 4096; // 4KB chunks for snapshot data
	Options.bEnableCompression = true;
	Options.TimeoutSeconds = 60.0f; // Longer timeout for large snapshots
	Options.MaxRetries = 5;
	Options.bAdaptiveChunking = true;
	Options.SlidingWindowSize = 8; // Larger window for snapshot data
	Options.BandwidthLimit = 0; // No specific limit for restore data
	
	UE_LOG(LogRecallRestore, Verbose, TEXT("%hs: Snapshot transfer options - ChunkSize: %d, Compression: %s, Timeout: %.1f, WindowSize: %d"), 
	       __FUNCTION__, Options.ChunkSize, Options.bEnableCompression ? TEXT("Yes") : TEXT("No"), Options.TimeoutSeconds, Options.SlidingWindowSize);
	
	return Options;
}

bool ProcessCombinedRestoreData(const UObject* WorldContextObject, const TArray<uint8>& Data, uint32& OutTargetFrame, uint32& OutSnapshotEventCount)
{
	// Validate minimum data size
	if (Data.Num() < sizeof(uint8) + sizeof(int32) + sizeof(uint32) + sizeof(uint32)) // Version + snapshot size + frame + event count
	{
		UE_LOG(LogRecallRestore, Error, TEXT("%hs: Data buffer too small: %d bytes"), __FUNCTION__, Data.Num());
		return false;
	}
	
	FMemoryReader Reader(Data);
	
	// Read header
	uint8 DataVersion = 0;
	Reader << DataVersion;
	
	if (DataVersion != 2)
	{
		UE_LOG(LogRecallRestore, Error, TEXT("%hs: Unknown data version: %d"), __FUNCTION__, DataVersion);
		return false;
	}
	
	// Read snapshot data
	int32 SnapshotSize = 0;
	uint32 SnapshotFrame = 0;
	uint32 SnapshotEventCount = 0;
	Reader << SnapshotSize;
	Reader << SnapshotFrame;
	Reader << SnapshotEventCount;
	
	if (SnapshotSize > 0)
	{
		TArray<uint8> SnapshotMemory;
		SnapshotMemory.SetNum(SnapshotSize);
		Reader.Serialize(SnapshotMemory.GetData(), SnapshotSize);
		
		UE_LOG(LogRecallRestore, Log, TEXT("%hs: Received snapshot data - Size: %d bytes, Frame: %d, EventCount: %d"), 
		       __FUNCTION__, SnapshotSize, SnapshotFrame, SnapshotEventCount);
		
		// Load the snapshot
		LoadClientSnapshot(WorldContextObject, SnapshotMemory);
	}
	
	OutTargetFrame = SnapshotFrame;
	OutSnapshotEventCount = SnapshotEventCount;
	return true;
}

void PrepareCombinedData(const TArray<uint8>& SnapshotMemory, uint32 SnapshotFrame, uint32 SnapshotEventCount, TArray<uint8>& OutCombinedDataBuffer)
{
	// Clear previous buffer
	OutCombinedDataBuffer.Reset();
	
	// Combine snapshot and input data
	FMemoryWriter Writer(OutCombinedDataBuffer);
	
	// Write header
	uint8 DataVersion = 2; // Bumped to 2: added SnapshotEventCount for the late-join sync gate fixup
	Writer << DataVersion;
	
	// Write snapshot data
	int32 SnapshotSize = SnapshotMemory.Num();
	Writer << SnapshotSize;
	Writer << SnapshotFrame;
	Writer << SnapshotEventCount;
	Writer.Serialize(const_cast<uint8*>(SnapshotMemory.GetData()), SnapshotSize);
	
	UE_LOG(LogRecallRestore, Log, TEXT("%hs: Prepared combined restore data - Snapshot: %d bytes, EventCount: %d, Total: %d bytes"), 
		   __FUNCTION__, SnapshotSize, SnapshotEventCount, OutCombinedDataBuffer.Num());
}
	
} // namespace Recall::Restore::Utils
