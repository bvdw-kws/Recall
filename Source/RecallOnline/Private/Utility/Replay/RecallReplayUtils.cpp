// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Replay/RecallReplayUtils.h"

#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformFile.h"
#include "HAL/FileManager.h"
#include "RecallFrontendUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Online/RecallGameState_InGame.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "System/Input/RecallInputQueueInterface.h"
#include "System/Player/RecallPlayerQueueInterface.h"
#include "System/Random/RecallRandomNumberInterface.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"
#include "Utility/Simulation/RecallSimulationUtils.h"
#include "Utility/MultiWorldUtils.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
#include "Engine/GameInstance.h"
#include "System/Debug/DebugMenuSubsystem.h"
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

static const FString ReplayExtension(TEXT("replay"));

namespace Recall::Replay::Utils
{

static void GenerateWorldReplay(const UWorld* World, FRecallWorldReplay& OutReplay)
{
	if (IsValid(World) == false)
	{
		checkNoEntry();
		return;
	}

	OutReplay.Seed = Recall::Frontend::Utils::GetRefByWorld<IRecallRandomNumberInterface>(World).GetSeed();
	OutReplay.PlayerQueue = Recall::Frontend::Utils::GetRefByWorld<IRecallPlayerQueueInterface>(World).GetPlayerQueue();
}

static bool LoadWorldReplay(const UWorld* World, const FRecallWorldReplay& Replay)
{
	Recall::Frontend::Utils::GetRefByWorld<IRecallRandomNumberInterface>(World).SetSeed(Replay.Seed);
	Recall::Frontend::Utils::GetRefByWorld<IRecallPlayerQueueInterface>(World).SetPlayerQueue(Replay.PlayerQueue);

	return true;
}

bool GenerateReplay(const UObject* WorldContextObject, FRecallReplay& OutReplay, const TArray<uint8>& DebugMenu)
{
	if (const TScriptInterface<IRecallSimulationControllerInterface> SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(WorldContextObject))
	{
		OutReplay.UtcTimeStamp = FDateTime::UtcNow();

		OutReplay.FrameCount = SimulationController->GetElapsedFrame();
		OutReplay.Duration = SimulationController->GetElapsedTime();

		OutReplay.MapName = GetMapName(WorldContextObject);

		OutReplay.InputQueue = Recall::Frontend::Utils::GetRef<IRecallInputQueueInterface>(WorldContextObject).GetInputQueue();
		OutReplay.InputQueue.RemoveInputPastFrame(OutReplay.FrameCount);

		if (const ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(WorldContextObject)))
		{
			const URecallGameSimulationComponent* GameSimulationComponent = InGameState->GetGameSimulationComponentChecked();
			OutReplay.StartParams = GameSimulationComponent->GetGameSimStartParams();
			
			InGameState->SaveReplayCustomData(OutReplay.CustomData);
		}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		// OutReplay.DebugMenu = DebugMenu;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

		const int32 WorldCount = MultiWorld::Utils::GetWorldCount(WorldContextObject);
		OutReplay.Worlds.SetNum(WorldCount);

		for (int32 WorldIndex = 0; WorldIndex < WorldCount; WorldIndex++)
		{
			if (const UWorld* NestedWorld = MultiWorld::Utils::GetWorldByIndex(WorldContextObject, WorldIndex))
			{
				GenerateWorldReplay(NestedWorld, OutReplay.Worlds[WorldIndex]);
			}
		}

		return true;
	}

	return false;
}

FString GetMapName(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (IsValid(World) == false)
	{
		return FString();
	}

	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);
	return MapName;
}

void InitReplay(const UObject* WorldContextObject, const FRecallReplay& Replay)
{
	if (IsValid(WorldContextObject) == false)
	{
		return;
	}

	checkf(!Recall::Frontend::Utils::GetRef<IRecallSimulationControllerInterface>(WorldContextObject).HasSimulationStarted(), TEXT("Should not start simulation before loading replay"));

	Recall::Frontend::Utils::GetRef<IRecallInputQueueInterface>(WorldContextObject).SetInputQueue(Replay.InputQueue);

	if (ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(WorldContextObject)))
	{
		if (InGameState->HasAuthority())
		{
			URecallGameSimulationComponent* GameSimulationComponent = InGameState->GetGameSimulationComponentChecked();
			GameSimulationComponent->SetGameSimStartParams(Replay.StartParams);
		
			InGameState->LoadReplayCustomData(Replay.CustomData);
		}
	}

	for (int32 WorldIndex = 0; WorldIndex < MultiWorld::Utils::GetWorldCount(WorldContextObject); WorldIndex++)
	{
		if (UWorld* NestedWorld = MultiWorld::Utils::GetWorldByIndex(WorldContextObject, WorldIndex))
		{
			if (ensure(Replay.Worlds.IsValidIndex(WorldIndex)) == false) continue;

			const FRecallWorldReplay& WorldReplay = Replay.Worlds[WorldIndex];

			check(Recall::Simulation::Utils::GetFrame(NestedWorld) == 0);
			LoadWorldReplay(NestedWorld, WorldReplay);
		}
	}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	/*
	if (const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		if (UDebugMenuSubsystem* DebugMenuSystem = GameInstance->GetSubsystem<UDebugMenuSubsystem>())
		{
			DebugMenuSystem->LoadDebugMenu(Replay.DebugMenu);
		}
	}
	*/
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void RestoreReplay(const UObject* WorldContextObject, uint32 Frame)
{
	Recall::Frontend::Utils::GetRef<IRecallInputQueueInterface>(WorldContextObject).ClearInputQueuePastFrame(Frame);

	for (int32 WorldIndex = 0; WorldIndex < MultiWorld::Utils::GetWorldCount(WorldContextObject); WorldIndex++)
	{
		if (const UWorld* World = MultiWorld::Utils::GetWorldByIndex(WorldContextObject, WorldIndex))
		{
			Recall::Frontend::Utils::GetRefByWorld<IRecallPlayerQueueInterface>(World).ClearPlayerQueuePastFrame(Frame);
		}
	}
}

FString GetReplayPath(const FString& ReplayName)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*GetReplayDirectoryPath()))
	{
		PlatformFile.CreateDirectory(*GetReplayDirectoryPath());
	}

	return FString::Printf(TEXT("%s%s.%s"), *GetReplayDirectoryPath(), *ReplayName, *ReplayExtension);
}

TArray<FString> GetReplayNames()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> FilePaths;
	PlatformFile.FindFiles(FilePaths, *GetReplayDirectoryPath(), *ReplayExtension);

	TArray<FString> FileNames;
	FileNames.Reserve(FilePaths.Num());

	Algo::Transform(FilePaths, FileNames, [](const FString& FilePath)
		{
			FString FileName;

			if (!FilePath.Split(TEXT("/"), nullptr, &FileName, ESearchCase::IgnoreCase, ESearchDir::FromEnd) &&
				!FilePath.Split(TEXT("\\"), nullptr, &FileName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
			{
				FileName = FilePath;
				FileName.RemoveFromStart(GetReplayDirectoryPath());
			}
			
			FileName.RemoveFromEnd(FString::Printf(TEXT(".%s"), *ReplayExtension));
			return FileName;
		}
	);

	return FileNames;
}

FString GetReplayDirectoryPath()
{
	return FString::Printf(TEXT("%sSaved/Replays/"), *FPaths::ProjectDir());
}

void SaveReplay(const FRecallReplay& Replay, TArray<uint8>& OutMemory)
{
	OutMemory.Empty();
	FMemoryWriter MemoryWriter(OutMemory);
	FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
	FRecallReplay::StaticStruct()->SerializeBin(Ar, (uint8*)&Replay);
}

void LoadReplay(const TArray<uint8>& Memory, FRecallReplay& OutReplay)
{
	FMemoryReader MemoryReader(Memory);
	FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
	FRecallReplay::StaticStruct()->SerializeBin(Ar, &OutReplay);
}

void SaveReplayFile(const FRecallReplay& Replay, const FString& FileName)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Replay_SaveFile);

	const FString ReplayFilePath = Recall::Replay::Utils::GetReplayPath(FileName);

	TArray<uint8> Memory;
	SaveReplay(Replay, Memory);

	FFileHelper::SaveArrayToFile(Memory, *ReplayFilePath);
}

bool LoadReplayFile(const FString& FileName, FRecallReplay& OutReplay)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Replay_LoadFile);

	const FString ReplayFilePath = Recall::Replay::Utils::GetReplayPath(FileName);

	TArray<uint8> Memory;
	if (!FFileHelper::LoadFileToArray(Memory, *ReplayFilePath))
	{
		return false;
	}

	LoadReplay(Memory, OutReplay);

	return true;
}

} // namespace Recall::Replay::Utils
