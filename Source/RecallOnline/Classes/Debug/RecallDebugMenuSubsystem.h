// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"

#include "RecallDebugMenuSubsystem.generated.h"

class IDebugMenu;
class IDebugMenuListHandle;
class IDebugMenuItemHandle;

/**
 * Subsystem that register and manage debug menu items for Recall
 */
UCLASS()
class RECALLONLINE_API URecallDebugMenuSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	URecallDebugMenuSubsystem();

protected:
	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem implementation End

	// FTickableGameObject implementation Begin
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	// FTickableGameObject implementation End

private:
#ifdef WITH_DEBUG_MENU
	TWeakObjectPtr<class UDebugMenuSubsystem> DebugMenuSubsystem;
#endif // WITH_DEBUG_MENU
	UPROPERTY(Transient)
	bool bIsJustPressedGamepadSpecialLeft{ false };
	UPROPERTY(Transient)
	bool bIsHoldGamepadSpecialLeft{ false };

	UPROPERTY(Transient)
	bool bIsHoldGamepadLeftTrigger{ false };

	TWeakPtr<IDebugMenuItemHandle> PauseHandle;
	TWeakPtr<IDebugMenuItemHandle> StepHandle;
	TWeakPtr<IDebugMenuItemHandle> QuickSaveHandle;
	TWeakPtr<IDebugMenuItemHandle> QuickLoadHandle;
	TWeakPtr<IDebugMenuItemHandle> ReplayHandle;
	TWeakPtr<IDebugMenuItemHandle> ResetHandle;

	TWeakPtr<IDebugMenuItemHandle> TwoPlayerHandle;
	TWeakPtr<IDebugMenuItemHandle> OnePlayerHandle;
	TWeakPtr<IDebugMenuItemHandle> PossessPlayerTwoHandle;

	TWeakPtr<IDebugMenuListHandle> LoadReplayHandle;
	bool bLoadingSnapshotFlag = false;
	TArray<FString> SnapshotFiles;
	mutable FCriticalSection LoadingSnapshotLock;
	TWeakPtr<IDebugMenuListHandle> LoadSnapshotHandle;
	
	TWeakPtr<IDebugMenuListHandle> PlayerOneCharacterHandle;
	TWeakPtr<IDebugMenuListHandle> PlayerTwoCharacterHandle;
	
	void CheckPlayerOneShortcuts(APlayerController* PlayerController);
	void CheckPlayerTwoShortcuts(APlayerController* PlayerController);

	bool HasPlayerTwoJoined(bool bDebugJoin = false) const;
	bool IsSimulationPaused() const;
	
	void CreateDebugMenuItems(IDebugMenu& DebugMenu);
	void RefreshDebugMenu();

	void OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS);
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

};
