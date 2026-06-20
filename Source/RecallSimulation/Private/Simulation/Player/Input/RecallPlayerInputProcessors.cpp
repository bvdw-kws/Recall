// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "RecallPlayerInputProcessors.h"

#include "Data/Game/RecallGameRuleBaseAsset.h"
#include "Desync/RecallDesyncLog.h"
#include "MassExecutionContext.h"
#include "RecallSignalSubsystem.h"
#include "Simulation/GameplayTag/RecallGameplayTagFragments.h"
#include "Simulation/Controller/RecallControllerFragments.h"
#include "Simulation/Player/Input/RecallPlayerInputFragments.h"
#include "Simulation/Player/RecallPlayerProcessorGroupTypes.h"
#include "Simulation/Player/RecallPlayerSignalTypes.h"
#include "System/Game/RecallGameRuleSubsystem.h"
#include "System/Input/RecallInputQueueSubsystem.h"

//----------------------------------------------------------------------//
// URecallPlayerInputProcessor
//----------------------------------------------------------------------//
URecallPlayerInputProcessor::URecallPlayerInputProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteInGroup = Recall::Player::ProcessorGroupNames::Input;
}

struct FRecallPlayerInputCacheManager
{
	TArray<FMassEntityHandle> EntitiesToSignal;

	void ResetCache()
	{
		EntitiesToSignal.Reset();
	}
};

void URecallPlayerInputProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& InEntityManager)
{
	Super::InitializeInternal(Owner, InEntityManager);

	CacheManager = MakeShared<FRecallPlayerInputCacheManager>();
}

void URecallPlayerInputProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRecallPlayerInputFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRecallControllerFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRecallGameplayTagFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	EntityQuery.AddSubsystemRequirement<URecallInputQueueSubsystem>(EMassFragmentAccess::ReadOnly);

	ProcessorRequirements.AddSubsystemRequirement<URecallGameRuleSubsystem>(EMassFragmentAccess::ReadOnly);
	ProcessorRequirements.AddSubsystemRequirement<URecallSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URecallPlayerInputProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(Recall_CombatInput_Execute);

	check(CacheManager.IsValid());
	CacheManager->ResetCache();

	const URecallGameRuleSubsystem& GameRuleSystem = Context.GetSubsystemChecked<URecallGameRuleSubsystem>();
	const URecallGameRuleBaseAsset* GameRuleAsset = GameRuleSystem.GetGameRuleAsset<URecallGameRuleBaseAsset>();
	const bool bGameRuleBlockInput = IsValid(GameRuleAsset) && GameRuleAsset->IsBlockInputForMatchState(GameRuleSystem.GetMatchState());
	
	TArray<FMassEntityHandle>& EntitiesToSignal = CacheManager->EntitiesToSignal;

	EntityQuery.ForEachEntityChunk(Context, [&EntitiesToSignal, bGameRuleBlockInput](FMassExecutionContext& Context)
	{
		const URecallInputQueueSubsystem& InputQueueSystem = Context.GetSubsystemChecked<URecallInputQueueSubsystem>();

		const TConstArrayView<FRecallControllerFragment> PlayerControllerList = Context.GetFragmentView<FRecallControllerFragment>();
		const TConstArrayView<FRecallGameplayTagFragment> GameplayTagList = Context.GetFragmentView<FRecallGameplayTagFragment>();
		const TArrayView<FRecallPlayerInputFragment> PlayerInputList = Context.GetMutableFragmentView<FRecallPlayerInputFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); EntityIndex++)
		{
			const FRecallControllerFragment& PlayerControllerFragment = PlayerControllerList[EntityIndex];
			const FRecallGameplayTagFragment* const GameplayTagFragmentPtr = GameplayTagList.IsValidIndex(EntityIndex) ? &GameplayTagList[EntityIndex] : nullptr;
			FRecallPlayerInputFragment& PlayerInputFragment = PlayerInputList[EntityIndex];

			FRecallInput InputQueueItem;
			if (!ensureAlwaysMsgf(InputQueueSystem.GetFrameInput(PlayerControllerFragment.ControllerID, InputQueueItem, false),
				TEXT("%hs Should always have input."), __FUNCTION__))
			{
				continue;
			}
			
			if (bGameRuleBlockInput || InputQueueSystem.IsLockControl() ||
				(GameplayTagFragmentPtr != nullptr && GameplayTagFragmentPtr->GameplayTagCountMap.HasTag(State_BlockControl)))
			{
				InputQueueItem = FRecallInput();
			}

			const bool bWasAnyInputPressed = PlayerInputFragment.IsAnyInputPressed(RECALL_INPUT_BUFFER_MAX_DURATION);

			// Update axis
			PlayerInputFragment.LeftStickAxis.X = FMath::IsNearlyZero(FMath::Abs(InputQueueItem.LeftStickAxis.X)) ? 0.0f : InputQueueItem.LeftStickAxis.X;
			PlayerInputFragment.LeftStickAxis.Y = FMath::IsNearlyZero(FMath::Abs(InputQueueItem.LeftStickAxis.Y)) ? 0.0f : InputQueueItem.LeftStickAxis.Y;

			PlayerInputFragment.RightStickAxis.X = FMath::IsNearlyZero(FMath::Abs(InputQueueItem.RightStickAxis.X)) ? 0.0f : InputQueueItem.RightStickAxis.X;
			PlayerInputFragment.RightStickAxis.Y = FMath::IsNearlyZero(FMath::Abs(InputQueueItem.RightStickAxis.Y)) ? 0.0f : InputQueueItem.RightStickAxis.Y;

			// Combine buttons and analog input into one flag.
			const uint16 InputCommand = InputQueueItem.InputCommand;

			// Record positive edge inputs.
			PlayerInputFragment.PressedInputs = (PlayerInputFragment.Inputs ^ 0xFFFF) & InputCommand;

			// Record held inputs
			PlayerInputFragment.Inputs = InputCommand;

			// Update the current held input history index.
			PlayerInputFragment.HeldInputBackIndex = (PlayerInputFragment.HeldInputBackIndex + 1) % PlayerInputFragment.HeldInputHistory.Num();

			// Update held input buffer. We include the direction commands as well so they can be recognized when reading the input buffer.
			PlayerInputFragment.HeldInputHistory[PlayerInputFragment.HeldInputBackIndex] = PlayerInputFragment.Inputs;

			// Update input timers
			{
				uint16 InputIndex = 0;
				for (int32& Timer : PlayerInputFragment.PressedInputTimers)
				{
					if (Timer < RECALL_INPUT_BUFFER_MAX_DURATION)
					{
						Timer++;
					}

					// Counting time duration after a button is pressed.
					if (EnumHasAnyFlags(PlayerInputFragment.PressedInputs, static_cast<ERecallControllerInputCommand>(1 << InputIndex)))
					{
						Timer = 0;
					}

					InputIndex++;
				}
			}

			// Update input axis
			{
				PlayerInputFragment.AxisInputMap.Reset();
				
				for (const TPair<uint16, FVector2D_NetQuantizeDirection>& AxisInput : InputQueueItem.AxisInputMap)
				{
					PlayerInputFragment.AxisInputMap.Add(AxisInput.Key, FVector2f(AxisInput.Value.X, AxisInput.Value.Y));
				}
			}

			if (PlayerInputFragment.IsAnyInputPressed(RECALL_INPUT_BUFFER_MAX_DURATION) || bWasAnyInputPressed)
			{
				EntitiesToSignal.Add(Context.GetEntity(EntityIndex));
			}

#if RECALL_DESYNC_LOG
			RECALL_DESYNC_LOG_STR(Context.GetWorld(), ControllerID, PlayerControllerFragment.ControllerID);
			RECALL_DESYNC_LOG_INT(Context.GetWorld(), Inputs, PlayerInputFragment.Inputs);
			RECALL_DESYNC_LOG_INT(Context.GetWorld(), PressedInputs, PlayerInputFragment.PressedInputs);
			RECALL_DESYNC_LOG_VEC(Context.GetWorld(), LeftStickAxis, PlayerInputFragment.LeftStickAxis);
			RECALL_DESYNC_LOG_VEC(Context.GetWorld(), RightStickAxis, PlayerInputFragment.RightStickAxis);
			RECALL_DESYNC_LOG_VEC(Context.GetWorld(), MousePosition, InputQueueItem.MousePosition.ToDVector());
			RECALL_DESYNC_LOG_STR(Context.GetWorld(), Options, InputQueueItem.Options);
#endif // RECALL_DESYNC_LOG
		}
	});

	if (EntitiesToSignal.Num() > 0)
	{
		URecallSignalSubsystem& SignalSystem = Context.GetMutableSubsystemChecked<URecallSignalSubsystem>();
		SignalSystem.SignalEntities(Recall::Player::Signals::InputPressed, EntitiesToSignal);
	}
}
