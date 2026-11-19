// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"
#include "Physics/JPRPhysicsBody.h"
#include "Physics/RecallPhysicsBodyHandle.h"
#include "Physics/RecallPhysicsConstrainHandle.h"
#include "Physics/RecallPhysicsTypes.h"
#include "StructUtils/InstancedStruct.h"

struct FRecallPhysicsSnapshot;
struct FJPRPhysicsBodySnapshot;

/** Book-keeping for one body owned by the physics subsystem. */
struct FRecallPhysicsBodyRef
{
	FMassEntityHandle Entity;
	TSharedPtr<FJPRPhysicsBody> Body;
	FInstancedStruct Shape;
	FJPRPhysicsBodyParameters Params;
	bool bRestoreBody = true;
	TUniquePtr<uint16> LayerOverride;
	TUniquePtr<EJPRPhysicsMotionType> MotionTypeOverride;
};

/**
 * Everything the tracker needs from the outside world (Jolt, the object factory, the entity system).
 * URecallPhysicsSubsystem provides the production implementation, automation tests provide a standalone one.
 */
class IRecallPhysicsBodyOps
{
public:
	virtual ~IRecallPhysicsBodyOps() = default;

	/**
	 * Create the body described by Snapshot using exactly Handle (BodyID == Handle.SerialNumber) and register it
	 * in the tracker (FRecallPhysicsBodyTracker::RegisterBody).
	 */
	virtual void CreateBody(const FRecallPhysicsBodyHandle& Handle, const FJPRPhysicsBodySnapshot& Snapshot) = 0;

	/** Create the Jolt constraint and register it in the tracker (FRecallPhysicsBodyTracker::AddConstrainRef). */
	virtual void CreateConstraint(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2) = 0;

	/**
	 * Remove the Jolt constraint(s) between the two bodies. Only the Jolt side: the tracker removes its own
	 * reference (FRecallPhysicsBodyTracker::RemoveConstrainRef). Must run while both bodies still exist.
	 */
	virtual void RemoveConstraint(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2) = 0;

	virtual void SetLayerOverride(const FRecallPhysicsBodyHandle& Handle, uint16 Layer) = 0;
	virtual void ClearLayerOverride(const FRecallPhysicsBodyHandle& Handle) = 0;
	virtual void SetMotionType(const FRecallPhysicsBodyHandle& Handle, EJPRPhysicsMotionType MotionType) = 0;
	virtual void ResetMotionType(const FRecallPhysicsBodyHandle& Handle) = 0;

	/** Serialize the Jolt world and the given bodies (sorted by serial number). */
	virtual TArray<uint8> SaveState(const TArray<TSharedPtr<FJPRPhysicsBody>>& Bodies) = 0;
	virtual bool RestoreState(const TArray<uint8>& StateData, const TArray<TSharedPtr<FJPRPhysicsBody>>& Bodies) = 0;
};

/**
 * Owns the handle -> body bookkeeping of the physics subsystem and implements the snapshot save/restore diff
 * on top of it. It is a plain C++ class (no UObject, no UWorld) so it can be driven by automation tests.
 * The bookkeeping is private: it only changes through the methods below, which keep the maps consistent.
 */
class RECALLPHYSICSMODULE_API FRecallPhysicsBodyTracker
{
public:
	FRecallPhysicsBodyTracker() = default;
	// Body refs own unique state (overrides), so the tracker is move-only in spirit; nothing needs to copy it.
	FRecallPhysicsBodyTracker(const FRecallPhysicsBodyTracker&) = delete;
	FRecallPhysicsBodyTracker& operator=(const FRecallPhysicsBodyTracker&) = delete;

	//----------------------------------------------------------------------//
	// Handles
	//----------------------------------------------------------------------//

	/**
	 * Hands out the next free body handle. Serial numbers count up and wrap around to 1 once the maximum is
	 * reached, skipping serials that are still in use, so the BodyID space is never exhausted by churn alone.
	 * Returns an invalid handle if every serial is in use.
	 */
	FRecallPhysicsBodyHandle AllocateHandle();

	/**
	 * The last serial handed out by AllocateHandle. It is part of the snapshot, so allocation is a pure function
	 * of the restored state (deterministic for rollback).
	 */
	uint32 GetSerialNumberGenerator() const { return SerialNumberGenerator; }
	void ResetSerialNumberGenerator() { SerialNumberGenerator = 0; }

	/**
	 * Highest serial number that can be handed out. The serial number is used as the Jolt BodyID, and Jolt only
	 * accepts BodyID indices below its MaxBodies, so this must be MaxBodies - 1. 0 means unbounded.
	 */
	uint32 GetMaxSerialNumber() const { return MaxSerialNumber; }
	void SetMaxSerialNumber(const uint32 InMaxSerialNumber) { MaxSerialNumber = InMaxSerialNumber; }

	//----------------------------------------------------------------------//
	// Bodies
	//----------------------------------------------------------------------//

	void RegisterBody(const FRecallPhysicsBodyHandle& Handle, FRecallPhysicsBodyRef&& BodyRef);

	/**
	 * Unregister and release the Jolt object of a body. Does nothing if the handle is unknown.
	 * Constraints attached to the body are removed first (Jolt keeps raw pointers to the bodies of its constraints).
	 * Handle may be the key of a body map entry (callers iterate the map), which is destroyed with the entry:
	 * the implementation must not read it after removing the entry.
	 */
	void ReleaseBody(IRecallPhysicsBodyOps& Ops, const FRecallPhysicsBodyHandle& Handle);

	/** Remove every constraint, then release every body. */
	void ReleaseAll(IRecallPhysicsBodyOps& Ops);

	const FRecallPhysicsBodyRef* FindBodyRef(const FRecallPhysicsBodyHandle& Handle) const { return BodyRefMap.Find(Handle); }
	FRecallPhysicsBodyRef* FindBodyRef(const FRecallPhysicsBodyHandle& Handle) { return BodyRefMap.Find(Handle); }

	/** The handle of the body whose Jolt BodyID is BodyID, or nullptr. */
	const FRecallPhysicsBodyHandle* FindHandle(const uint32 BodyID) const { return BodyHandleMap.Find(BodyID); }

	const TMap<FRecallPhysicsBodyHandle, FRecallPhysicsBodyRef>& GetBodyRefMap() const { return BodyRefMap; }
	const TMap<uint32, FRecallPhysicsBodyHandle>& GetBodyHandleMap() const { return BodyHandleMap; }

	TArray<FRecallPhysicsBodyHandle> GetHandlesSortedBySerialNumber() const;

	//----------------------------------------------------------------------//
	// Constraints
	//----------------------------------------------------------------------//

	void AddConstrainRef(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2);
	void RemoveConstrainRef(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2);

	const TSet<FRecallPhysicsConstrainHandle>& GetConstrainRefs() const { return ConstrainRefs; }

	//----------------------------------------------------------------------//
	// Snapshots
	//----------------------------------------------------------------------//

	void Save(IRecallPhysicsBodyOps& Ops, FRecallPhysicsSnapshot& OutSnapshot) const;
	void Restore(IRecallPhysicsBodyOps& Ops, const FRecallPhysicsSnapshot& InSnapshot);

private:
	FJPRPhysicsBodySnapshot TakeBodySnapshot(const FRecallPhysicsBodyHandle& Handle) const;
	TArray<TSharedPtr<FJPRPhysicsBody>> GetBodiesSortedBySerialNumber() const;

	uint32 SerialNumberGenerator{ 0 };
	uint32 MaxSerialNumber{ 0 };

	TMap<FRecallPhysicsBodyHandle, FRecallPhysicsBodyRef> BodyRefMap;
	TMap<uint32, FRecallPhysicsBodyHandle> BodyHandleMap;
	TSet<FRecallPhysicsConstrainHandle> ConstrainRefs;
};
