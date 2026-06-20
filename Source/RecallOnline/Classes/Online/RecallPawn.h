// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Containers/Queue.h"
#include "Online/RecallInputNetSerializers.h"

#include "RecallPawn.generated.h"

class URecallGameSimViewComponent;
class URecallInputTokenPawnComponent;
class URecallViewTargetPawnComponent;

/*
* Perform simulation input for a player entity.
* Can be possessed to perform input for the owned player entity.
*/
UCLASS()
class RECALLONLINE_API ARecallPawn :
	public APawn
{
	GENERATED_UCLASS_BODY()

public:
	void PushInput(const FRecallInput& Input);

	const FString& GetSimPlayerId() const { return SimPlayerId; }
	void SetSimPlayerId(const FString& InSimPlayerId, bool bOverride = false);

	int32 GetInputRequestNum() const { return InputRequest.FrameInputs.Num(); }
	void ClearInputRequests() { InputRequest = FRecallInputRequest(); }

	URecallInputTokenPawnComponent* GetInputTokenComponentChecked() const { check(InputTokenComponent); return InputTokenComponent; }
	URecallGameSimViewComponent* GetGameSimViewComponent() const { check(GameSimViewComponent); return GameSimViewComponent; }
	URecallViewTargetPawnComponent* GetViewTargetComponentChecked() const { check(ViewTargetComponent); return ViewTargetComponent; }

	//~ Begin AActor Interface
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~ End AActor Interface

protected:
	UPROPERTY(ReplicatedUsing=OnRep_SimPlayerId, Transient)
	FString SimPlayerId;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallInputTokenPawnComponent> InputTokenComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallGameSimViewComponent> GameSimViewComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallViewTargetPawnComponent> ViewTargetComponent;
	
private:
	UPROPERTY(Transient)
	TScriptInterface<class IRecallSimulationControllerInterface> SimulationController;

	UPROPERTY(Transient)
	FRecallInput PreviousInput;
	UPROPERTY(Transient)
	FRecallInput LastInput;
	UPROPERTY(Transient)
	TArray<FRecallInput> InputQueue;

	struct FRecallInputRequest
	{
		TArray<FRecallFrameInput> FrameInputs;
	};

	FRecallInputRequest InputRequest;

	bool IsOwnedByLocalPlayer() const;
	
	void OnInputPhase(uint32 Frame, const FString& PlayerId);

	void PushLocalFrameInputs(uint32 Frame, uint32 NextInputFrame, TArray<FRecallFrameInput>& OutFrameInputs);
	bool PushLocalInput(uint32 Frame, const FRecallInput& Input) const;

	void ResendInput();
	void SendInput(const TArray<FRecallFrameInput>& FrameInputs) const;

	UFUNCTION()
	void OnRep_SimPlayerId();

};
