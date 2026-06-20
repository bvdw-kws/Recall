// This file is part of RoundCornerBox, a single c++ header for the creation of
// round-corner boxes.
//
// Copyright (C) 2016 Raymond Fei <fyun@acm.org>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "CoreMinimal.h"

namespace Recall::Geometry::Utils
{
	
RECALLPHYSICSMODULE_API extern void CreateRoundCornerBox(int32 N, const FVector& Extents, float EdgeRadius,
	TArray<FVector>& OutVertices, TArray<int32>& OutIndices);
	
} // namespace Recall::Geometry::Utils
