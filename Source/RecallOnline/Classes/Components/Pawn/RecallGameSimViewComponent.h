// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/PawnComponent.h"
#include "Observer/Representation/RecallRepresentationReactInterface.h"

#include "RecallGameSimViewComponent.generated.h"

/**
 * Component to handle the view target from the game simulation.
 */
UCLASS()
class RECALLONLINE_API URecallGameSimViewComponent :
	public UPawnComponent,
	public IRecallRepresentationReactInterface
{
	GENERATED_UCLASS_BODY()

public:
	void SetGameSimViewTargetControllerID(const FString& ControllerID);

	const TArray<FString>& GetGameSimControllerIDs() const { return GameSimControllerIDs; }
	const FString& GetGameSimViewTargetControllerID() const { return GameSimViewTargetControllerID; }

	AActor* GetGameSimPawn() const { return GameSimPawn.Get(); }
	AActor* GetGameSimViewTarget() const { return GameSimViewTarget.Get(); }
	
	template<typename T=AActor>
	T* GetGameSimPawn() const { return Cast<T>(GameSimPawn.Get()); }
	
	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface
	
	// IRecallRepresentationReactInterface implementation Begin
protected:
	virtual void OnRender() override;
	// IRecallRepresentationReactInterface implementation End

private:
	/**
	 * The pawn owned by this pawn inside the game simulation.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> GameSimPawn;
	
	/**
	* The view target owned by this pawn inside the game simulation.
	*/
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> GameSimViewTarget;

	/**
	 * Controller ID of the view target in the game simulation.
	 */
	UPROPERTY(Transient)
	FString GameSimViewTargetControllerID;

	/**
	 * All the controller IDs from the game simulation.
	 */
	UPROPERTY(Transient)
	TArray<FString> GameSimControllerIDs;

	FString GetGameSimControllerID() const;
};
