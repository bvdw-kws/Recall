// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "UObject/SoftObjectPtr.h"

#include "RecallAudioSubsystem.generated.h"

class USoundBase;

/** Queues sound playback until the request is confirmed by Recall's rollback event system. */
UCLASS()
class RECALLSIMULATION_API URecallAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Queues a 2D sound for playback once the current simulation frame is confirmed.
	 * This method is safe to call from Mass processors running on worker threads.
	 */
	void PlaySoundAsync(const TSoftObjectPtr<USoundBase>& Sound);

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void PlayConfirmedSound(const FSoftObjectPath& SoundPath);

	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallRepresentationEventSubsystem> RepresentationEventSystem;

	UPROPERTY(Transient)
	TObjectPtr<class URecallAudioGameThreadQueue> GameThreadQueue;
};

template<>
struct TMassExternalSubsystemTraits<URecallAudioSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
