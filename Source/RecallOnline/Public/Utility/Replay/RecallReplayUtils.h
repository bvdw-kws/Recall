// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "System/Replay/RecallReplayTypes.h"

namespace Recall::Replay::Utils
{

RECALLONLINE_API extern bool GenerateReplay(const UObject* WorldContextObject, FRecallReplay& OutReplay, const TArray<uint8>& DebugMenu);
RECALLONLINE_API extern void InitReplay(const UObject* WorldContextObject, const FRecallReplay& Replay);
RECALLONLINE_API extern void RestoreReplay(const UObject* WorldContextObject, uint32 Frame);
RECALLONLINE_API extern FString GetReplayPath(const FString& ReplayName);
RECALLONLINE_API extern TArray<FString> GetReplayNames();
RECALLONLINE_API extern FString GetReplayDirectoryPath();
RECALLONLINE_API extern FString GetMapName(const UObject* WorldContextObject);

RECALLONLINE_API extern void SaveReplay(const FRecallReplay& Replay, TArray<uint8>& OutMemory);
RECALLONLINE_API extern void LoadReplay(const TArray<uint8>& Memory, FRecallReplay& OutReplay);

RECALLONLINE_API extern void SaveReplayFile(const FRecallReplay& Replay, const FString& FileName);
RECALLONLINE_API extern bool LoadReplayFile(const FString& FileName, FRecallReplay& OutReplay);

} // namespace Recall::Replay::Utils
