// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "RecallHUD_InGame.generated.h"

UCLASS()
class RECALLONLINE_API ARecallHUD_InGame : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginSpectating();
	virtual void EndSpectating();
	virtual void BeginPlaying();
	virtual void EndPlaying();
	virtual void OnPlayerDefeat(const FString& Reason);

	UFUNCTION(BlueprintNativeEvent, Category="HUD")
	void OnGameEnd(const FString& Reason);
	
	// AHUD Interface implementation begin
protected:
	virtual void DrawHUD() override;
	// AHUD Interface implementation end

	//~ Begin AActor Interface
protected:
	virtual void PostInitializeComponents() override;
	//~ End AActor Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category="HUD")
	TSoftClassPtr<class UCommonUserWidget> PlayingHUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category="HUD")
	TSoftClassPtr<class UCommonUserWidget> SpectatingHUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category="HUD")
	TSoftClassPtr<class UCommonActivatableWidget> DefeatWidgetClass;
	
	UPROPERTY(Transient)
	TScriptInterface<class IRecallSimulationControllerInterface> SimulationController;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<class UCommonUserWidget> HUDWidget;

	TSharedPtr<struct FStreamableHandle> HUDWidgetStreamingHandle;

	void PushHUDWidget(const TSoftClassPtr<class UCommonUserWidget>& HUDWidgetClass);
	void RemoveHUDWidget();
};
