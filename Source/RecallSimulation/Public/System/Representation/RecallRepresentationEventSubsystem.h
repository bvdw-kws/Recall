// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Interface/RecallSimulationReactSystemInterface.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "RecallRepresentationEventTypes.h"
#include "Containers/RingBuffer.h"

#include "RecallRepresentationEventSubsystem.generated.h"

#define RECALL_REPRESENTATION_EVENT_MAX 1024

/**
* Queue of representation events which wait until we are past the confirm frame to be triggered
*/
UCLASS()
class RECALLSIMULATION_API URecallRepresentationEventSubsystem : public UWorldSubsystem, public IRecallSimulationReactSystemInterface
{
	GENERATED_BODY()

public:
	void PushEvent(FRecallRepresentationEventFunc Callback);

	template<typename InterfaceType>
	inline void PushObserverEvent(FRecallRepresentationObserverEventFunc<InterfaceType> Callback)
	{
		PushEvent([&, Callback]()
			{
				for (UObject* Object : GetObserverObjects(InterfaceType::UClassType::StaticClass()))
				{
					if (IsValid(Object))
					{
						Callback(TRecallTemplateObserver<InterfaceType>{ *CastChecked<InterfaceType>(Object), Object });
					}
				}
			}
		);
	}

	template<typename InterfaceType>
	inline void PushMultiObserverEvent(FRecallRepresentationMultiObserverEventFunc<InterfaceType> Callback)
	{
		PushEvent([&, Callback]()
			{
				const TArray<UObject*> ObjectObjects = GetObserverObjects(InterfaceType::UClassType::StaticClass());

				TArray<InterfaceType*> Observers;
				Observers.SetNum(ObjectObjects.Num());

				for (int32 Index = 0; Index < ObjectObjects.Num(); Index++)
				{
					Observers[Index] = Cast<InterfaceType>(ObjectObjects[Index]);
				}

				Callback(Observers);
			}
		);
	}
	
protected:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation End

	// IRecallSimulationReactSystemInterface implementation Begin
	void Reset() override final;
	void Save(const FRecallSnapshotContext& Context, FInstancedStruct& OutSnapshot) override final;
	void Restore(const FRecallSnapshotContext& Context, const FInstancedStruct& InSnapshot) override final;
	// IRecallSimulationReactSystemInterface implementation End

protected:
	UFUNCTION()
	void OnTickStart(float DeltaTime);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<class URecallObserverSubjectSubsystem> ObserverSubjectSystem;

	FCriticalSection DataGuard;
	TRingBuffer<FRecallRepresentationEvent, TFixedAllocator<RECALL_REPRESENTATION_EVENT_MAX>> EventQueue;

	TArray<UObject*> GetObserverObjects(UClass* Class) const;
};

template<>
struct TMassExternalSubsystemTraits<URecallRepresentationEventSubsystem> final
{
	enum
	{
		GameThreadOnly = false
	};
};
