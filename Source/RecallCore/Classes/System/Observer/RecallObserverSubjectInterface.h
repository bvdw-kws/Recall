// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RecallObserverSubjectInterface.generated.h"

UINTERFACE()
class RECALLCORE_API URecallObserverSubjectInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class RECALLCORE_API IRecallObserverSubjectInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	template <typename InterfaceType>
	void RegisterObserver(UObject* SourceObject)
	{
		UClass* Class = InterfaceType::UClassType::StaticClass();
		RegisterObserver(Class, SourceObject);
	}

	virtual void RegisterObserver(UClass* Class, UObject* ObjectPointer) = 0;
	virtual void UnregisterObserver(UObject* SourceObject) = 0;
};
