// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

struct FRecallSnapshot;

namespace Recall::SnapshotFile::Utils
{

RECALLONLINE_API extern bool SaveSnapshot(const UObject* WorldContextObject, FRecallSnapshot& OutSnapshot);
RECALLONLINE_API extern bool LoadSnapshot(const UObject* WorldContextObject, const FRecallSnapshot& Snapshot);

RECALLONLINE_API extern void SaveQuickSnapshotFile(const UObject* WorldContextObject, const FString& FileName);
RECALLONLINE_API extern bool LoadQuickSnapshotFile(const UObject* WorldContextObject, const FString& FileName);
RECALLONLINE_API extern TArray<FString> GetSnapshotFileNames(const UObject* WorldContextObject, bool bCurrentMapOnly = true);

RECALLONLINE_API extern void SaveSnapshotFile(const FString& FileName, const FRecallSnapshot& Snapshot);
RECALLONLINE_API extern bool LoadSnapshotFile(const FString& FileName, FRecallSnapshot& OutSnapshot);

RECALLONLINE_API extern FString GetSnapshotPath(const FString& FileName);
RECALLONLINE_API extern FString GetSnapshotDirectoryPath();

RECALLONLINE_API extern FString GetMapName(const UObject* WorldContextObject);

} // namespace Recall::SnapshotFile::Utils
