// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "MassExternalSubsystemTraits.h"
#include "RecallStateTreeInstanceTypes.h"

#include "RecallStateTreeSubsystem.generated.h"

struct FStateTreeEvent;

/**
* A subsystem managing StateTree assets that can be rollback (cf UMassStateTreeSubsystem)
*/
UCLASS()
class RECALLSIMULATION_API URecallStateTreeSubsystem : public UWorldSubsystem, public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()
	
public:
	/**
	 * Allocates new instance data for specified StateTree.
	 * @param StateTree StateTree to allocated the data for.
	 * @return Handle to the data.
	 */
	FRecallStateTreeInstanceHandle AllocateInstanceData();

	/**
	 * Frees instance data.
	 * @param Handle Instance data handle to free.
	 */
	void FreeInstanceData(FRecallStateTreeInstanceHandle& Handle);

	/** @return Pointer to instance data held by the handle, or nullptr if handle is not valid. */
	FStateTreeInstanceData* GetInstanceData(const FRecallStateTreeInstanceHandle& Handle);

	/** @return True if the handle points to active instance data. */
	bool IsValidHandle(const FRecallStateTreeInstanceHandle& Handle) const;

	/** Sends event to the StateTree. */
	void SendStateTreeEvent(const FRecallStateTreeInstanceHandle& Handle, const FStateTreeEvent& Event);

	/** Hash used to compare clients simulation */
	uint32 GetRandomStreamCombinedHash() const;

protected:
	// IRecallSimulationReactSystemInterface implementation Begin
	void Reset() override final;

	void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End
	
private:
	mutable FCriticalSection DataGuard;

	TArray<int32, TFixedAllocator<RECALL_STATE_TREE_INSTANCE_MAX>> InstanceDataFreelist;
	TArray<FRecallStateTreeInstanceDataItem, TFixedAllocator<RECALL_STATE_TREE_INSTANCE_MAX>> InstanceDataArray;

};

template<>
struct TMassExternalSubsystemTraits<URecallStateTreeSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
