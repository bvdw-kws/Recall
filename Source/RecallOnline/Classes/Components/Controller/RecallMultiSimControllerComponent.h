// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/ControllerComponent.h"

#include "RecallMultiSimControllerComponent.generated.h"

/**
 * Component to handle game simulation transition between worlds.
 */
UCLASS()
class RECALLONLINE_API URecallMultiSimControllerComponent : public UControllerComponent
{
	GENERATED_BODY()
	
public:
	void GoToWorld(int32 WorldIndex, bool bRespawnPlayerCamera = false);
	UWorld* GetCurrentWorld() const;
	FString GetDefaultSimPlayerId() const;
	
	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface
	
#pragma region RPC
public:
	/**
	 * Switch between worlds while using the multi-world system.
	 */
	UFUNCTION(Client, Reliable)
	void Client_GoToWorld(int32 WorldIndex);
#pragma endregion RPC

protected:
	virtual void OnLoadSnapshot(uint32 Frame, double TimeSeconds, bool bIsRollback);

};
