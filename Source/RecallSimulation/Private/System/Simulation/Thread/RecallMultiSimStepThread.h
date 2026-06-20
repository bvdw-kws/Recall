// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

class URecallMultiSimSubsystem;

class FRecallMultiSimStepThread : public FRunnable
{
public:
	FRecallMultiSimStepThread(const TWeakObjectPtr<URecallMultiSimSubsystem>& InMultiSimSystem);

public:
	void StartTick();

	void Rollback_Step(int32 Count);
	void Simulation_Step(int32 Count);

	void WaitForStep() const;
	bool IsStepDone() const;

public:
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

private:
	bool bIsRunning = false;

	FEvent* TickEvent = nullptr;
	TUniquePtr<FRunnableThread> Thread;

	uint32 Step = 0;
	FCriticalSection StepGuard;

	TWeakObjectPtr<URecallMultiSimSubsystem> MultiSimSystem;
};
