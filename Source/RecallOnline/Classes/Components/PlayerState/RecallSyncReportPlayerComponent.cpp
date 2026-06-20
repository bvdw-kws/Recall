// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSyncReportPlayerComponent.h"

#include "Components/GameState/RecallSyncReportGameComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RecallFrontendUtils.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/RecallPlayerState_InGame.h"
#include "Settings/RecallSimulationSettings.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"
#include "System/Simulation/Insight/RecallSimulationInsightInterface.h"
#include "System/Synchronization/RecallSynchronizationInterface.h"
#include "Utility/Simulation/RecallSimulationUtils.h"
#include "Utility/Synchronization/RecallSynchronizationUtils.h"

URecallSyncReportPlayerComponent::URecallSyncReportPlayerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void URecallSyncReportPlayerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(this);
	
	ARecallPlayerState_InGame* PlayerState = GetPlayerStateChecked<ARecallPlayerState_InGame>();
	if (!IsValid(PlayerState) || PlayerState->IsABot() || !PlayerState->HasLocalNetOwner())
	{
		return;
	}
	
	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->GetOnTickStartEvent().AddUObject(this, &ThisClass::OnSimTickStart);
	}
}

void URecallSyncReportPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->GetOnTickStartEvent().RemoveAll(this);
	}
}

void URecallSyncReportPlayerComponent::OnSimTickStart(float DeltaTime)
{
	// Share sim report to check if confirmed frames are synced
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	if (SimulationSettings->bOutOfSyncCheck)
	{
		ShareDebugSyncedFrameReport();
	}
}

void URecallSyncReportPlayerComponent::ShareDebugSyncedFrameReport()
{
	const ARecallPlayerState_InGame* PlayerState = GetPlayerStateChecked<ARecallPlayerState_InGame>();
	if (PlayerState->IsRestoring())
	{
		return;
	}
	
	const ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(InGameState))
	{
		return;
	}
	
	const uint32 Frame = Recall::Simulation::Utils::GetFrame(GetWorld());
	
	const IRecallSynchronizationInterface& Sync = Recall::Synchronization::Utils::GetSynchronizationRef(this);
	const uint32 LastSyncedFrame = Sync.GetLastSyncedFrame();

	if (Frame < LastSyncedFrame)
	{
		return;
	}

	const URecallSyncReportGameComponent* SyncReportGameComponent = InGameState->GetSyncReportComponentChecked();
	const uint32 DebugSyncedFrame = SyncReportGameComponent->GetDebugSyncedFrame();
	uint32 ReportStartFrame = DebugSyncedFrame + 1;
	uint32 ReportEndFrame = LastSyncedFrame;

	// Start sending frames past the last frame we sent.
	if (!LocalDebugSimReport.Frames.IsEmpty())
	{
		ReportStartFrame = FMath::Max(ReportStartFrame, LocalDebugSimReport.Frames.Last().Frame + 1);
	}

	// Limit how many frames can be sent at once.
	const URecallSimulationSettings* SimulationSettings = GetDefault<URecallSimulationSettings>();
	ReportEndFrame = FMath::Min(ReportStartFrame + SimulationSettings->OutOfSyncBunchSize, ReportEndFrame);

	// Share our simulation state.
	const IRecallSimulationInsightInterface& Insight = Recall::Frontend::Utils::GetRef<IRecallSimulationInsightInterface>(this);
	const FRecallSimulationInsight Report = Insight.GenerateReportInRange(ReportStartFrame, ReportEndFrame);

	if (Report.Frames.Num())
	{
		UE_LOG(LogPlayerController, Verbose, TEXT("%hs Send Insight from frame %d-%d (count: %d)"),
			__FUNCTION__, ReportStartFrame, ReportEndFrame, Report.Frames.Num());

		UpdateDebugSimulationReport(Report);
	}
}

void URecallSyncReportPlayerComponent::UpdateDebugSimulationReport(const FRecallSimulationInsight& Report)
{
	LocalDebugSimReport.Frames.Append(Report.Frames);

	if (const ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		const URecallSyncReportGameComponent* SyncReportGameComponent = InGameState->GetSyncReportComponentChecked();
		const uint32 DebugSyncedFrame = SyncReportGameComponent->GetDebugSyncedFrame();

		LocalDebugSimReport.Frames.RemoveAll([&](const FRecallSimulationFrameInsight& Frame)
		{
			return Frame.Frame <= DebugSyncedFrame;
		});
	}

	if (HasAuthority())
	{
		UpdateDebugSyncedFrame();
	}
	else if (!Report.IsEmpty())
	{
		Server_UpdateDebugSimulationReport(Report);
	}
}

void URecallSyncReportPlayerComponent::UpdateDebugSyncedFrame()
{
#if WITH_SERVER_CODE
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(TEXT("URecallSyncReportPlayerComponent::UpdateDebugSyncedFrame"));
	QUICK_SCOPE_CYCLE_COUNTER(Recall_SyncReportComponent_UpdateDebugSyncedFrame);
	
	const ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(
		UGameplayStatics::GetGameState(this));
	if (!IsValid(InGameState) || LocalDebugSimReport.IsEmpty())
	{
		return;
	}

	URecallSyncReportGameComponent* SyncReportGameComponent = InGameState->GetSyncReportComponentChecked();
	const uint32 PreviousSyncedFrame = SyncReportGameComponent->GetDebugSyncedFrame();
	const uint32 NextSyncedFrame = PreviousSyncedFrame + 1;

	if (!LocalDebugSimReport.HasFrame(NextSyncedFrame))
	{
		return;
	}

	ARecallPlayerState_InGame* OwnerPlayerState = GetPlayerStateChecked<ARecallPlayerState_InGame>();
	
	uint32 SyncedFrame = LocalDebugSimReport.GetLastFrame();

	for (APlayerState* PlayerState : InGameState->PlayerArray)
	{
		ARecallPlayerState_InGame* InGamePlayerState = Cast<ARecallPlayerState_InGame>(PlayerState);
		if (!IsValid(InGamePlayerState) || InGamePlayerState->IsInactive() || InGamePlayerState->IsABot() ||
			InGamePlayerState == OwnerPlayerState || InGamePlayerState->IsRestoring())
		{
			continue;
		}

		const URecallSyncReportPlayerComponent* SyncReportComponent = InGamePlayerState->GetSyncReportComponent();
		if (!IsValid(SyncReportComponent) || !SyncReportComponent->LocalDebugSimReport.HasFrame(NextSyncedFrame))
		{
			return;
		}

		uint32 NewSyncedFrame = 0;
		if (LocalDebugSimReport.FindSyncedFrame(SyncReportComponent->LocalDebugSimReport,
			NextSyncedFrame, NewSyncedFrame))
		{
			SyncedFrame = FMath::Min(SyncedFrame, NewSyncedFrame);
		}
		else
		{
			ARecallPlayerState_InGame* OutOfSyncPlayerState = InGamePlayerState->HasLocalNetOwner() ? OwnerPlayerState : InGamePlayerState;
			
			SyncReportGameComponent->SignalOutOfSyncPlayer(OutOfSyncPlayerState);
			
			SyncedFrame = PreviousSyncedFrame;

			// Not having any shared frames means that we ran out of sync.
			LocalDebugSimReport.LogDiff(SyncReportComponent->LocalDebugSimReport, NextSyncedFrame);
		}
	}

	if (PreviousSyncedFrame != SyncedFrame)
	{
		UE_LOG(LogPlayerController, VeryVerbose,
			TEXT("[%hs] Synced frame from %d to %d"), __FUNCTION__, PreviousSyncedFrame, SyncedFrame);

		SyncReportGameComponent->SetDebugSyncedFrame(SyncedFrame);
	}
#endif // WITH_SERVER_CODE
}

#pragma region RPC
void URecallSyncReportPlayerComponent::Server_UpdateDebugSimulationReport_Implementation(const FRecallSimulationInsight& Report)
{
#if WITH_SERVER_CODE
	UpdateDebugSimulationReport(Report);
#endif // WITH_SERVER_CODE
}
#pragma endregion RPC
