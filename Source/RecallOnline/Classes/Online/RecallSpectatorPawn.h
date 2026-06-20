// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "GameFramework/SpectatorPawn.h"

#include "RecallSpectatorPawn.generated.h"

class URecallGameSimViewComponent;

/*
* Pawn to spectate game simulation pawns.
*/
UCLASS()
class RECALLONLINE_API ARecallSpectatorPawn :
	public ASpectatorPawn
{
	GENERATED_UCLASS_BODY()

public:
	void ViewPrevPlayer();
	void ViewNextPlayer();

	void ChangeViewPlayer(bool bNext) const;
	
	//~ Begin AActor Interface
public:
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TObjectPtr<URecallGameSimViewComponent> GameSimViewComponent;
	
};
