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

#include "LeabraTICtxtConSpec.h"

TA_BASEFUNS_CTORS_DEFN(LeabraTICtxtConSpec);

void LeabraTICtxtConSpec::Initialize() {
  SetUnique("wt_scale", true);
  wt_scale.rel = 3.0;           // good default
}

void LeabraTICtxtConSpec::GetPrjnName(Projection& prjn, String& nm) {
  nm = "Ctxt_" + nm;
}

void LeabraTICtxtConSpec::Trial_Init_Specs(LeabraNetwork* net) {
  inherited::Trial_Init_Specs(net);
  net->net_misc.ti = true;
}
