// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Simulation/Insight/RecallSimulationInsightTypes.h"

#include "Serialization/MemoryReader.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
#include "HAL/IConsoleManager.h"

static bool bDebugBreakOutOfSync = false;
static FAutoConsoleVariableRef CVarRecallBreakOutOfSync(
	TEXT("recall.online.BreakOutOfSync"),
	bDebugBreakOutOfSync,
	TEXT("Break Out Of Sync")
);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

DEFINE_LOG_CATEGORY(LogRecallSimInsight);

template<typename T>
FORCEINLINE void LogDiffNamed(const FString& Name, T A, T B)
{
	if (A != B)
	{
		UE_LOG(LogRecallSimInsight, Warning, TEXT("%s"), *FString::Format(TEXT("{0}:\t {1} != {2}"), { *Name, A, B }));

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (bDebugBreakOutOfSync)
		{
			UE_DEBUG_BREAK();
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}
}

template<>
FORCEINLINE void LogDiffNamed(const FString& Name, FName A, FName B)
{
	if (A != B)
	{
		UE_LOG(LogRecallSimInsight, Warning, TEXT("%s"), *FString::Format(TEXT("{0}:\t {1} != {2}"), { *Name, *A.ToString(), *B.ToString()}));

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (bDebugBreakOutOfSync)
		{
			UE_DEBUG_BREAK();
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}
}

template<>
FORCEINLINE void LogDiffNamed(const FString& Name, FString A, FString B)
{
	if (A != B)
	{
		UE_LOG(LogRecallSimInsight, Warning, TEXT("%s"), *FString::Format(TEXT("{0}:\t {1} != {2}"), { *Name, *A, *B }));

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (bDebugBreakOutOfSync)
		{
			UE_DEBUG_BREAK();
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}
}

template<typename T>
FORCEINLINE void LogDiffSize(const FString& Name, const TArray<T>& A, const TArray<T>& B)
{
	UE_LOG(LogRecallSimInsight, Warning, TEXT("%s"), *FString::Format(TEXT("{0} (Size):\t {1} != {2}"), { *Name, A.Num(), B.Num() }));

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (bDebugBreakOutOfSync)
	{
		UE_DEBUG_BREAK();
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

template<typename T>
FORCEINLINE void LogDiffNamed(const FString& Name, const TArray<T>& A, const TArray<T>& B)
{
	if (A != B)
	{
		if (A.Num() != B.Num())
		{
			LogDiffSize(Name, A, B);
		}
		else
		{
			for (int32 ItemIndex = 0; ItemIndex < A.Num(); ItemIndex++)
			{
				LogDiffNamed(Name, A[ItemIndex], B[ItemIndex]);
			}
		}
	}
}

#define LOG_DIFF(A, B)			LogDiffNamed(TEXT(#A), A, B);

void FRecallSimulationFrameReport::LogDiff(const FRecallSimulationFrameReport& Other) const
{
	LOG_DIFF(Frame, Other.Frame);
	LOG_DIFF(Seed, Other.Seed);
	LOG_DIFF(StateTreeHash, Other.StateTreeHash);
	LOG_DIFF(MutableEntityCount, Other.MutableEntityCount);
	LOG_DIFF(MutableEntityHash, Other.MutableEntityHash);
	LOG_DIFF(SerialNumberGenerator, Other.SerialNumberGenerator);
}

template<typename T>
static void SerializeNonNullParameter(T& Parameter, FArchive& Ar,
	UPackageMap* Map, bool& bOutSuccess)
{
	uint8 B = Parameter != 0;
	Ar.SerializeBits(&B, 1);
	if (B)
	{
		Ar << Parameter;
	}
	else
	{
		Parameter = 0;
	}
}

template<typename T>
static void SerializeNonNullArray(TArray<T>& Array, FArchive& Ar,
	UPackageMap* Map, bool& bOutSuccess)
{
	uint8 B = Array.Num();
	Ar.SerializeBits(&B, 1);
	if (B)
	{
		Ar << Array;
	}
	else
	{
		Array.Reset();
	}
}

bool FRecallSimulationFrameReport::NetSerialize(FArchive& Ar,
	UPackageMap* Map, bool& bOutSuccess)
{
	// Seed should never be null
	Ar << Seed;
	Ar << StateTreeHash;
	Ar << MutableEntityCount;
	Ar << MutableEntityHash;

	SerializeNonNullParameter(SerialNumberGenerator, Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

template<typename T>
static void SerializeReportParameterFromPrevious(const T& Old, T& New,
	FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	uint8 B = (Old != New);
	Ar.SerializeBits(&B, 1);
	if (B)
	{
		Ar << New;
	}
	else
	{
		New = Old;
	}
}

bool FRecallSimulationFrameReport::NetSerializeFromPrevious(const FRecallSimulationFrameReport& Previous,
	FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	SerializeReportParameterFromPrevious(Previous.Seed, Seed, Ar, Map, bOutSuccess);
	SerializeReportParameterFromPrevious(Previous.StateTreeHash, StateTreeHash, Ar, Map, bOutSuccess);
	SerializeReportParameterFromPrevious(Previous.MutableEntityCount, MutableEntityCount, Ar, Map, bOutSuccess);
	SerializeReportParameterFromPrevious(Previous.MutableEntityHash, MutableEntityHash, Ar, Map, bOutSuccess);
	SerializeReportParameterFromPrevious(Previous.SerialNumberGenerator, SerialNumberGenerator, Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

void FRecallSimulationFrameInsight::LogDiff(const FRecallSimulationFrameInsight& Other) const
{
	LOG_DIFF(Frame, Other.Frame);

	if (Fields.Num() == Other.Fields.Num())
	{
		for (int32 FieldIndex = 0; FieldIndex < Fields.Num(); FieldIndex++)
		{
			if (Fields[FieldIndex] != Other.Fields[FieldIndex])
			{
				Fields[FieldIndex].LogDiff(Other.Fields[FieldIndex]);
			}
		}
	}
	else
	{
		LogDiffSize(TEXT("Fields"), Fields, Other.Fields);
	}
}

void FRecallSimulationInsight::LogDiff(const FRecallSimulationInsight& Other, uint32 StartFrame) const
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	const TArray<FRecallSimulationFrameInsight> FramesFrom = GetFramesFrom(StartFrame);
	const TArray<FRecallSimulationFrameInsight> OtherFramesFrom = Other.GetFramesFrom(StartFrame);

	const int32 FrameCount = FMath::Min(FramesFrom.Num(), OtherFramesFrom.Num());

	for (int32 FrameIndex = 0; FrameIndex < FrameCount; FrameIndex++)
	{
		if (FramesFrom[FrameIndex] != OtherFramesFrom[FrameIndex])
		{
			const FString DebugString = FString::Printf(
				TEXT("Out of sync at frame: %d"), FramesFrom[FrameIndex].Frame);			
			UE_LOG(LogRecallSimInsight, Warning, TEXT("%s"), *DebugString);
			
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, DebugString);
			}
			
			FramesFrom[FrameIndex].LogDiff(OtherFramesFrom[FrameIndex]);
			break;
		}
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

uint32 FRecallSimulationInsight::GetLastFrame() const
{
	return Frames.Last().Frame;
}

bool FRecallSimulationInsight::FindSyncedFrame(const FRecallSimulationInsight& Other, uint32 StartFrame, uint32& OutSyncedFrame) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("FRecallSimulationInsight::FindSyncedFrame"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_SimulationInsight_FindSyncedFrame);
	
	const TArray<FRecallSimulationFrameInsight> FramesFrom = GetFramesFrom(StartFrame);
	const TArray<FRecallSimulationFrameInsight> OtherFramesFrom = Other.GetFramesFrom(StartFrame);

	if (FramesFrom.Num() == 0 || OtherFramesFrom.Num() == 0)
	{
		return false;
	}

	int32 FrameIndex = 0;

	if (FramesFrom[FrameIndex] != OtherFramesFrom[FrameIndex])
	{
		return false;
	}

	const int32 FrameCount = FMath::Min(FramesFrom.Num(), OtherFramesFrom.Num());

	do
	{
		OutSyncedFrame = FramesFrom[FrameIndex].Frame;
		FrameIndex++;
	} while (FrameIndex < FrameCount && FramesFrom[FrameIndex] == OtherFramesFrom[FrameIndex]);

	return true;
}

bool FRecallSimulationInsight::HasFrame(uint32 Frame) const
{
	return Frames.ContainsByPredicate([Frame](const FRecallSimulationFrameInsight& FrameComparator)
		{
			return FrameComparator.Frame == Frame;
		});
}

TArray<FRecallSimulationFrameInsight> FRecallSimulationInsight::GetFramesFrom(uint32 Frame) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("FRecallSimulationInsight::GetFramesFrom"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_SimulationInsight_GetFramesFrom);
	
	return Frames.FilterByPredicate([Frame](const FRecallSimulationFrameInsight& FrameComparator)
		{
			return FrameComparator.Frame >= Frame;
		}
	);
}

bool FRecallSimulationFrameInsight::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint8 ItemCount = Fields.Num();
	Ar << ItemCount;

	if (Ar.IsLoading())
	{
		Fields.SetNum(ItemCount);
	}

	for (FRecallSimulationFrameReport& FrameReport : Fields)
	{
		FrameReport.Frame = Frame;
		FrameReport.NetSerialize(Ar, Map, bOutSuccess);
	}

	bOutSuccess = true;
	return true;
}

bool FRecallSimulationFrameInsight::NetSerializeFromPrevious(const FRecallSimulationFrameInsight& Previous, FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	if (Ar.IsLoading())
	{
		// Field count is constant
		Fields.SetNum(Previous.Fields.Num());
	}

	for (int32 FieldIndex = 0; FieldIndex < Fields.Num(); FieldIndex++)
	{
		Fields[FieldIndex].Frame = Frame;
		Fields[FieldIndex].NetSerializeFromPrevious(Previous.Fields[FieldIndex], Ar, Map, bOutSuccess);
	}

	bOutSuccess = true;
	return true;
}

bool FRecallSimulationInsight::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint8 ItemCount = Frames.Num();
	Ar << ItemCount;

	if (Ar.IsLoading())
	{
		Frames.SetNum(ItemCount);
	}

	if (Frames.Num())
	{
		uint32 Frame = Frames[0].Frame;
		Ar << Frame;

		Frames[0].Frame = Frame;
		Frames[0].NetSerialize(Ar, Map, bOutSuccess);

		for (int32 Index = 1; Index < Frames.Num(); Index++)
		{
			const FRecallSimulationFrameInsight& PreviousFrameInsight = Frames[Index - 1];
			FRecallSimulationFrameInsight& NewFrameInsight = Frames[Index];

			uint32 NewFrame = Frame + static_cast<uint32>(Index);
			if (Ar.IsSaving())
			{
				checkf(NewFrame == NewFrameInsight.Frame, TEXT("Should be ordered by frame"));
			}
			else
			{
				NewFrameInsight.Frame = NewFrame;
			}

			NewFrameInsight.NetSerializeFromPrevious(PreviousFrameInsight, Ar, Map, bOutSuccess);
		}
	}

	bOutSuccess = true;
	return true;
}
