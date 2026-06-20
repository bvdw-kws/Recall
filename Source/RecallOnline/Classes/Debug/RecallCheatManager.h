// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "GameFramework/CheatManager.h"

#include "RecallCheatManager.generated.h"

UCLASS()
class RECALLONLINE_API URecallCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	URecallCheatManager();

	// UObject implementation Begin
public:
	virtual void PostInitProperties() override;
	// UObject implementation End

protected:
	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void DumpDesyncLog();
	
	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void LoadReplay(const FString& ReplayName);
	
	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void SaveReplay(const FString& ReplayName);

	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void StartReplay();

	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void SaveSnapshot(const FString& FileName);

	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void LoadSnapshot(const FString& FileName);

	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void SaveQuickSnapshot();

	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void LoadQuickSnapshot();
		
	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void RunAutoSettings(int32 WorkScale = 10, float CPUMultiplier = 1.0f, float GPUMultiplier = 1.0f);

	UFUNCTION(exec,BlueprintCallable, Category="Cheat Manager")
	void ResetDebugMenuWindow();
	
	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void AddPlayer();
	
	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void RemovePlayer();

	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void PossessNextPlayer();
		
	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void StepSimulation();
		
	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void ResetSimulation();

	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void SetPlayerOneCharacter(const FName& CharacterName);

	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void SetPlayerTwoCharacter(const FName& CharacterName);
	
	UFUNCTION(exec, BlueprintCallable, Category="Cheat Manager")
	void EndGame(const FString& Reason);
};
