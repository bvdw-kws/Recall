// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#include "System/Physics/RecallPhysicsBodyTracker.h"

#include "Async/ParallelFor.h"
#include "System/Physics/RecallPhysicsSnapshot.h"

FRecallPhysicsBodyHandle FRecallPhysicsBodyTracker::AllocateHandle()
{
	if (MaxSerialNumber == 0)
	{
		return FRecallPhysicsBodyHandle{ ++SerialNumberGenerator };
	}

	// At most one full lap of the serial space. The generator only moves forward and wraps, so it
	// stays a pure function of the snapshotted state.
	for (uint32 Attempt = 0; Attempt < MaxSerialNumber; ++Attempt)
	{
		SerialNumberGenerator = SerialNumberGenerator >= MaxSerialNumber ? 1 : SerialNumberGenerator + 1;
		if (!BodyHandleMap.Contains(SerialNumberGenerator))
		{
			return FRecallPhysicsBodyHandle{ SerialNumberGenerator };
		}
	}

	return FRecallPhysicsBodyHandle{};
}

void FRecallPhysicsBodyTracker::RegisterBody(const FRecallPhysicsBodyHandle& Handle, FRecallPhysicsBodyRef&& BodyRef)
{
	BodyRefMap.Add(Handle, MoveTemp(BodyRef));
	BodyHandleMap.Add(Handle.SerialNumber, Handle);
}

void FRecallPhysicsBodyTracker::ReleaseBody(IRecallPhysicsBodyOps& Ops, const FRecallPhysicsBodyHandle& Handle)
{
	if (!BodyRefMap.Contains(Handle))
	{
		return;
	}

	// Constraints must go before the body: Jolt's constraints point at the bodies they connect.
	TArray<FRecallPhysicsConstrainHandle> AttachedConstrains;
	for (const FRecallPhysicsConstrainHandle& ConstrainRef : ConstrainRefs)
	{
		if (ConstrainRef.Body1 == Handle || ConstrainRef.Body2 == Handle)
		{
			AttachedConstrains.Add(ConstrainRef);
		}
	}
	for (const FRecallPhysicsConstrainHandle& ConstrainRef : AttachedConstrains)
	{
		Ops.RemoveConstraint(ConstrainRef.Body1, ConstrainRef.Body2);
		RemoveConstrainRef(ConstrainRef.Body1, ConstrainRef.Body2);
	}

	// Keyed like RegisterBody does. The body's own ID is unset when Jolt refused to create it.
	// This must stay before the BodyRefMap removal: callers may pass the key of BodyRefMap itself (e.g. while iterating
	// it), which is destroyed along with its entry, so Handle must not be read once the entry is removed.
	BodyHandleMap.Remove(Handle.SerialNumber);

	FRecallPhysicsBodyRef BodyRef;
	if (BodyRefMap.RemoveAndCopyValue(Handle, BodyRef))
	{
		if (ensure(BodyRef.Body.IsValid()))
		{
			BodyRef.Body->ReleasePhysicsObject();
		}
	}
}

void FRecallPhysicsBodyTracker::ReleaseAll(IRecallPhysicsBodyOps& Ops)
{
	for (auto It = ConstrainRefs.CreateConstIterator(); It; ++It)
	{
		Ops.RemoveConstraint(It->Body1, It->Body2);
		RemoveConstrainRef(It->Body1, It->Body2);
	}
	check(ConstrainRefs.IsEmpty());

	for (auto It = BodyRefMap.CreateConstIterator(); It; ++It)
	{
		ReleaseBody(Ops, It.Key());
	}
	check(BodyHandleMap.IsEmpty());
	check(BodyRefMap.IsEmpty());
}

void FRecallPhysicsBodyTracker::AddConstrainRef(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	const FRecallPhysicsConstrainHandle NewHandle(Handle1, Handle2);
	check(!ConstrainRefs.Contains(NewHandle));
	ConstrainRefs.Add(NewHandle);
}

void FRecallPhysicsBodyTracker::RemoveConstrainRef(const FRecallPhysicsBodyHandle& Handle1, const FRecallPhysicsBodyHandle& Handle2)
{
	const FRecallPhysicsConstrainHandle Handle(Handle1, Handle2);
	check(ConstrainRefs.Contains(Handle));
	ConstrainRefs.Remove(Handle);
}

FJPRPhysicsBodySnapshot FRecallPhysicsBodyTracker::TakeBodySnapshot(const FRecallPhysicsBodyHandle& Handle) const
{
	const FRecallPhysicsBodyRef& PhysicsBodyRef = BodyRefMap.FindChecked(Handle);

	FJPRPhysicsBodySnapshot BodySnapshot;
	BodySnapshot.Entity = PhysicsBodyRef.Entity;
	BodySnapshot.Shape = PhysicsBodyRef.Shape;
	BodySnapshot.Params = PhysicsBodyRef.Params;

	if (ensure(PhysicsBodyRef.Body.IsValid()))
	{
		BodySnapshot.bEnabled = PhysicsBodyRef.Body->IsEnabled();
	}

	if (PhysicsBodyRef.LayerOverride.IsValid())
	{
		BodySnapshot.bUseLayerOverride = true;
		BodySnapshot.LayerOverride = *PhysicsBodyRef.LayerOverride.Get();
	}

	if (PhysicsBodyRef.MotionTypeOverride.IsValid())
	{
		BodySnapshot.bUseMotionTypeOverride = true;
		BodySnapshot.MotionTypeOverride = *PhysicsBodyRef.MotionTypeOverride.Get();
	}

	return BodySnapshot;
}

TArray<FRecallPhysicsBodyHandle> FRecallPhysicsBodyTracker::GetHandlesSortedBySerialNumber() const
{
	TArray<FRecallPhysicsBodyHandle> BodyHandles;
	BodyRefMap.GenerateKeyArray(BodyHandles);
	BodyHandles.Sort([](const FRecallPhysicsBodyHandle& Left, const FRecallPhysicsBodyHandle& Right)
	{
		return Left.SerialNumber < Right.SerialNumber;
	});
	return BodyHandles;
}

TArray<TSharedPtr<FJPRPhysicsBody>> FRecallPhysicsBodyTracker::GetBodiesSortedBySerialNumber() const
{
	const TArray<FRecallPhysicsBodyHandle> BodyHandles = GetHandlesSortedBySerialNumber();

	TArray<TSharedPtr<FJPRPhysicsBody>> Bodies;
	Bodies.Reserve(BodyHandles.Num());
	for (const FRecallPhysicsBodyHandle& BodyHandle : BodyHandles)
	{
		Bodies.Add(BodyRefMap.FindChecked(BodyHandle).Body);
	}
	return Bodies;
}

void FRecallPhysicsBodyTracker::Save(IRecallPhysicsBodyOps& Ops, FRecallPhysicsSnapshot& OutSnapshot) const
{
	OutSnapshot.SerialNumberGenerator = SerialNumberGenerator;

	constexpr int32 PhysicsSaveTaskCount = 3;
	ParallelFor(PhysicsSaveTaskCount, [this, &Ops, &OutSnapshot](int32 Index)
		{
			switch (Index)
			{
			case 0: // Save State
			{
				OutSnapshot.Data = Ops.SaveState(GetBodiesSortedBySerialNumber());
			}
			break;

			case 1: // Save Bodies
			{
				OutSnapshot.Bodies.Reserve(BodyRefMap.Num());

				for (const TPair<FRecallPhysicsBodyHandle, FRecallPhysicsBodyRef>& PhysicsBodyRef : BodyRefMap)
				{
					if (!PhysicsBodyRef.Value.bRestoreBody)
					{
						continue;
					}

					const FJPRPhysicsBodySnapshot BodySnapshot = TakeBodySnapshot(PhysicsBodyRef.Key);
					OutSnapshot.Bodies.Add(PhysicsBodyRef.Key, BodySnapshot);
				}
			}
			break;

			case 2: // Save Constrains
				OutSnapshot.Constrains = ConstrainRefs;
				break;

			default:
				unimplemented();
				break;
			}
		}
	);
}

void FRecallPhysicsBodyTracker::Restore(IRecallPhysicsBodyOps& Ops, const FRecallPhysicsSnapshot& InData)
{
	SerialNumberGenerator = InData.SerialNumberGenerator;

	TSet<FRecallPhysicsConstrainHandle> NewConstrains = InData.Constrains;
	const TSet<FRecallPhysicsConstrainHandle> OldConstrains = ConstrainRefs;

	TSet<FRecallPhysicsBodyHandle> NewHandles;
	InData.Bodies.GetKeys(NewHandles);

	TArray<FRecallPhysicsBodyHandle> OldHandles;
	BodyRefMap.GenerateKeyArray(OldHandles);

	// Bodies to destroy. They are only released after the stale constrains are gone, see below.
	TSet<FRecallPhysicsBodyHandle> ReleasedHandles;
	TArray<FRecallPhysicsBodyHandle> HandlesToRelease;

	for (const FRecallPhysicsBodyHandle& OldHandle : OldHandles)
	{
		// No need to take care of static bodies since they are not saved.
		const FRecallPhysicsBodyRef& OldBodyRef = BodyRefMap[OldHandle];
		if (!OldBodyRef.bRestoreBody)
		{
			checkf(!NewHandles.Contains(OldHandle),
				TEXT("%hs Should not save bodies that must not be restored"), __FUNCTION__);
			continue;
		}

		// Keep existing bodies so they can be restored.
		if (NewHandles.Contains(OldHandle))
		{
			const FJPRPhysicsBodySnapshot& NewBodySnapshot = InData.Bodies[OldHandle];
			if (NewBodySnapshot.Shape == OldBodyRef.Shape && NewBodySnapshot.Params == OldBodyRef.Params)
			{
				NewHandles.Remove(OldHandle);
				continue;
			}
		}

		// Destroy non-existing bodies
		HandlesToRelease.Add(OldHandle);
		ReleasedHandles.Add(OldHandle);
	}

	// Release deprecated constrains. The Jolt constraint has to be removed as well, not only our reference, and
	// before the bodies it connects are destroyed.
	for (const FRecallPhysicsConstrainHandle& ConstrainRef : OldConstrains)
	{
		if (NewConstrains.Contains(ConstrainRef))
		{
			if (!ReleasedHandles.Contains(ConstrainRef.Body1) && !ReleasedHandles.Contains(ConstrainRef.Body2))
			{
				NewConstrains.Remove(ConstrainRef);
				continue;
			}
		}

		Ops.RemoveConstraint(ConstrainRef.Body1, ConstrainRef.Body2);
		RemoveConstrainRef(ConstrainRef.Body1, ConstrainRef.Body2);
	}

	for (const FRecallPhysicsBodyHandle& HandleToRelease : HandlesToRelease)
	{
		ReleaseBody(Ops, HandleToRelease);
	}

	// Create non-existing bodies
	for (const FRecallPhysicsBodyHandle& NewHandle : NewHandles)
	{
		check(NewHandle.IsValid());
		const FJPRPhysicsBodySnapshot& BodySnapshot = InData.Bodies[NewHandle];
		Ops.CreateBody(NewHandle, BodySnapshot);
	}

	// Re-apply body settings
	for (const TPair<FRecallPhysicsBodyHandle, FJPRPhysicsBodySnapshot>& RestorePair : InData.Bodies)
	{
		const FRecallPhysicsBodyHandle& Handle = RestorePair.Key;

		const FRecallPhysicsBodyRef& BodyRef = BodyRefMap.FindChecked(Handle);
		const TSharedPtr<FJPRPhysicsBody>& Body = BodyRef.Body;
		check(Body.IsValid());

		const FJPRPhysicsBodySnapshot& BodySnapshot = RestorePair.Value;

		if (BodySnapshot.bEnabled)
		{
			Body->Activate();
		}
		else
		{
			Body->Desactivate();
		}

		if (BodySnapshot.bUseLayerOverride)
		{
			Ops.SetLayerOverride(Handle, BodySnapshot.LayerOverride);
		}
		else if (BodyRef.LayerOverride.IsValid())
		{
			Ops.ClearLayerOverride(Handle);
		}

		if (BodySnapshot.bUseMotionTypeOverride)
		{
			Ops.SetMotionType(Handle, BodySnapshot.MotionTypeOverride);
		}
		else if (BodyRef.MotionTypeOverride.IsValid())
		{
			Ops.ResetMotionType(Handle);
		}
	}

	// Create new constrains
	for (const FRecallPhysicsConstrainHandle& ConstrainSnapshot : NewConstrains)
	{
		Ops.CreateConstraint(ConstrainSnapshot.Body1, ConstrainSnapshot.Body2);
	}

	Ops.RestoreState(InData.Data, GetBodiesSortedBySerialNumber());
}
