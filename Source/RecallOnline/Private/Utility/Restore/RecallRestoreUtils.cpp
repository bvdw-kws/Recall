// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Restore/RecallRestoreUtils.h"

#include "Components/GameState/RecallClientRestoreComponent.h"
#include "Components/GameState/RecallGameSimulationComponent.h"
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
#include "Utility/MultiWorldUtils.h"
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
}

static void InitGameSimulationWorld(const UObject* WorldContextObject, int32 WorldIndex, bool bSeed)
{
	if (const UWorld* World = MultiWorld::Utils::GetWorldByIndex(WorldContextObject, WorldIndex))
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
	const int32 WorldCount = MultiWorld::Utils::GetWorldCount(WorldContextObject);

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

bool ProcessCombinedRestoreData(const UObject* WorldContextObject, const TArray<uint8>& Data, uint32& OutTargetFrame)
{
	// Validate minimum data size
	if (Data.Num() < sizeof(uint8) + sizeof(int32) + sizeof(uint32)) // Version + snapshot size + frame
	{
		UE_LOG(LogRecallRestore, Error, TEXT("%hs: Data buffer too small: %d bytes"), __FUNCTION__, Data.Num());
		return false;
	}
	
	FMemoryReader Reader(Data);
	
	// Read header
	uint8 DataVersion = 0;
	Reader << DataVersion;
	
	if (DataVersion != 1)
	{
		UE_LOG(LogRecallRestore, Error, TEXT("%hs: Unknown data version: %d"), __FUNCTION__, DataVersion);
		return false;
	}
	
	// Read snapshot data
	int32 SnapshotSize = 0;
	uint32 SnapshotFrame = 0;
	Reader << SnapshotSize;
	Reader << SnapshotFrame;
	
	if (SnapshotSize > 0)
	{
		TArray<uint8> SnapshotMemory;
		SnapshotMemory.SetNum(SnapshotSize);
		Reader.Serialize(SnapshotMemory.GetData(), SnapshotSize);
		
		UE_LOG(LogRecallRestore, Log, TEXT("%hs: Received snapshot data - Size: %d bytes, Frame: %d"), 
		       __FUNCTION__, SnapshotSize, SnapshotFrame);
		
		// Load the snapshot
		LoadClientSnapshot(WorldContextObject, SnapshotMemory);
	}
	
	OutTargetFrame = SnapshotFrame;
	return true;
}

void PrepareCombinedData(const TArray<uint8>& SnapshotMemory, uint32 SnapshotFrame, TArray<uint8>& OutCombinedDataBuffer)
{
	// Clear previous buffer
	OutCombinedDataBuffer.Reset();
	
	// Combine snapshot and input data
	FMemoryWriter Writer(OutCombinedDataBuffer);
	
	// Write header
	uint8 DataVersion = 1; // Version for future compatibility
	Writer << DataVersion;
	
	// Write snapshot data
	int32 SnapshotSize = SnapshotMemory.Num();
	Writer << SnapshotSize;
	Writer << SnapshotFrame;
	Writer.Serialize(const_cast<uint8*>(SnapshotMemory.GetData()), SnapshotSize);
	
	UE_LOG(LogRecallRestore, Log, TEXT("%hs: Prepared combined restore data - Snapshot: %d bytes, Total: %d bytes"), 
		   __FUNCTION__, SnapshotSize, OutCombinedDataBuffer.Num());
}
	
} // namespace Recall::Restore::Utils
