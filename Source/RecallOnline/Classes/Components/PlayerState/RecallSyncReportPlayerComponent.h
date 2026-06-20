// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/PlayerStateComponent.h"
#include "System/Simulation/Insight/RecallSimulationInsightTypes.h"

#include "RecallSyncReportPlayerComponent.generated.h"

/**
 * Component so players can share simulation sync report and make sure that they stay sync with each other.
 */
UCLASS()
class RECALLONLINE_API URecallSyncReportPlayerComponent : public UPlayerStateComponent
{
	GENERATED_UCLASS_BODY()

public:
	void UpdateDebugSimulationReport(const FRecallSimulationInsight& Report);
	void ClearDebugSimulationReport() { LocalDebugSimReport = FRecallSimulationInsight(); }

	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface
	
#pragma region RPC
protected:
	UFUNCTION(Server, Reliable)
	void Server_UpdateDebugSimulationReport(const FRecallSimulationInsight& Report);
#pragma endregion RPC
	
protected:
	UPROPERTY(Transient)
	TScriptInterface<class IRecallSimulationControllerInterface> SimulationController;
	
	/** Keep track of the simulation state, until the confirm frame, to compare it with other players. */
	UPROPERTY(Transient)
	FRecallSimulationInsight LocalDebugSimReport;
	
	void OnSimTickStart(float DeltaTime);
	
	void ShareDebugSyncedFrameReport();
	void UpdateDebugSyncedFrame();
};
