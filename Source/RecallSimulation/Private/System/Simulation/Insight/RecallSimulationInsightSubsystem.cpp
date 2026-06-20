// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Simulation/Insight/RecallSimulationInsightSubsystem.h"

#include "Engine/World.h"
#include "MassEntityManager.h"
#include "MassEntityUtils.h"
#include "MassEntityView.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "Subsystems/SubsystemCollection.h"
#include "System/AI/RecallStateTreeSubsystem.h"
#include "System/Entity/RecallEntitySubsystem.h"
#include "System/Random/RecallRandomNumberSubsystem.h"
#ifdef WITH_MULTI_WORLD
#include "System/MultiWorldSubsystem.h"
#endif // WITH_MULTI_WORLD
#include "Utility/Entity/RecallEntityUtils.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
#include "HAL/IConsoleManager.h"
#include "System/Simulation/RecallSimulationSubsystem.h"

static bool bDebugSimulationInsight = true;
static FAutoConsoleVariableRef CVarFRecallActorRepresentationDescSimulationInsight(
	TEXT("recall.simulation.Insight"),
	bDebugSimulationInsight,
	TEXT("Simulation Insight")
);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

URecallSimulationInsightSubsystem::URecallSimulationInsightSubsystem()
	: Super()
{
}

void URecallSimulationInsightSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
#ifdef WITH_MULTI_WORLD
	Collection.InitializeDependency<UMultiWorldSubsystem>();
	MultiWorldSystem = UWorld::GetSubsystem<UMultiWorldSubsystem>(GetWorld());
#endif // WITH_MULTI_WORLD
	Collection.InitializeDependency<URecallRandomNumberSubsystem>();
	Collection.InitializeDependency<URecallStateTreeSubsystem>();
	Collection.InitializeDependency<URecallEntitySubsystem>();
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	Collection.InitializeDependency<URecallSimulationSubsystem>();
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	RandomNumberSystem = UWorld::GetSubsystem<URecallRandomNumberSubsystem>(GetWorld());
	StateTreeSystem = UWorld::GetSubsystem<URecallStateTreeSubsystem>(GetWorld());
	EntitySystem = UWorld::GetSubsystem<URecallEntitySubsystem>(GetWorld());

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(GetWorld()))
	{
		SimulationSystem->OnFrameStart.AddUObject(this, &ThisClass::OnFrameStart);
		SimulationSystem->OnFrameEnd.AddUObject(this, &ThisClass::OnFrameEnd);
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallSimulationInsightSubsystem::Deinitialize()
{
	Super::Deinitialize();

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (URecallSimulationSubsystem* SimulationSystem = UWorld::GetSubsystem<URecallSimulationSubsystem>(GetWorld()))
	{
		SimulationSystem->OnFrameStart.RemoveAll(this);
		SimulationSystem->OnFrameEnd.RemoveAll(this);
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallSimulationInsightSubsystem::Start(const FRecallSimulationStartParams& Params)
{
}

void URecallSimulationInsightSubsystem::Reset()
{
	FScopeLock Lock(&DataGuard);
	
	LastFrames.Reset();
}

void URecallSimulationInsightSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallSimulationInsightSubsystem::Save"));
}

void URecallSimulationInsightSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallSimulationInsightSubsystem::Restore"));

	FScopeLock Lock(&DataGuard);
	
	if (Context.IsRollback())
	{
		int32 DeprecatedReportCount = 0;

		for (int32 FrameReportIndex = LastFrames.Num() - 1; FrameReportIndex >= 0; FrameReportIndex--)
		{
			if (LastFrames[FrameReportIndex].Frame >= Context.Frame)
			{
				DeprecatedReportCount++;
			}
			else
			{
				break;
			}
		}

		LastFrames.Pop(DeprecatedReportCount);
	}
	else if (Context.IsSnapshot())
	{
		LastFrames.Reset();
	}
}

void URecallSimulationInsightSubsystem::OnFrameStart(uint32 Frame)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (bDebugSimulationInsight)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallSimulationInsightSubsystem::OnFrameStart"));
		QUICK_SCOPE_CYCLE_COUNTER(Recall_Insight_OnFrameStart);

		// Cleanup old frame reports.
		{
			FScopeLock Lock(&DataGuard);
		
			int32 OldReportCount = 0;

			// Delete old frame reports.
			for (int32 FrameIndex = 0; FrameIndex < LastFrames.Num(); FrameIndex++)
			{
				const FRecallSimulationFrameReport& OldFrameReport = LastFrames[FrameIndex];

				if (OldFrameReport.Frame < FromFrame)
				{
					OldReportCount++;
				}
				else
				{
					break;
				}
			}

			LastFrames.PopFront(OldReportCount);
		}

		// Record this frame.
		const FRecallSimulationFrameReport FrameReport = GenerateFrameReport(Frame);
		{
			FScopeLock Lock(&DataGuard);		
			LastFrames.Add(FrameReport);
		}
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallSimulationInsightSubsystem::OnFrameEnd(uint32 Frame)
{
}

void URecallSimulationInsightSubsystem::SetReportFromFrame(uint32 Frame)
{
	for (const UWorld* World : GetMultiWorlds())
	{
		if (URecallSimulationInsightSubsystem* InsightSystem = UWorld::GetSubsystem<URecallSimulationInsightSubsystem>(World))
		{
			InsightSystem->SetReportFromFrame_Internal(Frame);
		}
	}
}

void URecallSimulationInsightSubsystem::SetReportFromFrame_Internal(uint32 Frame)
{
	FromFrame = Frame;
}

FRecallSimulationInsight URecallSimulationInsightSubsystem::GenerateReportInRange(uint32 StartFrame, uint32 EndFrame) const
{
	FRecallSimulationInsight Report;
	const TArray<const UWorld*> Worlds = GetMultiWorlds();

	for (uint32 Frame = StartFrame; Frame <= EndFrame; Frame++)
	{
		FRecallSimulationFrameInsight FrameInsight;
		FrameInsight.Frame = Frame;

		for (const UWorld* World : Worlds)
		{
			if (const URecallSimulationInsightSubsystem* InsightSystem = UWorld::GetSubsystem<URecallSimulationInsightSubsystem>(World))
			{
				FRecallSimulationFrameReport FrameReport;
				if (InsightSystem->FindFrameReport(Frame, FrameReport))
				{
					FrameInsight.Fields.Add(FrameReport);
				}
			}
		}

		if (FrameInsight.Fields.Num() == 0)
		{
			continue;
		}

		check(FrameInsight.Fields.Num() == Worlds.Num());

		Report.Frames.Add(FrameInsight);
	}

	return Report;
}

bool URecallSimulationInsightSubsystem::FindFrameReport(uint32 Frame, FRecallSimulationFrameReport& OutReport) const
{
	FScopeLock Lock(&DataGuard);
	
	for (int32 FrameIndex = 0; FrameIndex < LastFrames.Num(); FrameIndex++)
	{
		const FRecallSimulationFrameReport& FrameReport = LastFrames[FrameIndex];

		if (FrameReport.Frame == Frame)
		{
			OutReport = FrameReport;
			return true;
		}
	}

	return false;
}

FRecallSimulationFrameReport URecallSimulationInsightSubsystem::GenerateFrameReport(uint32 Frame) const
{
	QUICK_SCOPE_CYCLE_COUNTER(FRecallActorRepresentationDesc_Insight_GenerateFrameReport);

	FRecallSimulationFrameReport NewFrameReport;
	
	const UWorld* World = GetWorld();
	if (!ensureMsgf(IsValid(World), TEXT("Invalid world")))
	{
		return NewFrameReport;
	}

	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	
	NewFrameReport.Frame = Frame;

	if (RandomNumberSystem.IsValid())
	{
		NewFrameReport.Seed = RandomNumberSystem->GetCurrentSeed();
	}

	if (StateTreeSystem.IsValid())
	{
		NewFrameReport.StateTreeHash = StateTreeSystem->GetRandomStreamCombinedHash();
	}

	TArray<FMassEntityHandle> Entities = Recall::Entity::Utils::GetAllMutableEntities(GetWorld());
	
	// Sort entities for deterministic ordering across clients to ensure consistent hash calculation
	Entities.Sort([](const FMassEntityHandle& A, const FMassEntityHandle& B)
	{
		return A < B;
	});

	NewFrameReport.MutableEntityCount = Entities.Num();

	TArray<uint32> EntitiesHash;
	EntitiesHash.SetNum(Entities.Num());
	
	ParallelFor(Entities.Num(), [&EntityManager, &Entities, &EntitiesHash](int32 Index)
	{
		const FMassEntityHandle& Entity = Entities[Index];
		const FMassEntityView EntityView(EntityManager, Entity);

		uint32& EntityHash = EntitiesHash[Index] = GetTypeHash(Entity);
		
		if (const FRecallTransformFragment* FragmentDataPtr = EntityView.GetFragmentDataPtr<FRecallTransformFragment>())
		{
			EntityHash = HashCombine(EntityHash, GetTypeHash(FragmentDataPtr->Position));
			EntityHash = HashCombine(EntityHash, GetTypeHash(FragmentDataPtr->Rotation));
		}
	});

	for (const uint32 EntityHash : EntitiesHash)
	{
		NewFrameReport.MutableEntityHash = HashCombine(NewFrameReport.MutableEntityHash, EntityHash);	
	}

	NewFrameReport.SerialNumberGenerator = EntityManager.GetSerialNumberGenerator();

	return NewFrameReport;
}

TArray<const UWorld*> URecallSimulationInsightSubsystem::GetMultiWorlds() const
{
	TArray<const UWorld*> Worlds;
#ifdef WITH_MULTI_WORLD
	if (MultiWorldSystem.IsValid())
	{
		Worlds = MultiWorldSystem->GetNestedWorlds();
	}
#else // WITH_MULTI_WORLD
	Worlds.Add(GetWorld());
#endif // WITH_MULTI_WORLD
	return Worlds;
}
