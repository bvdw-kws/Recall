// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "RecallPhysicsSnapshotOps.h"

#include "Physics/JPRPhysicsShapeTypes.h"
#include "System/Physics/RecallPhysicsSnapshot.h"
#include "System/Physics/RecallPhysicsSubsystem.h"

void FRecallPhysicsSnapshotOps::CreateBody(const FRecallPhysicsBodyHandle& Handle, const FJPRPhysicsBodySnapshot& Snapshot)
{
	const FJPRPhysicsShape& Shape = Snapshot.Shape.Get<FJPRPhysicsShape>();
	FRecallPhysicsBodyHandle NewHandle = Handle;
	Subsystem.CreateShape_Internal(Snapshot.Entity, Snapshot.Shape, Shape.FactoryClass, Snapshot.Params, NewHandle);
}

void FRecallPhysicsSnapshotOps::CreateConstraint(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	Subsystem.CreateFixedConstrain(Handle1, Handle2);
}

void FRecallPhysicsSnapshotOps::RemoveConstraint(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	// Jolt side only, the tracker owns the reference.
	Subsystem.RemoveFixedConstraints(Handle1.SerialNumber, Handle2.SerialNumber);
}

void FRecallPhysicsSnapshotOps::SetLayerOverride(const FRecallPhysicsBodyHandle& Handle, const uint16 Layer)
{
	Subsystem.SetLayerOverride(Handle, Layer);
}

void FRecallPhysicsSnapshotOps::ClearLayerOverride(const FRecallPhysicsBodyHandle& Handle)
{
	Subsystem.ClearLayerOverride(Handle);
}

void FRecallPhysicsSnapshotOps::SetMotionType(const FRecallPhysicsBodyHandle& Handle, const EJPRPhysicsMotionType MotionType)
{
	Subsystem.SetMotionType(Handle, MotionType);
}

void FRecallPhysicsSnapshotOps::ResetMotionType(const FRecallPhysicsBodyHandle& Handle)
{
	Subsystem.ResetMotionType(Handle);
}

TArray<uint8> FRecallPhysicsSnapshotOps::SaveState(const TArray<TSharedPtr<FJPRPhysicsBody>>& Bodies)
{
	return Subsystem.SavePhysicsState(Bodies);
}

bool FRecallPhysicsSnapshotOps::RestoreState(const TArray<uint8>& StateData, const TArray<TSharedPtr<FJPRPhysicsBody>>& Bodies)
{
	return Subsystem.RestorePhysicsState(StateData, Bodies);
}
