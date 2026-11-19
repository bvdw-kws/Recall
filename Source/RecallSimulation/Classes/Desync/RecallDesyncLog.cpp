// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallDesyncLog.h"

#if RECALL_DESYNC_LOG

#include "Kismet/KismetStringLibrary.h"
#include "System/Desync/RecallDesyncLogSubsystem.h"

namespace Recall::Desync
{

void Log(const UObject* WorldContextObject, const FString& Log, const FString& Function)
{
	if (WorldContextObject)
	{
		if (URecallDesyncLogSubsystem* DesyncLogSystem = UWorld::GetSubsystem<URecallDesyncLogSubsystem>(WorldContextObject->GetWorld()))
		{
			DesyncLogSystem->Log(Log, Function);
		}
	}
}

FString Conv_BoolToString(bool Value)
{
	return UKismetStringLibrary::Conv_BoolToString(Value);
}

static TAutoConsoleVariable<bool> CVarVerboseEntityLog(
	TEXT("recall.desync.VerboseEntityLog"),
	false,
	TEXT("Log every entity of the Movement and PhysicsObject processors in the desync log (very large logs)."),
	ECVF_Default);

bool IsVerboseEntityLogEnabled()
{
	return CVarVerboseEntityLog.GetValueOnAnyThread();
}
} // Recall::Desync

#endif // RECALL_DESYNC_LOG
