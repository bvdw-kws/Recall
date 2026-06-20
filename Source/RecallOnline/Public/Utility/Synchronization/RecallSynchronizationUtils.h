// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/ScriptInterface.h"

class IRecallSynchronizationInterface;

namespace Recall::Synchronization::Utils
{

RECALLONLINE_API TScriptInterface<IRecallSynchronizationInterface> GetSynchronization(const UObject* WorldContextObject);
RECALLONLINE_API extern IRecallSynchronizationInterface& GetSynchronizationRef(const UObject* WorldContextObject);

} // namespace Recall::Synchronization::Utils
