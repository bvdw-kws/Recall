// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallMultiSimStepThread.h"

#include "HAL/RunnableThread.h"
#include "System/Simulation/RecallMultiSimSubsystem.h"

FRecallMultiSimStepThread::FRecallMultiSimStepThread(const TWeakObjectPtr<URecallMultiSimSubsystem>& InMultiSimSystem)
{
	MultiSimSystem = InMultiSimSystem;
}

void FRecallMultiSimStepThread::StartTick()
{
	if (!bIsRunning)
	{
		bIsRunning = true;
		TickEvent = FGenericPlatformProcess::GetSynchEventFromPool();
		check(TickEvent);
		Thread.Reset(FRunnableThread::Create(this, TEXT("MultiSimThread"), 0, TPri_TimeCritical));
	}
}

bool FRecallMultiSimStepThread::Init()
{
	/* Should the thread start? */
	return MultiSimSystem.IsValid();
}

uint32 FRecallMultiSimStepThread::Run()
{
	FTimespan TickTimeSpan = FTimespan::FromMilliseconds(10.0);

	while (bIsRunning)
	{
		{
			TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("FStepThread::Run"));
			QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSystem_StepThread_Run);

			while (Step.Load() > 0)
			{
				if (MultiSimSystem.IsValid())
				{
					MultiSimSystem->StepSimulation();
				}
				Step = Step.Load() - 1;
			}
		}

		check(TickEvent);
		TickEvent->Wait(TickTimeSpan);
	}

	return 0;
}

void FRecallMultiSimStepThread::Exit()
{
	if (!bIsRunning)
	{
		return;
	}

	bIsRunning = false;

	if (TickEvent)
	{
		TickEvent->Trigger();
	}

	Thread->WaitForCompletion();

	if (TickEvent)
	{
		FGenericPlatformProcess::ReturnSynchEventToPool(TickEvent);
		TickEvent = nullptr;
	}

	MultiSimSystem.Reset();
}

void FRecallMultiSimStepThread::Rollback_Step(int32 Count)
{
	Step = FMath::Max(Step.Load(), Step.Load() + Count);
}

void FRecallMultiSimStepThread::Simulation_Step(int32 Count)
{
	check(Step.Load()  == 0);
	
	Step = FMath::Max(Step.Load(), Step.Load() + Count);

	if (TickEvent)
	{
		TickEvent->Trigger();
	}
}

void FRecallMultiSimStepThread::WaitForStep() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("FStepThread::WaitForStep"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_MultiSimSystem_StepThread_WaitForStep);
	
	FGenericPlatformProcess::ConditionalSleep([this]() { return IsStepDone(); });
}

bool FRecallMultiSimStepThread::IsStepDone() const
{
	return Step.Load() == 0;
}
