// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "System/Observer/RecallObserverSubjectTypes.h"

using FRecallRepresentationEventFunc					= TFunction<void()>;

template<typename T>
using FRecallRepresentationObserverEventFunc			= TFunction<void(const TRecallTemplateObserver<T>& Observer)>;

template<typename T>
using FRecallRepresentationMultiObserverEventFunc		= TFunction<void(const TArray<T*>& Observers)>;

struct FRecallRepresentationEvent
{
	uint32 Frame = 0;
	FRecallRepresentationEventFunc Callback;
};
