// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Snapshot/RecallSnapshotFileUtils.h"

#include "Engine/World.h"
#include "HAL/PlatformFile.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "RecallFrontendUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "System/Snapshot/RecallSnapshotInterface.h"
#include "Utility/Snapshot/RecallSnapshotFileTypes.h"

static const FString SnapshotExtension(TEXT("snapshot"));

namespace Recall::SnapshotFile::Utils
{

FString GetMapName(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World))
	{
		return FString();
	}

	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);
	return MapName;
}

bool SaveSnapshot(const UObject* WorldContextObject, FRecallSnapshot& OutSnapshot)
{
	if (const TScriptInterface<IRecallSnapshotInterface> SnapshotInterface = Recall::Frontend::Utils::Get<IRecallSnapshotInterface>(WorldContextObject))
	{
		if (SnapshotInterface->TakeSnapshot(OutSnapshot.SnapshotMemory))
		{
			OutSnapshot.MapName = GetMapName(WorldContextObject);
			OutSnapshot.UtcTimeStamp = FDateTime::UtcNow();

#if WITH_EDITORONLY_DATA
			OutSnapshot.bIsEditorOnlyData = true;
#else // WITH_EDITORONLY_DATA
			OutSnapshot.bIsEditorOnlyData = false;
#endif
			return true;
		}
	}
	return false;
}

bool LoadSnapshot(const UObject* WorldContextObject, const FRecallSnapshot& Snapshot)
{
	const FString MapName = GetMapName(WorldContextObject);
	if (MapName != Snapshot.MapName)
	{
		return false;
	}

	if (const TScriptInterface<IRecallSnapshotInterface> SnapshotInterface = Recall::Frontend::Utils::Get<IRecallSnapshotInterface>(WorldContextObject))
	{
		return SnapshotInterface->LoadSnapshot(Snapshot.SnapshotMemory);
	}
	return false;
}

void SaveQuickSnapshotFile(const UObject* WorldContextObject, const FString& FileName)
{
	FRecallSnapshot Snapshot;
	if (SaveSnapshot(WorldContextObject, Snapshot))
	{
		SaveSnapshotFile(FileName, Snapshot);
	}
}

bool LoadQuickSnapshotFile(const UObject* WorldContextObject, const FString& FileName)
{
	FRecallSnapshot Snapshot;
	if (LoadSnapshotFile(FileName, Snapshot))
	{
		return LoadSnapshot(WorldContextObject, Snapshot);
	}
	return false;
}

TArray<FString> GetSnapshotFileNames(const UObject* WorldContextObject, bool bCurrentMapOnly /*= true*/)
{
	const FString SnapshotDirectoryPath = GetSnapshotDirectoryPath();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> FilePaths;
	PlatformFile.FindFiles(FilePaths, *SnapshotDirectoryPath, *SnapshotExtension);

	TArray<FString> FileNames;
	FileNames.Reserve(FilePaths.Num());

	Algo::Transform(FilePaths, FileNames, [&SnapshotDirectoryPath](const FString& FilePath)
		{
			FString FileName;

			if (!FilePath.Split(TEXT("/"), nullptr, &FileName, ESearchCase::IgnoreCase, ESearchDir::FromEnd) &&
				!FilePath.Split(TEXT("\\"), nullptr, &FileName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
			{
				FileName = FilePath;
				FileName.RemoveFromStart(SnapshotDirectoryPath);
			}
			
			FileName.RemoveFromEnd(FString::Printf(TEXT(".%s"), *SnapshotExtension));
			return FileName;
		}
	);

	// Cache and filter snapshots
	const FString MapName = GetMapName(WorldContextObject);

	TMap<FString, FRecallSnapshot> SnapshotMap;
	SnapshotMap.Reserve(FileNames.Num());

	auto ShouldSkipSnapshot = [bCurrentMapOnly, &MapName](const FRecallSnapshot& Snapshot)
	{
#if WITH_EDITORONLY_DATA
		constexpr bool bIsEditorOnlyData = true;
#else // WITH_EDITORONLY_DATA
		constexpr bool bIsEditorOnlyData = false;
#endif

		if (bIsEditorOnlyData != Snapshot.bIsEditorOnlyData)
		{
			return true;
		}

		if (bCurrentMapOnly && Snapshot.MapName != MapName)
		{
			return true;
		}

		return false;
	};

	for (int32 FileIndex = FileNames.Num() - 1; FileIndex >= 0; FileIndex--)
	{
		const FString& FileName = FileNames[FileIndex];

		// Todo: Validate data?
		FRecallSnapshot& Snapshot = SnapshotMap.Add(FileName);
		if (!LoadSnapshotFile(FileName, Snapshot) ||
			ShouldSkipSnapshot(Snapshot))
		{
			FileNames.RemoveAt(FileIndex);
		}
	}

	// Order by timestamp
	FileNames.Sort([&SnapshotMap](const FString& lFileName, const FString& rFileName)
		{
			return SnapshotMap[lFileName].UtcTimeStamp > SnapshotMap[rFileName].UtcTimeStamp;
		}
	);

	return FileNames;
}

void SaveSnapshotFile(const FString& FileName, const FRecallSnapshot& Snapshot)
{
	const FString ReplayFilePath = GetSnapshotPath(FileName);

	TArray<uint8> Memory;
	FMemoryWriter MemoryWriter(Memory, true);
	FRecallSnapshot::StaticStruct()->SerializeBin(MemoryWriter, (uint8*)&Snapshot);

	FFileHelper::SaveArrayToFile(Memory, *ReplayFilePath);
}

bool LoadSnapshotFile(const FString& FileName, FRecallSnapshot& OutSnapshot)
{
	const FString FilePath = GetSnapshotPath(FileName);

	TArray<uint8> Memory;
	if (!FFileHelper::LoadFileToArray(Memory, *FilePath))
	{
		return false;
	}

	FMemoryReader MemoryReader(Memory, true);
	FRecallSnapshot::StaticStruct()->SerializeBin(MemoryReader, &OutSnapshot);

	return true;
}

FString GetSnapshotPath(const FString& FileName)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*GetSnapshotDirectoryPath()))
	{
		PlatformFile.CreateDirectory(*GetSnapshotDirectoryPath());
	}

	return FString::Printf(TEXT("%s%s.%s"), *GetSnapshotDirectoryPath(), *FileName, *SnapshotExtension);
}

FString GetSnapshotDirectoryPath()
{
	return FString::Printf(TEXT("%sSnapshots/"), *FPaths::ProjectSavedDir());
}

} // namespace Recall::SnapshotFile::Utils
