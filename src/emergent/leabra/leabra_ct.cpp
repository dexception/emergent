// Copyright, 1995-2007, Regents of the University of Colorado,
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

#include "leabra_ct.h"

// #include "netstru_extra.h"

#include <math.h>
#include <limits.h>
#include <float.h>

//////////////////////////////////
// 	Ct cons
//////////////////////////////////

void CtCaDepSpec::Initialize() {
  intwt_dt = 0.001f;
  ca_inc = .01f;
  ca_dec = .01f;
  sd_ca_thr = 0.2f;
  sd_ca_gain = 0.3f;
  sd_ca_thr_rescale = sd_ca_gain / (1.0f - sd_ca_thr);
}

void CtCaDepSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  sd_ca_thr_rescale = sd_ca_gain / (1.0f - sd_ca_thr);
}

void CtLearnSpec::Initialize() {
  sravg_dt = 0.1f;
  use_sravg_m = false;
}

void CtLearnSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
}

void CtLeabraConSpec::Initialize() {
  min_obj_type = &TA_CtLeabraCon;
  savg_cor.thresh = -1.0f;
}

void CtLeabraConSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  ca_dep.UpdateAfterEdit();
}

void CtLeabraRecvCons::Initialize() {
  SetConType(&TA_CtLeabraCon);
}

void CtLeabraBiasSpec::Initialize() {
  //  min_obj_type = &TA_CtLeabraCon;
}

//////////////////////////////////
// 	Ct Unit
//////////////////////////////////

void CtLeabraUnit::Initialize() {
  bias.con_type = &TA_CtLeabraCon;
  cai_avg = 0.0f;
  cai_max = 0.0f;
  syndep_avg = 0.0f;
  syndep_max = 0.0f;
}

void CtLeabraUnitSpec::Initialize() {
  min_obj_type = &TA_CtLeabraUnit;
  bias_con_type = &TA_CtLeabraCon;
  opt_thresh.learn = -1.0f;
  opt_thresh.updt_wts = false;
}

void CtLeabraUnitSpec::Compute_CtCycle(CtLeabraUnit* u, CtLeabraLayer*, CtLeabraNetwork* net) {
  CtLeabraConSpec* cspec = NULL;
  u->cai_avg = 0.0f;
  u->cai_max = 0.0f;
  for(int g=0; g<u->recv.size; g++) {
    CtLeabraRecvCons* recv_gp = (CtLeabraRecvCons*)u->recv.FastEl(g);
    if(recv_gp->prjn->from->lesioned() || !recv_gp->cons.size) continue;
    recv_gp->Compute_CtCycle(u, u->cai_avg, u->cai_max);
    cspec = (CtLeabraConSpec*)recv_gp->GetConSpec();
  }
  //  ((CtLeabraBiasSpec*)bias_spec.SPtr())->B_Compute_CtCycle((CtLeabraCon*)u->bias.Cn(0), u);
  if(u->n_recv_cons > 0)
    u->cai_avg /= (float)u->n_recv_cons;
  if(cspec) {
    u->syndep_avg = 1.0f - cspec->ca_dep.SynDep(u->cai_avg);
    u->syndep_max = 1.0f - cspec->ca_dep.SynDep(u->cai_max);
  }
}

void CtLeabraUnitSpec::Compute_SrAvg(CtLeabraUnit* u, CtLeabraLayer*, CtLeabraNetwork* net) {
  for(int g=0; g<u->recv.size; g++) {
    CtLeabraRecvCons* recv_gp = (CtLeabraRecvCons*)u->recv.FastEl(g);
    if(recv_gp->prjn->from->lesioned() || !recv_gp->cons.size) continue;
    recv_gp->Compute_SrAvg(u);
  }
  // todo:
  //  ((CtLeabraBiasSpec*)bias_spec.SPtr())->B_Compute_SrAvg((CtLeabraCon*)u->bias.Cn(0), u);
}

void CtLeabraUnitSpec::Compute_dWtFlip(CtLeabraUnit* u, CtLeabraLayer*, CtLeabraNetwork* net) {
  for(int g=0; g<u->recv.size; g++) {
    CtLeabraRecvCons* recv_gp = (CtLeabraRecvCons*)u->recv.FastEl(g);
    if(recv_gp->prjn->from->lesioned() || !recv_gp->cons.size) continue;
    recv_gp->Compute_dWtFlip(u);
  }
  // todo:
  //  ((CtLeabraBiasSpec*)bias_spec.SPtr())->B_Compute_dWtFlip((CtLeabraCon*)u->bias.Cn(0), u);
}

void CtLeabraUnitSpec::Compute_ActMP(CtLeabraUnit* u, CtLeabraLayer*, CtLeabraNetwork*) {
  u->p_act_m = u->p_act_p;	// update activations for learning
  u->p_act_p = u->act_eq;
}

void CtLeabraUnitSpec::Compute_ActM(CtLeabraUnit* u, CtLeabraLayer*, CtLeabraNetwork*) {
  u->p_act_m = u->act_eq;
}

void CtLeabraUnitSpec::Compute_ActP(CtLeabraUnit* u, CtLeabraLayer*, CtLeabraNetwork*) {
  u->p_act_p = u->act_eq;
}

//////////////////////////////////
// 	Ct Layer
//////////////////////////////////

void CtLeabraLayer::Initialize() {
  spec.SetBaseType(&TA_CtLeabraLayerSpec);
  units.SetBaseType(&TA_CtLeabraUnit);
  unit_spec.SetBaseType(&TA_CtLeabraUnitSpec);
}  

void CtLeabraLayerSpec::Initialize() {
  min_obj_type = &TA_CtLeabraLayer;
  decay.event = 0.0f;
  decay.phase = 0.0f;
}

void CtLeabraLayerSpec::Compute_CtCycle(CtLeabraLayer* lay, CtLeabraNetwork* net) {
  if(lay->hard_clamped) return;	// this is key!  clamped layers do not age!

  CtLeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(CtLeabraUnit, u, lay->units., i) {
    u->Compute_CtCycle(lay, net);
  }
}

void CtLeabraLayerSpec::Compute_SrAvg(CtLeabraLayer* lay, CtLeabraNetwork* net) {
  CtLeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(CtLeabraUnit, u, lay->units., i) {
    u->Compute_SrAvg(lay, net);
  }
}

void CtLeabraLayerSpec::Compute_dWtFlip(CtLeabraLayer* lay, CtLeabraNetwork* net) {
  CtLeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(CtLeabraUnit, u, lay->units., i) {
    u->Compute_dWtFlip(lay, net);
  }
}

void CtLeabraLayerSpec::Compute_ActMP(CtLeabraLayer* lay, CtLeabraNetwork* net) {
  CtLeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(CtLeabraUnit, u, lay->units., i) {
    u->Compute_ActMP(lay, net);
  }
}

void CtLeabraLayerSpec::Compute_ActM(CtLeabraLayer* lay, CtLeabraNetwork* net) {
  CtLeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(CtLeabraUnit, u, lay->units., i) {
    u->Compute_ActM(lay, net);
  }
}

void CtLeabraLayerSpec::Compute_ActP(CtLeabraLayer* lay, CtLeabraNetwork* net) {
  CtLeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(CtLeabraUnit, u, lay->units., i) {
    u->Compute_ActP(lay, net);
  }
}


//////////////////////////////////
// 	Ct Network
//////////////////////////////////

void CtLeabraNetwork::Initialize() {
  layers.SetBaseType(&TA_CtLeabraLayer);
  cycles_per_tick = 10;
}

void CtLeabraNetwork::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
}

void CtLeabraNetwork::SetProjectionDefaultTypes(Projection* prjn) {
  inherited::SetProjectionDefaultTypes(prjn);
  prjn->con_type = &TA_CtLeabraCon;
  prjn->recvcons_type = &TA_CtLeabraRecvCons;
  prjn->sendcons_type = &TA_LeabraSendCons;
  prjn->con_spec.SetBaseType(&TA_CtLeabraConSpec);
}

void CtLeabraNetwork::Compute_CtCycle() {
  CtLeabraLayer* lay;
  taLeafItr l;
  FOR_ITR_EL(CtLeabraLayer, lay, layers., l) {
    if(lay->lesioned())	continue;
    lay->Compute_CtCycle(this);
  }
}
  
void CtLeabraNetwork::Compute_SrAvg() {
  CtLeabraLayer* lay;
  taLeafItr l;
  FOR_ITR_EL(CtLeabraLayer, lay, layers., l) {
    if(lay->lesioned())	continue;
    lay->Compute_SrAvg(this);
  }
}
  
void CtLeabraNetwork::Compute_dWtFlip() {
  CtLeabraLayer* lay;
  taLeafItr l;
  FOR_ITR_EL(CtLeabraLayer, lay, layers., l) {
    if(lay->lesioned())	continue;
    lay->Compute_dWtFlip(this);
  }
}
  
void CtLeabraNetwork::Cycle_Run() {
  inherited::Cycle_Run();
  Compute_CtCycle();
  // program code needs to do this after incrementing time++:
//   if(time % cycles_per_tick == 0) {
//     tick++;
//     Compute_dWt();
//   }
}

void CtLeabraNetwork::Compute_ActMP() {
  CtLeabraLayer* lay;
  taLeafItr l;
  FOR_ITR_EL(CtLeabraLayer, lay, layers., l) {
    if(lay->lesioned())	continue;
    lay->Compute_ActMP(this);
  }
}

void CtLeabraNetwork::Compute_ActM() {
  CtLeabraLayer* lay;
  taLeafItr l;
  FOR_ITR_EL(CtLeabraLayer, lay, layers., l) {
    if(lay->lesioned())	continue;
    lay->Compute_ActM(this);
  }
}
void CtLeabraNetwork::Compute_ActP() {
  CtLeabraLayer* lay;
  taLeafItr l;
  FOR_ITR_EL(CtLeabraLayer, lay, layers., l) {
    if(lay->lesioned())	continue;
    lay->Compute_ActP(this);
  }
}
  

////////////////////////////////////////////////
//		TODO

