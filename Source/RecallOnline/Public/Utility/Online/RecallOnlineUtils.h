// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

namespace Recall::Online::Utils
{
	
RECALLONLINE_API extern void PrintDebug(const UObject* WorldContextObject);
RECALLONLINE_API extern float GetRoundTripTimeInSeconds(const UObject* WorldContextObject);
RECALLONLINE_API extern float GetNetworkLatencyInSeconds(const UObject* WorldContextObject);
RECALLONLINE_API extern int32 GetNetworkLatencyInFramesCeiled(const UObject* WorldContextObject);
RECALLONLINE_API extern int32 GetNetworkLatencyInFramesRounded(const UObject* WorldContextObject);


} // namespace Recall::Online::Utils
