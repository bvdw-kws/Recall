// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Desync/RecallDesyncLogSubsystem.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/RecallSimulationSettings.h"
#include "Subsystems/SubsystemCollection.h"
#include "System/Simulation/RecallSimulationSubsystem.h"
#ifdef WITH_MULTI_WORLD
#include "System/MultiWorldSubsystem.h"
#include "Utility/MultiWorldUtils.h"
#endif // WITH_MULTI_WORLD
#include "Utility/Simulation/RecallSimulationUtils.h"

void URecallDesyncLogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
#ifdef WITH_MULTI_WORLD
	Collection.InitializeDependency<UMultiWorldSubsystem>();
	MultiWorldSystem = UWorld::GetSubsystem<UMultiWorldSubsystem>(GetWorld());
#endif // WITH_MULTI_WORLD
	Collection.InitializeDependency<URecallSimulationSubsystem>();

	if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(GetWorld()))
	{
		SimulationSystem->OnFrameEnd.AddUObject(this, &ThisClass::OnFrameEnd);
	}
}

void URecallDesyncLogSubsystem::Deinitialize()
{
	Super::Deinitialize();

#ifdef WITH_MULTI_WORLD
	MultiWorldSystem.Reset();
#endif // WITH_MULTI_WORLD

	if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(GetWorld()))
	{
		SimulationSystem->OnFrameEnd.RemoveAll(this);
	}
}

void URecallDesyncLogSubsystem::Start(const FRecallSimulationStartParams& Params)
{
}

void URecallDesyncLogSubsystem::Reset()
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	FScopeLock Lock(&DataGuard);
	FrameLines.Reset();
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallDesyncLogSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallDesyncLogSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Desync_Save);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallDesyncLogSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallDesyncLogSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Desync_Restore);

	FScopeLock Lock(&DataGuard);
	if (Context.IsRollback())
	{
		while (!FrameLines.IsEmpty() && FrameLines.Last().Frame >= Context.Frame)
		{
			FrameLines.Pop();
		}
	}
	else if (Context.IsSnapshot())
	{
		FrameLines.Reset();
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallDesyncLogSubsystem::Log(const FString& Format, const FString& Function)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallDesyncLogSubsystem::Log"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Desync_Log);

	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	if (!SimulationSettings->bUseDesyncLog)
	{
		return;
	}

	const uint32 Frame = Recall::Simulation::Utils::GetFrame(GetWorld());
	const FString ZoneName = FPaths::GetBaseFilename(GetWorld()->GetMapName());
	const FString NewLine = FString::Printf(TEXT("[%d][%s] %s (%s)"), Frame, *ZoneName, *Format, *Function);

	{
		FScopeLock Lock(&DataGuard);
		FRecallFrameDesyncLines& FrameData = !FrameLines.IsEmpty() && FrameLines.Last().Frame == Frame ? FrameLines.Last() : FrameLines.Add_GetRef(FRecallFrameDesyncLines{ Frame });
		FrameData.Lines.Add(NewLine);
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallDesyncLogSubsystem::OnFrameEnd(uint32 Frame)
{
	TrimLog(Frame);
}

void URecallDesyncLogSubsystem::TrimLog(uint32 Frame)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Desync_TrimLog);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	FScopeLock Lock(&DataGuard);
	while (!FrameLines.IsEmpty() && FrameLines.Num() > SimulationSettings->FramesCount)
	{
		FrameLines.PopFrontNoCheck();
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallDesyncLogSubsystem::DumpDesyncLog()
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Desync_DumpLog);

	TArray<const UWorld*> WorldsToLog;
#ifdef WITH_MULTI_WORLD
	checkf(MultiWorld::Utils::IsMainWorld(this), TEXT("Only main world should dump log"));
	if (MultiWorldSystem.IsValid())
	{
		WorldsToLog = MultiWorldSystem->GetNestedWorlds();
	}
#else // WITH_MULTI_WORLD
	WorldsToLog.Add(GetWorld());
#endif

	if (!WorldsToLog.IsEmpty())
	{
		TMap<uint32, TArray<FString>> LinesFrameMap;
		int32 LineCount = 0;

		for (const UWorld* World : WorldsToLog)
		{
			if (const URecallDesyncLogSubsystem* DesyncLogSystem = UWorld::GetSubsystem<URecallDesyncLogSubsystem>(World))
			{
				FScopeLock Lock(&DesyncLogSystem->DataGuard);
				for (const auto& FrameLinesPair : DesyncLogSystem->FrameLines)
				{
					LinesFrameMap.FindOrAdd(FrameLinesPair.Frame).Append(FrameLinesPair.Lines);
					LineCount += FrameLinesPair.Lines.Num();
				}
			}
		}

		TArray<FString> Lines;
		Lines.Reserve(LineCount);

		{
			TArray<uint32> Frames;
			LinesFrameMap.GenerateKeyArray(Frames);

			Frames.Sort();

			for (const uint32 Frame : Frames)
			{
				Lines.Append(LinesFrameMap[Frame]);
			}
		}

		FFileHelper::SaveStringArrayToFile(Lines, *GetFileName());
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

FString URecallDesyncLogSubsystem::GetFileName() const
{
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	const FString Dir = FString::Printf(TEXT("%s/%s/"), *FPaths::ProjectSavedDir(), *SimulationSettings->Directory);

	if (!IPlatformFile::GetPlatformPhysical().DirectoryExists(*Dir))
	{
		IPlatformFile::GetPlatformPhysical().CreateDirectory(*Dir);
	}

	const int32 ProcessId = FPlatformProcess::GetCurrentProcessId();
#if WITH_EDITOR
	const int32 PIEInstanceID = GetOutermost()->GetPIEInstanceID();
	const FString ClientFileName = FString::Printf(
		TEXT("%s_%d_%d"), *SimulationSettings->FileName, ProcessId, PIEInstanceID);
#else // WITH_EDITOR
	const FString ClientFileName = FString::Printf(
		TEXT("%s_%d"), *SimulationSettings->FileName, ProcessId);
#endif

	const FString Result = FString::Printf(TEXT("%s%s"), *Dir, *ClientFileName);

	return FString::Printf(TEXT("%s.log"), *Result);
}
