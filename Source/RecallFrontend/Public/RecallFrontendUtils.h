// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/ScriptInterface.h"

namespace Recall::Frontend::Utils
{

/**
* Getter for frontend interface reference
*/
template<typename T>
RECALLFRONTEND_API extern T& GetRef(const UObject* WorldContextObject);

/**
* Getter for frontend interface
*/
template<typename T>
RECALLFRONTEND_API extern TScriptInterface<T> Get(const UObject* WorldContextObject);

/**
* Getter for frontend interface reference
*/
template<typename T>
RECALLFRONTEND_API extern T& GetRefByWorld(const UWorld* World);

/**
 * Register an observer to the observer subject subsystem.
 */
RECALLFRONTEND_API extern void RegisterGlobalObserver(UClass* Class, UObject* ObjectPointer);
	
/**
 * Unregister an observer from the observer subject subsystem.
 */
RECALLFRONTEND_API extern void UnregisterAllGlobalObservers(UObject* SourceObject);

/**
 * Template to register any UInterface type as an observer.
 */
template <typename InterfaceType>
FORCEINLINE void RegisterGlobalObserver(UObject* SourceObject)
{
	UClass* Class = InterfaceType::UClassType::StaticClass();
	RegisterGlobalObserver(Class, SourceObject);
}
	
} // namespace Recall::Frontend::Utils
