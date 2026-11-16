// Copyright (C) 2024 Van de Walle Bastien
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "CoreMinimal.h"

namespace Recall::Camera::ProcessorGroupNames
{

	/** Updates the player camera entity transform. The camera transform is final once this group is done. */
	const FName Follow = FName(TEXT("RecallPlayerCameraFollow"));

} // namespace Recall::Camera::ProcessorGroupNames
