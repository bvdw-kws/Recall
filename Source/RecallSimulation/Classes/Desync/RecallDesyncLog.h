// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

#ifndef RECALL_DESYNC_ENABLED
	#define RECALL_DESYNC_ENABLED 0
#endif

#define RECALL_DESYNC_LOG (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT) & RECALL_DESYNC_ENABLED

#if RECALL_DESYNC_LOG

namespace Recall::Desync
{

extern RECALLSIMULATION_API void Log(const UObject* WorldContextObject, const FString& Log, const FString& Function);
extern RECALLSIMULATION_API FString Conv_BoolToString(bool Value);
	
} // Recall::Desync


#define RECALL_DESYNC_LOG_CONTEXT(WorldContextObject, Format) { Recall::Desync::Log(WorldContextObject, Format, FString(ANSI_TO_TCHAR(__FUNCTION__))); }

#define RECALL_DESYNC_LOG_INT(WorldContextObject, Name, Value) RECALL_DESYNC_LOG_CONTEXT(WorldContextObject, FString::Printf(TEXT("%s: Value<%d>"), *FString(TEXT(#Name)), Value))
#define RECALL_DESYNC_LOG_FLOAT(WorldContextObject, Name, Value) RECALL_DESYNC_LOG_CONTEXT(WorldContextObject, FString::Printf(TEXT("%s: Value<%f>"), *FString(TEXT(#Name)), Value))
#define RECALL_DESYNC_LOG_VEC(WorldContextObject, Name, Value) RECALL_DESYNC_LOG_CONTEXT(WorldContextObject, FString::Printf(TEXT("%s: Value<%s>"), *FString(TEXT(#Name)), *Value.ToString()))
#define RECALL_DESYNC_LOG_STR(WorldContextObject, Name, Value) RECALL_DESYNC_LOG_CONTEXT(WorldContextObject, FString::Printf(TEXT("%s: Value<%s>"), *FString(TEXT(#Name)), *Value))
#define RECALL_DESYNC_LOG_QUAT(WorldContextObject, Name, Value) RECALL_DESYNC_LOG_CONTEXT(WorldContextObject, FString::Printf(TEXT("%s: Value<%s>"), *FString(TEXT(#Name)), *Value.ToString()))
#define RECALL_DESYNC_LOG_BOOL(WorldContextObject, Name, Value) RECALL_DESYNC_LOG_CONTEXT(WorldContextObject, FString::Printf(TEXT("%s: Value<%s>"), *FString(TEXT(#Name)), *Recall::Desync::Conv_BoolToString(Value)))

#endif // RECALL_DESYNC_LOG
