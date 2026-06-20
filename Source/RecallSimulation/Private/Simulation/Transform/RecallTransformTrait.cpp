// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0


#include "Simulation/Transform/RecallTransformTrait.h"

#include "MassEntityTemplateRegistry.h"
#include "MassEntityView.h"
#include "Simulation/Transform/RecallTransformFragments.h"
#include "Utility/Trait/RecallTraitUtils.h"

//----------------------------------------------------------------------//
// URecallTransformTrait
//----------------------------------------------------------------------//
URecallTransformTrait::URecallTransformTrait()
	: Super()
{
}

void URecallTransformTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment<FRecallTransformFragment>();

	BuildContext.GetMutableObjectFragmentInitializers().Add([this](UObject& Owner,
		FMassEntityView& EntityView, const EMassTranslationDirection CurrentDirection)
		{
			if (const AActor* Actor = Recall::Trait::Utils::AsActor(Owner))
			{
				FRecallTransformFragment& TransformFragment = EntityView.GetFragmentData<FRecallTransformFragment>();
				TransformFragment.Position = Actor->GetActorLocation();

				if (bUseActorRotation)
				{
					TransformFragment.Rotation = Actor->GetActorRotation().Quaternion();
				}
			}
		}
	);
}
