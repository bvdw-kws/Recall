#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Engine/World.h"

#include "RecallReplaySubsystem.generated.h"

struct FRecallReplay;

UCLASS()
class RECALLONLINE_API URecallReplaySubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	URecallReplaySubsystem();

	// UWorldSubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override final;
	virtual void Deinitialize() override final;
	// UWorldSubsystem implementation Begin

	// FTickableGameObject implementation Begin
	virtual void Tick(float DeltaTime) override final;
	virtual TStatId GetStatId() const override final;
	// FTickableGameObject implementation End

public:
	void InstantReplay();
	void InstantReplay(const FRecallReplay& Replay);

	void SaveReplay(const FString& ReplayName);
	void LoadReplay(const FRecallReplay& Replay);
	void LoadReplay(const FString& ReplayName);

	void SaveCacheReplay();
	void LoadCacheReplay();

	bool IsPlayReplay() const { return CurrentReplay.IsValid(); }

public:

	UFUNCTION(BlueprintCallable)
	void PauseReplay() { bPauseReplay = true; }
	
	UFUNCTION(BlueprintCallable)
	void ResumeReplay() { bPauseReplay = false; }

	UFUNCTION(BlueprintCallable)
	void StepReplay(int32 Frames = 1) { FastForwardFrames = Frames; }

	UFUNCTION(BlueprintCallable)
	void SpeedDown();
	
	UFUNCTION(BlueprintCallable)
	void SpeedUp();

	UFUNCTION(BlueprintCallable)
	void RestartReplay();
	
	UFUNCTION(BlueprintPure)
	bool IsPausedReplay() const { return bPauseReplay; }

	UFUNCTION(BlueprintPure)
	int32 GetReplaySpeed() const;

	UFUNCTION(BlueprintPure)
	double GetReplayElapsedTimeSeconds() const;
	
	UFUNCTION(BlueprintPure)
	double GetReplayDurationSeconds() const;

	UFUNCTION(BlueprintPure)
	int32 GetReplayElapsedFrame() const;
	
	UFUNCTION(BlueprintPure)
	int32 GetReplayDurationFrame() const;

	UFUNCTION(BlueprintPure)
	float GetReplayPercent() const;

	UFUNCTION(BlueprintCallable)
	void RestoreReplay();

protected:
	UPROPERTY(EditDefaultsOnly, Category=Debug)
	TSubclassOf<class UUserWidget> DebugReplayPlayerWidgetClass;

private:
	UPROPERTY(Transient)
	TScriptInterface<class IRecallSimulationControllerInterface> SimulationController;

	UPROPERTY(Transient)
	TObjectPtr<class UUserWidget> DebugReplayPlayerWidget;

	/* Map has been loaded and replay has begun. */
	UPROPERTY(Transient)
	bool bReplayBegun{ false };
	UPROPERTY(Transient)
	bool bReplayWaitPC{ false };

	/* Replay controls. */
	UPROPERTY(Transient)
	bool bPauseReplay{ false };
	UPROPERTY(Transient)
	int32 ReplaySpeedIndex{ 0 };
	UPROPERTY(Transient)
	int32 FastForwardFrames{ 0 };

	/* Current replay being played. */
	TSharedPtr<FRecallReplay> CurrentReplay;
	
	UPROPERTY(Transient)
	bool bOpenLevel = false;

	UPROPERTY(Transient)
	FString AutoSaveReplayName;

	UPROPERTY(Transient)
	float InstantReplayWaitDuration{ 0.0f };

	// Serialize our replay to test serialization
	TArray<uint8> CacheReplayMemory;

	// Cache the memory of the debug menu
	TArray<uint8> DebugMenu;
	
	void OnPreWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS);
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	void RegisterWorld(UWorld* World);

	void BeginReplay(const FRecallReplay& Replay);
	void TickReplay(const FRecallReplay& Replay, bool& bOutResume);
	void CheckReplayShortcuts(APlayerController* PlayerController);
	void ResumeGame();

	void OnFrameStart(uint32 Frame);
	const FRecallReplay& GetPlayingReplayChecked() const;

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void SetDebugReplayPlayer(bool bShow);
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

};
