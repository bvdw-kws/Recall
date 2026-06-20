// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "System/Snapshot/RecallMultiSimSnapshotTypes.h"
#include "System/Synchronization/RecallSynchronizationTypes.h"

RECALLONLINE_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallRollback, Log, All);

class RECALLONLINE_API FRecallRollbackFrame
{
public:
	FRecallSynchronizationFrameComparator Comparator;
	bool bValidSnapshot = false;
	TSharedPtr<FRecallMultiSimSnapshot> Snapshot;
};

#define RECALL_ROLLBACK_FRAME_BUFFER 32

/**
* Frame buffer that keeps using the same memory
*/
struct FRecallRollbackFrameBuffer
{
public:
	FRecallRollbackFrameBuffer()
	{
		Values.SetNum(RECALL_ROLLBACK_FRAME_BUFFER);
		for (int32 Index = 0; Index < RECALL_ROLLBACK_FRAME_BUFFER; Index++)
		{
			Values[Index].Snapshot = MakeShared<FRecallMultiSimSnapshot>();
		}
	}

	FORCEINLINE int32 Num() const { return Size; }

	FORCEINLINE bool IsEmpty() const { return Size == 0; }

	FORCEINLINE FRecallRollbackFrame& Last()
	{
		return (*this)[Num() - 1];
	}

	FORCEINLINE void Reset()
	{
		FrontIndex = 0;
		Size = 0;
	}

	FORCEINLINE FRecallRollbackFrame& Add_GetRef()
	{
		check(Size < RECALL_ROLLBACK_FRAME_BUFFER);
		Size++;
		return Last();
	}

	FORCEINLINE void PopFront(int32 PopCount = 1)
	{
		checkf(Size >= PopCount, TEXT("%hs Try to pop %d frames but size is %d"), __FUNCTION__, PopCount, Size);
		FrontIndex = (FrontIndex + PopCount) % RECALL_ROLLBACK_FRAME_BUFFER;
		Size -= PopCount;
	}

	FORCEINLINE void Pop(int32 PopCount = 1)
	{
		check(Size >= PopCount);
		Size -= PopCount;
	}

	FRecallRollbackFrame& operator[](int32 Index)
	{
		checkf(Index < Size, TEXT("Invalid index"))
		const int32 RangeIndex = GetRangeIndex(Index);
		return Values[RangeIndex];
	}

	const FRecallRollbackFrame& operator[](int32 Index) const
	{
		return const_cast<FRecallRollbackFrameBuffer&>(*this)[Index];
	}

private:
	TArray<FRecallRollbackFrame, TFixedAllocator<RECALL_ROLLBACK_FRAME_BUFFER>> Values;
	int32 FrontIndex = 0;
	int32 Size = 0;

	FORCEINLINE int32 GetRangeIndex(int32 Index) const
	{
		return (FrontIndex + Index) % RECALL_ROLLBACK_FRAME_BUFFER;
	}
};
