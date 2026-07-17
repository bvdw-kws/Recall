// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"

#include "RecallGamethreadQueue.generated.h"

RECALLSIMULATION_API DECLARE_LOG_CATEGORY_EXTERN(LogRecallGamethreadRunner, Log, All);

USTRUCT()
struct RECALLSIMULATION_API FRecallGamethreadRunnerData
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere)
	uint32 AsyncStartFrame = 0;
	
	UPROPERTY(VisibleAnywhere)
	uint32 AsyncEndFrame = 0;

	FORCEINLINE bool IsFinished(uint32 Frame) const
	{
		return Frame >= AsyncEndFrame;
	}

	bool operator==(const FRecallGamethreadRunnerData& Other) const
	{
		return AsyncStartFrame == Other.AsyncStartFrame
			&& AsyncEndFrame == Other.AsyncEndFrame;
	}

	bool operator!=(const FRecallGamethreadRunnerData& Other) const
	{
		return !operator==(Other);
	}
};

UCLASS(Within=RecallGamethreadQueue)
class RECALLSIMULATION_API URecallGamethreadRunnerTask : public UObject
{
	GENERATED_BODY()

public:
	void Run(const TSharedPtr<FRecallGamethreadRunnerData>& Data);
	void Stop(bool bFinish = true);

public:
	bool IsFinished() const { return bIsFinished; }
	const FRecallGamethreadRunnerData& GetRunnerData() const { check(RunnerData.IsValid()); return *RunnerData.Get(); }
	
	template<typename T>
	TWeakPtr<T> GetRunnerData() const { return StaticCastSharedPtr<T>(RunnerData); }

	template<typename T>
	const T& GetRunnerDataRef() const { check(RunnerData.IsValid()); return *StaticCastSharedPtr<T>(RunnerData); }
	
	virtual bool IsIdenticalData(const TSharedPtr<FRecallGamethreadRunnerData>& OtherData) const
	{
		unimplemented();
		return false;
	}

protected:
	virtual void OnRun() {}
	virtual void OnForceStop() {}
	virtual void OnForceFinish() {}

protected:
	void MarkAsFinished() { bIsFinished = true; }
	
private:
	UPROPERTY()
	bool bIsFinished = false;

	TSharedPtr<FRecallGamethreadRunnerData> RunnerData;
};

USTRUCT()
struct FRecallGamethreadRunner
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<URecallGamethreadRunnerTask> Task;

	// Last frame this runner was still requested. Refreshed every tick it's still in use, so the
	// grace period below counts down from when the asset was last needed, not from the fixed
	// AsyncEndFrame computed when the request first started (which can be far in the past by the
	// time the asset actually stops being requested).
	UPROPERTY()
	uint32 LastActiveFrame = 0;
};

/**
* Manager our queue of fake async tasks on the gamethread
*/
UCLASS()
class RECALLSIMULATION_API URecallGamethreadQueue : public UObject
{
	GENERATED_BODY()

public:
	URecallGamethreadQueue();

	/* Update our env query runners on the gamethread to catch-up with the new requests */
	void UpdateRunners(const TMap<uint32, TSharedPtr<FRecallGamethreadRunnerData>>& DataMap);
	void ReleaseAllRunners();

protected:
	UPROPERTY(Transient)
	TSubclassOf<URecallGamethreadRunnerTask> RunnerTaskClass;

	template<typename T=URecallGamethreadRunnerTask>
	T* GetRunnerTask(uint32 HandleId) const
	{
		const FRecallGamethreadRunner* RunnerResult = RunnerMap.Find(HandleId);
		if (!ensureAlwaysMsgf(RunnerResult != nullptr, TEXT("Runner should exist")))
		{
			return nullptr;
		}

		return Cast<T>(RunnerResult->Task);
	}
	
	uint32 GetCutoffFrame(uint32 Frame) const;

private:
	UPROPERTY(Transient)
	TMap<uint32, FRecallGamethreadRunner> RunnerMap;

	void ReleaseRunner_Internal(uint32 HandleId);
	FRecallGamethreadRunner& GetOrCreateRunner_Internal(uint32 HandleId, const TSharedPtr<FRecallGamethreadRunnerData>& Data);

	void CreateOrReleaseRunners_Internal(uint32 Frame, const TMap<uint32, TSharedPtr<FRecallGamethreadRunnerData>>& DataMap);
};
