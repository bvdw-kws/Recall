// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "System/Observer/RecallObserverSubjectSubsystem.h"

#include "System/Synchronization/RecallSynchronizationTypes.h"

void URecallObserverSubjectSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void URecallObserverSubjectSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URecallObserverSubjectSubsystem::RegisterObserver(UClass* Class, UObject* ObjectPointer)
{
	if (!ensureMsgf(Class, TEXT("Class should be valid.")))
	{
		return;
	}

	if (FRecallObserverCollection* ObserverCollection = ObserverCollections.Find(Class))
	{
		ObserverCollection->Observers.Add(ObjectPointer);
	}
	else
	{
		FRecallObserverCollection& NewObserverCollection = ObserverCollections.Add(Class);
		NewObserverCollection.Observers.Add(ObjectPointer);
	}
}

void URecallObserverSubjectSubsystem::UnregisterObserver(UObject* SourceObject)
{
	for (auto& ObserverCollection : ObserverCollections)
	{
		ObserverCollection.Value.Observers.Remove(SourceObject);
	}
}

TArray<UObject*> URecallObserverSubjectSubsystem::GetObserverObjects(UClass* Class) const
{
	if (ensureAlwaysMsgf(!IsRollback(this), TEXT("We should not dispatch events during rollback.")))
	{
		if (const FRecallObserverCollection* ObserverCollection = ObserverCollections.Find(Class))
		{
			return ObserverCollection->Observers;
		}
	}

	return {};
}
