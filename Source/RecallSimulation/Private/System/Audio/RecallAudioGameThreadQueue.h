// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "UObject/Object.h"

#include "RecallAudioGameThreadQueue.generated.h"

struct FStreamableHandle;

/** Game-thread-only loader and playback helper owned by URecallAudioSubsystem. */
UCLASS()
class URecallAudioGameThreadQueue : public UObject
{
	GENERATED_BODY()

public:
	void QueueSound(const FSoftObjectPath& SoundPath);
	void CancelPendingLoads();

private:
	void OnSoundLoaded(TSharedPtr<FStreamableHandle> Handle);
	void PlaySound(UObject* SoundObject);

	TSet<TSharedPtr<FStreamableHandle>> PendingLoads;
};
