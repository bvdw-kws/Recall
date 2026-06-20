// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSignalSubsystem.h"

#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "RecallSignalTypes.h"
#include "VisualLogger/VisualLogger.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(RecallSignalsCounters, true);

void URecallSignalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void URecallSignalSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URecallSignalSubsystem::Reset()
{
	TimeSeconds = 0.0;
	DelayedSignals.Reset();

	// Reset all buffers
	for (int32 Idx = 0; Idx < SignalsBuffersCount; Idx++)
	{
		FRecallSignalsBuffer& SignalsBuffer = SignalsBuffers[Idx];
		SignalsBuffer.CurrentFrameBufferIndex = 0;

		for (int32 BufferIndex = 0; BufferIndex < RecallSignalsBuffersCount; BufferIndex++)
		{
			FRecallFrameReceivedSignals& FrameBuffer = SignalsBuffer.FrameReceivedSignals[BufferIndex];
			FrameBuffer.ReceivedSignalRanges.Reset();
			FrameBuffer.SignaledEntities.Reset();
		}
	}
}

void URecallSignalSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallSignalSubsystem::Save"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Signal_Save);

	OutSnapshot.InitializeAs<FRecallSignalSnapshot>();

	FRecallSignalSnapshot& Data = OutSnapshot.GetMutable<FRecallSignalSnapshot>();
	Data.TimeSeconds = TimeSeconds;
	Data.SetDelayedSignals(DelayedSignals);
	Data.SetSignalsBuffers(SignalsBuffers, SignalsBuffersCount);
}

void URecallSignalSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallSignalSubsystem::Restore"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_Signal_Restore);

	if (const FRecallSignalSnapshot* InData = InSnapshot.GetPtr<FRecallSignalSnapshot>())
	{
		TimeSeconds = InData->TimeSeconds;
		InData->GetDelayedSignals(DelayedSignals);
		InData->GetSignalsBuffers(SignalsBuffers, SignalsBuffersCount);
	}
}

void URecallSignalSubsystem::SignalEntity(FName SignalName, const FMassEntityHandle Entity)
{
	checkf(Entity.IsSet(), TEXT("Expecting a valid entity to signal"));
	SignalEntities(SignalName, MakeArrayView(&Entity, 1));
}

void URecallSignalSubsystem::SignalEntities(FName SignalName, TConstArrayView<FMassEntityHandle> Entities)
{
	checkf(Entities.Num() > 0, TEXT("Expecting entities to signal"));

	{
		FScopeLock Lock(&NamedSignalsGuard);
		const UE::RecallSignal::FSignalDelegate& SignalDelegate = GetSignalDelegateByName(SignalName);
		SignalDelegate.Broadcast(SignalName, Entities);
	}

#if CSV_PROFILER
	FCsvProfiler::RecordCustomStat(*SignalName.ToString(), CSV_CATEGORY_INDEX(RecallSignalsCounters), Entities.Num(), ECsvCustomStatOp::Accumulate);
#endif // CSV_PROFILER

	UE_CVLOG(Entities.Num() == 1, this, LogRecallSignals, Log, TEXT("Raising signal [%s] to entity [%s]"), *SignalName.ToString(), *Entities[0].DebugGetDescription());
	UE_CVLOG(Entities.Num() > 1, this, LogRecallSignals, Log, TEXT("Raising signal [%s] to %d entities"), *SignalName.ToString(), Entities.Num());
}

void URecallSignalSubsystem::DelaySignalEntity(FName SignalName, const FMassEntityHandle Entity, const float DelayInSeconds)
{
	checkf(Entity.IsSet(), TEXT("Expecting a valid entity to signal"));
	DelaySignalEntities(SignalName, MakeArrayView(&Entity, 1), DelayInSeconds);
}

void URecallSignalSubsystem::DelaySignalEntities(FName SignalName, TConstArrayView<FMassEntityHandle> Entities, const float DelayInSeconds)
{
	const float TargetTimestamp = TimeSeconds + static_cast<double>(DelayInSeconds);

	{
		FScopeLock Lock(&DelayedSignalsGuard);

		FRecallDelayedSignal* ExistingDelayedSignal = DelayedSignals.FindByPredicate([&SignalName, TargetTimestamp](const FRecallDelayedSignal& DelayedSignal)
			{
				return DelayedSignal.SignalName == SignalName
					&& DelayedSignal.TargetTimestamp == TargetTimestamp;
			}
		);

		if (ExistingDelayedSignal != nullptr)
		{
			ExistingDelayedSignal->Entities.Append(Entities);
		}
		else
		{
			FRecallDelayedSignal DelayedSignal;
			DelayedSignal.SignalName = SignalName;
			DelayedSignal.TargetTimestamp = TargetTimestamp;
			DelayedSignal.Entities = Entities;

			checkf(DelayedSignals.Num() < RecallDelayedSignalsCount, TEXT("Increase buffer size"));
			DelayedSignals.Emplace(DelayedSignal);
			DelayedSignals.Sort([](const FRecallDelayedSignal& lDelayedSignal, const FRecallDelayedSignal& rDelayedSignal)
				{
					const int32 Compare = lDelayedSignal.SignalName.Compare(rDelayedSignal.SignalName);
					if (Compare == 0)
					{
						return lDelayedSignal.TargetTimestamp < rDelayedSignal.TargetTimestamp;
					}
					return Compare < 0;
				}
			);
		}
	}

	UE_CVLOG(Entities.Num() == 1, this, LogRecallSignals, Log, TEXT("Delay signal [%s] to entity [%s] in %.2f"), *SignalName.ToString(), *Entities[0].DebugGetDescription(), DelayInSeconds);
	UE_CVLOG(Entities.Num() > 1,this, LogRecallSignals, Log, TEXT("Delay signal [%s] to %d entities in %.2f"), *SignalName.ToString(), Entities.Num(), DelayInSeconds);
}

void URecallSignalSubsystem::SignalEntityDeferred(FMassExecutionContext& Context, FName SignalName, const FMassEntityHandle Entity)
{
	checkf(Entity.IsSet(), TEXT("Expecting a valid entity to signal"));
	SignalEntitiesDeferred(Context, SignalName, MakeArrayView(&Entity, 1));
}

void URecallSignalSubsystem::SignalEntitiesDeferred(FMassExecutionContext& Context, FName SignalName, TConstArrayView<FMassEntityHandle> Entities)
{
	checkf(Entities.Num() > 0, TEXT("Expecting entities to signal"));
	Context.Defer().PushCommand<FMassDeferredSetCommand>([SignalName, InEntities = TArray<FMassEntityHandle>(Entities)](const FMassEntityManager& System)
	{
		URecallSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<URecallSignalSubsystem>(System.GetWorld());
		SignalSubsystem->SignalEntities(SignalName, InEntities);
	});

	UE_CVLOG(Entities.Num() == 1, this, LogRecallSignals, Log, TEXT("Raising deferred signal [%s] to entity [%s]"), *SignalName.ToString(), *Entities[0].DebugGetDescription());
	UE_CVLOG(Entities.Num() > 1, this, LogRecallSignals, Log, TEXT("Raising deferred signal [%s] to %d entities"), *SignalName.ToString(), Entities.Num());
}

void URecallSignalSubsystem::DelaySignalEntityDeferred(FMassExecutionContext& Context, FName SignalName, const FMassEntityHandle Entity, const float DelayInSeconds)
{
	checkf(Entity.IsSet(), TEXT("Expecting a valid entity to signal"));
	DelaySignalEntitiesDeferred(Context, SignalName, MakeArrayView(&Entity, 1), DelayInSeconds);
}

void URecallSignalSubsystem::DelaySignalEntitiesDeferred(FMassExecutionContext& Context, FName SignalName, TConstArrayView<FMassEntityHandle> Entities, const float DelayInSeconds)
{
	checkf(Entities.Num() > 0, TEXT("Expecting entities to signal"));
	Context.Defer().PushCommand<FMassDeferredSetCommand>([SignalName, InEntities = TArray<FMassEntityHandle>(Entities), DelayInSeconds](const FMassEntityManager& System)
	{
		URecallSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<URecallSignalSubsystem>(System.GetWorld());
		SignalSubsystem->DelaySignalEntities(SignalName, InEntities, DelayInSeconds);
	});

	UE_CVLOG(Entities.Num() == 1, this, LogRecallSignals, Log, TEXT("Delay deferred signal [%s] to entity [%s] in %.2f"), *SignalName.ToString(), *Entities[0].DebugGetDescription(), DelayInSeconds);
	UE_CVLOG(Entities.Num() > 1,this, LogRecallSignals, Log, TEXT("Delay deferred signal [%s] to %d entities in %.2f"), *SignalName.ToString(), Entities.Num(), DelayInSeconds);
}

void URecallSignalSubsystem::UpdateDelayedSignals(float DeltaTime)
{
	TimeSeconds += DeltaTime;

	FScopeLock Lock(&DelayedSignalsGuard);

	for (int32 SignalIndex = 0; SignalIndex < DelayedSignals.Num();)
	{
		const FRecallDelayedSignal& DelayedSignal = DelayedSignals[SignalIndex];

		if (DelayedSignal.TargetTimestamp <= TimeSeconds)
		{
			SignalEntities(DelayedSignal.SignalName, MakeArrayView(DelayedSignal.Entities));
			DelayedSignals.RemoveAt(SignalIndex, EAllowShrinking::No);
		}
		else
		{
			SignalIndex++;
		}
	}
}
