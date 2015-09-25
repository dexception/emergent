// Copyright, 1995-2013, Regents of the University of Colorado,
// Carnegie Mellon University, Princeton University.
//
// This file is part of Emergent
//
//   Emergent is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   Emergent is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.

#include "D1D2UnitSpec.h"

TA_BASEFUNS_CTORS_DEFN(D1D2UnitSpec);

void D1D2UnitSpec::Initialize() {
}

void D1D2UnitSpec::Defaults_init() {
  deep.d_to_d = 1.0f;
  deep.thal_to_d = 0.0f;
  deep_qtr = QALL;
  deep_norm.raw_val = DeepNormSpec::NORM_NET;
  deep_norm.mod = true;
  deep_norm.immed = true;
}
