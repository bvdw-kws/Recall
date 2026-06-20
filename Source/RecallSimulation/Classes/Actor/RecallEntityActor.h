// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "GameFramework/Actor.h"

#include "RecallEntityActor.generated.h"

class URecallEntityComponent;

/*
* Base class to setup our Entity Actor.
*/
UCLASS(Abstract, Blueprintable, DisplayName="MS Entity Actor", ComponentWrapperClass)
class RECALLSIMULATION_API ARecallEntityActor : public AActor
{
	GENERATED_UCLASS_BODY()

	//~ Begin AActor Interface
public:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	//~ End AActor Interface

	/**
	 * @return Returns the entity component hosted by this actor
	 */
	TObjectPtr<URecallEntityComponent> GetEntityComponent() const;
	FName GetEntityAssetName() const;
	
protected:
	/** Fires off whenever the mass entity component's entity config is edited */
	UFUNCTION(BlueprintNativeEvent, CallInEditor, BlueprintCallable, Category=Editor)
	void OnEditTraits();

	UPROPERTY(EditDefaultsOnly)
	bool bHideActorInGameOnBeginPlay = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallEntityComponent> EntityComponent;
};
