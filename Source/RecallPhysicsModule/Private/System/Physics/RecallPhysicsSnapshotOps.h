// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "System/Physics/RecallPhysicsBodyTracker.h"

class URecallPhysicsSubsystem;

/**
 * Production implementation of the tracker operations, backed by the Jolt world and the object factories of the
 * physics subsystem. It is a friend of URecallPhysicsSubsystem so it can reach the protected creation/Jolt entry points.
 */
class FRecallPhysicsSnapshotOps final : public IRecallPhysicsBodyOps
{
public:
	explicit FRecallPhysicsSnapshotOps(URecallPhysicsSubsystem& InSubsystem) : Subsystem(InSubsystem) {}

	virtual void CreateBody(const FRecallPhysicsBodyHandle& Handle, const FJPRPhysicsBodySnapshot& Snapshot) override;
	virtual void CreateConstraint(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2) override;
	virtual void RemoveConstraint(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2) override;
	virtual void SetLayerOverride(const FRecallPhysicsBodyHandle& Handle, uint16 Layer) override;
	virtual void ClearLayerOverride(const FRecallPhysicsBodyHandle& Handle) override;
	virtual void SetMotionType(const FRecallPhysicsBodyHandle& Handle, EJPRPhysicsMotionType MotionType) override;
	virtual void ResetMotionType(const FRecallPhysicsBodyHandle& Handle) override;
	virtual TArray<uint8> SaveState(const TArray<TSharedPtr<FJPRPhysicsBody>>& Bodies) override;
	virtual bool RestoreState(const TArray<uint8>& StateData, const TArray<TSharedPtr<FJPRPhysicsBody>>& Bodies) override;

private:
	URecallPhysicsSubsystem& Subsystem;
};
