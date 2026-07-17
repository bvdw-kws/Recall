// Van de Walel Bastien


#include "RecallGamethreadQueue.h"

#include "Utility/Simulation/RecallSimulationUtils.h"

DEFINE_LOG_CATEGORY(LogRecallGamethreadRunner);

//----------------------------------------------------------------------//
// URecallGamethreadRunnerTask
//----------------------------------------------------------------------//
void URecallGamethreadRunnerTask::Run(const TSharedPtr<FRecallGamethreadRunnerData>& Data)
{
	check(IsInGameThread());

	RunnerData = Data;

	OnRun();
}

void URecallGamethreadRunnerTask::Stop(bool bFinish /*= true*/)
{
	check(IsInGameThread());

	OnForceStop();

	if (bFinish && !IsFinished())
	{
		OnForceFinish();		
		MarkAsFinished();
	}
}

//----------------------------------------------------------------------//
// URecallGamethreadQueue
//----------------------------------------------------------------------//
URecallGamethreadQueue::URecallGamethreadQueue()
	: Super()
{
	RunnerTaskClass = URecallGamethreadRunnerTask::StaticClass();
}

auto URecallGamethreadQueue::UpdateRunners(
	const TMap<uint32, TSharedPtr<FRecallGamethreadRunnerData>>& DataMap) -> void
{
	check(IsInGameThread());

	const uint32 Frame = Recall::Simulation::Utils::GetFrame(this);
	const uint32 CutoffFrame = GetCutoffFrame(Frame);

	CreateOrReleaseRunners_Internal(Frame, DataMap);

	// Finish env queries that won't have time to end until the next step
	for (const TPair<uint32, TSharedPtr<FRecallGamethreadRunnerData>>& Data : DataMap)
	{
		check(Data.Value != nullptr);
		
		const uint32 EndFrame = Data.Value->AsyncEndFrame;
		if (EndFrame > CutoffFrame)
		{
			continue;
		}

		const FRecallGamethreadRunner& Runner = RunnerMap.FindChecked(Data.Key);
		if (!ensure(Runner.Task) || Runner.Task->IsFinished())
		{
			continue;
		}

		UE_LOG(LogRecallGamethreadRunner, Warning,
			TEXT("%hs %s did not finish in time (Cutoff: %d), consider increasing GamethreadDuration"),
			__FUNCTION__, *GetFName().ToString(), CutoffFrame - EndFrame);

		Runner.Task->Stop(true);
	}
}

void URecallGamethreadQueue::CreateOrReleaseRunners_Internal(uint32 Frame, const TMap<uint32, TSharedPtr<FRecallGamethreadRunnerData>>& DataMap)
{
	check(IsInGameThread());

	const uint32 MaxStepCount = Recall::Simulation::Utils::GetMaxStepCount(this);
	
	TArray<uint32> NewHandles;
	DataMap.GenerateKeyArray(NewHandles);

	TArray<uint32> OldHandles;
	RunnerMap.GenerateKeyArray(OldHandles);

	for (const uint32& OldHandle : OldHandles)
	{
		const TSharedPtr<FRecallGamethreadRunnerData> NewData = DataMap.FindRef(OldHandle);
		FRecallGamethreadRunner& OldRunner = RunnerMap[OldHandle];

		if (NewData.IsValid())
		{
			OldRunner.LastActiveFrame = Frame;
		}

		// Skip existing handles
		if (NewData.IsValid() && OldRunner.Task->IsIdenticalData(NewData))
		{
			NewHandles.Remove(OldHandle);
		}
		else
		{
			// Keep data for a while to be safe if a rollback occurs
			constexpr uint32 RunnerCacheDuration = 10;
			if (!NewData.IsValid() && OldRunner.LastActiveFrame + MaxStepCount + RunnerCacheDuration >= Frame)
			{
				continue;
			}

			ReleaseRunner_Internal(OldHandle);
		}
	}

	for (const uint32& NewHandle : NewHandles)
	{
		const TSharedPtr<FRecallGamethreadRunnerData>& NewData = DataMap.FindRef(NewHandle);
		check(NewData.IsValid());

		if (Frame < NewData->AsyncStartFrame)
		{
			continue;
		}

		FRecallGamethreadRunner& Runner = GetOrCreateRunner_Internal(NewHandle, NewData);
		Runner.LastActiveFrame = Frame;
	}
}

uint32 URecallGamethreadQueue::GetCutoffFrame(uint32 Frame) const
{
	const uint32 MaxStepCount = Recall::Simulation::Utils::GetMaxStepCount(this);
	const uint32 CutoffFrame = Frame + MaxStepCount + 1;
	return CutoffFrame;
}

void URecallGamethreadQueue::ReleaseRunner_Internal(uint32 HandleId)
{
	check(IsInGameThread());

	FRecallGamethreadRunner Runner;
	if (RunnerMap.RemoveAndCopyValue(HandleId, Runner) && Runner.Task)
	{
		Runner.Task->Stop(false);
	}
}

void URecallGamethreadQueue::ReleaseAllRunners()
{
	check(IsInGameThread());

	TArray<FRecallGamethreadRunner> Runners;
	RunnerMap.GenerateValueArray(Runners);

	for (FRecallGamethreadRunner& Runner : Runners)
	{
		if (Runner.Task)
		{
			Runner.Task->Stop(false);
		}
		Runner.Task = nullptr;
	}

	RunnerMap.Reset();
}

FRecallGamethreadRunner& URecallGamethreadQueue::GetOrCreateRunner_Internal(uint32 HandleId, const TSharedPtr<FRecallGamethreadRunnerData>& Data)
{
	check(IsInGameThread());

	FRecallGamethreadRunner* OldRunner = RunnerMap.Find(HandleId);
	if (OldRunner != nullptr)
	{
		return *OldRunner;
	}

	FRecallGamethreadRunner& Runner = RunnerMap.Add(HandleId);
	Runner.Task = NewObject<URecallGamethreadRunnerTask>(this, RunnerTaskClass);
	Runner.Task->Run(Data);

	return Runner;
}
