// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "RecallSnapshotFileSubsystem.generated.h"

UCLASS()
class RECALLONLINE_API URecallSnapshotFileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	URecallSnapshotFileSubsystem();

public:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation Begin

public:
	bool SaveSnapshot(const FString& FileName) const;
	bool LoadSnapshot(const FString& FileName);

	bool SaveQuickSnapshot();
	bool LoadQuickSnapshot();

private:
	TSharedPtr<struct FRecallSnapshot> QuickSnapshot;

	bool CanSaveSnapshot() const;
	bool CanLoadSnapshot() const;

};
