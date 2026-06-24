// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "Settings/RecallStreamingConfig.h"
#include "UObject/SoftObjectPtr.h"

#include "RecallSimulationSettings.generated.h"

/**
 * Settings for the Recall simulation.
 */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="Recall"))
class RECALLCORE_API URecallSimulationSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	URecallSimulationSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Default asset to be used for the physics layer of this project.
	 * It is possible to override the physics layer asset from the world settings directly.
	 */
	UPROPERTY(EditAnywhere, config, Category=JoltPhysics)
	TSoftObjectPtr<class URecallPhysicsLayerDataAsset> DefaultLayer;

	/// Maximum amount of threads to allow
	UPROPERTY(EditAnywhere, config, Category=JoltPhysics)
	int32 MaxNumThreads = 4;

	// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
	UPROPERTY(EditAnywhere, config, Category=JoltPhysics)
	int32 CollisionSteps = 1;

	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	UPROPERTY(EditAnywhere, config, Category=JoltPhysics)
	uint32 MaxBodies = 65536;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	UPROPERTY(EditAnywhere, config, Category=JoltPhysics)
	uint32 NumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	UPROPERTY(EditAnywhere, config, Category=JoltPhysics)
	uint32 MaxBodyPairs = 65536;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	UPROPERTY(EditAnywhere, config, Category=JoltPhysics)
	uint32 MaxContactConstraints = 10240;
	
	UPROPERTY(EditAnywhere, config, Category="Desync|Log")
	bool bUseDesyncLog = true;

	UPROPERTY(EditAnywhere, config, Category="Desync|Log", meta=(ClampMin=1, ClampMax=2047))
	int32 FramesCount = 64;

	UPROPERTY(EditAnywhere, config, Category="Desync|Log")
	FString FileName = TEXT("Desync");

	UPROPERTY(EditAnywhere, config, Category="Desync|Log")
	FString Directory = TEXT("Logs");

	UPROPERTY(EditAnywhere, config, Category="Desync")
	bool bOutOfSyncCheck = true;
	
	UPROPERTY(EditAnywhere, config, Category="Desync", meta=(EditCondition="bOutOfSyncCheck", ClampMin=1, ClampMax=100))
	int32 OutOfSyncBunchSize = 10;
	
	/**
	 * Fixed tick-rate of the game simulation.
	 * It can be lower or higher than the framerate.
	 */
	UPROPERTY(EditAnywhere, config, Category=Simulation, meta=(ClampMin=15, ClampMax=120))
	int32 FramesPerSeconds = 60;

	/**
	 * Limit to how many frames can be roll-back.
	 */
	UPROPERTY(EditAnywhere, config, Category=Online, meta=(ClampMin=0, ClampMax=16))
	int32 RollbackFrameCount = 8;

	/**
	 * Adjust our input latency between this range to compensate the ping
	 * 6 frames should keep the input latency below 100 milliseconds.
	 */
	UPROPERTY(EditAnywhere, config, Category=Online, meta=(ClampMin=0, ClampMax=10))
	FInt32Interval InputLatencyRange{ 2, 6 };

	/**
	 * Buffer to add some frames of latency so the input latency does not fall to zero.
	 */
	UPROPERTY(EditAnywhere, config, Category=Online, meta=(ClampMin=0, ClampMax=6))
	int32 InputNetworkLatencyBuffer = 0;

	/**
	 * Curve to adjust the simulation speed based on the frame drift.
	 */
	UPROPERTY(EditAnywhere, config, Category=Online)
	TSoftObjectPtr<class UCurveFloat> SimulationSpeedCurve;
	
	/**
	 * How many input tokens can be stored.
	 * These tokens are used to request sending input to other players without spamming them.
	 */
	UPROPERTY(EditDefaultsOnly, Category=Online, meta=(ClampMin=1))
	int32 InputTokenLimit = 5;

	/**
	 * Limit how many inputs can be sent at once.
	 */
	UPROPERTY(EditAnywhere, config, Category=Online, meta=(ClampMin=1, ClampMax=100))
	int32 InputBunchLimit = 50;

	/**
	 * Keep some frames, in this interval, as a buffer to keep a smooth movement.
	 */
	UPROPERTY(EditAnywhere, config, Category=Representation, meta=(ClampMin=0, ClampMax=10))
	FInt32Interval MovementInterpolationFrameBuffer{ 5, 10 };

	/**
	 * How fast the movement can speed up to consume the frame buffer.
	 */
	UPROPERTY(EditAnywhere, config, Category=Representation, meta=(ClampMin=1.0, ClampMax=10.0))
	float MovementInterpolationMaximumSpeed = 2.0f;

	/**
	 * Interpolation speed to adjust the speed scale of the movement to consume the frame buffer.
	 */
	UPROPERTY(EditAnywhere, config, Category=Representation, meta=(ClampMin=0.1, ClampMax=20.0))
	float MovementInterpolationSpeedScaleInterpSpeed = 10.0f;

	/** Entity streaming configuration */
	UPROPERTY(EditAnywhere, config, Category="Streaming")
	FRecallStreamingConfig StreamingConfig;
};
