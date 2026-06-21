// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallFrontendUtils.h"

#include "Engine/World.h"
#include "System/Desync/RecallDesyncLogSubsystem.h"
#include "System/Input/RecallInputQueueSubsystem.h"
#include "System/Observer/RecallObserverSubjectSubsystem.h"
#include "System/Player/RecallPlayerQueueSubsystem.h"
#include "System/Random/RecallRandomNumberSubsystem.h"
#include "System/Simulation/Insight/RecallSimulationInsightSubsystem.h"
#include "System/Simulation/RecallMultiSimSubsystem.h"
#include "System/Snapshot/RecallMultiSimSnapshotSubsystem.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"

namespace Recall::Frontend::Utils
{

template<>
RECALLFRONTEND_API TScriptInterface<IRecallObserverSubjectInterface> Get<IRecallObserverSubjectInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	return UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld);
}

template<>
RECALLFRONTEND_API IRecallObserverSubjectInterface& GetRef<IRecallObserverSubjectInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld);
	check(MultiSimSystem != nullptr);
	return *MultiSimSystem;
}

template<>
RECALLFRONTEND_API IRecallObserverSubjectInterface& GetRefByWorld<IRecallObserverSubjectInterface>(const UWorld* World)
{
	URecallObserverSubjectSubsystem* ObserverSubjectSystem = UWorld::GetSubsystem<URecallObserverSubjectSubsystem>(World);
	check(ObserverSubjectSystem != nullptr);
	return *ObserverSubjectSystem;
}

template<>
RECALLFRONTEND_API IRecallSimulationControllerInterface& GetRef<IRecallSimulationControllerInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld);
	check(MultiSimSystem != nullptr);
	return *MultiSimSystem;
}

template<>
RECALLFRONTEND_API TScriptInterface<IRecallSimulationControllerInterface> Get<IRecallSimulationControllerInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);

	if (URecallMultiSimSubsystem* MultiSimSystem = UWorld::GetSubsystem<URecallMultiSimSubsystem>(MainWorld))
	{
		return MultiSimSystem;
	}

	return nullptr;
}

template<>
RECALLFRONTEND_API IRecallInputQueueInterface& GetRef<IRecallInputQueueInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	URecallInputQueueSubsystem* InputQueueSystem = UWorld::GetSubsystem<URecallInputQueueSubsystem>(MainWorld);
	check(InputQueueSystem != nullptr);
	return *InputQueueSystem;
}

template<>
RECALLFRONTEND_API TScriptInterface<IRecallInputQueueInterface> Get<IRecallInputQueueInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	return UWorld::GetSubsystem<URecallInputQueueSubsystem>(MainWorld);
}

template<>
RECALLFRONTEND_API IRecallPlayerQueueInterface& GetRefByWorld<IRecallPlayerQueueInterface>(const UWorld* World)
{
	URecallPlayerQueueSubsystem* PlayerQueueSystem = UWorld::GetSubsystem<URecallPlayerQueueSubsystem>(World);
	check(PlayerQueueSystem != nullptr);
	return *PlayerQueueSystem;
}

template<>
RECALLFRONTEND_API IRecallRandomNumberInterface& GetRefByWorld<IRecallRandomNumberInterface>(const UWorld* World)
{
	URecallRandomNumberSubsystem* RandomNumberSystem = UWorld::GetSubsystem<URecallRandomNumberSubsystem>(World);
	check(RandomNumberSystem != nullptr);
	return *RandomNumberSystem;
}

template<>
RECALLFRONTEND_API TScriptInterface<IRecallSimulationInsightInterface> Get<IRecallSimulationInsightInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	return UWorld::GetSubsystem<URecallSimulationInsightSubsystem>(MainWorld);
}

template<>
RECALLFRONTEND_API IRecallSimulationInsightInterface& GetRef<IRecallSimulationInsightInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	URecallSimulationInsightSubsystem* InsightSystem = UWorld::GetSubsystem<URecallSimulationInsightSubsystem>(MainWorld);
	check(InsightSystem != nullptr);
	return *InsightSystem;
}

template<>
RECALLFRONTEND_API TScriptInterface<IRecallSnapshotInterface> Get<IRecallSnapshotInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	return UWorld::GetSubsystem<URecallMultiSimSnapshotSubsystem>(MainWorld);
}

template<>
RECALLFRONTEND_API IRecallSnapshotInterface& GetRef<IRecallSnapshotInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	URecallMultiSimSnapshotSubsystem* MultiSimSnapshotSystem = UWorld::GetSubsystem<URecallMultiSimSnapshotSubsystem>(MainWorld);
	check(MultiSimSnapshotSystem != nullptr);
	return *MultiSimSnapshotSystem;
}

template<>
RECALLFRONTEND_API TScriptInterface<IRecallDesyncLogInterface> Get<IRecallDesyncLogInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	return UWorld::GetSubsystem<URecallDesyncLogSubsystem>(MainWorld);
}

template<>
RECALLFRONTEND_API IRecallDesyncLogInterface& GetRef<IRecallDesyncLogInterface>(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	URecallDesyncLogSubsystem* DesyncLogSystem = UWorld::GetSubsystem<URecallDesyncLogSubsystem>(MainWorld);
	check(DesyncLogSystem != nullptr);
	return *DesyncLogSystem;
}
	
void RegisterGlobalObserver(UClass* Class, UObject* ObjectPointer)
{
	GetRef<IRecallObserverSubjectInterface>(ObjectPointer).RegisterObserver(Class, ObjectPointer);
}

void UnregisterAllGlobalObservers(UObject* SourceObject)
{
	GetRef<IRecallObserverSubjectInterface>(SourceObject).UnregisterObserver(SourceObject);
}
	
} // namespace Recall::Frontend::Utils
