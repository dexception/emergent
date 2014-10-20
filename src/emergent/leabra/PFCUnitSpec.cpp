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

#include "PFCUnitSpec.h"
#include <LeabraNetwork>

TA_BASEFUNS_CTORS_DEFN(PFCMaintSpec);
TA_BASEFUNS_CTORS_DEFN(PFCUnitSpec);

void PFCMaintSpec::Initialize() {
  Defaults_init();
}

void PFCMaintSpec::Defaults_init() {
  maint_last_row = -1;
  maint_d5b_gain = 0.8f;
  out_d5b_gain = 0.5f;
  d5b_updt_tau = 10.0f;
  
  d5b_updt_dt = 1.0f / d5b_updt_tau;
}

void PFCMaintSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  d5b_updt_dt = 1.0f / d5b_updt_tau;
}

void PFCUnitSpec::Initialize() {
  Defaults_init();
}

void PFCUnitSpec::Defaults_init() {
  act_avg.l_up_inc = 0.1f;       // needs a slower upside due to longer maintenance window..
  cifer.on = true;
  cifer.thal_thr = 0.1f;
  cifer.thal_bin = true;
  //  cifer.phase = true; // not yet..
  // todo: other cifer defaults
}

bool  PFCUnitSpec::ActiveMaint(LeabraUnit* u) {
  LeabraLayer* lay = (LeabraLayer*)u->own_lay();
  taVector2i ugpos = lay->UnitGpPosFmIdx(u->UnitGpIdx());
  int lst_row = pfc_maint.maint_last_row;
  if(lst_row < 0)
    lst_row = lay->gp_geom.y + lst_row;
  return (ugpos.y <= lst_row);
}

float PFCUnitSpec::Compute_NetinExtras(float& net_syn, LeabraUnit* u,
                                       LeabraNetwork* net, int thread_no) {
  float net_ex = inherited::Compute_NetinExtras(net_syn, u, net, thread_no);
  bool act_mnt = ActiveMaint(u);
  if(act_mnt) {
    net_ex += pfc_maint.maint_d5b_gain * u->deep5b;
  }
  else {
    net_ex += pfc_maint.out_d5b_gain * u->deep5b;
  }
  return net_ex;
}

void PFCUnitSpec::TI_Compute_Deep5bAct(LeabraUnit* u, LeabraNetwork* net) {
  if(!cifer.on) return;
  float act5b = u->act_eq;
  if(act5b < cifer.act5b_thr) {
    act5b = 0.0f;
  }
  act5b *= u->thal;

  bool act_mnt = ActiveMaint(u);
  if(act_mnt && u->thal_prv > 0.0f && u->thal > 0.0f) { // ongoing maintenance
    u->deep5b += pfc_maint.d5b_updt_dt * (act5b - u->deep5b);
  }
  else {                        // first update or off..
    u->deep5b = act5b;
  }

  u->thal_prv = u->thal;        // this is point of update
}

