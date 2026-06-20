// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Slowmo/RecallSlowMotionSubsystem.h"

#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#ifdef WITH_MULTI_WORLD
#include "System/MultiWorldSubsystem.h"
#include "Utility/MultiWorldUtils.h"
#endif // WITH_MULTI_WORLD

void URecallSlowMotionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
#ifdef WITH_MULTI_WORLD
	Collection.InitializeDependency<UMultiWorldSubsystem>();

	const UWorld* MainWorld = MultiWorld::Utils::GetMainWorld(this);
	if (MainWorld && MainWorld == GetWorld())
	{
		MainSlowMoData = MakeShared<FRecallSlowMotionData>();

		for (const UWorld* NestedWorld : GetMultiWorlds())
		{
			RegisterNestedWorld(NestedWorld);
		}

		if (UMultiWorldSubsystem* MultiWorldSystem = UWorld::GetSubsystem<UMultiWorldSubsystem>(MainWorld))
		{
			MultiWorldSystem->OnAddNestedWorld.AddUObject(this, &ThisClass::OnAddNestedWorld);
		}
	}
#else // WITH_MULTI_WORLD
	MainSlowMoData = MakeShared<FRecallSlowMotionData>();
	RegisterNestedWorld(GetWorld());
#endif // WITH_MULTI_WORLD
}

void URecallSlowMotionSubsystem::Deinitialize()
{
	Super::Deinitialize();

	MainSlowMoData.Reset();
	SlowMoCopyData.Reset();
}

void URecallSlowMotionSubsystem::Reset()
{
	if (MainSlowMoData.IsValid())
	{
		MainSlowMoData.Get()->Reset();
	}
}

void URecallSlowMotionSubsystem::Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallSlowMotionSubsystem::Save"));

	if (MainSlowMoData.IsValid())
	{
		OutSnapshot.InitializeAs<FRecallSlowMotionData>(*MainSlowMoData.Get());
	}
}

void URecallSlowMotionSubsystem::Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallSlowMotionSubsystem::Restore"));

	if (const FRecallSlowMotionData* InData = InSnapshot.GetPtr<FRecallSlowMotionData>())
	{
		if (MainSlowMoData.IsValid())
		{
			(*MainSlowMoData.Get()) = *InData;
		}
	}
}

void URecallSlowMotionSubsystem::OnAddNestedWorld(UWorld* World)
{
	RegisterNestedWorld(World);
}

void URecallSlowMotionSubsystem::RegisterNestedWorld(const UWorld* World)
{
	if (URecallSlowMotionSubsystem* SlowMotionSystem = UWorld::GetSubsystem<URecallSlowMotionSubsystem>(World))
	{
		SlowMotionSystem->SlowMoCopyData = MainSlowMoData;
	}
}

void URecallSlowMotionSubsystem::AddSlowMotionEvent(float TimeDilatation, int32 Duration)
{
	if (Duration <= 0)
	{
		return;
	}

	check(SlowMoCopyData.IsValid());

	FScopeLock Lock(&DataGuard);

	FRecallSlowMotionEvent& NewEvent = SlowMoCopyData.Pin()->Events.AddDefaulted_GetRef();
	NewEvent.StartFrame = SlowMoCopyData.Pin()->ElapsedDilatedFrames;
	NewEvent.Duration = static_cast<uint32>(Duration);
	NewEvent.TimeDilatation = TimeDilatation;
}

void URecallSlowMotionSubsystem::AddSlowMotionEvent(const TObjectPtr<class UCurveFloat>& TimeDilatationCurve, int32 Duration)
{
	if (Duration <= 0 || !TimeDilatationCurve)
	{
		return;
	}

	check(SlowMoCopyData.IsValid());

	FScopeLock Lock(&DataGuard);

	FRecallSlowMotionEvent& NewEvent = SlowMoCopyData.Pin()->Events.AddDefaulted_GetRef();
	NewEvent.StartFrame = SlowMoCopyData.Pin()->ElapsedDilatedFrames;
	NewEvent.Duration = static_cast<uint32>(Duration);
	NewEvent.TimeDilatationCurve = TimeDilatationCurve;
}

void URecallSlowMotionSubsystem::ClearSlowMotionEvents()
{
	if (SlowMoCopyData.IsValid())
	{
		FScopeLock Lock(&DataGuard);
		SlowMoCopyData.Pin()->Events.Reset();
	}
}

static float GetEventTimeDilatation(const FRecallSlowMotionEvent& Event, uint32 CurrentFrame)
{
	if (!Event.TimeDilatationCurve.IsNull())
	{
		if (const UCurveFloat* TimeDilatationCurve = Event.TimeDilatationCurve.LoadSynchronous())
		{
			check(Event.Duration > 0);
			check(CurrentFrame >= Event.StartFrame);
			const float Alpha = static_cast<float>(CurrentFrame - Event.StartFrame) / static_cast<float>(Event.Duration);
			const float NewTimeDilatation = TimeDilatationCurve->GetFloatValue(Alpha);

			return FMath::Max(0.0f, NewTimeDilatation);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("%hs Failed to find curve float at path: %s"), __FUNCTION__, *Event.TimeDilatationCurve.ToString());
		}
	}

	return FMath::Max(0.0f, Event.TimeDilatation);
}

static float GetAvgEventsTimeDilatation(const TArray<FRecallSlowMotionEvent>& Events, uint32 CurrentFrame)
{
	if (Events.Num() == 0)
	{
		return 1.0f;
	}

	float TimeDilatationSum = GetEventTimeDilatation(Events[0], CurrentFrame);
	uint8 TimeDilatationEventCount = 1;

	for (int32 EventIndex = 1; EventIndex < Events.Num(); EventIndex++)
	{
		const FRecallSlowMotionEvent& Event = Events[EventIndex];

		TimeDilatationSum += GetEventTimeDilatation(Event, CurrentFrame);
		TimeDilatationEventCount++;
	}

	return TimeDilatationSum / static_cast<float>(TimeDilatationEventCount);
}

static void RemoveTimeOutEvents(TArray<FRecallSlowMotionEvent>& Events, uint32 CurrentFrame)
{
	for (int32 EventIndex = Events.Num() - 1; EventIndex >= 0; EventIndex--)
	{
		const FRecallSlowMotionEvent& Event = Events[EventIndex];

		const double EndFrame = Event.StartFrame + static_cast<double>(Event.Duration);
		if (CurrentFrame >= EndFrame)
		{
			Events.RemoveAtSwap(EventIndex);
		}
	}
}

void URecallSlowMotionSubsystem::UpdateTimeDilatation()
{
	if (!MainSlowMoData.IsValid()) // Only update from the main field
	{
		return;
	}

	FScopeLock Lock(&DataGuard);

	const double CurrentFrame = static_cast<uint32>(MainSlowMoData->ElapsedDilatedFrames);

	MainSlowMoData->TimeDilatation = GetAvgEventsTimeDilatation(MainSlowMoData->Events, CurrentFrame);

	RemoveTimeOutEvents(MainSlowMoData->Events, CurrentFrame);

	if (MainSlowMoData->Events.Num() == 0) // Reset our internal timer when no event is active
	{
		MainSlowMoData->ElapsedDilatedFrames = 0.0;
	}
	else if (MainSlowMoData->TimeDilatation == 0.0f)
	{
		MainSlowMoData->ElapsedDilatedFrames += 1.0;
	}
	else
	{
		MainSlowMoData->ElapsedDilatedFrames += MainSlowMoData->TimeDilatation;
	}
}

float URecallSlowMotionSubsystem::GetTimeDilatation() const
{
	check(SlowMoCopyData.IsValid());
	FScopeLock Lock(&DataGuard);
	return SlowMoCopyData.Pin()->TimeDilatation;
}

TArray<const UWorld*> URecallSlowMotionSubsystem::GetMultiWorlds() const
{
	TArray<const UWorld*> Worlds;
#ifdef WITH_MULTI_WORLD
	if (UMultiWorldSubsystem* MultiWorldSystem = UWorld::GetSubsystem<UMultiWorldSubsystem>(GetWorld()))
	{
		Worlds = MultiWorldSystem->GetNestedWorlds();
	}
#else // WITH_MULTI_WORLD
	Worlds.Add(GetWorld());
#endif // WITH_MULTI_WORLD
	return Worlds;
}
