// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StructUtils/InstancedStruct.h"

#include "RecallSimulationReactSystemInterface.generated.h"

class IRecallSimulationReactSystemInterface;

namespace Recall
{
	namespace SimReactSystem
	{
		namespace StartOrder
		{
			static constexpr int32 LowPriority = 0;
			static constexpr int32 MediumPriority = 1;
			static constexpr int32 HighPriority = 2;
		} // namespace StartOrder

		namespace RestoreOrder
		{
			static constexpr int32 Default = 0;
			static constexpr int32 Actor = 1;
		} // namespace RestoreOrder
	} // namespace SimReactSystem

	namespace SnapshotReason
	{
		static const FName Snapshot = TEXT("Snapshot");
		static const FName Rollback = TEXT("Rollback");
	} // namespace SnapshotReason
} // namespace Recall

struct FRecallSnapshotContext
{
	uint32 Frame = 0;
	FName Reason = NAME_None;

	bool IsRollback() const { return Reason == Recall::SnapshotReason::Rollback; }
	bool IsSnapshot() const { return Reason == Recall::SnapshotReason::Snapshot; }
};

struct FRecallSimulationStartParams;

/**
* Interface to notify our simulation world systems
*/
UINTERFACE()
class RECALLCORE_API URecallSimulationReactSystemInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()

public:
	static TArray<TScriptInterface<IRecallSimulationReactSystemInterface>> GetSimulationReactSystems(UWorld* World);

};

class RECALLCORE_API IRecallSimulationReactSystemInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	// Triggered at the start of the level.
	virtual void Start(const FRecallSimulationStartParams& Params) {}
	virtual int32 GetStartOrderPriority() const { return Recall::SimReactSystem::StartOrder::LowPriority; }

	// Triggered when simulation is reset
	virtual void Reset() {}

	// Events to Save/Restore state
	virtual void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) {}
	virtual void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) {}

	/*
	 * Pre-restore and post-restore are executed on the game-thread for snapshots.
	 */
	virtual void PreRestore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) {}
	virtual void PostRestore() {}
	virtual int32 GetPostRestoreOrderPriority() const { return Recall::SimReactSystem::RestoreOrder::Default; }

};
