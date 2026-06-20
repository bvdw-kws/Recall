// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "RecallGameplayStatics.generated.h"

UCLASS()
class RECALLONLINE_API URecallGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"), DisplayName="Get Recall Player Controller")
	static class ARecallPlayerController* GetRecallPlayerController(const UObject* WorldContextObject, int32 PlayerIndex);
	
	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"), DisplayName="Get Recall Game Mode")
	static class ARecallGameMode* GetRecallGameMode(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"), DisplayName="Get Recall In Game State")
	static class ARecallGameState_InGame* GetRecallInGameState(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"), DisplayName="Get Recall Player Camera Manager")
	static class APlayerCameraManager* GetRecallPlayerCameraManager(const UObject* WorldContextObject, int32 PlayerIndex);
	
	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static int32 GetNumPlayerJoinedGame(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static int32 GetNumLocalPlayerJoinedGame(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static TArray<ARecallPlayerController*> GetLocalPlayerJoinedGame(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static bool IsPlayingReplay(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static bool IsRestoringGameSimulation(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Game|Recall", meta=(WorldContext="WorldContextObject")) 
	static void GetFormattedTime(const UObject* WorldContextObject, double TimeSeconds, int32& NumHours, int32& NumMinutes, int32& NumSeconds);

	/* Custom project to allow projections from behind to project. */
	static void ProjectWorldToScreen(APlayerController const* InPlayer, const FVector& InWorldPosition, FVector2D& OutScreenPosition);

	UFUNCTION(BlueprintCallable, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static void ReturnToMainMenu(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable, Category="Game|Recall", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static void RestartLevel(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="Game|Recall", meta=(UnsafeDuringActorConstruction="true"))
	static bool ProjectWorldToSceneCaptureScreenPosition(class USceneCaptureComponent2D* SceneCaptureComponent2D, const FVector& WorldPosition, FVector2D& OutScreenPosition);
	
	UFUNCTION(BlueprintCallable, Category="Game|Recall", meta=(UnsafeDuringActorConstruction="true"))
	static bool ProjectWorldToCameraScreenPosition(class UCameraComponent* CameraComponent, const FVector& WorldPosition, FVector2D& OutScreenPosition);
	
	UFUNCTION(BlueprintCallable, Category="Game|Recall", meta=(UnsafeDuringActorConstruction="true"))
	static void DeprojectCameraScreenPositionToWorld(class UCameraComponent* CameraComponent, float ScreenX, float ScreenY, FVector& OutWorldPosition, FVector& OutWorldDirection);
	
	static bool FindCameraViewRect(class UCameraComponent* CameraComponent, FIntRect& OutViewRect);
};
