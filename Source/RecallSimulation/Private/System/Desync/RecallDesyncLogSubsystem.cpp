// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Desync/RecallDesyncLogSubsystem.h"

#include "Desync/RecallDesyncLog.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassEntityTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/RecallSimulationSettings.h"
#include "Subsystems/SubsystemCollection.h"
#include "System/Simulation/RecallSimulationSubsystem.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

void URecallDesyncLogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Recall::MultiWorld::Utils::InitializeMultiWorldDependency(Collection);
	Collection.InitializeDependency<URecallSimulationSubsystem>();

	if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(GetWorld()))
	{
		SimulationSystem->OnFrameEnd.AddUObject(this, &ThisClass::OnFrameEnd);
	}
}

void URecallDesyncLogSubsystem::Deinitialize()
{
	Super::Deinitialize();

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
	const FString ZoneName = UWorld::RemovePIEPrefix(FPaths::GetBaseFilename(GetWorld()->GetMapName()));
	const FString NewLine = FString::Printf(TEXT("[%d][%s] %s (%s)"), Frame, *ZoneName, *Format, *Function);

	{
		FScopeLock Lock(&DataGuard);
		FRecallFrameDesyncLines& FrameData = !FrameLines.IsEmpty() && FrameLines.Last().Frame == Frame ? FrameLines.Last() : FrameLines.Add_GetRef(FRecallFrameDesyncLines{ Frame });
		FrameData.Lines.Add(NewLine);
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

#if RECALL_DESYNC_LOG && (WITH_EDITOR || WITH_MASSENTITY_DEBUG)
namespace
{
	struct FArchetypeLayoutEntry
	{
		uint32 CompositionKey = 0;
		FString Tags;
		int32 FragmentCount = 0;
		int32 EntityCount = 0;
		int32 ChunkCount = 0;
		uint32 EntityOrderHash = 0;
	};

	/**
	 * Logs one line per non-empty archetype: a stable key of its composition (type names, not Mass type indices which
	 * differ between processes), its entity/chunk counts and a hash of the entities in storage order.
	 * The first frame where an OrderHash differs between two clients is where the entity order diverged.
	 */
	void LogArchetypeLayout(URecallDesyncLogSubsystem& DesyncLog, const FMassEntityManager& EntityManager)
	{
		TArray<FArchetypeLayoutEntry> Entries;

		EntityManager.ForEachArchetype(0, MAX_int32, [&Entries](const FMassEntityManager&, FMassArchetypeHandle& ArchetypeHandle)
		{
			FArchetypeLayoutEntry Entry;
			uint32 EntityOrderHash = 0;

			FMassEntityManager::ForEachArchetypeChunkEntityList(ArchetypeHandle, [&Entry, &EntityOrderHash](TConstArrayView<FMassEntityHandle> Entities)
			{
				Entry.ChunkCount++;
				Entry.EntityCount += Entities.Num();
				for (const FMassEntityHandle& Entity : Entities)
				{
					EntityOrderHash = HashCombine(EntityOrderHash, HashCombine(::GetTypeHash(Entity.Index), ::GetTypeHash(Entity.SerialNumber)));
				}
			});

			if (Entry.EntityCount == 0)
			{
				return;
			}

			TArray<FString> TagNames;
			TArray<FString> AllNames;
			FMassEntityManager::ForEachArchetypeElementType(ArchetypeHandle, [&Entry, &TagNames, &AllNames](const UScriptStruct* ElementType)
			{
				const FString Name = ElementType->GetName();
				AllNames.Add(Name);
				if (ElementType->IsChildOf(FMassTag::StaticStruct()))
				{
					TagNames.Add(Name);
				}
				else
				{
					Entry.FragmentCount++;
				}
				return true;
			});

			AllNames.Sort();
			TagNames.Sort();
			Entry.CompositionKey = FCrc::StrCrc32(*FString::Join(AllNames, TEXT(",")));
			Entry.Tags = FString::Join(TagNames, TEXT("|"));
			Entry.EntityOrderHash = EntityOrderHash;
			Entries.Add(MoveTemp(Entry));
		});

		// Stable output order so the same archetypes line up between clients.
		Entries.Sort([](const FArchetypeLayoutEntry& A, const FArchetypeLayoutEntry& B)
		{
			if (A.CompositionKey != B.CompositionKey)
			{
				return A.CompositionKey < B.CompositionKey;
			}
			if (A.EntityCount != B.EntityCount)
			{
				return A.EntityCount < B.EntityCount;
			}
			return A.EntityOrderHash < B.EntityOrderHash;
		});

		for (const FArchetypeLayoutEntry& Entry : Entries)
		{
			DesyncLog.Log(FString::Printf(TEXT("ArchetypeLayout: Value<Key: %08X, Tags: %s, Fragments: %d, Entities: %d, Chunks: %d, OrderHash: %08X>"),
				Entry.CompositionKey, *Entry.Tags, Entry.FragmentCount, Entry.EntityCount, Entry.ChunkCount, Entry.EntityOrderHash),
				TEXT("LogArchetypeLayout"));
		}
	}
}
#endif // RECALL_DESYNC_LOG && (WITH_EDITOR || WITH_MASSENTITY_DEBUG)

void URecallDesyncLogSubsystem::OnFrameEnd(uint32 Frame)
{
#if RECALL_DESYNC_LOG && (WITH_EDITOR || WITH_MASSENTITY_DEBUG)
	if (GetDefault<URecallSimulationSettings>()->bUseDesyncLog)
	{
		if (const UMassEntitySubsystem* EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld()))
		{
			LogArchetypeLayout(*this, EntitySubsystem->GetEntityManager());
		}
	}
#endif

	TrimLog(Frame);
}

static TAutoConsoleVariable<int32> CVarDesyncLogFrames(
	TEXT("recall.desync.LogFrames"), 0,
	TEXT("Number of frames kept in the desync log when larger than the simulation FramesCount (very large logs)."), ECVF_Default);

void URecallDesyncLogSubsystem::TrimLog(uint32 Frame)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Desync_TrimLog);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	FScopeLock Lock(&DataGuard);
	const int32 MaxFrames = FMath::Max<int32>(SimulationSettings->FramesCount, CVarDesyncLogFrames.GetValueOnAnyThread());
	while (!FrameLines.IsEmpty() && FrameLines.Num() > MaxFrames)
	{
		FrameLines.PopFrontNoCheck();
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallDesyncLogSubsystem::DumpDesyncLog()
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Desync_DumpLog);

	TArray<const UWorld*> WorldsToLog = Recall::MultiWorld::Utils::GetMultiWorlds(this);
	checkf(Recall::MultiWorld::Utils::IsMainWorld(this), TEXT("Only main world should dump log"));

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
