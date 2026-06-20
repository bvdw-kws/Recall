// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallClientRestoreComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/Controller/RecallInputControllerComponent.h"
#include "DataTransfer/Subsystems/EasyDataTransferSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "RecallFrontendUtils.h"
#include "RecallGameSimulationComponent.h"
#include "RecallSyncInputGameComponent.h"
#include "Online/RecallGameState_InGame.h"
#include "Online/Base/RecallPlayerControllerBase.h"
#include "System/Restore/RecallRestoreTypes.h"
#include "System/Restore/RecallRestoreSettings.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"
#include "Utility/Restore/RecallRestoreUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

void URecallClientRestoreComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Subscribe to EasyDataTransfer events
	UEasyDataTransferSubsystem* DataTransferSubsystem = GetDataTransferSubsystem();
	if (DataTransferSubsystem)
	{
		// Bind to data received events for combined transfers
		DataTransferSubsystem->OnDataReceived.AddUObject(this, &URecallClientRestoreComponent::OnCombinedRestoreDataReceived);
		DataTransferSubsystem->OnDataProgress.AddUObject(this, &URecallClientRestoreComponent::OnSnapshotTransferProgressUpdate);
		DataTransferSubsystem->OnDataError.AddUObject(this, &URecallClientRestoreComponent::OnSnapshotTransferError);
		
		UE_LOG(LogRecallRestore, Log, TEXT("%hs: Subscribed to EasyDataTransfer events"), __FUNCTION__);
	}
}

void URecallClientRestoreComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unsubscribe from EasyDataTransfer events before cleanup
	UEasyDataTransferSubsystem* DataTransferSubsystem = GetDataTransferSubsystem();
	if (DataTransferSubsystem)
	{
		DataTransferSubsystem->OnDataReceived.RemoveAll(this);
		DataTransferSubsystem->OnDataProgress.RemoveAll(this);
		DataTransferSubsystem->OnDataError.RemoveAll(this);
		
		UE_LOG(LogRecallRestore, Log, TEXT("%hs: Unsubscribed from EasyDataTransfer events"), __FUNCTION__);
	}
	
	Super::EndPlay(EndPlayReason);
	
	if (RestoringWorld.IsValid())
	{
		StopRestoreGameSimulation(true);
	}
}

bool URecallClientRestoreComponent::HasStarted() const
{
	return RestoreInfo.IsValid() && RestoreInfo->bStarted;
}

bool URecallClientRestoreComponent::IsRestoringGameSimulation() const
{
	return RestoringWorld.IsValid();
}

uint32 URecallClientRestoreComponent::GetTargetFrame() const
{
	if (RestoreInfo.IsValid() && RestoreInfo->InputTargetFrame != 0)
	{
		return RestoreInfo->InputTargetFrame;
	}
	else
	{
		return 0;
	}
}

uint32 URecallClientRestoreComponent::ValidateTargetFrame(uint32 CurrentFrame, uint32 TargetFrame) const
{
	return FMath::Min(FMath::Min(CurrentFrame, TargetFrame), CurrentFrame + GetRestoreSpeed());
}

void URecallClientRestoreComponent::InitGameSimulation(bool bSeed)
{
	Recall::Restore::Utils::InitGameSimulation(this, bSeed);
}

void URecallClientRestoreComponent::RestoreGameSimulation(const FRecallSimulationStartParams& StartParams,
	bool bStart /*= true*/)
{
	if (IsRestoringGameSimulation())
	{
		UE_LOG(LogRecallRestore, Warning, TEXT("%hs Already restoring"), __FUNCTION__);
		return;
	}

	UE_LOG(LogRecallRestore, Log, TEXT("%hs Start Restoring"), __FUNCTION__);

	RestoringWorld = GetWorld();
	if (!ensureAlwaysMsgf(RestoringWorld.IsValid(),
			TEXT("%hs Invalid world"), __FUNCTION__))
	{
		return;
	}

	RestoreInfo = MakeShared<FRecallRestoreInfo>();

	SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(RestoringWorld.Get());
	checkf(SimulationController, TEXT("%hs Invalid simulation controller"), __FUNCTION__);

	if (bStart)
	{
		StartGameSimulation(StartParams);
	}
}

void URecallClientRestoreComponent::StartGameSimulation(const FRecallSimulationStartParams& StartParams)
{
	checkf(IsRestoringGameSimulation(), TEXT("Call RestoreGameSimulation first"));
	check(RestoreInfo.IsValid() && !RestoreInfo->bStarted);
	checkf(Recall::Simulation::Utils::GetFrame(GetWorld()) == 0, TEXT("Make sure to reset game sim"));

	RestoreInfo->bStarted = true;

	SimulationController->StartSimulation(StartParams);
	SimulationController->PauseSimulation();
	SimulationController->SetRenderSimulation(false);
	SimulationController->GetOnTickStartEvent().AddUObject(this, &ThisClass::OnTickSimulation);

	const URecallRestoreSettings* RestoreSettings = GetDefault<URecallRestoreSettings>();
	if (RestoreSettings->RestoreWidgetClass)
	{
		RestoreWidget = CreateWidget<UUserWidget>(GetWorld(), RestoreSettings->RestoreWidgetClass);
		if (RestoreWidget)
		{
			RestoreWidget->AddToViewport();
		}
	}
}

void URecallClientRestoreComponent::StopRestoreGameSimulation(bool bCleanup /*= false*/)
{
	UE_LOG(LogRecallRestore, Log, TEXT("%hs Stop Restoring"), __FUNCTION__);

	if (RestoringWorld.IsValid())
	{
		if (SimulationController)
		{
			if (!bCleanup)
			{
				SimulationController->SetRenderSimulation(true);
			}

			SimulationController->GetOnTickStartEvent().RemoveAll(this);
		}

		RestoringWorld.Reset();
		RestoreInfo.Reset();
		SimulationController = nullptr;

		if (RestoreWidget)
		{
			RestoreWidget->RemoveFromParent();
			RestoreWidget = nullptr;
		}
	}
}

void URecallClientRestoreComponent::OnTickSimulation(float DeltaTime)
{
	if (!IsRestoringGameSimulation() || !ensure(HasStarted()))
	{
		return;
	}

	const ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(GameState))
	{
		return;
	}
	
	const URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();	
	const uint32 CurrentFrame = GameSimulationComponent->GetLocalSimulationFrame();
	const uint32 TargetFrame = GetTargetFrame();
	
	checkf(CurrentFrame <= TargetFrame,
		TEXT("%hs Should not get ahead of our target frame"), __FUNCTION__);

	const uint32 FrameLeft = TargetFrame - CurrentFrame;	
	if (FrameLeft == 0 && CanFinishRestore())
	{
		const URecallSyncInputGameComponent* SyncInputComponent = GameState->GetSyncInputComponentChecked();
		UE_LOG(LogRecallRestore, Log,
			TEXT("%hs Finish restoring at Frame %d, ConfirmFrame: %d, ServerSharedConfirmFrame: %d"), __FUNCTION__,
			CurrentFrame, SyncInputComponent->GetLocalConfirmFrame(), SyncInputComponent->GetServerSharedConfirmFrame());

		FinishRestore();
	}
	else
	{
		const uint32 FoundTargetFrame = ValidateTargetFrame(CurrentFrame, TargetFrame);
		check(CurrentFrame <= FoundTargetFrame);

		const uint32 StepFrameCount = FoundTargetFrame - CurrentFrame;
		
		UE_LOG(LogRecallRestore, Verbose,
			TEXT("%hs Step %d frames (CurrentFrame: %d, TargetFrame: %d, FoundTargetFrame: %d)"),
			__FUNCTION__, StepFrameCount, CurrentFrame, TargetFrame, FoundTargetFrame);

		if (StepFrameCount > 0)
		{
			SendIdleInput(FoundTargetFrame);
			
			UE_LOG(LogRecallRestore, Log,
				TEXT("%hs Restoring (CurrentFrame: %d, InputTargetFrame: %d, ValidateTargetFrame: %d, StepCount: %d)"),
				__FUNCTION__, CurrentFrame, TargetFrame, FoundTargetFrame, StepFrameCount);

			if (SimulationController)
			{
				SimulationController->SetForceStepSim(StepFrameCount);
			}
		}
		else
		{
			UE_LOG(LogRecallRestore, Verbose,
				TEXT("%hs Waiting for the next input bunch (CurrentFrame: %d)"), __FUNCTION__, CurrentFrame);
		}
	}
}

void URecallClientRestoreComponent::SendIdleInput(uint32 Frame) const
{
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			ARecallPlayerControllerBase* PC = Cast<ARecallPlayerControllerBase>(Iterator->Get());
			if (!IsValid(PC) || !PC->IsLocalPlayerController())
			{
				continue;
			}

			PC->GetGameSimulationInputComponentChecked()->SendIdleInput(Frame);
		}
	}
}

bool URecallClientRestoreComponent::CanFinishRestore() const
{
	if (!RestoreInfo.IsValid() || !RestoreInfo->bSynced)
	{
		return false;
	}

	return true;
}

void URecallClientRestoreComponent::FinishRestore()
{
	StopRestoreGameSimulation();
	
	if (ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();
		GameSimulationComponent->ResumeSimulation();
	}
}

int32 URecallClientRestoreComponent::GetRestoreSpeed() const
{
	// Cap our restore speed to how fast our simulation can be stepped
	const int32 MaxSpeedCount = Recall::Simulation::Utils::GetMaxStepCount(this);
	const URecallRestoreSettings* RestoreSettings = GetDefault<URecallRestoreSettings>();
	return FMath::Clamp(RestoreSettings->RestoreSpeed, 1, MaxSpeedCount);
}

float URecallClientRestoreComponent::GetPercent() const
{
	// TODO: Data transfer progress
	
	return 0.0f;
}

void URecallClientRestoreComponent::OnSnapshotTransferProgressUpdate(int32 Handle, const FString& ChannelName, float Progress)
{
	UE_LOG(LogRecallRestore, Verbose, TEXT("%hs: EasyDataTransfer progress update - Handle: %d, Channel: %s, Progress: %.1f%%"), 
	       __FUNCTION__, Handle, *ChannelName, Progress * 100.0f);
	
	if (ChannelName != TEXT("RecallCombinedRestore"))
	{
		UE_LOG(LogRecallRestore, Verbose, TEXT("%hs: Ignoring progress for non-restore channel: %s"), __FUNCTION__, *ChannelName);
		return;
	}
	
	// Update UI or progress indicators
	UE_LOG(LogRecallRestore, Verbose, TEXT("%hs: Combined restore transfer progress: %.1f%%"), __FUNCTION__, Progress * 100.0f);
}

void URecallClientRestoreComponent::OnSnapshotTransferError(int32 Handle, const FString& ChannelName, EDataTransferError Error)
{
	UE_LOG(LogRecallRestore, Error, TEXT("%hs: EasyDataTransfer error - Handle: %d, Channel: %s, Error: %d"), 
	       __FUNCTION__, Handle, *ChannelName, static_cast<int32>(Error));
	
	if (ChannelName != TEXT("RecallCombinedRestore"))
	{
		UE_LOG(LogRecallRestore, Verbose, TEXT("%hs: Ignoring error for non-restore channel: %s"), __FUNCTION__, *ChannelName);
		return;
	}
	
	UE_LOG(LogRecallRestore, Error, TEXT("%hs: Combined restore transfer error: Handle=%d, Error=%d"), __FUNCTION__, Handle, static_cast<int32>(Error));
}

UEasyDataTransferSubsystem* URecallClientRestoreComponent::GetDataTransferSubsystem() const
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UEasyDataTransferSubsystem>();
		}
	}
	return nullptr;
}

void URecallClientRestoreComponent::OnCombinedRestoreDataReceived(int32 Handle, const FString& ChannelName, const TArray<uint8>& Data)
{
	UE_LOG(LogRecallRestore, Log, TEXT("%hs: Received EasyDataTransfer data - Handle: %d, Channel: %s, Size: %d bytes"), 
	       __FUNCTION__, Handle, *ChannelName, Data.Num());
	
	if (ChannelName != TEXT("RecallCombinedRestore"))
	{
		UE_LOG(LogRecallRestore, Verbose, TEXT("%hs: Ignoring non-combined-restore channel: %s"), __FUNCTION__, *ChannelName);
		return; // Not a combined restore transfer
	}
	
	if (!IsRestoringGameSimulation())
	{
		UE_LOG(LogRecallRestore, Warning, TEXT("%hs: Received combined restore data but not restoring"), __FUNCTION__);
		return;
	}
	
	ProcessCombinedRestoreData(Data);
}

void URecallClientRestoreComponent::ProcessCombinedRestoreData(const TArray<uint8>& Data)
{
	if (!RestoreInfo.IsValid())
	{
		UE_LOG(LogRecallRestore, Error, TEXT("%hs: RestoreInfo is not valid"), __FUNCTION__);
		return;
	}
	
	uint32 TargetFrame = 0;
	if (Recall::Restore::Utils::ProcessCombinedRestoreData(this, Data, TargetFrame))
	{
		RestoreInfo->InputTargetFrame = TargetFrame;
		RestoreInfo->bSynced = true;
	}
}
