// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

struct FEasyDataTransferOptions;
struct FRecallRestoreClientInfo;
struct FRecallRestoreInputBunch;

namespace Recall::Restore::Utils
{

/**
 * Initializes restore client info by taking a snapshot of the current game state.
 * @param WorldContextObject Context for world access
 * @param RestoreClientInfo Restore info struct to populate with snapshot data and frame
 */
RECALLONLINE_API extern void InitRestoreClientInfo(
	const UObject* WorldContextObject, FRecallRestoreClientInfo& RestoreClientInfo);

/**
 * Loads a client snapshot into the game simulation, restoring the game state.
 * @param WorldContextObject Context for world access
 * @param SnapshotMemory Serialized snapshot data to load
 */
RECALLONLINE_API extern void LoadClientSnapshot(
	const UObject* WorldContextObject, const TArray<uint8>& SnapshotMemory);
	
/**
 * Initializes game simulation for all worlds, optionally seeding random number generators.
 * @param WorldContextObject Context for world access
 * @param bSeed Whether to seed random number generators with stored seeds
 */
RECALLONLINE_API extern void InitGameSimulation(const UObject* WorldContextObject, bool bSeed = true);

/**
 * Processes combined restore data received from server, extracting and loading snapshot.
 * @param WorldContextObject Context for world access
 * @param Data Combined restore data buffer containing version, snapshot size, frame, and snapshot data
 * @param OutTargetFrame Returns the frame number at which the snapshot was taken
 * @return True if processing succeeded, false if data was invalid or processing failed
 */
RECALLONLINE_API extern bool ProcessCombinedRestoreData(
	const UObject* WorldContextObject, const TArray<uint8>& Data, uint32& OutTargetFrame);

/**
 * Prepares combined restore data by serializing snapshot and frame information.
 * @param SnapshotMemory Snapshot data to include in combined buffer
 * @param SnapshotFrame Frame number at which snapshot was taken
 * @param OutCombinedDataBuffer Output buffer containing version header, snapshot size, frame, and snapshot data
 */
RECALLONLINE_API extern void PrepareCombinedData(
	const TArray<uint8>& SnapshotMemory, uint32 SnapshotFrame, TArray<uint8>& OutCombinedDataBuffer);
	
	/**
	 * Gets the data transfer options configured for snapshot transfers.
	 * @return Configured options for EasyDataTransfer
	 */
RECALLONLINE_API FEasyDataTransferOptions GetSnapshotTransferOptions();
	
} // namespace Recall::Restore::Utils
