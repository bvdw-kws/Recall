// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Utility/Slowmo/RecallSlowmoUtils.h"

#include "System/Slowmo/RecallSlowMotionSubsystem.h"

namespace Recall::Slowmo::Utils
{

float GetTimeDilatation(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	
	if (const URecallSlowMotionSubsystem* SlowmoSystem = UWorld::GetSubsystem<URecallSlowMotionSubsystem>(World))
	{
		return SlowmoSystem->GetTimeDilatation();
	}
	else
	{
		return 1.0f;
	}
}

} // namespace Recall::Slowmo::Utils
