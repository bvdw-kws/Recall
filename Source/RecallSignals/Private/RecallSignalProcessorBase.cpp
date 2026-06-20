// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallSignalProcessorBase.h"

#include "RecallSignalSubsystem.h"
#include "MassArchetypeTypes.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"
#include "Misc/ScopeLock.h"


URecallSignalProcessorBase::URecallSignalProcessorBase(const FObjectInitializer& ObjectInitializer)
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteAfter.Add(Recall::Signals::ProcessorGroupNames::UpdateDelayedSignals);
}

void URecallSignalProcessorBase::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	InternalSignalSystem = UWorld::GetSubsystem<URecallSignalSubsystem>(GetWorld());
	if (InternalSignalSystem.IsValid())
	{
		SignalsBufferIndex = InternalSignalSystem->CreateSignalsBuffer();
	}

	Super::InitializeInternal(Owner, InEntityManager);
}

void URecallSignalProcessorBase::BeginDestroy()
{
	if (InternalSignalSystem.IsValid())
	{
		for (const FName& SignalName : RegisteredSignals)
		{
			InternalSignalSystem->GetSignalDelegateByName(SignalName).RemoveAll(this);
		}
		InternalSignalSystem.Reset();
	}

	Super::BeginDestroy();
}

void URecallSignalProcessorBase::SubscribeToSignal(const FName SignalName)
{
	check(InternalSignalSystem.IsValid());
	check(!RegisteredSignals.Contains(SignalName));
	RegisteredSignals.Add(SignalName);
	if (InternalSignalSystem.IsValid())
	{
		InternalSignalSystem->GetSignalDelegateByName(SignalName).AddUObject(this, &URecallSignalProcessorBase::OnSignalReceived);
	}
}

void URecallSignalProcessorBase::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(SignalEntities);

	check(InternalSignalSystem.IsValid());
	FRecallSignalsBuffer& SignalsBuffer = InternalSignalSystem->GetSignalsBuffer(SignalsBufferIndex);

	const int32 ProcessingFrameBufferIndex = SignalsBuffer.CurrentFrameBufferIndex;
	{
		// we only need to lock the part where we change the current buffer index. Once that's done the incoming signals will end up 
		// in the other buffer
		UE::TScopeLock<UE::FSpinLock> ScopeLock(ReceivedSignalLock);
		SignalsBuffer.CurrentFrameBufferIndex = (SignalsBuffer.CurrentFrameBufferIndex + 1) % RecallSignalsBuffersCount;
	}
	
	FRecallFrameReceivedSignals& ProcessingFrameBuffer = SignalsBuffer.FrameReceivedSignals[ProcessingFrameBufferIndex];
	TArray<FRecallEntitySignalRange>& ReceivedSignalRanges = ProcessingFrameBuffer.ReceivedSignalRanges;
	TArray<FMassEntityHandle>& SignaledEntities = ProcessingFrameBuffer.SignaledEntities;

	if (ReceivedSignalRanges.IsEmpty())
	{
		return;
	}

	TArray<FMassArchetypeHandle> ValidArchetypes;
	GetArchetypesMatchingOwnedQueries(EntityManager, ValidArchetypes);

	if (ValidArchetypes.Num() > 0)
	{
		// EntitySet stores unique array of entities per specified archetype.
		// FMassArchetypeEntityCollection expects an array of entities, a set is used to detect unique ones.
		struct FEntitySet
		{
			void Reset()
			{
				Entities.Reset();
			}

			FMassArchetypeHandle Archetype;
			TArray<FMassEntityHandle> Entities;
		};

		TArray<FEntitySet> EntitySets;
		EntitySets.Reserve(ValidArchetypes.Num());

		for (const FMassArchetypeHandle& Archetype : ValidArchetypes)
		{
			FEntitySet& Set = EntitySets.AddDefaulted_GetRef();
			Set.Archetype = Archetype;
		}

		// SignalNameLookup has limit of how many signals it can handle at once, we'll do passes until all signals are processed.
		int32 SignalsToProcess = ReceivedSignalRanges.Num();
		while(SignalsToProcess > 0)
		{
			SignalNameLookup.Reset();

			// Convert signals with entity ids into arrays of entities per archetype.
			for (FRecallEntitySignalRange& Range : ReceivedSignalRanges)
			{
				if (Range.bProcessed)
				{
					continue;
				}
				// Get bitflag for the signal name
				const uint64 SignalFlag = SignalNameLookup.GetOrAddSignalName(Range.SignalName);
				if (SignalFlag == 0)
				{
					// Will process that signal in a second iteration
					continue;
				}

				// Entities for this signal
				TArrayView<FMassEntityHandle> Entities(&SignaledEntities[Range.Begin], Range.End - Range.Begin);
				FEntitySet* PrevSet = &EntitySets[0];
				for (const FMassEntityHandle Entity : Entities)
				{
					// Add to set of supported archetypes. Dont process if we don't care about the type.
					FMassArchetypeHandle Archetype = EntityManager.GetArchetypeForEntity(Entity);
					FEntitySet* Set = PrevSet->Archetype == Archetype ? PrevSet : EntitySets.FindByPredicate([&Archetype](const FEntitySet& Set) { return Archetype == Set.Archetype; });
					if (Set != nullptr)
					{
						// We don't care about duplicates here, the FMassArchetypeEntityCollection creation below will handle it
						Set->Entities.Add(Entity);
						SignalNameLookup.AddSignalToEntity(Entity, SignalFlag);
						PrevSet = Set;
					}
				}

				Range.bProcessed = true;
				SignalsToProcess--;
			}

			// Execute per archetype.
			for (FEntitySet& Set : EntitySets)
			{
				if (Set.Entities.Num() > 0)
				{
					Context.SetEntityCollection(FMassArchetypeEntityCollection(Set.Archetype, Set.Entities, FMassArchetypeEntityCollection::FoldDuplicates));
					SignalEntities(EntityManager, Context, SignalNameLookup);
					Context.ClearEntityCollection();
				}
				Set.Reset();
			}
		}
	}

	ReceivedSignalRanges.Reset();
	SignaledEntities.Reset();
}

void URecallSignalProcessorBase::OnSignalReceived(FName SignalName, TConstArrayView<FMassEntityHandle> Entities)
{
	check(InternalSignalSystem.IsValid());
	FRecallSignalsBuffer& SignalsBuffer = InternalSignalSystem->GetSignalsBuffer(SignalsBufferIndex);

	UE::TScopeLock<UE::FSpinLock> ScopeLock(ReceivedSignalLock);

	FRecallFrameReceivedSignals& CurrentFrameBuffer = SignalsBuffer.FrameReceivedSignals[SignalsBuffer.CurrentFrameBufferIndex];

	FRecallEntitySignalRange& Range = CurrentFrameBuffer.ReceivedSignalRanges.AddDefaulted_GetRef();
	Range.SignalName = SignalName;
	Range.Begin = CurrentFrameBuffer.SignaledEntities.Num();
	CurrentFrameBuffer.SignaledEntities.Append(Entities.GetData(), Entities.Num());
	Range.End = CurrentFrameBuffer.SignaledEntities.Num();
}
