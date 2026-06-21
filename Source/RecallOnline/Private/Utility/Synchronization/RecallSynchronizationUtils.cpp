// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Synchronization/RecallSynchronizationUtils.h"

#include "Engine/World.h"
#include "System/Synchronization/RecallRollbackSubsystem.h"
#include "Utility/MultiWorld/RecallMultiWorldUtils.h"

namespace Recall::Synchronization::Utils
{

TScriptInterface<IRecallSynchronizationInterface> GetSynchronization(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	URecallRollbackSubsystem* RollbackSystem = UWorld::GetSubsystem<URecallRollbackSubsystem>(MainWorld);
	return RollbackSystem;
}

IRecallSynchronizationInterface& GetSynchronizationRef(const UObject* WorldContextObject)
{
	const UWorld* MainWorld = Recall::MultiWorld::Utils::GetMainWorld(WorldContextObject);
	URecallRollbackSubsystem* RollbackSystem = UWorld::GetSubsystem<URecallRollbackSubsystem>(MainWorld);
	check(RollbackSystem != nullptr);
	return *RollbackSystem;
}

} // namespace Recall::Synchronization::Utils
