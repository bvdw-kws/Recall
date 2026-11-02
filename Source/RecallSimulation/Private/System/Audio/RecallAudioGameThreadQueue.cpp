// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "System/Audio/RecallAudioGameThreadQueue.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void URecallAudioGameThreadQueue::QueueSound(const FSoftObjectPath& SoundPath)
{
	check(IsInGameThread());
	if (SoundPath.IsNull())
	{
		return;
	}

	if (UObject* LoadedSound = SoundPath.ResolveObject())
	{
		PlaySound(LoadedSound);
		return;
	}

	TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		SoundPath,
		FStreamableDelegateWithHandle::CreateUObject(this, &ThisClass::OnSoundLoaded));

	if (Handle.IsValid())
	{
		PendingLoads.Add(MoveTemp(Handle));
	}
}

void URecallAudioGameThreadQueue::CancelPendingLoads()
{
	check(IsInGameThread());
	for (const TSharedPtr<FStreamableHandle>& PendingLoad : PendingLoads)
	{
		if (PendingLoad.IsValid())
		{
			PendingLoad->CancelHandle();
		}
	}
	PendingLoads.Reset();
}

void URecallAudioGameThreadQueue::OnSoundLoaded(TSharedPtr<FStreamableHandle> Handle)
{
	check(IsInGameThread());
	PendingLoads.Remove(Handle);

	PlaySound(Handle.IsValid() ? Handle->GetLoadedAsset() : nullptr);
}

void URecallAudioGameThreadQueue::PlaySound(UObject* SoundObject)
{
	check(IsInGameThread());
	USoundBase* Sound = Cast<USoundBase>(SoundObject);
	if (IsValid(Sound))
	{
		UGameplayStatics::PlaySound2D(GetOuter(), Sound);
	}
}
