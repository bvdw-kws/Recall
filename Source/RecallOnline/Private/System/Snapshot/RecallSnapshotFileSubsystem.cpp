// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Snapshot/RecallSnapshotFileSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Online/RecallGameMode.h"
#include "Utility/Snapshot/RecallSnapshotFileTypes.h"
#include "Utility/Snapshot/RecallSnapshotFileUtils.h"

URecallSnapshotFileSubsystem::URecallSnapshotFileSubsystem()
	: Super()
{
}

void URecallSnapshotFileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void URecallSnapshotFileSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

bool URecallSnapshotFileSubsystem::SaveSnapshot(const FString& FileName) const
{
	if (CanSaveSnapshot())
	{
		Recall::SnapshotFile::Utils::SaveQuickSnapshotFile(this, FileName);
		return true;
	}
	else
	{
		return false;
	}
}

bool URecallSnapshotFileSubsystem::LoadSnapshot(const FString& FileName)
{
	if (CanLoadSnapshot())
	{
		return Recall::SnapshotFile::Utils::LoadQuickSnapshotFile(this, FileName);
	}
	else
	{
		return false;
	}
}

bool URecallSnapshotFileSubsystem::CanSaveSnapshot() const
{
	if (const ARecallGameMode* GameMode = Cast<ARecallGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		return GameMode->CanSaveSnapshot();
	}
	else
	{
		return false;
	}
}

bool URecallSnapshotFileSubsystem::CanLoadSnapshot() const
{
	if (const ARecallGameMode* GameMode = Cast<ARecallGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		return GameMode->CanLoadSnapshot();
	}
	else
	{
		return false;
	}
}

bool URecallSnapshotFileSubsystem::SaveQuickSnapshot()
{
	if (CanSaveSnapshot())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("Save Quick Snapshot"));
		}
		
		QuickSnapshot = MakeShared<FRecallSnapshot>();
		return Recall::SnapshotFile::Utils::SaveSnapshot(this, *QuickSnapshot.Get());
	}
	else
	{
		QuickSnapshot.Reset();
		return false;
	}
}

bool URecallSnapshotFileSubsystem::LoadQuickSnapshot()
{
	if (QuickSnapshot.IsValid() && CanLoadSnapshot())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("Load Quick Snapshot"));
		}
		
		return Recall::SnapshotFile::Utils::LoadSnapshot(this, *QuickSnapshot.Get());
	}
	else
	{
		return false;
	}
}
