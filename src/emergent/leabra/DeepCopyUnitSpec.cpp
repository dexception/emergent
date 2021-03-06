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

#include "DeepCopyUnitSpec.h"
#include <LeabraNetwork>

TA_BASEFUNS_CTORS_DEFN(DeepCopyUnitSpec);

void DeepCopyUnitSpec::Initialize() {
  deep_var = DEEP_RAW;
  Defaults_init();
}

void DeepCopyUnitSpec::Defaults_init() {
}

bool DeepCopyUnitSpec::CheckConfig_Unit(Layer* lay, bool quiet) {
  bool rval = inherited::CheckConfig_Unit(lay, quiet);

  LeabraNetwork* net = (LeabraNetwork*)lay->own_net;

  if(lay->units.leaves == 0) return rval;
  LeabraUnit* un = (LeabraUnit*)lay->units.Leaf(0); // take first one
  
  LeabraConGroup* cg = (LeabraConGroup*)un->RecvConGroupSafe(0);
  if(lay->CheckError(!cg, quiet, rval,
                     "Requires one recv projection!")) {
    return false;
  }
  LeabraUnit* su = (LeabraUnit*)cg->SafeUn(0);
  if(lay->CheckError(!su, quiet, rval, 
                     "Requires one unit in recv projection!")) {
    return false;
  }

  return rval;
}

void DeepCopyUnitSpec::Compute_ActFmSource(LeabraUnitVars* u, LeabraNetwork* net,
                                         int thr_no) {
  LeabraConGroup* cg = (LeabraConGroup*)u->RecvConGroupSafe(net, thr_no, 0);
  LeabraUnitVars* su = (LeabraUnitVars*)cg->UnVars(0, net);
  LeabraLayer* fmlay = (LeabraLayer*)cg->prjn->from.ptr();
  if(fmlay->lesioned()) {
    u->act = 0.0f;
    return;
  }
  float var = 0.0f;
  switch(deep_var) {
  case DEEP_RAW:
    var = su->deep_raw;
    break;
  case DEEP_MOD:
    var = su->deep_mod;
    break;
  case DEEP_LRN:
    var = su->deep_lrn;
    break;
  case DEEP_CTXT:
    var = su->deep_ctxt;
    break;
  }
  u->act = var;
  u->act_eq = u->act_nd = u->act;
  TestWrite(u->da, 0.0f);
  // u->AddToActBuf(syn_delay); // todo:

  if(deep.on && Quarter_DeepRawNow(net->quarter)) {
    Compute_DeepRaw(u, net, thr_no);
  }
}

void DeepCopyUnitSpec::Compute_Act_Rate(LeabraUnitVars* u, LeabraNetwork* net, int thr_no) {
  Compute_ActFmSource(u, net, thr_no);
}

void DeepCopyUnitSpec::Compute_Act_Spike(LeabraUnitVars* u, LeabraNetwork* net, int thr_no) {
  Compute_ActFmSource(u, net, thr_no);
}

