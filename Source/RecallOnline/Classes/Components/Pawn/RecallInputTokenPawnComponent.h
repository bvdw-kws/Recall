// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "Components/PawnComponent.h"

#include "RecallInputTokenPawnComponent.generated.h"

/**
 * Component to handle input tokens.
 * These tokens are made to limit how many inputs a client can send to other players.
 */
UCLASS()
class RECALLONLINE_API URecallInputTokenPawnComponent : public UPawnComponent
{
	GENERATED_UCLASS_BODY()

public:
	bool ConsumeInputToken();
	void RegenerateInputToken();
	int32 GetInputTokenCount() const { return InputTokenCount; }

	//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//~ End UActorComponent Interface

protected:
	/**
	 * How many inputs we can send, to not spam the other player.
	 */
	UPROPERTY(Transient)
	int32 InputTokenCount = 0;

	/**
	 * Timer used to regenerate input token once its value is above RegenerateInputTokenFrequency.
	 */
	UPROPERTY(Transient)
	float RegenerateInputTokenTimer{ 0.0f };

};
