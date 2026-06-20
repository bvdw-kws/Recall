// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/Observer/RecallObserverSubjectInterface.h"
#include "MassExternalSubsystemTraits.h"
#include "RecallObserverSubjectTypes.h"

#include "RecallObserverSubjectSubsystem.generated.h"

USTRUCT()
struct FRecallObserverCollection
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Transient)
	TArray<UObject*> Observers;
};

UCLASS()
class RECALLSIMULATION_API URecallObserverSubjectSubsystem : public UWorldSubsystem, public IRecallObserverSubjectInterface
{
	GENERATED_BODY()

public:
	template <typename InterfaceType>
	TArray<TRecallTemplateObserver<InterfaceType>> GetObservers() const
	{
		TArray<TRecallTemplateObserver<InterfaceType>> Observers;

		for (UObject* Object : GetObserverObjects(InterfaceType::UClassType::StaticClass()))
		{
			if (IsValid(Object))
			{
				Observers.Add(TRecallTemplateObserver<InterfaceType>({ *CastChecked<InterfaceType>(Object), Object }));
			}
		}

		return Observers;
	}

	TArray<UObject*> GetObserverObjects(UClass* Class) const;

protected:
	// UWorldSubsystem implementation Begin
	void Initialize(FSubsystemCollectionBase& Collection) override final;
	void Deinitialize() override final;
	// UWorldSubsystem implementation Begin

	// IRecallObserverSubjectInterface implementation Begin
	void RegisterObserver(UClass* Class, UObject* ObjectPointer) override final;
	void UnregisterObserver(UObject* SourceObject) override final;
	// IRecallObserverSubjectInterface implementation Begin

protected:
	UPROPERTY(VisibleAnywhere, Transient)
	TMap<UClass*, FRecallObserverCollection> ObserverCollections;

};

template<>
struct TMassExternalSubsystemTraits<URecallObserverSubjectSubsystem> final
{
	enum
	{
		GameThreadOnly = true
	};
};
