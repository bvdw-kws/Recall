// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "System/Audio/RecallAudioSubsystem.h"

#include "Engine/World.h"
#include "System/Audio/RecallAudioGameThreadQueue.h"
#include "System/Representation/RecallRepresentationEventSubsystem.h"

void URecallAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<URecallRepresentationEventSubsystem>();
	RepresentationEventSystem = UWorld::GetSubsystem<URecallRepresentationEventSubsystem>(GetWorld());

	GameThreadQueue = NewObject<URecallAudioGameThreadQueue>(this);
}

void URecallAudioSubsystem::Deinitialize()
{
	RepresentationEventSystem.Reset();

	if (GameThreadQueue)
	{
		GameThreadQueue->CancelPendingLoads();
		GameThreadQueue = nullptr;
	}

	Super::Deinitialize();
}

void URecallAudioSubsystem::PlaySoundAsync(const TSoftObjectPtr<USoundBase>& Sound)
{
	const FSoftObjectPath SoundPath = Sound.ToSoftObjectPath();
	if (SoundPath.IsNull() || !RepresentationEventSystem.IsValid())
	{
		return;
	}

	const TWeakObjectPtr<URecallAudioSubsystem> WeakThis(this);
	RepresentationEventSystem->PushEvent([WeakThis, SoundPath]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->PlayConfirmedSound(SoundPath);
		}
	});
}

void URecallAudioSubsystem::PlayConfirmedSound(const FSoftObjectPath& SoundPath)
{
	check(IsInGameThread());
	if (GameThreadQueue)
	{
		GameThreadQueue->QueueSound(SoundPath);
	}
}
