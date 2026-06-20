// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Replay/RecallReplaySubsystem.h"

#include "Async/Async.h"
#include "Blueprint/UserWidget.h"
#include "Components/GameState/RecallGameSimulationComponent.h"
#include "Components/GameState/RecallSyncReportGameComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RecallFrontendUtils.h"
#include "Online/RecallGameState_InGame.h"
#include "System/Simulation/RecallSimulationControllerInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/Replay/RecallReplayUtils.h"
#include "Utility/Simulation/RecallSimulationUtils.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
#include "Engine/GameInstance.h"
#include "System/Debug/DebugMenuSubsystem.h"

static FAutoConsoleCommandWithWorldAndArgs CVarRecallInstantReplay(
	TEXT("recall.replay.InstantReplay"),
	TEXT("Instantly replay the game until the current state"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (const UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (URecallReplaySubsystem* ReplaySystem = GameInstance->GetSubsystem<URecallReplaySubsystem>())
				{
					ReplaySystem->InstantReplay();
				}
			}
		}
	)
);

static bool bDebugReplayAutoSave = false;
static FAutoConsoleVariableRef CVarRecallDebugShowCharacterTrails(
	TEXT("recall.replay.AutoSave"),
	bDebugReplayAutoSave,
	TEXT("AutoSave")
);

static bool bDebugRandomInstantReplay = false;
static FAutoConsoleVariableRef CVarRecallRandomInstantReplay(
	TEXT("recall.replay.RandomInstantReplay"),
	bDebugRandomInstantReplay,
	TEXT("Random Instant Replay")
);

static float DebugRandomInstantReplayInterval = 5.0f;
static FAutoConsoleVariableRef CVarRecallRandomInstantReplayInterval(
	TEXT("recall.replay.RandomInstantReplayInterval"),
	DebugRandomInstantReplayInterval,
	TEXT("Random Instant Replay Interval")
);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

URecallReplaySubsystem::URecallReplaySubsystem()
	: Super()
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	static ConstructorHelpers::FClassFinder<UUserWidget> DebugReplayPlayerWidgetClassAsset(TEXT("/Recall/Replay/Widgets/WBP_ReplayPlayer"));
	DebugReplayPlayerWidgetClass = DebugReplayPlayerWidgetClassAsset.Class;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallReplaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FWorldDelegates::OnPreWorldInitialization.AddUObject(this, &ThisClass::OnPreWorldInitialization);
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::OnWorldCleanup);

	RegisterWorld(GetWorld());
}

void URecallReplaySubsystem::Deinitialize()
{
	Super::Deinitialize();

	SimulationController = nullptr;

	FWorldDelegates::OnPreWorldInitialization.RemoveAll(this);
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
}

void URecallReplaySubsystem::Tick(float DeltaTime)
{
	if (IsPlayReplay())
	{
		if (bReplayBegun)
		{
			CheckReplayShortcuts(UGameplayStatics::GetPlayerController(this, 0));

			if (!IsPausedReplay() || FastForwardFrames != 0)
			{
				bool bResume = true;
				TickReplay(GetPlayingReplayChecked(), bResume);

				if (bResume)
				{
					ResumeGame();
				}
			}
		}
		else if (bReplayWaitPC)
		{
			BeginReplay(GetPlayingReplayChecked());
		}
	}
	else
	{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		if (bDebugRandomInstantReplay)
		{
			InstantReplayWaitDuration += DeltaTime;

			if (InstantReplayWaitDuration >= DebugRandomInstantReplayInterval)
			{
				InstantReplayWaitDuration -= DebugRandomInstantReplayInterval;
				InstantReplay();
			}
		}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	}
}

TStatId URecallReplaySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URecallReplaySubsystem, STATGROUP_Tickables);
}

void URecallReplaySubsystem::CheckReplayShortcuts(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::SpaceBar))
	{
		if (IsPausedReplay())
		{
			ResumeReplay();
		}
		else
		{
			PauseReplay();
		}
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::Up))
	{
		SpeedUp();
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::Down))
	{
		SpeedDown();
	}

	if (PlayerController->WasInputKeyJustPressed(EKeys::Right))
	{
		StepReplay();
	}
}

void URecallReplaySubsystem::InstantReplay()
{
	FRecallReplay Replay;
	if (Recall::Replay::Utils::GenerateReplay(this, Replay, DebugMenu))
	{
		if (IsValid(SimulationController.GetObject()))
		{
			SimulationController->ResetSimulation();
		}

		InstantReplay(Replay);
	}
}

void URecallReplaySubsystem::InstantReplay(const FRecallReplay& Replay)
{
	Recall::Replay::Utils::InitReplay(this, Replay);

	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->StartSimulation(Replay.StartParams, false);
	}

	if (IsValid(SimulationController.GetObject()))
	{
		check(SimulationController->GetElapsedFrame() == 0);
		SimulationController->SetForceStepSim(Replay.FrameCount);
	}
}

void URecallReplaySubsystem::BeginReplay(const FRecallReplay& Replay)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(PlayerController))
	{
		bReplayWaitPC = true;
		return;
	}

	bReplayBegun = true;
	bReplayWaitPC = false;

	// Start the replay paused, but play the first frame.
	bPauseReplay = true;
	FastForwardFrames = 1;

	Recall::Replay::Utils::InitReplay(this, Replay);

	if (IsValid(SimulationController.GetObject()))
	{
		SimulationController->StartSimulation(Replay.StartParams, false);
	}

	if (ARecallGameState_InGame* InGameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this)))
	{
		URecallSyncReportGameComponent* SyncReportGameComponent = InGameState->GetSyncReportComponentChecked();	
		SyncReportGameComponent->SetDebugSyncedFrame(Replay.FrameCount);
	}

	PlayerController->bShowMouseCursor = true;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	SetDebugReplayPlayer(true);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallReplaySubsystem::TickReplay(const FRecallReplay& Replay, bool& bOutResume)
{
	bOutResume = true;

	if (IsValid(SimulationController.GetObject()))
	{
		const uint32 ElapsedFrame = SimulationController->GetElapsedFrame();
		const int32 FrameCount = IsPausedReplay() ? FastForwardFrames : GetReplaySpeed() + FastForwardFrames;

		if (Replay.FrameCount > ElapsedFrame && ensure(FrameCount >= 0))
		{
			const uint32 StepCount = FMath::Min(Replay.FrameCount - ElapsedFrame, static_cast<uint32>(FrameCount));

			SimulationController->SetForceStepSim(StepCount);

			bOutResume = false;
		}

		FastForwardFrames = 0;
	}
}

void URecallReplaySubsystem::ResumeGame()
{
	CurrentReplay.Reset();
	bReplayBegun = false;
	bReplayWaitPC = false;

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->bShowMouseCursor = false;
	}

	ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	URecallGameSimulationComponent* GameSimulationComponent = IsValid(GameState) ? GameState->GetGameSimulationComponentChecked() : nullptr;
	if (IsValid(GameSimulationComponent))
	{
		GameSimulationComponent->ResumeSimulation();
	}
	
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	SetDebugReplayPlayer(false);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallReplaySubsystem::SaveCacheReplay()
{
	FRecallReplay ReplayData;
	if (Recall::Replay::Utils::GenerateReplay(GetWorld(), ReplayData, DebugMenu))
	{
		Recall::Replay::Utils::SaveReplay(ReplayData, CacheReplayMemory);
	}
}

void URecallReplaySubsystem::LoadCacheReplay()
{
	if (CacheReplayMemory.Num())
	{
		FRecallReplay ReplayData;
		Recall::Replay::Utils::LoadReplay(CacheReplayMemory, ReplayData);
		LoadReplay(ReplayData);
	}
}

void URecallReplaySubsystem::LoadReplay(const FRecallReplay& Replay)
{
	ARecallGameState_InGame* GameState = Cast<ARecallGameState_InGame>(UGameplayStatics::GetGameState(this));
	if (!IsValid(GameState) || !GameState->CanLoadReplay())
	{
		return;
	}

	bReplayBegun = false;
	bReplayWaitPC = false;
	CurrentReplay = MakeShared<FRecallReplay>(Replay);
	
	URecallGameSimulationComponent* GameSimulationComponent = GameState->GetGameSimulationComponentChecked();

	// Reset simulation through our Player Controller, so we can reset our Camera as well.
	if (IsValid(GameSimulationComponent))
	{
		GameSimulationComponent->PauseSimulation();
	}

	if (Recall::Replay::Utils::GetMapName(this) == GetPlayingReplayChecked().MapName)
	{
		if (IsValid(GameSimulationComponent))
		{
			GameSimulationComponent->ResetSimulation(false, false);
		}

		BeginReplay(GetPlayingReplayChecked());
	}
	else
	{
		bOpenLevel = true;
		UGameplayStatics::OpenLevel(this, *GetPlayingReplayChecked().MapName);
	}
}

void URecallReplaySubsystem::LoadReplay(const FString& ReplayName)
{
	FRecallReplay Replay;
	if (Recall::Replay::Utils::LoadReplayFile(ReplayName, Replay))
	{
		LoadReplay(Replay);
	}
}

void URecallReplaySubsystem::OnPreWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS)
{
	RegisterWorld(World);
}

void URecallReplaySubsystem::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World && World == GetWorld())
	{
		if (SimulationController)
		{
			SimulationController->GetOnFrameStartEvent().RemoveAll(this);
			SimulationController = nullptr;
		}
	}
}

void URecallReplaySubsystem::RegisterWorld(UWorld* World)
{
	if (!IsValid(World) || World != GetWorld() || SimulationController)
	{
		return;
	}

	SimulationController = Recall::Frontend::Utils::Get<IRecallSimulationControllerInterface>(this);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (SimulationController)
	{
		SimulationController->GetOnFrameStartEvent().AddUObject(this, &ThisClass::OnFrameStart);
	}

	const FString MapName = Recall::Replay::Utils::GetMapName(this);
	AutoSaveReplayName = FString::Printf(TEXT("%s-AutoSave"), *MapName);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

	if (bOpenLevel && IsPlayReplay() &&
		Recall::Replay::Utils::GetMapName(this) == GetPlayingReplayChecked().MapName)
	{
		BeginReplay(GetPlayingReplayChecked());
	}
	else
	{
		CurrentReplay.Reset();
		bReplayBegun = false;
		bReplayWaitPC = false;
	}

	bOpenLevel = false;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (DebugReplayPlayerWidgetClass)
	{
		DebugReplayPlayerWidget = CreateWidget(GetWorld(), DebugReplayPlayerWidgetClass);
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
void URecallReplaySubsystem::SetDebugReplayPlayer(bool bShow)
{
	if (DebugReplayPlayerWidget)
	{
		if (!bShow && DebugReplayPlayerWidget->IsInViewport())
		{
			DebugReplayPlayerWidget->RemoveFromParent();
		}
		else if (bShow && !DebugReplayPlayerWidget->IsInViewport())
		{
			DebugReplayPlayerWidget->AddToViewport();
		}
	}
}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

void URecallReplaySubsystem::SaveReplay(const FString& ReplayName)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	auto GenerateCurrentReplay = [&]()
	{
		if (IsPlayReplay())
		{
			return GetPlayingReplayChecked();
		}
		else
		{
			FRecallReplay RecordReplay;
			Recall::Replay::Utils::GenerateReplay(this, RecordReplay, DebugMenu);
			return RecordReplay;
		}
	};

	const FRecallReplay RecordReplay = GenerateCurrentReplay();

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [RecordReplay, ReplayName]()
		{
			Recall::Replay::Utils::SaveReplayFile(RecordReplay, ReplayName);
		}
	);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void URecallReplaySubsystem::OnFrameStart(uint32 Frame)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (IsPlayReplay())
	{
		if (Frame == 0) // Start
		{
			DebugMenu = GetPlayingReplayChecked().DebugMenu;
		}
	}
	else
	{
		if (Frame == 0) // Start
		{
			if (const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this))
			{
				if (const UDebugMenuSubsystem* DebugMenuSystem = GameInstance->GetSubsystem<UDebugMenuSubsystem>())
				{
					DebugMenuSystem->SaveDebugMenu(DebugMenu);
				}
			}
		}

		if (bDebugReplayAutoSave)
		{
			QUICK_SCOPE_CYCLE_COUNTER(Recall_Replay_AutoSave);
			SaveReplay(AutoSaveReplayName);
		}
	}
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

const FRecallReplay& URecallReplaySubsystem::GetPlayingReplayChecked() const
{
	check(CurrentReplay.IsValid());
	return *CurrentReplay.Get();
}

constexpr int32 ReplaySpeedList[] = { 1, 2,	3, 4, 5, 10 };
constexpr int32 ReplaySpeedCount = sizeof(ReplaySpeedList) / sizeof(int32);

void URecallReplaySubsystem::SpeedDown() 
{ 
	ReplaySpeedIndex = FMath::Max(0, ReplaySpeedIndex - 1);
}

void URecallReplaySubsystem::SpeedUp() 
{ 
	ReplaySpeedIndex = FMath::Min(ReplaySpeedIndex + 1, ReplaySpeedCount - 1);
}

int32 URecallReplaySubsystem::GetReplaySpeed() const
{ 
	return ReplaySpeedList[ReplaySpeedIndex];
}

double URecallReplaySubsystem::GetReplayElapsedTimeSeconds() const
{
	if (SimulationController)
	{
		return SimulationController->GetElapsedTime();
	}

	return 0.0;
}

double URecallReplaySubsystem::GetReplayDurationSeconds() const
{
	if (IsPlayReplay())
	{
		return GetPlayingReplayChecked().Duration;
	}
	return 0.0f;
}

int32 URecallReplaySubsystem::GetReplayElapsedFrame() const
{
	if (SimulationController)
	{
		return SimulationController->GetElapsedFrame();
	}
	return 0;
}

int32 URecallReplaySubsystem::GetReplayDurationFrame() const
{
	if (IsPlayReplay())
	{
		return GetPlayingReplayChecked().FrameCount;
	}
	return 0;
}

float URecallReplaySubsystem::GetReplayPercent() const
{
	const float ReplayDurationSeconds = GetReplayDurationSeconds();

	if (ReplayDurationSeconds > 0)
	{
		return GetReplayElapsedTimeSeconds() / ReplayDurationSeconds;
	}

	return 0.0f;
}

void URecallReplaySubsystem::RestartReplay()
{
	if (IsPlayReplay())
	{
		LoadReplay(GetPlayingReplayChecked());
	}
}

void URecallReplaySubsystem::RestoreReplay()
{
	const uint32 Frame = Recall::Simulation::Utils::GetFrame(GetWorld());
	Recall::Replay::Utils::RestoreReplay(this, Frame);

	ResumeGame();
}
