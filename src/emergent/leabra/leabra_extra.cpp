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

#include "leabra_extra.h"

#include "netstru_extra.h"
#include "ta_dataproc.h"
#include "ta_dataanal.h"

//////////////////////////////////
//  	MarkerConSpec   	//
//////////////////////////////////

void MarkerConSpec::Initialize() {
  SetUnique("rnd", true);
  rnd.mean = 0.0f; rnd.var = 0.0f;
  SetUnique("wt_limits", true);
  wt_limits.sym = false;
  SetUnique("wt_scale", true);
  wt_scale.rel = 0.0f;
  SetUnique("lrate", true);
  lrate = 0.0f;
  cur_lrate = 0.0f;
}

//////////////////////////////////
//  	ContextLayerSpec	//
//////////////////////////////////

void CtxtUpdateSpec::Initialize() {
  fm_hid = 1.0f;
  fm_prv = 0.0f;
  to_out = 1.0f;
}

const String
LeabraContextLayerSpec::do_update_key("LeabraContextLayerSpec__do_update");

void LeabraContextLayerSpec::Initialize() {
  updt.fm_prv = 0.0f;
  updt.fm_hid = 1.0f;
  updt.to_out = 1.0f;
  SetUnique("decay", true);
  decay.event = 0.0f;
  decay.phase = 0.0f;
  update_criteria = UC_TRIAL;
}

// void LeabraContextLayerSpec::UpdateAfterEdit-impl() {
//   inherited::UpdateAfterEdit_impl();
//   hysteresis_c = 1.0f - hysteresis;
// }

bool LeabraContextLayerSpec::CheckConfig_Layer(Layer* ly, bool quiet) {
  LeabraLayer* lay = (LeabraLayer*)ly;
  bool rval = inherited::CheckConfig_Layer(lay, quiet);

//   LeabraNetwork* net = (LeabraNetwork*)lay->own_net;
  return rval;
}

void LeabraContextLayerSpec::Defaults() {
  inherited::Defaults();
  Initialize();
}

taBase::DumpQueryResult LeabraContextLayerSpec::Dump_QuerySaveMember(MemberDef* md) {
  // only save n_spec if needed (to ease backwards compat)
  if (md->name != "n_spec") 
    return inherited::Dump_QuerySaveMember(md);
  return (update_criteria == UC_N_TRIAL) ? DQR_SAVE : DQR_NO_SAVE;
}

void LeabraContextLayerSpec::Compute_Context(LeabraLayer* lay, LeabraUnit* u, LeabraNetwork* net) {
  if(net->phase == LeabraNetwork::PLUS_PHASE) {
    u->ext = u->act_m;		// just use previous minus phase value!
  }
  else {
    LeabraRecvCons* cg = (LeabraRecvCons*)u->recv.SafeEl(0);
    if(TestError(!cg, "Compute_Context", "requires one recv projection!")) {
      return;
    }
    LeabraUnit* su = (LeabraUnit*)cg->Un(0);
    if(TestError(!su, "Compute_Context", "requires one unit in recv projection!")) {
      return;
    }
    u->ext = updt.fm_prv * u->act_p + updt.fm_hid * su->act_p; // compute new value
  }
  u->SetExtFlag(Unit::EXT);
  u->Compute_HardClamp(net);
}

void LeabraContextLayerSpec::Compute_HardClamp(LeabraLayer* lay, LeabraNetwork* net) {
  lay->hard_clamped = true;	// cache this flag
  lay->SetExtFlag(Unit::EXT);
  bool do_update = lay->GetUserDataDef(do_update_key, false).toBool();
  if (do_update) {
    lay->SetUserData(do_update_key, false); // reset
  } else { // not explicit triger, so try other conditions
    switch (update_criteria) {
    case UC_TRIAL:
      do_update = true;
      break;
    case UC_MANUAL: break; // weren't triggered, so that's it
    case UC_N_TRIAL: {
      // do modulo the trial, adding offset -- add 1 so first trial is not trigger
      do_update = (((net->trial + n_spec.n_offs + 1) % n_spec.n_trials) == 0);
    } break;
    }
  }
  if (!do_update) return;
  
  lay->Inhib_SetVals(inhib.kwta_pt);		// assume 0 - 1 clamped inputs

  LeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(LeabraUnit, u, lay->units., i) {
    Compute_Context(lay, u, net);
  }
  Compute_CycleStats(lay, net);
}

void LeabraContextLayerSpec::TriggerUpdate(LeabraLayer* lay) {
  if (!lay) return;
  if (TestError((lay->spec.spec.ptr() != this),
    "TriggerUpdate", "Spec does not belong to the layer passed as arg"))
    return;
  lay->SetUserData(do_update_key, true);
}

void LeabraLayer::TriggerContextUpdate() {
  LeabraContextLayerSpec* cls = dynamic_cast<LeabraContextLayerSpec*>
    (spec.spec.ptr());
  if (cls) {
    cls->TriggerUpdate(this);
  }
}

//////////////////////////////////////////
// 	Misc Special Objects		//
//////////////////////////////////////////

//////////////////////////////////
// 	Linear Unit		//
//////////////////////////////////

void LeabraLinUnitSpec::Initialize() {
  SetUnique("act_fun", true);
  SetUnique("act_range", true);
  SetUnique("clamp_range", true);
  SetUnique("act", true);
  act_fun = LINEAR;
  act_range.max = 20;
  act_range.min = 0;
  act_range.UpdateAfterEdit();
  clamp_range.max = 1.0f;
  clamp_range.UpdateAfterEdit();
  act.gain = 2;
}

void LeabraLinUnitSpec::Defaults() {
  LeabraUnitSpec::Defaults();
  Initialize();
}

void LeabraLinUnitSpec::Compute_ActFmVm(LeabraUnit* u, LeabraNetwork* net) {
  float new_act = u->net * act.gain; // use linear netin as act

  u->da = new_act - u->act;
  if((noise_type == ACT_NOISE) && (noise.type != Random::NONE) && (net->cycle >= 0)) {
    new_act += Compute_Noise(u, net);
  }
  u->act = u->act_nd = u->act_eq = act_range.Clip(new_act);
}

//////////////////////////
// 	NegBias		//
//////////////////////////

void LeabraNegBiasSpec::Initialize() {
  decay = 0.0f;
  updt_immed = false;
}

//////////////////////////////////
// 	TrialSynDepConSpec	//
//////////////////////////////////

void TrialSynDepSpec::Initialize() {
  rec = 1.0f;
  depl = 1.1f;
}

void TrialSynDepConSpec::Initialize() {
  min_obj_type = &TA_TrialSynDepCon;
}

void TrialSynDepConSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  if(syn_dep.rec <= 0.0f)	// can't go to zero!
    syn_dep.rec = 1.0f;
}

//////////////////////////////////
// 	CycleSynDepConSpec	//
//////////////////////////////////

void CycleSynDepSpec::Initialize() {
  rec = 0.002f;
  asymp_act = 0.4f;
  depl = rec * (1.0f - asymp_act); // here the drive is constant
}

void CycleSynDepSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();

  if(rec < .00001f) rec = .00001f;
  // chg = rec * (1 - cur) - dep * drive = 0; // equilibrium point
  // rec * (1 - cur) = dep * drive
  // dep = rec * (1 - cur) / drive
  depl = rec * (1.0f - asymp_act); // here the drive is constant
  depl = MAX(depl, 0.0f);
}

void CycleSynDepConSpec::Initialize() {
  min_obj_type = &TA_CycleSynDepCon;
}

void CycleSynDepConSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  syn_dep.UpdateAfterEdit();
}

//////////////////////////////////
// 	CaiSynDepCon
//////////////////////////////////

void CaiSynDepSpec::Initialize() {
  ca_inc = .2f;			// base per-cycle is .01
  ca_dec = .2f;			// base per-cycle is .01
  sd_ca_thr = 0.2f;
  sd_ca_gain = 0.3f;
  sd_ca_thr_rescale = sd_ca_gain / (1.0f - sd_ca_thr);
}

void CaiSynDepSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  sd_ca_thr_rescale = sd_ca_gain / (1.0f - sd_ca_thr);
}

void CaiSynDepConSpec::Initialize() {
  min_obj_type = &TA_CaiSynDepCon;
}

void CaiSynDepConSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  ca_dep.UpdateAfterEdit();
}


//////////////////////////////////
// 	FastWtConSpec		//
//////////////////////////////////

void FastWtSpec::Initialize() {
  lrate = .05f;
  use_lrs = false;
  cur_lrate = .05f;
  decay = 1.0f;
  slw_sat = true;
  dk_mode = SU_THR;
}

void FastWtConSpec::Initialize() {
  min_obj_type = &TA_FastWtCon;
}

void FastWtConSpec::SetCurLrate(LeabraNetwork* net, int epoch) {
  LeabraConSpec::SetCurLrate(net, epoch);
  if(fast_wt.use_lrs)
    fast_wt.cur_lrate = fast_wt.lrate * lrate_sched.GetVal(epoch);
  else
    fast_wt.cur_lrate = fast_wt.lrate;
}

///////////////////////////////////////////////////////////////
//   ActAvgHebbConSpec

void ActAvgHebbMixSpec::Initialize() {
  act_avg = .5f;
  cur_act = .5f;
}

void ActAvgHebbMixSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  cur_act = 1.0f - act_avg;
}

void ActAvgHebbConSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  act_avg_hebb.UpdateAfterEdit();
}

void ActAvgHebbConSpec::Initialize() {
}

///////////////////////////////////////////////////////////////
//   LeabraDeltaConSpec

void LeabraDeltaConSpec::Initialize() {
  SetUnique("lmix", true);
  lmix.hebb = 0.0f;
  lmix.err = 1.0f;
//   lmix.err_sb = false;

  SetUnique("wt_limits", true);
  wt_limits.sym = false;

  SetUnique("wt_sig", true);
  wt_sig.gain = 1.0f;  wt_sig.off = 1.0f;

//   SetUnique("xcalm", true);
//   xcalm.use_sb = false;
}

void LeabraDeltaConSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  // these are enforced absolutely because the code does not use them:
  lmix.hebb = 0.0f;
  lmix.err = 1.0f;
}

///////////////////////////////////////////////////////////////
//   LeabraXCALSpikeConSpec

void XCALSpikeSpec::Initialize() {
  ca_norm = 5.0f;
  k_ca = 0.3f / ca_norm;
  ca_vgcc = 1.3f / ca_norm;
  ca_v_nmda = 0.0223f / ca_norm;
  ca_nmda = 0.5 / ca_norm;
  ca_dt = 20.0f;
  ca_rate = 1.0f / ca_dt;
  ca_off = 0.1f;
  nmda_dt = 40.0f;
  nmda_rate = 1.0f / nmda_dt;
  ss_dt = 0.2f;
  ss_time = 1.0f / ss_dt;
}

void XCALSpikeSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  k_ca = 0.3f / ca_norm;
  ca_vgcc = 1.3f / ca_norm;
  ca_v_nmda = 0.0223f / ca_norm;
  ca_nmda = 0.5 / ca_norm;
  ca_rate = 1.0f / ca_dt;
  nmda_rate = 1.0f / nmda_dt;
  ss_time = 1.0f / ss_dt;
}

void LeabraXCALSpikeConSpec::Initialize() {
  min_obj_type = &TA_LeabraSpikeCon;
}

void LeabraXCALSpikeConSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  xcal_spike.UpdateAfterEdit();
}

void LeabraXCALSpikeConSpec::GraphXCALSpikeSim(DataTable* graph_data,
					       LeabraUnitSpec* unit_spec,
					       float rate_min, float rate_max, float rate_inc,
					       float max_time, int reps_per_point,
					       float lin_norm) {
  taProject* proj = GET_MY_OWNER(taProject);
  if(!graph_data) {
    graph_data = proj->GetNewAnalysisDataTable(name + "_XCALSpikeSim", true);
  }

  bool local_us = false;
  if(!unit_spec) {
    unit_spec = new LeabraUnitSpec;
    local_us = true;
  }

  String sim_data_name = name + "_XCALSpikeSim_Tmp";
  DataTable* sim_data = proj->GetNewAnalysisDataTable(sim_data_name, true);

  sim_data->StructUpdate(true);
//   graph_data->ResetData();
  int idx;

//   DataTable sim_data;
//   taBase::Ref(sim_data);
  DataCol* sim_r_rate = sim_data->FindMakeColName("r_rate", idx, VT_FLOAT);
  DataCol* sim_s_rate = sim_data->FindMakeColName("s_rate", idx, VT_FLOAT);
  DataCol* sim_ca_avg = sim_data->FindMakeColName("ca_avg", idx, VT_FLOAT);
  DataCol* sim_sravg_ss = sim_data->FindMakeColName("sravg_ss", idx, VT_FLOAT);
  DataCol* sim_sravg_s = sim_data->FindMakeColName("sravg_s", idx, VT_FLOAT);
  DataCol* sim_sravg_m = sim_data->FindMakeColName("sravg_m", idx, VT_FLOAT);
  DataCol* sim_srprod_s = sim_data->FindMakeColName("srprod_s", idx, VT_FLOAT);
  DataCol* sim_srprod_m = sim_data->FindMakeColName("srprod_m", idx, VT_FLOAT);
  DataCol* sim_sravg_lin = sim_data->FindMakeColName("sravg_lin", idx, VT_FLOAT);

  float s_rate, r_rate;
  for(r_rate = rate_min; r_rate <= rate_max; r_rate += rate_inc) {
    for(s_rate = rate_min; s_rate <= rate_max; s_rate += rate_inc) {
      for(int rep=0; rep < reps_per_point; rep++) {
	float nmda = 0.0f;
	float ca = 0.0f;
	float ca_avg = 0.0f;
	float ca_sum = 0.0f;
	float vmd = 0.0f;
	float r_p = r_rate / 1000.0f;
	float s_p = s_rate / 1000.0f;
	float time = 0.0f;
	bool s_act = false;
	bool r_act = false;
	float s_avg = 0.0f;
	float r_avg = 0.0f;
	float s_lin = 0.0f;
	float r_lin = 0.0f;
	float s_avg_s = 0.15f;
	float s_avg_m = 0.15f;
	float r_avg_s = 0.15f;
	float r_avg_m = 0.15f;
	float sravg_ss = 0.15f;
	float sravg_s = 0.15f;
	float sravg_m = 0.15f;
	for(time = 0.0f; time < max_time; time += 1.0f) {
	  s_act = (bool)Random::Poisson(s_p);
	  r_act = (bool)Random::Poisson(r_p);
	  if(r_act) {
	    vmd += unit_spec->spike_misc.vm_dend;
	    r_avg += 1.0f;
	  }
	  vmd -= vmd / unit_spec->spike_misc.vm_dend_dt;
	  float dnmda = -nmda * xcal_spike.nmda_rate;
	  float dca = (nmda * (xcal_spike.ca_v_nmda * vmd + xcal_spike.ca_nmda))
	    - (ca * xcal_spike.ca_rate);
	  if(s_act) { s_avg += 1.0f; dnmda += xcal_spike.k_ca / (xcal_spike.k_ca + ca); }
	  if(r_act) { dca += xcal_spike.ca_vgcc; }
	  nmda += dnmda;
	  ca += dca;
	  ca_sum += ca;

	  float sr = (ca - xcal_spike.ca_off); if(sr < 0.0f) sr = 0.0f;
	  sravg_ss += xcal_spike.ss_dt * (sr - sravg_ss);
	  sravg_s += unit_spec->act_avg.s_dt * (sravg_ss - sravg_s);
	  sravg_m += unit_spec->act_avg.m_dt * (sravg_s - sravg_m);

	  r_avg_s += unit_spec->act_avg.s_dt * ((float)r_act - r_avg_s);
	  r_avg_m += unit_spec->act_avg.m_dt * (r_avg_s - r_avg_m);
	  s_avg_s += unit_spec->act_avg.s_dt * ((float)s_act - s_avg_s);
	  s_avg_m += unit_spec->act_avg.m_dt * (s_avg_s - s_avg_m);
	}
	ca_avg = ca_sum / max_time;
	s_lin *= lin_norm;
	r_lin *= lin_norm;
	float sravg_lin = s_avg * r_avg;

	float srprod_s = r_avg_s * s_avg_s;
	float srprod_m = r_avg_m * s_avg_m;

	sim_data->AddBlankRow();
	sim_r_rate->SetValAsFloat(r_rate, -1);
	sim_s_rate->SetValAsFloat(s_rate, -1);
	sim_ca_avg->SetValAsFloat(ca_avg, -1);
	sim_sravg_ss->SetValAsFloat(sravg_ss, -1);
	sim_sravg_s->SetValAsFloat(sravg_s, -1);
	sim_sravg_m->SetValAsFloat(sravg_m, -1);
	sim_srprod_s->SetValAsFloat(srprod_s, -1);
	sim_srprod_m->SetValAsFloat(srprod_m, -1);
	sim_sravg_lin->SetValAsFloat(sravg_lin, -1);
      }
    }
  }

  if(local_us) {
    delete unit_spec;
  }

  sim_data->StructUpdate(false);

  DataGroupSpec dgs;
  taBase::Ref(dgs);
  dgs.append_agg_name = false;
  //  dgs.SetDataTable(sim_data);
  dgs.AddAllColumns(sim_data);
  dgs.ClearColumns();

  ((DataGroupEl*)dgs.ops[0])->agg.op = Aggregate::GROUP; // r_rate
  ((DataGroupEl*)dgs.ops[1])->agg.op = Aggregate::GROUP; // s_rate
  for(int i=2; i< dgs.ops.size; i++)
    ((DataGroupEl*)dgs.ops[i])->agg.op = Aggregate::MEAN;

  taDataProc::Group(graph_data, sim_data, &dgs);
  dgs.ClearColumns();
  taDataAnal::Matrix3DGraph(graph_data, "s_rate", "r_rate");

  DataCol* gp_r_rate = graph_data->FindMakeColName("r_rate", idx, VT_FLOAT);
  gp_r_rate->SetUserData("X_AXIS", true);
  DataCol* gp_s_rate = graph_data->FindMakeColName("s_rate", idx, VT_FLOAT);
  gp_s_rate->SetUserData("Z_AXIS", true);
  DataCol* gp_sravg_m = graph_data->FindMakeColName("sravg_m", idx, VT_FLOAT);
  gp_sravg_m->SetUserData("PLOT_1", true);
  DataCol* gp_sravg_s = graph_data->FindMakeColName("sravg_s", idx, VT_FLOAT);
  gp_sravg_s->SetUserData("PLOT_2", true);
  DataCol* gp_srprod_m = graph_data->FindMakeColName("srprod_m", idx, VT_FLOAT);
  gp_srprod_m->SetUserData("PLOT_3", true);
  DataCol* gp_srprod_s = graph_data->FindMakeColName("srprod_s", idx, VT_FLOAT);
  gp_srprod_s->SetUserData("PLOT_4", true);

  graph_data->FindMakeGraphView();

  proj->data.RemoveLeafName(sim_data_name); // nuke it
}


///////////////////////////////////////////////////////////////
//   LeabraLimPrecConSpec

void LeabraLimPrecConSpec::Initialize() {
  prec_levels = 1024;
}

void LeabraDaNoise::Initialize() {
  da_noise = 1.0f;
  std_leabra = 1.0f;
}

void LeabraDaNoiseConSpec::Initialize() {
}

//////////////////////////////////
// 	Scalar Value Layer	//
//////////////////////////////////

void ScalarValSpec::Initialize() {
  rep = LOCALIST;
  un_width = .3f;
  norm_width = false;
  clamp_pat = false;
  min_sum_act = 0.2f;
  clip_val = true;
  send_thr = false;
  init_nms = true;

  min = val = 0.0f;
  range = incr = 1.0f;
  un_width_eff = un_width;
}

void ScalarValSpec::InitRange(float umin, float urng) {
  min = umin; range = urng;
  un_width_eff = un_width;
  if(norm_width)
    un_width_eff *= range;
}

void ScalarValSpec::InitVal(float sval, int ugp_size, float umin, float urng) {
  InitRange(umin, urng);
  val = sval;
  incr = range / (float)(ugp_size - 2); // skip 1st unit, and count end..
  //  incr -= .000001f;		// round-off tolerance..
}

// rep 1.5.  ugp_size = 4, incr = 1.5 / 3 = .5
// 0  .5   1
// oooo111122222 = val / incr

// 0 .5  1  val = .8, incr = .5
// 0 .4 .6
// (.4 * .5 + .6 * 1) / (.6 + .4) = .8

// act = 1.0 - (fabs(val - cur) / incr)


float ScalarValSpec::GetUnitAct(int unit_idx) {
  int eff_idx = unit_idx - 1;
  if(rep == GAUSSIAN) {
    float cur = min + incr * (float)eff_idx;
    float dist = (cur - val) / un_width_eff;
    return taMath_float::exp_fast(-(dist * dist));
  }
  else if(rep == LOCALIST) {
    float cur = min + incr * (float)eff_idx;
    if(fabs(val - cur) > incr) return 0.0f;
    return 1.0f - (fabs(val - cur) / incr);
  }
  return 0.0f;			// compiler food
}

float ScalarValSpec::GetUnitVal(int unit_idx) {
  int eff_idx = unit_idx - 1;
  float cur = min + incr * (float)eff_idx;
  return cur;
}

void ScalarValBias::Initialize() {
  un = NO_UN;
  un_shp = VAL;
  un_gain = 1.0f;
  wt = NO_WT;
  val = 0.0f;
  wt_gain = 1.0f;
}

void ScalarValLayerSpec::Initialize() {
  SetUnique("kwta", true);
  kwta.k_from = KWTASpec::USE_K;
  kwta.k = 1;
  gp_kwta.k_from = KWTASpec::USE_K;
  gp_kwta.k = 1;
  SetUnique("inhib_group", true);
  inhib_group = ENTIRE_LAYER;
  SetUnique("inhib", true);
  inhib.type = LeabraInhibSpec::KWTA_AVG_INHIB;
  inhib.kwta_pt = .9f;

  if(scalar.rep == ScalarValSpec::GAUSSIAN) {
    unit_range.min = -0.5f;   unit_range.max = 1.5f;
    unit_range.UpdateAfterEdit();
    scalar.InitRange(unit_range.min, unit_range.range); // needed for un_width_eff
    val_range.min = unit_range.min + (.5f * scalar.un_width_eff);
    val_range.max = unit_range.max - (.5f * scalar.un_width_eff);
  }
  else if(scalar.rep == ScalarValSpec::LOCALIST) {
    unit_range.min = 0.0f;  unit_range.max = 1.0f;
    unit_range.UpdateAfterEdit();
    val_range.min = unit_range.min;
    val_range.max = unit_range.max;
  }
  val_range.UpdateAfterEdit();
}

void ScalarValLayerSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  unit_range.UpdateAfterEdit();
  scalar.UpdateAfterEdit();
  if(scalar.rep == ScalarValSpec::GAUSSIAN) {
    scalar.InitRange(unit_range.min, unit_range.range); // needed for un_width_eff
    val_range.min = unit_range.min + (.5f * scalar.un_width_eff);
    val_range.max = unit_range.max - (.5f * scalar.un_width_eff);
  }
  else {
    val_range.min = unit_range.min;
    val_range.max = unit_range.max;
  }
  val_range.UpdateAfterEdit();
}

void ScalarValLayerSpec::HelpConfig() {
  String help = "ScalarValLayerSpec Computation:\n\
 Uses distributed coarse-coding units to represent a single scalar value.  Each unit\
 has a preferred value arranged evenly between the min-max range, and decoding\
 simply computes an activation-weighted average based on these preferred values.  The\
 current scalar value is displayed in the first unit in the layer, which can be clamped\
 and compared, etc (i.e., set the environment patterns to have just one unit and provide\
 the actual scalar value and it will automatically establish the appropriate distributed\
 representation in the rest of the units).  This first unit is only viewable as act_eq,\
 not act, because it must not send activation to other units.\n\
 \nScalarValLayerSpec Configuration:\n\
 - The bias_val settings allow you to specify a default initial and ongoing bias value\
 through a constant excitatory current (GC) or bias weights (BWT) to the unit, and initial\
 weight values.  These establish a distributed representation that represents the given .val\n\
 - A self connection using the ScalarValSelfPrjnSpec can be made, which provides a bias\
 for neighboring units to have similar values.  It should usually have a fairly small wt_scale.rel\
 parameter (e.g., .1)";
  cerr << help << endl << flush;
  taMisc::Confirm(help);
}

bool ScalarValLayerSpec::CheckConfig_Layer(Layer* ly, bool quiet) {
  LeabraLayer* lay = (LeabraLayer*)ly;
  bool rval = inherited::CheckConfig_Layer(lay, quiet);

  if(lay->CheckError(lay->un_geom.n < 3, quiet, rval,
		"coarse-coded scalar representation requires at least 3 units, I just set un_geom.n")) {
    if(scalar.rep == ScalarValSpec::LOCALIST) {
      lay->un_geom.n = 4;
      lay->un_geom.x = 4;
    }
    else if(scalar.rep == ScalarValSpec::GAUSSIAN) {
      lay->un_geom.n = 12;
      lay->un_geom.x = 12;
    }
  }

  if(scalar.rep == ScalarValSpec::LOCALIST) {
    kwta.k = 1;		// localist means 1 unit active!!
    gp_kwta.k = 1;
  }

  if(bias_val.un == ScalarValBias::GC) {
    LeabraUnitSpec* us = (LeabraUnitSpec*)lay->unit_spec.SPtr();
    if(lay->CheckError(us->hyst.init, quiet, rval,
		  "bias_val.un = GCH requires UnitSpec hyst.init = false, I just set it for you in spec:", us->name, "(make sure this is appropriate for all layers that use this spec!)")) {
      us->SetUnique("hyst", true);
      us->hyst.init = false;
    }
    if(lay->CheckError(us->acc.init, quiet, rval,
		  "bias_val.un = GC requires UnitSpec acc.init = false, I just set it for you in spec:", us->name, "(make sure this is appropriate for all layers that use this spec!)")) {
      us->SetUnique("acc", true);
      us->acc.init = false;
    }
  }

  // check for conspecs with correct params
  LeabraUnit* u = (LeabraUnit*)lay->units.Leaf(0);	// taking 1st unit as representative
  if(lay->CheckError(u == NULL, quiet, rval,
		"scalar val layer doesn't have any units:", lay->name)) {
    return false;		// fatal
  }
  
  for(int g=0; g<u->recv.size; g++) {
    LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
    if((recv_gp->prjn == NULL) || (recv_gp->prjn->spec.SPtr() == NULL)) continue;
    LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
    if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec)) {
      if(lay->CheckError(cs->wt_scale.rel > 0.5f, quiet, rval,
		    "scalar val self connections should have wt_scale < .5, I just set it to .1 for you (make sure this is appropriate for all connections that use this spec!)")) {
	cs->SetUnique("wt_scale", true);
	cs->wt_scale.rel = 0.1f;
      }
      if(lay->CheckError(cs->lrate > 0.0f, quiet, rval,
		    "scalar val self connections should have lrate = 0, I just set it for you in spec:", cs->name, "(make sure this is appropriate for all layers that use this spec!)")) {
	cs->SetUnique("lrate", true);
	cs->lrate = 0.0f;
      }
    }
    else if(cs->InheritsFrom(TA_MarkerConSpec)) {
      continue;
    }
  }
  return rval;
}

void ScalarValLayerSpec::ReConfig(Network* net, int n_units) {
  LeabraLayer* lay;
  taLeafItr li;
  FOR_ITR_EL(LeabraLayer, lay, net->layers., li) {
    if(lay->spec.SPtr() != this) continue;
    
    if(n_units > 0) {
      lay->un_geom.n = n_units;
      lay->un_geom.x = n_units;
    }

    LeabraUnitSpec* us = (LeabraUnitSpec*)lay->unit_spec.SPtr();
    LeabraUnit* u = (LeabraUnit*)lay->units.Leaf(0);	// taking 1st unit as representative
    
    if(scalar.rep == ScalarValSpec::LOCALIST) {
      scalar.min_sum_act = .2f;
      kwta.k = 1;
      inhib.type = LeabraInhibSpec::KWTA_AVG_INHIB;
      inhib.kwta_pt = 0.9f;
      us->g_bar.h = .03f; us->g_bar.a = .09f;
      us->act_fun = LeabraUnitSpec::NOISY_LINEAR;
      us->act.thr = .17f;
      us->act.gain = 220.0f;
      us->act.nvar = .01f;
      us->dt.vm = .05f;
      bias_val.un = ScalarValBias::GC;
      bias_val.wt = ScalarValBias::NO_WT;
      unit_range.min = 0.0f; unit_range.max = 1.0f;

      for(int g=0; g<u->recv.size; g++) {
	LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
	if((recv_gp->prjn == NULL) || (recv_gp->prjn->spec.SPtr() == NULL)) continue;
	LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
	if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec) ||
	   cs->InheritsFrom(TA_MarkerConSpec)) {
	  continue;
	}
	cs->lmix.err_sb = false; // false: this is critical for linear mapping of vals..
	cs->rnd.mean = 0.1f;
	cs->rnd.var = 0.0f;
	cs->wt_sig.gain = 1.0; cs->wt_sig.off = 1.0; 
      }
    }
    else if(scalar.rep == ScalarValSpec::GAUSSIAN) {
      inhib.type = LeabraInhibSpec::KWTA_INHIB;
      inhib.kwta_pt = 0.25f;
      us->g_bar.h = .015f; us->g_bar.a = .045f;
      us->act_fun = LeabraUnitSpec::NOISY_XX1;
      us->act.thr = .25f;
      us->act.gain = 600.0f;
      us->act.nvar = .005f;
      us->dt.vm = .2f;
      bias_val.un = ScalarValBias::GC;
      bias_val.wt = ScalarValBias::NO_WT;
      unit_range.min = -.5f; unit_range.max = 1.5f;

      for(int g=0; g<u->recv.size; g++) {
	LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
	if((recv_gp->prjn == NULL) || (recv_gp->prjn->spec.SPtr() == NULL)) continue;
	LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
	if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec) ||
	   cs->InheritsFrom(TA_MarkerConSpec)) {
	  continue;
	}
	cs->lmix.err_sb = true;
	cs->rnd.mean = 0.1f;
	cs->rnd.var = 0.0f;
	cs->wt_sig.gain = 1.0; cs->wt_sig.off = 1.0; 
      }
    }
    us->UpdateAfterEdit();
  }
  UpdateAfterEdit();
}

// todo: deal with lesion flag in lots of special purpose code like this!!!

void ScalarValLayerSpec::Compute_WtBias_Val(Unit_Group* ugp, float val) {
  if(ugp->size < 3) return;	// must be at least a few units..
  scalar.InitVal(val, ugp->size, unit_range.min, unit_range.range);
  int i;
  for(i=1;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float act = .03f * bias_val.wt_gain * scalar.GetUnitAct(i);
    for(int g=0; g<u->recv.size; g++) {
      LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
      LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
      if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec) ||
	 cs->InheritsFrom(TA_MarkerConSpec)) continue;
      for(int ci=0;ci<recv_gp->size;ci++) {
	LeabraCon* cn = (LeabraCon*)recv_gp->PtrCn(ci);
	cn->wt += act;
	if(cn->wt < cs->wt_limits.min) cn->wt = cs->wt_limits.min;
	if(cn->wt > cs->wt_limits.max) cn->wt = cs->wt_limits.max;
      }
      recv_gp->Init_Weights_post(u);
    }
  }
}

void ScalarValLayerSpec::Compute_UnBias_Val(Unit_Group* ugp, float val) {
  if(ugp->size < 3) return;	// must be at least a few units..
  scalar.InitVal(val, ugp->size, unit_range.min, unit_range.range);
  int i;
  for(i=1;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float act = bias_val.un_gain * scalar.GetUnitAct(i);
    if(bias_val.un == ScalarValBias::GC)
      u->vcb.g_h = act;
    else if(bias_val.un == ScalarValBias::BWT)
      u->bias.OwnCn(0)->wt = act;
  }
}

void ScalarValLayerSpec::Compute_UnBias_NegSlp(Unit_Group* ugp) {
  if(ugp->size < 3) return;	// must be at least a few units..
  float val = 0.0f;
  float incr = bias_val.un_gain / (float)(ugp->size - 2);
  int i;
  for(i=1;i<ugp->size;i++, val += incr) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    if(bias_val.un == ScalarValBias::GC)
      u->vcb.g_a = val;
    else if(bias_val.un == ScalarValBias::BWT)
      u->bias.OwnCn(0)->wt = -val;
  }
}

void ScalarValLayerSpec::Compute_UnBias_PosSlp(Unit_Group* ugp) {
  if(ugp->size < 3) return;	// must be at least a few units..
  float val = bias_val.un_gain;
  float incr = bias_val.un_gain / (float)(ugp->size - 2);
  int i;
  for(i=1;i<ugp->size;i++, val -= incr) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    if(bias_val.un == ScalarValBias::GC)
      u->vcb.g_h = val;
    else if(bias_val.un == ScalarValBias::BWT)
      u->bias.OwnCn(0)->wt = val;
  }
}

void ScalarValLayerSpec::Compute_BiasVal(LeabraLayer* lay, LeabraNetwork* net) {
  if(bias_val.un != ScalarValBias::NO_UN) {
    if(bias_val.un_shp == ScalarValBias::VAL) {
      UNIT_GP_ITR(lay, Compute_UnBias_Val(ugp, bias_val.val););
    }
    else if(bias_val.un_shp == ScalarValBias::NEG_SLP) {
      UNIT_GP_ITR(lay, Compute_UnBias_NegSlp(ugp););
    }
    else if(bias_val.un_shp == ScalarValBias::POS_SLP) {
      UNIT_GP_ITR(lay, Compute_UnBias_PosSlp(ugp););
    }
  }
  if(bias_val.wt == ScalarValBias::WT) {
    UNIT_GP_ITR(lay, Compute_WtBias_Val(ugp, bias_val.val););
  }
}

void ScalarValLayerSpec::BuildUnits_Threads_ugp(LeabraLayer* lay, Unit_Group* ug, 
						LeabraNetwork* net) {
  Unit* un;
  taLeafItr ui;
  int lf = 0;
  FOR_ITR_EL(Unit, un, ug->, ui) {
    if(lf == 0) { un->flat_idx = 0; lf++; continue; }
    un->flat_idx = net->units_flat.size;
    net->units_flat.Add(un);
    lf++;
  }
}

void ScalarValLayerSpec::BuildUnits_Threads(LeabraLayer* lay, LeabraNetwork* net) {
  lay->units_flat_idx = net->units_flat.size;
  UNIT_GP_ITR(lay, BuildUnits_Threads_ugp(lay, ugp, net););
}

void ScalarValLayerSpec::Init_Weights(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Init_Weights(lay, net);
  Compute_BiasVal(lay, net);
  if(scalar.init_nms)
    LabelUnits(lay, net);
}

void ScalarValLayerSpec::Compute_AvgMaxVals_ugp(LeabraLayer* lay, Unit_Group* ug,
						AvgMaxVals& vals, ta_memb_ptr mb_off) {
  vals.InitVals();
  LeabraUnit* u;
  taLeafItr i;
  int lf = 0;
  FOR_ITR_EL(LeabraUnit, u, ug->, i) {
    if(lf == 0) { lf++; continue; } // skip first unit
    float val = *((float*)MemberDef::GetOff_static((void*)u, 0, mb_off));
    vals.UpdtVals(val, lf);
    lf++;
  }
  vals.CalcAvg(ug->leaves);
}

void ScalarValLayerSpec::ClampValue_ugp(Unit_Group* ugp, LeabraNetwork*, float rescale) {
  if(ugp->size < 3) return;	// must be at least a few units..
  LeabraUnit* u = (LeabraUnit*)ugp->FastEl(0);
  LeabraUnitSpec* us = (LeabraUnitSpec*)u->GetUnitSpec();
  if(!clamp.hard)
    u->UnSetExtFlag(Unit::EXT);
  else
    u->SetExtFlag(Unit::EXT);
  float val = u->ext;
  if(scalar.clip_val)
    val = val_range.Clip(val);		// first unit has the value to clamp
  scalar.InitVal(val, ugp->size, unit_range.min, unit_range.range);
  int i;
  for(i=1;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float act = rescale * scalar.GetUnitAct(i);
    if(act < us->opt_thresh.send)
      act = 0.0f;
    u->SetExtFlag(Unit::EXT);
    u->ext = act;
  }
}

float ScalarValLayerSpec::ClampAvgAct(int ugp_size) {
  if(ugp_size < 3) return 0.0f;
  float val = val_range.min + .5f * val_range.Range(); // half way
  scalar.InitVal(val, ugp_size, unit_range.min, unit_range.range);
  float sum = 0.0f;
  int i;
  for(i=1;i<ugp_size;i++) {
    float act = scalar.GetUnitAct(i);
    sum += act;
  }
  sum /= (float)(ugp_size - 1);
  return sum;
}

float ScalarValLayerSpec::ReadValue_ugp(LeabraLayer* lay, Unit_Group* ugp, LeabraNetwork*) {
  if(ugp->size < 3) return 0.0f;	// must be at least a few units..

  scalar.InitVal(0.0f, ugp->size, unit_range.min, unit_range.range);
  float avg = 0.0f;
  float sum_act = 0.0f;
  int i;
  for(i=1;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    LeabraUnitSpec* us = (LeabraUnitSpec*)u->GetUnitSpec();
    float cur = scalar.GetUnitVal(i);
    float act_val = 0.0f;
    if(!scalar.send_thr || (u->act_eq >= us->opt_thresh.send)) // only if over sending thresh!
      act_val = us->clamp_range.Clip(u->act_eq) / us->clamp_range.max; // clipped & normalized!
    avg += cur * act_val;
    sum_act += act_val;
  }
  sum_act = MAX(sum_act, scalar.min_sum_act);
  if(sum_act > 0.0f)
    avg /= sum_act;
  // set the first unit in the group to represent the value
  LeabraUnit* u = (LeabraUnit*)ugp->FastEl(0);
  u->act_eq = u->act_nd = avg;
  u->act = 0.0f;		// very important to clamp act to 0: don't send!
  u->da = 0.0f;			// don't contribute to change in act
  return u->act_eq;
}

void ScalarValLayerSpec::ReadValue(LeabraLayer* lay, LeabraNetwork* net) {
  UNIT_GP_ITR(lay, ReadValue_ugp(lay, ugp, net); );
}

void ScalarValLayerSpec::Compute_ExtToPlus_ugp(Unit_Group* ugp, LeabraNetwork*) {
  if(ugp->size < 3) return;
  int i;
  for(i=0;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    LeabraUnitSpec* us = (LeabraUnitSpec*)u->GetUnitSpec();
    if(i > 0) u->act_p = us->clamp_range.Clip(u->ext);
    else u->act_p = u->ext;
    u->act_dif = u->act_p - u->act_m;
    // important to clear ext stuff, otherwise it will get added into netin next time around!!
    u->ext = 0.0f;
    u->ext_flag = Unit::NO_EXTERNAL;
  }
}

void ScalarValLayerSpec::HardClampExt(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Compute_HardClamp(lay, net);
  ResetAfterClamp(lay, net);
}

void ScalarValLayerSpec::ResetAfterClamp(LeabraLayer* lay, LeabraNetwork*) {
  UNIT_GP_ITR(lay, 
	      if(ugp->size > 2) {
		LeabraUnit* u = (LeabraUnit*)ugp->FastEl(0);
		u->act = 0.0f;		// must reset so it doesn't contribute!
		u->act_eq = u->act_nd = u->ext;	// avoid clamp_range!
	      }
	      );
}

void ScalarValLayerSpec::LabelUnits_ugp(Unit_Group* ugp) {
  if(ugp->size < 3) return;	// must be at least a few units..
  scalar.InitVal(0.0f, ugp->size, unit_range.min, unit_range.range);
  for(int i=1;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float cur = scalar.GetUnitVal(i);
    u->name = (String)cur;
  }
  LeabraUnit* u = (LeabraUnit*)ugp->FastEl(0);
  u->name = "val";		// overall value
}

void ScalarValLayerSpec::LabelUnits(LeabraLayer* lay, LeabraNetwork* net) {
  UNIT_GP_ITR(lay, LabelUnits_ugp(ugp); );
}

void ScalarValLayerSpec::LabelUnitsNet(LeabraNetwork* net) {
  LeabraLayer* l;
  taLeafItr li;
  FOR_ITR_EL(LeabraLayer, l, net->layers., li) {
    if(l->spec.SPtr() == this)
      LabelUnits(l, net);
  }
}

void ScalarValLayerSpec::Settle_Init_Unit0(LeabraLayer* lay, LeabraNetwork* net) {
  // very important: unit 0 in each layer is used for the netin scale parameter and
  // it is otherwise not computed on this unit b/c it is excluded from units_flat!
  // also the targflags need to be updated
  UNIT_GP_ITR(lay, 
	      if(ugp->size > 2) {
		LeabraUnit* u = (LeabraUnit*)ugp->FastEl(0);
		u->Settle_Init_Unit(net);
	      }
	      );
}

void ScalarValLayerSpec::Settle_Init_Layer(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Settle_Init_Layer(lay, net);

  Settle_Init_Unit0(lay, net);

  if(lay->hard_clamped) return;

  LeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(LeabraUnit, u, lay->units., i) {
    LeabraConSpec* bspec = (LeabraConSpec*)u->GetUnitSpec()->bias_spec.SPtr();
    u->bias_scale = bspec->wt_scale.abs;  // still have absolute scaling if wanted..
    u->bias_scale /= 100.0f; 		  // keep a constant scaling so it doesn't depend on network size!
  }
}

void ScalarValLayerSpec::Compute_HardClamp(LeabraLayer* lay, LeabraNetwork* net) {
  if(scalar.clamp_pat) {
    inherited::Compute_HardClamp(lay, net);
    return;
  }
  if(!lay->HasExtFlag(Unit::EXT)) {
    lay->hard_clamped = false;
    return;
  }
  // allow for soft-clamping: translates pattern into exts first
  UNIT_GP_ITR(lay, if(ugp->size > 2) { ClampValue_ugp(ugp, net); } );
  // now check for actual hard clamping
  if(!clamp.hard) {
    lay->hard_clamped = false;
    return;
  }
  HardClampExt(lay, net);
}

void ScalarValLayerSpec::Compute_CycleStats(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Compute_CycleStats(lay, net);
  ReadValue(lay, net);		// always read out the value
}

float ScalarValLayerSpec::Compute_SSE_ugp(Unit_Group* ugp, LeabraLayer* lay, int& n_vals) {
  LeabraUnit* u = (LeabraUnit*)ugp->FastEl(0);
  LeabraUnitSpec* us = (LeabraUnitSpec*)lay->unit_spec.SPtr();
  // only count if target value is within range -- otherwise considered a non-target
  if(u->HasExtFlag(Unit::TARG | Unit::COMP) && val_range.RangeTestEq(u->targ)) {
    n_vals++;
    float uerr = u->targ - u->act_m;
    if(fabsf(uerr) < us->sse_tol)
      return 0.0f;
    return uerr * uerr;
  }
  return 0.0f;
}

float ScalarValLayerSpec::Compute_SSE(LeabraLayer* lay, LeabraNetwork*, 
				      int& n_vals, bool unit_avg, bool sqrt) {
  n_vals = 0;
  if(!(lay->HasExtFlag(Unit::TARG | Unit::COMP))) return 0.0f;
  lay->sse = 0.0f;
  UNIT_GP_ITR(lay, 
	      lay->sse += Compute_SSE_ugp(ugp, lay, n_vals);
	      );
  float rval = lay->sse;
  if(unit_avg && n_vals > 0)
    lay->sse /= (float)n_vals;
  if(sqrt)
    lay->sse = sqrtf(lay->sse);
  if(lay->HasLayerFlag(Layer::NO_ADD_SSE) ||
     (lay->HasExtFlag(Unit::COMP) && lay->HasLayerFlag(Layer::NO_ADD_COMP_SSE))) {
    rval = 0.0f;
    n_vals = 0;
  }
  return rval;
}

float ScalarValLayerSpec::Compute_NormErr_ugp(LeabraLayer* lay, Unit_Group* ug,
					   LeabraInhib* thr, LeabraNetwork* net) {
  LeabraUnit* u = (LeabraUnit*)ug->FastEl(0);
  LeabraUnitSpec* us = (LeabraUnitSpec*)lay->unit_spec.SPtr();
  // only count if target value is within range -- otherwise considered a non-target
  if(u->HasExtFlag(Unit::TARG | Unit::COMP) && val_range.RangeTestEq(u->targ)) {
    float uerr = u->targ - u->act_m;
    if(fabsf(uerr) < us->sse_tol)
      return 0.0f;
    return fabsf(uerr);
  }
  return 0.0f;
}

float ScalarValLayerSpec::Compute_NormErr(LeabraLayer* lay, LeabraNetwork* net) {
  lay->norm_err = -1.0f;					 // assume not contributing
  if(!lay->HasExtFlag(Unit::TARG | Unit::COMP)) return -1.0f; // indicates not applicable

  float nerr = 0.0f;
  float ntot = 0;
  if((inhib_group != ENTIRE_LAYER) && (lay->units.gp.size > 0)) {
    for(int g=0; g<lay->units.gp.size; g++) {
      LeabraUnit_Group* rugp = (LeabraUnit_Group*)lay->units.gp[g];
      nerr += Compute_NormErr_ugp(lay, rugp, (LeabraInhib*)rugp, net);
      ntot += unit_range.range;
    }
  }
  else {
    nerr += Compute_NormErr_ugp(lay, &(lay->units), (LeabraInhib*)lay, net);
    ntot += unit_range.range;
  }
  if(ntot == 0.0f) return -1.0f;

  lay->norm_err = nerr / ntot;
  if(lay->norm_err > 1.0f) lay->norm_err = 1.0f;

  if(lay->HasLayerFlag(Layer::NO_ADD_SSE) ||
     (lay->HasExtFlag(Unit::COMP) && lay->HasLayerFlag(Layer::NO_ADD_COMP_SSE)))
    return -1.0f;		// no contributarse

  return lay->norm_err;
}


//////////////////////////////////
// 	Scalar Value Self Prjn	//
//////////////////////////////////

void ScalarValSelfPrjnSpec::Initialize() {
  init_wts = true;
  width = 3;
  wt_width = 2.0f;
  wt_max = 1.0f;
}

void ScalarValSelfPrjnSpec::Connect_UnitGroup(Unit_Group* gp, Projection* prjn) {
//   float neigh1 = 1.0f / wt_width;
//   float val1 = expf(-(neigh1 * neigh1));
//  float scale_val = wt_max / val1;

  int n_cons = 2*width + 1;

  for(int alloc_loop=1; alloc_loop>=0; alloc_loop--) {
    for(int i=0;i<gp->size;i++) {
      Unit* ru = (Unit*)gp->FastEl(i);

      if(!alloc_loop)
	ru->RecvConsPreAlloc(n_cons, prjn);

      int j;
      for(j=-width;j<=width;j++) {
	int sidx = i+j;
	if((sidx < 0) || (sidx >= gp->size)) continue;
	Unit* su = (Unit*)gp->FastEl(sidx);
	if(!self_con && (ru == su)) continue;
	ru->ConnectFromCk(su, prjn);
      }
    }
    if(alloc_loop) { // on first pass through alloc loop, do sending allocations
      prjn->from->SendConsPostAlloc(prjn);
    }
  }
}

void ScalarValSelfPrjnSpec::Connect_impl(Projection* prjn) {
  if(!prjn->from)	return;
  if(TestError(prjn->from.ptr() != prjn->layer, "Connect_impl", "must be used as a self-projection!")) {
    return;
  }

  Layer* lay = prjn->layer;
  UNIT_GP_ITR(lay, Connect_UnitGroup(ugp, prjn); );
}

void ScalarValSelfPrjnSpec::C_Init_Weights(Projection*, RecvCons* cg, Unit* ru) {
  float neigh1 = 1.0f / wt_width;
  float val1 = expf(-(neigh1 * neigh1));
  float scale_val = wt_max / val1;

  int ru_idx = ((Unit_Group*)ru->owner)->FindEl(ru);

  for(int i=0; i<cg->size; i++) {
    Unit* su = cg->Un(i);
    int su_idx = ((Unit_Group*)su->owner)->FindEl(su);
    float dist = (float)(ru_idx - su_idx) / wt_width;
    float wtval = scale_val * expf(-(dist * dist));
    cg->PtrCn(i)->wt = wtval;
  }
}

//////////////////////////////////
// 	MotorForceLayerSpec	//
//////////////////////////////////

void MotorForceSpec::Initialize() {
  pos_width = .2f;
  vel_width = .2f;
  norm_width = true;
  clip_vals = true;

  cur_pos = cur_vel = 0.0f;
  pos_min = vel_min = 0.0f;
  pos_range = vel_range = 1.0f;

  pos_width_eff = pos_width;
  vel_width_eff = vel_width;
}

void MotorForceSpec::InitRanges(float pos_min_, float pos_range_, float vel_min_, float vel_range_) {
  pos_min = pos_min_;
  pos_range = pos_range_;
  vel_min = vel_min_;
  vel_range = vel_range_;
  pos_width_eff = pos_width;
  vel_width_eff = vel_width;
  if(norm_width) {
    pos_width_eff *= pos_range;
    vel_width_eff *= vel_range;
  }
}

void MotorForceSpec::InitVals(float pos, int pos_size, float pos_min_, float pos_range_,
			      float vel, int vel_size, float vel_min_, float vel_range_) {
  InitRanges(pos_min_, pos_range_, vel_min_, vel_range_);
  cur_pos = pos;
  pos_incr = pos_range / (float)(pos_size-1);
  cur_vel = vel;
  vel_incr = vel_range / (float)(vel_size-1);
}

float MotorForceSpec::GetWt(int pos_gp_idx, int vel_gp_idx) {
  float ug_pos = pos_min + pos_incr * (float)pos_gp_idx;
  float pos_dist = (ug_pos - cur_pos) / pos_width_eff;
  float ug_vel = vel_min + vel_incr * (float)vel_gp_idx;
  float vel_dist = (ug_vel - cur_vel) / vel_width_eff;
  return taMath_float::exp_fast(-(pos_dist * pos_dist + vel_dist * vel_dist));
}

void MotorForceLayerSpec::Initialize() {
  pos_range.min = 0.0f;
  pos_range.max = 2.0f;
  vel_range.min = -.1f;
  vel_range.max = .1f;
  add_noise = true;
  force_noise.type = Random::GAUSSIAN;
  force_noise.var = .01f;
}

void MotorForceLayerSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  pos_range.UpdateAfterEdit();
  vel_range.UpdateAfterEdit();
  force_noise.UpdateAfterEdit();
}

bool MotorForceLayerSpec::CheckConfig_Layer(Layer* ly, bool quiet) {
  LeabraLayer* lay = (LeabraLayer*)ly;
  bool rval = inherited::CheckConfig_Layer(lay, quiet);
  if(!rval) return rval;
  
  if(lay->CheckError(!lay->unit_groups, quiet, rval,
		"requires unit groups -- I just set it for you")) {
    lay->unit_groups = true;
  }
  if(lay->CheckError(lay->gp_geom.x < 3, quiet, rval,
		"requires at least 3 unit groups in x axis -- I just set it for you")) {
    lay->gp_geom.x = 5;
  }
  if(lay->CheckError(lay->gp_geom.y < 3, quiet, rval,
		"requires at least 3 unit groups in y axis -- I just set it for you")) {
    lay->gp_geom.y = 5;
  }
  return rval;
}

float MotorForceLayerSpec::ReadForce(LeabraLayer* lay, LeabraNetwork* net, float pos, float vel) {
  if(motor_force.clip_vals) {
    pos = pos_range.Clip(pos);
    vel = vel_range.Clip(vel);
  }
  motor_force.InitVals(pos, lay->gp_geom.x, pos_range.min, pos_range.range,
		       vel, lay->gp_geom.y, vel_range.min, vel_range.range);

  float force = 0.0f;
  float wt_sum = 0.0f;
  for(int y=0; y<lay->gp_geom.y; y++) {
    for(int x=0; x<lay->gp_geom.x; x++) {
      float wt = motor_force.GetWt(x,y);
      Unit_Group* ug = lay->FindUnitGpFmCoord(x, y);
      if(!ug || ug->size == 0) continue;
      LeabraUnit* un0 = (LeabraUnit*)ug->FastEl(0);
      force += wt * un0->act_eq;
      wt_sum += wt;
    }
  }
  if(wt_sum > 0.0f)
    force /= wt_sum;
  if(add_noise)
    force += force_noise.Gen();
  return force;
}

void MotorForceLayerSpec::ClampForce(LeabraLayer* lay, LeabraNetwork* net, float force, float pos, float vel) {
  if(motor_force.clip_vals) {
    pos = pos_range.Clip(pos);
    vel = vel_range.Clip(vel);
  }
  motor_force.InitVals(pos, lay->gp_geom.x, pos_range.min, pos_range.range,
		       vel, lay->gp_geom.y, vel_range.min, vel_range.range);

  for(int y=0; y<lay->gp_geom.y; y++) {
    for(int x=0; x<lay->gp_geom.x; x++) {
      float wt = motor_force.GetWt(x,y);
      Unit_Group* ugp = lay->FindUnitGpFmCoord(x, y);
      if(!ugp || ugp->size == 0) continue;
      LeabraUnit* un0 = (LeabraUnit*)ugp->FastEl(0);
      un0->ext = force;
      ClampValue_ugp(ugp, net, wt);
    }
  }
  lay->SetExtFlag(Unit::EXT);
  lay->hard_clamped = clamp.hard;
  HardClampExt(lay, net);
  scalar.clamp_pat = true;	// must have this to keep this clamped val
  UNIT_GP_ITR(lay, 
	      LeabraUnit* u = (LeabraUnit*)ugp->FastEl(0);
	      u->ext = 0.0f;		// must reset so it doesn't contribute!
	      );
}

void MotorForceLayerSpec::Compute_BiasVal(LeabraLayer* lay, LeabraNetwork* net) {
  float vel_mid = .5f * (float)(lay->gp_geom.y-1);
  float pos_mid = .5f * (float)(lay->gp_geom.x-1);
  for(int y=0; y<lay->gp_geom.y; y++) {
    float vel_dist = -((float)y - vel_mid) / vel_mid;
    for(int x=0; x<lay->gp_geom.x; x++) {
      float pos_dist = -((float)x - pos_mid) / pos_mid;
      Unit_Group* ugp = lay->FindUnitGpFmCoord(x, y);
      if(!ugp || ugp->size == 0) continue;
      float sum_val = .5f * vel_dist + .5f * pos_dist;

      if(bias_val.un != ScalarValBias::NO_UN) {
	Compute_UnBias_Val(ugp, sum_val);
      }
      if(bias_val.wt == ScalarValBias::WT) {
	Compute_WtBias_Val(ugp, sum_val);
      }
    }
  }
}

//////////////////////////////////
// 	TwoD Value Layer	//
//////////////////////////////////

void TwoDValLeabraLayer::Initialize() {
}

void TwoDValLeabraLayer::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  UpdateTwoDValsGeom();
}

void TwoDValLeabraLayer::UpdateTwoDValsGeom() {
  TwoDValLayerSpec* ls = (TwoDValLayerSpec*)GetLayerSpec();
  if(!ls) return;
  if(ls->InheritsFrom(&TA_TwoDValLayerSpec)) {
    if(unit_groups)
      twod_vals.SetGeom(5, 2, TWOD_N, ls->twod.n_vals, gp_geom.x, gp_geom.y);
    else 
      twod_vals.SetGeom(5, 2, TWOD_N, ls->twod.n_vals, 1, 1);
  }
}

void TwoDValLeabraLayer::ApplyInputData_2d(taMatrix* data, Unit::ExtType ext_flags,
			      Random* ran, const TwoDCoord& offs, bool na_by_range) {
  // only no unit_group supported!
  if(TestError(unit_groups, "ApplyInputData_2d",
	       "input data must be 4d for layers with unit_groups: outer 2 are group dims, inner 2 are x,y vals and n_vals")) {
    return;
  }
  for(int d_y = 0; d_y < data->dim(1); d_y++) {
    int val_idx = offs.y + d_y;
    for(int d_x = 0; d_x < data->dim(0); d_x++) {
      int xy_idx = offs.x + d_x;
      Variant val = data->SafeElAsVar(d_x, d_y);
      if(ext_flags & Unit::EXT)
	twod_vals.SetFmVar(val, xy_idx, TWOD_EXT, val_idx, 0, 0);
      else
	twod_vals.SetFmVar(val, xy_idx, TWOD_TARG, val_idx, 0, 0);
    }
  }
}

void TwoDValLeabraLayer::ApplyInputData_Flat4d(taMatrix* data, Unit::ExtType ext_flags,
				  Random* ran, const TwoDCoord& offs, bool na_by_range) {
  // outer-loop is data-group (groups of x-y data items)
  if(TestError(!unit_groups, "ApplyInputData_Flat4d",
	       "input data must be 2d for layers without unit_groups: x,y vals and n_vals")) {
    return;
  }
  for(int dg_y = 0; dg_y < data->dim(3); dg_y++) {
    for(int dg_x = 0; dg_x < data->dim(2); dg_x++) {

      for(int d_y = 0; d_y < data->dim(1); d_y++) {
	int u_y = offs.y + dg_y * data->dim(1) + d_y; // multiply out data indicies
	for(int d_x = 0; d_x < data->dim(0); d_x++) {
	  int u_x = offs.x + dg_x * data->dim(0) + d_x; // multiply out data indicies
	  Unit* un = FindUnitFmCoord(u_x, u_y);
	  if(un) {
	    float val = data->SafeElAsVar(d_x, d_y, dg_x, dg_y).toFloat();
	    un->ApplyInputData(val, ext_flags, ran, na_by_range);
	  }
	}
      }
    }
  }
}

void TwoDValLeabraLayer::ApplyInputData_Gp4d(taMatrix* data, Unit::ExtType ext_flags, Random* ran,
				bool na_by_range) {
  // outer-loop is data-group (groups of x-y data items)
  for(int dg_y = 0; dg_y < data->dim(3); dg_y++) {
    for(int dg_x = 0; dg_x < data->dim(2); dg_x++) {

      for(int d_y = 0; d_y < data->dim(1); d_y++) {
	int val_idx = d_y;
	for(int d_x = 0; d_x < data->dim(0); d_x++) {
	  int xy_idx = d_x;
	  Variant val = data->SafeElAsVar(d_x, d_y, dg_x, dg_y);
	  if(ext_flags & Unit::EXT)
	    twod_vals.SetFmVar(val, xy_idx, TWOD_EXT, val_idx, dg_x, dg_y);
	  else
	    twod_vals.SetFmVar(val, xy_idx, TWOD_TARG, val_idx, dg_x, dg_y);
	}
      }
    }
  }
}

///////////////////////////////////////////////////////
//		TwoDValLayerSpec

void TwoDValSpec::Initialize() {
  rep = GAUSSIAN;
  n_vals = 1;
  un_width = .3f;
  norm_width = false;
  clamp_pat = false;
  min_sum_act = 0.2f;
  mn_dst = 0.5f;
  clip_val = true;

  x_min = x_val = y_min = y_val = 0.0f;
  x_range = x_incr = y_range = y_incr = 1.0f;
  x_size = y_size = 1;
  un_width_x = un_width_y = un_width;
}

void TwoDValSpec::InitRange(float xmin, float xrng, float ymin, float yrng) {
  x_min = xmin; x_range = xrng; y_min = ymin; y_range = yrng;
  un_width_x = un_width;
  un_width_y = un_width;
  if(norm_width) {
    un_width_x *= x_range;
    un_width_y *= y_range;
  }
}

void TwoDValSpec::InitVal(float xval, float yval, int xsize, int ysize, float xmin, float xrng, float ymin, float yrng) {
  InitRange(xmin, xrng, ymin, yrng);
  x_val = xval; y_val = yval;
  x_size = xsize; y_size = ysize;
  x_incr = x_range / (float)(x_size - 1); // DON'T skip 1st row, and count end..
  y_incr = y_range / (float)(y_size - 1); // DON'T skip 1st row, and count end..
  //  incr -= .000001f;		// round-off tolerance..
}

float TwoDValSpec::GetUnitAct(int unit_idx) {
  int x_idx = unit_idx % x_size;
  int y_idx = (unit_idx / x_size);
  if(rep == GAUSSIAN) {
    float x_cur = x_min + x_incr * (float)x_idx;
    float x_dist = (x_cur - x_val) / un_width_x;
    float y_cur = y_min + y_incr * (float)y_idx;
    float y_dist = (y_cur - y_val) / un_width_y;
    float dist = x_dist * x_dist + y_dist * y_dist;
    return expf(-dist);
  }
  else if(rep == LOCALIST) {
    float x_cur = x_min + x_incr * (float)x_idx;
    float y_cur = y_min + y_incr * (float)y_idx;
    float x_dist = fabs(x_val - x_cur);
    float y_dist = fabs(y_val - y_cur);
    if((x_dist > x_incr) && (y_dist > y_incr)) return 0.0f;
    
    return 1.0f - .5 * ((x_dist / x_incr) + (y_dist / y_incr)); // todo: no idea if this is right.
  }
  return 0.0f;
}

void TwoDValSpec::GetUnitVal(int unit_idx, float& x_cur, float& y_cur) {
  int x_idx = unit_idx % x_size;
  int y_idx = (unit_idx / x_size);
  x_cur = x_min + x_incr * (float)x_idx;
  y_cur = y_min + y_incr * (float)y_idx;
}

void TwoDValBias::Initialize() {
  un = NO_UN;
  un_gain = 1.0f;
  wt = NO_WT;
  wt_gain = 1.0f;
  x_val = 0.0f;
  y_val = 0.0f;
}

void TwoDValLayerSpec::Initialize() {
  min_obj_type = &TA_TwoDValLeabraLayer;

  SetUnique("kwta", true);
  kwta.k_from = KWTASpec::USE_K;
  kwta.k = 9;
  gp_kwta.k_from = KWTASpec::USE_K;
  gp_kwta.k = 9;
  SetUnique("inhib_group", true);
  inhib_group = ENTIRE_LAYER;
  SetUnique("inhib", true);
  inhib.type = LeabraInhibSpec::KWTA_AVG_INHIB;
  inhib.kwta_pt = .6f;

  if(twod.rep == TwoDValSpec::GAUSSIAN) {
    x_range.min = -0.5f;   x_range.max = 1.5f; x_range.UpdateAfterEdit();
    y_range.min = -0.5f;   y_range.max = 1.5f; y_range.UpdateAfterEdit();
    twod.InitRange(x_range.min, x_range.range, y_range.min, y_range.range);
    x_val_range.min = x_range.min + (.5f * twod.un_width_x);
    x_val_range.max = x_range.max - (.5f * twod.un_width_x);
    y_val_range.min = y_range.min + (.5f * twod.un_width_y);
    y_val_range.max = y_range.max - (.5f * twod.un_width_y);
  }
  else if(twod.rep == TwoDValSpec::LOCALIST) {
    x_range.min = 0.0f;  x_range.max = 1.0f;  x_range.UpdateAfterEdit();
    y_range.min = 0.0f;  y_range.max = 1.0f;  y_range.UpdateAfterEdit();
    x_val_range.min = x_range.min;  x_val_range.max = x_range.max;
    y_val_range.min = y_range.min;  y_val_range.max = y_range.max;
  }
  x_val_range.UpdateAfterEdit(); y_val_range.UpdateAfterEdit();
}

void TwoDValLayerSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  x_range.UpdateAfterEdit(); y_range.UpdateAfterEdit();
  twod.UpdateAfterEdit();
  if(twod.rep == TwoDValSpec::GAUSSIAN) {
    twod.InitRange(x_range.min, x_range.range, y_range.min, y_range.range);
    x_val_range.min = x_range.min + (.5f * twod.un_width_x);
    y_val_range.min = y_range.min + (.5f * twod.un_width_y);
    x_val_range.max = x_range.max - (.5f * twod.un_width_x);
    y_val_range.max = y_range.max - (.5f * twod.un_width_y);
  }
  else {
    x_val_range.min = x_range.min;    y_val_range.min = y_range.min;
    x_val_range.max = x_range.max;    y_val_range.max = y_range.max;
  }
  x_val_range.UpdateAfterEdit(); y_val_range.UpdateAfterEdit();
}

void TwoDValLayerSpec::HelpConfig() {
  String help = "TwoDValLayerSpec Computation:\n\
 Uses distributed coarse-coding units to represent two-dimensional values.  Each unit\
 has a preferred value arranged evenly between the min-max range, and decoding\
 simply computes an activation-weighted average based on these preferred values.  The\
 current twod value is encoded in the twod_vals member of the TwoDValLeabraLayer (x1,y1, x2,y2, etc),\
 which are set by input data, and updated to reflect current values encoded over layer.\
 For no unit groups case, input data should be 2d with inner dim of size 2 (x,y) and outer dim\
 of n_vals size.  For unit_groups, data should be 4d with two extra outer dims of gp_x, gp_y.\
 Provide the actual twod values in input data and it will automatically establish the \
 appropriate distributed representation in the rest of the units.\n\
 \nTwoDValLayerSpec Configuration:\n\
 - The bias_val settings allow you to specify a default initial and ongoing bias value\
 through a constant excitatory current (GC) or bias weights (BWT) to the unit, and initial\
 weight values.  These establish a distributed representation that represents the given .val\n\
 - A self connection using the TwoDValSelfPrjnSpec can be made, which provides a bias\
 for neighboring units to have similar values.  It should usually have a fairly small wt_scale.rel\
 parameter (e.g., .1)";
  cerr << help << endl << flush;
  taMisc::Confirm(help);
}

bool TwoDValLayerSpec::CheckConfig_Layer(Layer* ly, bool quiet) {
  LeabraLayer* lay = (LeabraLayer*)ly;
  bool rval = inherited::CheckConfig_Layer(lay, quiet);

  if(lay->CheckError(lay->un_geom.n < 3, quiet, rval,
		"coarse-coded twod representation requires at least 3 units, I just set un_geom.n")) {
    if(twod.rep == TwoDValSpec::LOCALIST) {
      lay->un_geom.n = 9;
      lay->un_geom.x = 3;
      lay->un_geom.y = 3;
    }
    else if(twod.rep == TwoDValSpec::GAUSSIAN) {
      lay->un_geom.n = 121;
      lay->un_geom.x = 11;
      lay->un_geom.y = 11;
    }
  }

  if(lay->InheritsFrom(&TA_TwoDValLeabraLayer)) { // inh will be flagged above
    ((TwoDValLeabraLayer*)lay)->UpdateTwoDValsGeom();
  }

  if(twod.rep == TwoDValSpec::LOCALIST) {
    kwta.k = 1;		// localist means 1 unit active!!
    gp_kwta.k = 1;
  }

  if(bias_val.un == TwoDValBias::GC) {
    LeabraUnitSpec* us = (LeabraUnitSpec*)lay->unit_spec.SPtr();
    if(lay->CheckError(us->hyst.init, quiet, rval,
		  "bias_val.un = GCH requires UnitSpec hyst.init = false, I just set it for you in spec:", us->name, "(make sure this is appropriate for all layers that use this spec!)")) {
      us->SetUnique("hyst", true);
      us->hyst.init = false;
    }
    if(lay->CheckError(us->acc.init, quiet, rval,
		  "bias_val.un = GC requires UnitSpec acc.init = false, I just set it for you in spec:", us->name, "(make sure this is appropriate for all layers that use this spec!)")) {
      us->SetUnique("acc", true);
      us->acc.init = false;
    }
  }

  // check for conspecs with correct params
  LeabraUnit* u = (LeabraUnit*)lay->units.Leaf(0);	// taking 1st unit as representative
  if(lay->CheckError(u == NULL, quiet, rval,
		"twod val layer doesn't have any units:", lay->name)) {
    return false;		// fatal
  }
    
  for(int g=0; g<u->recv.size; g++) {
    LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
    if((recv_gp->prjn == NULL) || (recv_gp->prjn->spec.SPtr() == NULL)) continue;
    LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
    if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec)) {
      if(lay->CheckError(cs->wt_scale.rel > 0.5f, quiet, rval,
		    "twod val self connections should have wt_scale < .5, I just set it to .1 for you (make sure this is appropriate for all connections that use this spec!)")) {
	cs->SetUnique("wt_scale", true);
	cs->wt_scale.rel = 0.1f;
      }
      if(lay->CheckError(cs->lrate > 0.0f, quiet, rval,
		    "twod val self connections should have lrate = 0, I just set it for you in spec:", cs->name, "(make sure this is appropriate for all layers that use this spec!)")) {
	cs->SetUnique("lrate", true);
	cs->lrate = 0.0f;
      }
    }
    else if(cs->InheritsFrom(TA_MarkerConSpec)) {
      continue;
    }
  }
  return rval;
}

void TwoDValLayerSpec::ReConfig(Network* net, int n_units) {
  LeabraLayer* lay;
  taLeafItr li;
  FOR_ITR_EL(LeabraLayer, lay, net->layers., li) {
    if(lay->spec.SPtr() != this) continue;
    
    if(n_units > 0) {
      lay->SetNUnits(n_units);
    }

    LeabraUnitSpec* us = (LeabraUnitSpec*)lay->unit_spec.SPtr();
    LeabraUnit* u = (LeabraUnit*)lay->units.Leaf(0);	// taking 1st unit as representative
    
    if(twod.rep == TwoDValSpec::LOCALIST) {
      twod.min_sum_act = .2f;
      kwta.k = 1;
      inhib.type = LeabraInhibSpec::KWTA_AVG_INHIB;
      inhib.kwta_pt = 0.9f;
      us->g_bar.h = .03f; us->g_bar.a = .09f;
      us->act_fun = LeabraUnitSpec::NOISY_LINEAR;
      us->act.thr = .17f;
      us->act.gain = 220.0f;
      us->act.nvar = .01f;
      us->dt.vm = .05f;
      bias_val.un = TwoDValBias::GC; bias_val.wt = TwoDValBias::NO_WT;
      x_range.min = 0.0f; x_range.max = 1.0f;
      y_range.min = 0.0f; y_range.max = 1.0f;

      for(int g=0; g<u->recv.size; g++) {
	LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
	if((recv_gp->prjn == NULL) || (recv_gp->prjn->spec.SPtr() == NULL)) continue;
	LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
	if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec) ||
	   cs->InheritsFrom(TA_MarkerConSpec)) {
	  continue;
	}
	cs->lmix.err_sb = false; // false: this is critical for linear mapping of vals..
	cs->rnd.mean = 0.1f;
	cs->rnd.var = 0.0f;
	cs->wt_sig.gain = 1.0; cs->wt_sig.off = 1.0; 
      }
    }
    else if(twod.rep == TwoDValSpec::GAUSSIAN) {
      inhib.type = LeabraInhibSpec::KWTA_INHIB;
      inhib.kwta_pt = 0.25f;
      us->g_bar.h = .015f; us->g_bar.a = .045f;
      us->act_fun = LeabraUnitSpec::NOISY_XX1;
      us->act.thr = .25f;
      us->act.gain = 600.0f;
      us->act.nvar = .005f;
      us->dt.vm = .2f;
      bias_val.un = TwoDValBias::GC;  bias_val.wt = TwoDValBias::NO_WT;
      x_range.min = -.5f; x_range.max = 1.5f;
      y_range.min = -.5f; y_range.max = 1.5f;

      for(int g=0; g<u->recv.size; g++) {
	LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
	if((recv_gp->prjn == NULL) || (recv_gp->prjn->spec.SPtr() == NULL)) continue;
	LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
	if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec) ||
	   cs->InheritsFrom(TA_MarkerConSpec)) {
	  continue;
	}
	cs->lmix.err_sb = true;
	cs->rnd.mean = 0.1f;
	cs->rnd.var = 0.0f;
	cs->wt_sig.gain = 1.0; cs->wt_sig.off = 1.0; 
      }
    }
    us->UpdateAfterEdit();
  }
  UpdateAfterEdit();
}

// todo: deal with lesion flag in lots of special purpose code like this!!!

void TwoDValLayerSpec::Compute_WtBias_Val(Unit_Group* ugp, float x_val, float y_val) {
  if(ugp->size < 3) return;	// must be at least a few units..
  Layer* lay = ugp->own_lay;
  twod.InitVal(x_val, y_val, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
  for(int i=0;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float act = .03f * bias_val.wt_gain * twod.GetUnitAct(i);
    for(int g=0; g<u->recv.size; g++) {
      LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
      LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
      if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec) ||
	 cs->InheritsFrom(TA_MarkerConSpec)) continue;
      for(int ci=0;ci<recv_gp->size;ci++) {
	LeabraCon* cn = (LeabraCon*)recv_gp->PtrCn(ci);
	cn->wt += act;
	if(cn->wt < cs->wt_limits.min) cn->wt = cs->wt_limits.min;
	if(cn->wt > cs->wt_limits.max) cn->wt = cs->wt_limits.max;
      }
      recv_gp->Init_Weights_post(u);
    }
  }
}

void TwoDValLayerSpec::Compute_UnBias_Val(Unit_Group* ugp, float x_val, float y_val) {
  if(ugp->size < 3) return;	// must be at least a few units..
  Layer* lay = ugp->own_lay;
  twod.InitVal(x_val, y_val, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
  for(int i=0;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float act = bias_val.un_gain * twod.GetUnitAct(i);
    if(bias_val.un == TwoDValBias::GC)
      u->vcb.g_h = act;
    else if(bias_val.un == TwoDValBias::BWT)
      u->bias.OwnCn(0)->wt = act;
  }
}

void TwoDValLayerSpec::Compute_BiasVal(LeabraLayer* lay, LeabraNetwork* net) {
  if(bias_val.un != TwoDValBias::NO_UN) {
    UNIT_GP_ITR(lay, Compute_UnBias_Val(ugp, bias_val.x_val, bias_val.y_val););
  }
  if(bias_val.wt == TwoDValBias::WT) {
    UNIT_GP_ITR(lay, Compute_WtBias_Val(ugp, bias_val.x_val, bias_val.y_val););
  }
}

void TwoDValLayerSpec::Init_Weights(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Init_Weights(lay, net);
  Compute_BiasVal(lay, net);
}

void TwoDValLayerSpec::ClampValue_ugp(Unit_Group* ugp, LeabraNetwork*, float rescale) {
  if(ugp->size < 3) return;	// must be at least a few units..
  TwoDValLeabraLayer* lay = (TwoDValLeabraLayer*)ugp->own_lay;
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
  // first initialize to zero
  LeabraUnitSpec* us = (LeabraUnitSpec*)ugp->FastEl(0)->GetUnitSpec();
  for(int i=0;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    u->SetExtFlag(Unit::EXT);
    u->ext = 0.0;
  }
  for(int k=0;k<twod.n_vals;k++) {
    float x_val = lay->GetTwoDVal(TwoDValLeabraLayer::TWOD_X, TwoDValLeabraLayer::TWOD_EXT,
				  k, gp_geom_pos.x, gp_geom_pos.y);
    float y_val = lay->GetTwoDVal(TwoDValLeabraLayer::TWOD_Y, TwoDValLeabraLayer::TWOD_EXT,
				  k, gp_geom_pos.x, gp_geom_pos.y);
    if(twod.clip_val) {
      x_val = x_val_range.Clip(x_val);
      y_val = y_val_range.Clip(y_val);
    }
    twod.InitVal(x_val, y_val, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
    for(int i=0;i<ugp->size;i++) {
      LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
      float act = rescale * twod.GetUnitAct(i);
      if(act < us->opt_thresh.send)
	act = 0.0f;
      u->ext += act;
    }
  }
}

void TwoDValLayerSpec::ReadValue(TwoDValLeabraLayer* lay, LeabraNetwork* net) {
  UNIT_GP_ITR(lay, ReadValue_ugp(lay, ugp, net); );
}

void TwoDValLayerSpec::ReadValue_ugp(TwoDValLeabraLayer* lay, Unit_Group* ugp, LeabraNetwork* net) {
  if(ugp->size < 3) return;	// must be at least a few units..
  twod.InitVal(0.0f, 0.0f, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
  if(twod.n_vals == 1) {	// special case
    float x_avg = 0.0f; float y_avg = 0.0f;
    float sum_act = 0.0f;
    for(int i=0;i<ugp->size;i++) {
      LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
      LeabraUnitSpec* us = (LeabraUnitSpec*)u->GetUnitSpec();
      float x_cur, y_cur;  twod.GetUnitVal(i, x_cur, y_cur);
      float act_val = us->clamp_range.Clip(u->act_eq) / us->clamp_range.max; // clipped & normalized!
      x_avg += x_cur * act_val;
      y_avg += y_cur * act_val;
      sum_act += act_val;
    }
    sum_act = MAX(sum_act, twod.min_sum_act);
    if(sum_act > 0.0f) {
      x_avg /= sum_act; y_avg /= sum_act;
    }
    // encode the value
    lay->SetTwoDVal(x_avg, TwoDValLeabraLayer::TWOD_X, TwoDValLeabraLayer::TWOD_ACT,
		    0, gp_geom_pos.x, gp_geom_pos.y);
    lay->SetTwoDVal(y_avg, TwoDValLeabraLayer::TWOD_Y, TwoDValLeabraLayer::TWOD_ACT,
		    0, gp_geom_pos.x, gp_geom_pos.y);
  }
  else {			// multiple items
    // first find the max values, using sum of -1..+1 region
    static ValIdx_Array sort_ary;
    sort_ary.Reset();
    for(int i=0;i<ugp->size;i++) {
      float sum = 0.0f;
      float nsum = 0.0f;
      for(int x=-1;x<=1;x++) {
	for(int y=-1;y<=1;y++) {
	  int idx = i + y * lay->un_geom.x + x;
	  if(idx < 0 || idx >= ugp->size) continue;
	  LeabraUnit* u = (LeabraUnit*)ugp->FastEl(idx);
	  LeabraUnitSpec* us = (LeabraUnitSpec*)u->GetUnitSpec();
	  float act_val = us->clamp_range.Clip(u->act_eq) / us->clamp_range.max; // clipped & normalized!
	  nsum += 1.0f;
	  sum += act_val;
	}
      }
      if(nsum > 0.0f) sum /= nsum;
      ValIdx vi(sum, i);
      sort_ary.Add(vi);
    }
    sort_ary.Sort();
    float mn_x = twod.mn_dst * twod.un_width_x * x_range.Range();
    float mn_y = twod.mn_dst * twod.un_width_y * y_range.Range();
    float mn_dist = mn_x * mn_x + mn_y * mn_y;
    int outi = 0;  int j = 0;
    while((outi < twod.n_vals) && (j < sort_ary.size)) {
      ValIdx& vi = sort_ary[sort_ary.size - j - 1]; // going backward through sort_ary
      float x_cur, y_cur;  twod.GetUnitVal(vi.idx, x_cur, y_cur);
      // check distance from all previous!
      float my_mn = x_range.Range() + y_range.Range();
      for(int k=0; k<j; k++) {
	ValIdx& vo = sort_ary[sort_ary.size - k - 1];
	if(vo.val == -1.0f) continue; // guy we skipped over before
	float x_prv, y_prv;  twod.GetUnitVal(vo.idx, x_prv, y_prv);
	float x_d = x_cur - x_prv; float y_d = y_cur - y_prv; 
	float dist = x_d * x_d + y_d * y_d;
	my_mn = MIN(dist, my_mn);
      }
      if(my_mn < mn_dist) { vi.val = -1.0f; j++; continue; } // mark with -1 so we know we skipped it

      // encode the value
      lay->SetTwoDVal(x_cur, TwoDValLeabraLayer::TWOD_X, TwoDValLeabraLayer::TWOD_ACT,
		      0, gp_geom_pos.x, gp_geom_pos.y);
      lay->SetTwoDVal(y_cur, TwoDValLeabraLayer::TWOD_Y, TwoDValLeabraLayer::TWOD_ACT,
		      0, gp_geom_pos.x, gp_geom_pos.y);
      j++; outi++;
    }
  }
}

void TwoDValLayerSpec::LabelUnits_ugp(Unit_Group* ugp) {
  if(ugp->size < 3) return;	// must be at least a few units..
  Layer* lay = ugp->own_lay;
  twod.InitVal(0.0f, 0.0f, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
  for(int i=0;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float x_cur, y_cur; twod.GetUnitVal(i, x_cur, y_cur);
    u->name = (String)x_cur + "," + String(y_cur);
  }
}

void TwoDValLayerSpec::LabelUnits(LeabraLayer* lay, LeabraNetwork* net) {
  UNIT_GP_ITR(lay, LabelUnits_ugp(ugp); );
}

void TwoDValLayerSpec::LabelUnitsNet(LeabraNetwork* net) {
  LeabraLayer* l;
  taLeafItr li;
  FOR_ITR_EL(LeabraLayer, l, net->layers., li) {
    if(l->spec.SPtr() == this)
      LabelUnits(l, net);
  }
}

void TwoDValLayerSpec::HardClampExt(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Compute_HardClamp(lay, net);
}

void TwoDValLayerSpec::Settle_Init_TargFlags_Layer(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Settle_Init_TargFlags_Layer(lay, net);
  // need to actually copy over targ to ext vals!
  TwoDValLeabraLayer* tdlay = (TwoDValLeabraLayer*)lay;
  if(lay->HasExtFlag(Unit::TARG)) {	// only process target layers..
    if(net->phase == LeabraNetwork::PLUS_PHASE) {
      int gi = 0;
      if(tdlay->units.gp.size > 0) {
	for(gi=0; gi<tdlay->units.gp.size; gi++) {
	  Unit_Group* ugp = (Unit_Group*)tdlay->units.gp[gi];
	  for(int k=0;k<twod.n_vals;k++) {
	    TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
	    float x_val, y_val;
	    tdlay->GetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_TARG,
			       k, gp_geom_pos.x, gp_geom_pos.y);
	    tdlay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_EXT,
			       k, gp_geom_pos.x, gp_geom_pos.y);
	  }
	}
      }
      else {
	Unit_Group* ugp = (Unit_Group*)&(tdlay->units);
	for(int k=0;k<twod.n_vals;k++) {
	  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
	  float x_val, y_val;
	  tdlay->GetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_TARG,
			     k, gp_geom_pos.x, gp_geom_pos.y);
	  tdlay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_EXT,
			     k, gp_geom_pos.x, gp_geom_pos.y);
	}
      } 
//       UNIT_GP_ITR(tdlay,
// 		  for(int k=0;k<twod.n_vals;k++) {
// 		    TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
// 		    float x_val, y_val;
// 		    tdlay->GetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_TARG,
// 				       k, gp_geom_pos.x, gp_geom_pos.y);
// 		    tdlay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_EXT,
// 				       k, gp_geom_pos.x, gp_geom_pos.y);
// 		    }});
    }
  }
}


void TwoDValLayerSpec::Settle_Init_Layer(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Settle_Init_Layer(lay, net);

  TwoDValLeabraLayer* tdlay = (TwoDValLeabraLayer*)lay;
  tdlay->UpdateTwoDValsGeom();	// quick, make sure no mismatch

  if(lay->hard_clamped) return;

  LeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(LeabraUnit, u, lay->units., i) {
    LeabraConSpec* bspec = (LeabraConSpec*)u->GetUnitSpec()->bias_spec.SPtr();
    u->bias_scale = bspec->wt_scale.abs;  // still have absolute scaling if wanted..
    u->bias_scale /= 100.0f; 		  // keep a constant scaling so it doesn't depend on network size!
  }
}

void TwoDValLayerSpec::Compute_HardClamp(LeabraLayer* lay, LeabraNetwork* net) {
  if(twod.clamp_pat) {
    inherited::Compute_HardClamp(lay, net);
    return;
  }
  if(!(lay->ext_flag & Unit::EXT)) {
    lay->hard_clamped = false;
    return;
  }
  // allow for soft-clamping: translates pattern into exts first
  UNIT_GP_ITR(lay, if(ugp->size > 2) { ClampValue_ugp(ugp, net); } );
  // now check for actual hard clamping
  if(!clamp.hard) {
    lay->hard_clamped = false;
    return;
  }
  HardClampExt(lay, net);
}

void TwoDValLayerSpec::Compute_CycleStats(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Compute_CycleStats(lay, net);
  ReadValue((TwoDValLeabraLayer*)lay, net);		// always read out the value
}

void TwoDValLayerSpec::PostSettle(LeabraLayer* ly, LeabraNetwork* net) {
  inherited::PostSettle(ly, net);
  TwoDValLeabraLayer* lay = (TwoDValLeabraLayer*)ly;
  UNIT_GP_ITR(lay, PostSettle_ugp(lay, ugp, net); );
}

void TwoDValLayerSpec::PostSettle_ugp(TwoDValLeabraLayer* lay, Unit_Group* ugp, LeabraNetwork* net) {
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();

  bool no_plus_testing = false;
  if(net->no_plus_test && (net->train_mode == LeabraNetwork::TEST)) {
    no_plus_testing = true;
  }

  for(int k=0;k<twod.n_vals;k++) {
    float x_val, y_val, x_m, y_m, x_p, y_p;
    lay->GetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT,
		     k, gp_geom_pos.x, gp_geom_pos.y);
    lay->GetTwoDVals(x_m, y_m, TwoDValLeabraLayer::TWOD_ACT_M,
		     k, gp_geom_pos.x, gp_geom_pos.y);
    lay->GetTwoDVals(x_p, y_p, TwoDValLeabraLayer::TWOD_ACT_P,
		     k, gp_geom_pos.x, gp_geom_pos.y);

    switch(net->phase_order) {
    case LeabraNetwork::MINUS_PLUS:
      if(no_plus_testing) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
			 k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(0.0f, 0.0f, TwoDValLeabraLayer::TWOD_ACT_DIF,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	if(net->phase == LeabraNetwork::MINUS_PHASE) {
	  lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	}
	else {
	  lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	  lay->SetTwoDVals(x_val - x_m, y_val - y_m, TwoDValLeabraLayer::TWOD_ACT_DIF,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	}
      }
      break;
    case LeabraNetwork::PLUS_MINUS:
      if(no_plus_testing) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
			 k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P,
			 k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(0.0f, 0.0f, TwoDValLeabraLayer::TWOD_ACT_DIF,
			 k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	if(net->phase == LeabraNetwork::MINUS_PHASE) {
	  lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
			   k, gp_geom_pos.x, gp_geom_pos.y);
	  lay->SetTwoDVals(x_p - x_val, y_p - y_val, TwoDValLeabraLayer::TWOD_ACT_DIF,
			   k, gp_geom_pos.x, gp_geom_pos.y);
	}
	else {
	  lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	}
      }
      break;
    case LeabraNetwork::PLUS_ONLY:
      lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
		      k, gp_geom_pos.x, gp_geom_pos.y);
      lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P,
		      k, gp_geom_pos.x, gp_geom_pos.y);
      lay->SetTwoDVals(0.0f, 0.0f, TwoDValLeabraLayer::TWOD_ACT_DIF,
		      k, gp_geom_pos.x, gp_geom_pos.y);
      break;
    case LeabraNetwork::MINUS_PLUS_NOTHING:
    case LeabraNetwork::MINUS_PLUS_MINUS:
      // don't use actual phase values because pluses might be minuses with testing
      if(net->phase_no == 0) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
			 k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else if(net->phase_no == 1) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(x_val - x_m, y_val - y_m, TwoDValLeabraLayer::TWOD_ACT_DIF,
			 k, gp_geom_pos.x, gp_geom_pos.y);
	if(no_plus_testing) {
	  // update act_m because it is actually another test case!
	  lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
			   k, gp_geom_pos.x, gp_geom_pos.y);
	}
      }
      else {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M2,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(x_p - x_val, y_p - y_val, TwoDValLeabraLayer::TWOD_ACT_DIF,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      break;
    case LeabraNetwork::PLUS_NOTHING:
      // don't use actual phase values because pluses might be minuses with testing
      if(net->phase_no == 0) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(x_p - x_val, y_p - y_val, TwoDValLeabraLayer::TWOD_ACT_DIF,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      break;
    case LeabraNetwork::MINUS_PLUS_PLUS:
      // don't use actual phase values because pluses might be minuses with testing
      if(net->phase_no == 0) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else if(net->phase_no == 1) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(x_val - x_m, y_val - y_m, TwoDValLeabraLayer::TWOD_ACT_DIF,
			 k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P2,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(x_val - x_p, y_val - y_p, TwoDValLeabraLayer::TWOD_ACT_DIF2,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      break;
    case LeabraNetwork::MINUS_PLUS_PLUS_NOTHING:
    case LeabraNetwork::MINUS_PLUS_PLUS_MINUS:
      // don't use actual phase values because pluses might be minuses with testing
      if(net->phase_no == 0) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else if(net->phase_no == 1) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(x_val - x_m, y_val - y_m, TwoDValLeabraLayer::TWOD_ACT_DIF,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else if(net->phase_no == 2) {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_P2,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(x_val - x_p, y_val - y_p, TwoDValLeabraLayer::TWOD_ACT_DIF2,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	lay->SetTwoDVals(x_val, y_val, TwoDValLeabraLayer::TWOD_ACT_M2,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetTwoDVals(x_p - x_val, y_p - y_val, TwoDValLeabraLayer::TWOD_ACT_DIF2,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      break;
    }
  }
}

float TwoDValLayerSpec::Compute_SSE_ugp(Unit_Group* ugp, LeabraLayer* ly, int& n_vals) {
  TwoDValLeabraLayer* lay = (TwoDValLeabraLayer*)ly;
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
  LeabraUnitSpec* us = (LeabraUnitSpec*)ugp->FastEl(0)->GetUnitSpec();
  float rval = 0.0f;
  for(int k=0;k<twod.n_vals;k++) { // first loop over and find potential target values
    float x_targ, y_targ;
    lay->GetTwoDVals(x_targ, y_targ, TwoDValLeabraLayer::TWOD_TARG, k,
		     gp_geom_pos.x, gp_geom_pos.y);
    // only count if target value is within range -- otherwise considered a non-target
    if(x_val_range.RangeTestEq(x_targ) && y_val_range.RangeTestEq(y_targ)) {
      n_vals++;
      // now find minimum dist actual activations
      float mn_dist = taMath::flt_max;
      for(int j=0;j<twod.n_vals;j++) {
	float x_act_m, y_act_m;
	lay->GetTwoDVals(x_act_m, y_act_m, TwoDValLeabraLayer::TWOD_ACT_M,
					j, gp_geom_pos.x, gp_geom_pos.y);
	float dx = x_targ - x_act_m;
	float dy = y_targ - y_act_m;
	if(fabsf(dx) < us->sse_tol) dx = 0.0f;
	if(fabsf(dy) < us->sse_tol) dy = 0.0f;
	float dist = dx * dx + dy * dy;
	if(dist < mn_dist) {
	  mn_dist = dist;
	  lay->SetTwoDVals(dx, dy, TwoDValLeabraLayer::TWOD_ERR,
			   k, gp_geom_pos.x, gp_geom_pos.y);
	  lay->SetTwoDVals(dx*dx, dy*dy, TwoDValLeabraLayer::TWOD_SQERR,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	}
      }
      rval += mn_dist;
    }
  }
  return rval;
}

float TwoDValLayerSpec::Compute_SSE(LeabraLayer* lay, LeabraNetwork*, 
				    int& n_vals, bool unit_avg, bool sqrt) {
  n_vals = 0;
  if(!(lay->ext_flag & (Unit::TARG | Unit::COMP))) return 0.0f;
  lay->sse = 0.0f;
  UNIT_GP_ITR(lay, 
	      lay->sse += Compute_SSE_ugp(ugp, lay, n_vals);
	      );
  float rval = lay->sse;
  if(unit_avg && n_vals > 0)
    lay->sse /= (float)n_vals;
  if(sqrt)
    lay->sse = sqrtf(lay->sse);
  if(lay->HasLayerFlag(Layer::NO_ADD_SSE) ||
     ((lay->ext_flag & Unit::COMP) && lay->HasLayerFlag(Layer::NO_ADD_COMP_SSE))) {
    rval = 0.0f;
    n_vals = 0;
  }
  return rval;
}

float TwoDValLayerSpec::Compute_NormErr_ugp(LeabraLayer* ly, Unit_Group* ugp,
					   LeabraInhib* thr, LeabraNetwork* net) {
  TwoDValLeabraLayer* lay = (TwoDValLeabraLayer*)ly;
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
  LeabraUnitSpec* us = (LeabraUnitSpec*)ugp->FastEl(0)->GetUnitSpec();
  float rval = 0.0f;
  for(int k=0;k<twod.n_vals;k++) { // first loop over and find potential target values
    float x_targ, y_targ;
    lay->GetTwoDVals(x_targ, y_targ, TwoDValLeabraLayer::TWOD_TARG, k,
		     gp_geom_pos.x, gp_geom_pos.y);
    // only count if target value is within range -- otherwise considered a non-target
    if(x_val_range.RangeTestEq(x_targ) && y_val_range.RangeTestEq(y_targ)) {
      // now find minimum dist actual activations
      float mn_dist = taMath::flt_max;
      for(int j=0;j<twod.n_vals;j++) {
	float x_act_m, y_act_m;
	lay->GetTwoDVals(x_act_m, y_act_m, TwoDValLeabraLayer::TWOD_ACT_M,
					j, gp_geom_pos.x, gp_geom_pos.y);
	float dx = x_targ - x_act_m;
	float dy = y_targ - y_act_m;
	if(fabsf(dx) < us->sse_tol) dx = 0.0f;
	if(fabsf(dy) < us->sse_tol) dy = 0.0f;
	float dist = fabsf(dx) + fabsf(dy); // only diff from sse!
	if(dist < mn_dist)
	  mn_dist = dist;
      }
      rval += mn_dist;
    }
  }
  return rval;
}

float TwoDValLayerSpec::Compute_NormErr(LeabraLayer* lay, LeabraNetwork* net) {
  lay->norm_err = -1.0f;					 // assume not contributing
  if(!(lay->ext_flag & (Unit::TARG | Unit::COMP))) return -1.0f; // indicates not applicable

  float nerr = 0.0f;
  float ntot = 0;
  if((inhib_group != ENTIRE_LAYER) && (lay->units.gp.size > 0)) {
    for(int g=0; g<lay->units.gp.size; g++) {
      LeabraUnit_Group* rugp = (LeabraUnit_Group*)lay->units.gp[g];
      nerr += Compute_NormErr_ugp(lay, rugp, (LeabraInhib*)rugp, net);
      ntot += x_range.range + y_range.range;
    }
  }
  else {
    nerr += Compute_NormErr_ugp(lay, &(lay->units), (LeabraInhib*)lay, net);
    ntot += x_range.range + y_range.range;
  }
  if(ntot == 0.0f) return -1.0f;

  lay->norm_err = nerr / ntot;
  if(lay->norm_err > 1.0f) lay->norm_err = 1.0f;

  if(lay->HasLayerFlag(Layer::NO_ADD_SSE) ||
     ((lay->ext_flag & Unit::COMP) && lay->HasLayerFlag(Layer::NO_ADD_COMP_SSE)))
    return -1.0f;		// no contributarse

  return lay->norm_err;
}

///////////////////////////////////////////////////////////////
//   DecodeTwoDValLayerSpec

void DecodeTwoDValLayerSpec::Initialize() {
}

void DecodeTwoDValLayerSpec::Compute_Inhib(LeabraLayer*, LeabraNetwork*) {
  return;			// do nothing!
}

void DecodeTwoDValLayerSpec::ReadValue_ugp(TwoDValLeabraLayer* lay, Unit_Group* ug, LeabraNetwork* net) {
  LeabraUnit* u;
  taLeafItr ui;
  FOR_ITR_EL(LeabraUnit, u, ug->, ui) {
    if(u->recv.size == 0) continue;
    LeabraRecvCons* cg = (LeabraRecvCons*)u->recv[0];
    if(cg->size == 0) continue;
    LeabraUnit* su = (LeabraUnit*)cg->Un(0);
    u->net = su->net;
    u->act = su->act;
    u->act_eq = su->act_eq;
    u->act_nd = su->act_nd;
  }
  inherited::ReadValue_ugp(lay, ug, net);
}

/*

//////////////////////////////////
// 	FourD Value Layer	//
//////////////////////////////////

void FourDValLeabraLayer::Initialize() {
}

void FourDValLeabraLayer::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  UpdateFourDValsGeom();
}

void FourDValLeabraLayer::UpdateFourDValsGeom() {
  FourDValLayerSpec* ls = (FourDValLayerSpec*)GetLayerSpec();
  if(!ls) return;
  if(ls->InheritsFrom(&TA_FourDValLayerSpec)) {
    if(unit_groups)
      fourd_vals.SetGeom(5, 2, FOURD_N, ls->fourd.n_vals, gp_geom.x, gp_geom.y);
    else 
      fourd_vals.SetGeom(5, 2, FOURD_N, ls->fourd.n_vals, 1, 1);
  }
}

void FourDValLeabraLayer::ApplyInputData_2d(taMatrix* data, Unit::ExtType ext_flags,
			      Random* ran, const TwoDCoord& offs, bool na_by_range) {
  // only no unit_group supported!
  if(TestError(unit_groups, "ApplyInputData_2d",
	       "input data must be 4d for layers with unit_groups: outer 2 are group dims, inner 2 are x,y vals and n_vals")) {
    return;
  }
  for(int d_y = 0; d_y < data->dim(1); d_y++) {
    int val_idx = offs.y + d_y;
    for(int d_x = 0; d_x < data->dim(0); d_x++) {
      int xy_idx = offs.x + d_x;
      Variant val = data->SafeElAsVar(d_x, d_y);
      if(ext_flags & Unit::EXT)
	fourd_vals.SetFmVar(val, xy_idx, FOURD_EXT, val_idx, 0, 0);
      else
	fourd_vals.SetFmVar(val, xy_idx, FOURD_TARG, val_idx, 0, 0);
    }
  }
}

void FourDValLeabraLayer::ApplyInputData_Flat4d(taMatrix* data, Unit::ExtType ext_flags,
				  Random* ran, const TwoDCoord& offs, bool na_by_range) {
  // outer-loop is data-group (groups of x-y data items)
  if(TestError(!unit_groups, "ApplyInputData_Flat4d",
	       "input data must be 2d for layers without unit_groups: x,y vals and n_vals")) {
    return;
  }
  for(int dg_y = 0; dg_y < data->dim(3); dg_y++) {
    for(int dg_x = 0; dg_x < data->dim(2); dg_x++) {

      for(int d_y = 0; d_y < data->dim(1); d_y++) {
	int u_y = offs.y + dg_y * data->dim(1) + d_y; // multiply out data indicies
	for(int d_x = 0; d_x < data->dim(0); d_x++) {
	  int u_x = offs.x + dg_x * data->dim(0) + d_x; // multiply out data indicies
	  Unit* un = FindUnitFmCoord(u_x, u_y);
	  if(un) {
	    float val = data->SafeElAsVar(d_x, d_y, dg_x, dg_y).toFloat();
	    un->ApplyInputData(val, ext_flags, ran, na_by_range);
	  }
	}
      }
    }
  }
}

void FourDValLeabraLayer::ApplyInputData_Gp4d(taMatrix* data, Unit::ExtType ext_flags, Random* ran,
				bool na_by_range) {
  // outer-loop is data-group (groups of x-y data items)
  for(int dg_y = 0; dg_y < data->dim(3); dg_y++) {
    for(int dg_x = 0; dg_x < data->dim(2); dg_x++) {

      for(int d_y = 0; d_y < data->dim(1); d_y++) {
	int val_idx = d_y;
	for(int d_x = 0; d_x < data->dim(0); d_x++) {
	  int xy_idx = d_x;
	  Variant val = data->SafeElAsVar(d_x, d_y, dg_x, dg_y);
	  if(ext_flags & Unit::EXT)
	    fourd_vals.SetFmVar(val, xy_idx, FOURD_EXT, val_idx, dg_x, dg_y);
	  else
	    fourd_vals.SetFmVar(val, xy_idx, FOURD_TARG, val_idx, dg_x, dg_y);
	}
      }
    }
  }
}

///////////////////////////////////////////////////////
//		FourDValLayerSpec

void FourDValSpec::Initialize() {
  rep = GAUSSIAN;
  n_vals = 1;
  un_width = .3f;
  norm_width = false;
  clamp_pat = false;
  min_sum_act = 0.2f;
  mn_dst = 0.5f;
  clip_val = true;

  x_min = x_val = y_min = y_val = 0.0f;
  x_range = x_incr = y_range = y_incr = 1.0f;
  x_size = y_size = 1;
  un_width_x = un_width_y = un_width;
}

void FourDValSpec::InitRange(float xmin, float xrng, float ymin, float yrng) {
  x_min = xmin; x_range = xrng; y_min = ymin; y_range = yrng;
  un_width_x = un_width;
  un_width_y = un_width;
  if(norm_width) {
    un_width_x *= x_range;
    un_width_y *= y_range;
  }
}

void FourDValSpec::InitVal(float xval, float yval, int xsize, int ysize, float xmin, float xrng, float ymin, float yrng) {
  InitRange(xmin, xrng, ymin, yrng);
  x_val = xval; y_val = yval;
  x_size = xsize; y_size = ysize;
  x_incr = x_range / (float)(x_size - 1); // DON'T skip 1st row, and count end..
  y_incr = y_range / (float)(y_size - 1); // DON'T skip 1st row, and count end..
  //  incr -= .000001f;		// round-off tolerance..
}

float FourDValSpec::GetUnitAct(int unit_idx) {
  int x_idx = unit_idx % x_size;
  int y_idx = (unit_idx / x_size);
  if(rep == GAUSSIAN) {
    float x_cur = x_min + x_incr * (float)x_idx;
    float x_dist = (x_cur - x_val) / un_width_x;
    float y_cur = y_min + y_incr * (float)y_idx;
    float y_dist = (y_cur - y_val) / un_width_y;
    float dist = x_dist * x_dist + y_dist * y_dist;
    return expf(-dist);
  }
  else if(rep == LOCALIST) {
    float x_cur = x_min + x_incr * (float)x_idx;
    float y_cur = y_min + y_incr * (float)y_idx;
    float x_dist = fabs(x_val - x_cur);
    float y_dist = fabs(y_val - y_cur);
    if((x_dist > x_incr) && (y_dist > y_incr)) return 0.0f;
    
    return 1.0f - .5 * ((x_dist / x_incr) + (y_dist / y_incr)); // todo: no idea if this is right.
  }
  return 0.0f;
}

void FourDValSpec::GetUnitVal(int unit_idx, float& x_cur, float& y_cur) {
  int x_idx = unit_idx % x_size;
  int y_idx = (unit_idx / x_size);
  x_cur = x_min + x_incr * (float)x_idx;
  y_cur = y_min + y_incr * (float)y_idx;
}

void FourDValBias::Initialize() {
  un = NO_UN;
  un_gain = 1.0f;
  wt = NO_WT;
  wt_gain = 1.0f;
  x_val = 0.0f;
  y_val = 0.0f;
}

void FourDValLayerSpec::Initialize() {
  min_obj_type = &TA_FourDValLeabraLayer;

  SetUnique("kwta", true);
  kwta.k_from = KWTASpec::USE_K;
  kwta.k = 9;
  gp_kwta.k_from = KWTASpec::USE_K;
  gp_kwta.k = 9;
  SetUnique("inhib_group", true);
  inhib_group = ENTIRE_LAYER;
  SetUnique("inhib", true);
  inhib.type = LeabraInhibSpec::KWTA_AVG_INHIB;
  inhib.kwta_pt = .6f;

  if(fourd.rep == FourDValSpec::GAUSSIAN) {
    x_range.min = -0.5f;   x_range.max = 1.5f; x_range.UpdateAfterEdit();
    y_range.min = -0.5f;   y_range.max = 1.5f; y_range.UpdateAfterEdit();
    fourd.InitRange(x_range.min, x_range.range, y_range.min, y_range.range);
    x_val_range.min = x_range.min + (.5f * fourd.un_width_x);
    x_val_range.max = x_range.max - (.5f * fourd.un_width_x);
    y_val_range.min = y_range.min + (.5f * fourd.un_width_y);
    y_val_range.max = y_range.max - (.5f * fourd.un_width_y);
  }
  else if(fourd.rep == FourDValSpec::LOCALIST) {
    x_range.min = 0.0f;  x_range.max = 1.0f;  x_range.UpdateAfterEdit();
    y_range.min = 0.0f;  y_range.max = 1.0f;  y_range.UpdateAfterEdit();
    x_val_range.min = x_range.min;  x_val_range.max = x_range.max;
    y_val_range.min = y_range.min;  y_val_range.max = y_range.max;
  }
  x_val_range.UpdateAfterEdit(); y_val_range.UpdateAfterEdit();
}

void FourDValLayerSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  x_range.UpdateAfterEdit(); y_range.UpdateAfterEdit();
  fourd.UpdateAfterEdit();
  if(fourd.rep == FourDValSpec::GAUSSIAN) {
    fourd.InitRange(x_range.min, x_range.range, y_range.min, y_range.range);
    x_val_range.min = x_range.min + (.5f * fourd.un_width_x);
    y_val_range.min = y_range.min + (.5f * fourd.un_width_y);
    x_val_range.max = x_range.max - (.5f * fourd.un_width_x);
    y_val_range.max = y_range.max - (.5f * fourd.un_width_y);
  }
  else {
    x_val_range.min = x_range.min;    y_val_range.min = y_range.min;
    x_val_range.max = x_range.max;    y_val_range.max = y_range.max;
  }
  x_val_range.UpdateAfterEdit(); y_val_range.UpdateAfterEdit();
}

void FourDValLayerSpec::HelpConfig() {
  String help = "FourDValLayerSpec Computation:\n\
 Uses distributed coarse-coding units to represent two-dimensional values.  Each unit\
 has a preferred value arranged evenly between the min-max range, and decoding\
 simply computes an activation-weighted average based on these preferred values.  The\
 current fourd value is encoded in the fourd_vals member of the FourDValLeabraLayer (x1,y1, x2,y2, etc),\
 which are set by input data, and updated to reflect current values encoded over layer.\
 For no unit groups case, input data should be 2d with inner dim of size 2 (x,y) and outer dim\
 of n_vals size.  For unit_groups, data should be 4d with two extra outer dims of gp_x, gp_y.\
 Provide the actual fourd values in input data and it will automatically establish the \
 appropriate distributed representation in the rest of the units.\n\
 \nFourDValLayerSpec Configuration:\n\
 - The bias_val settings allow you to specify a default initial and ongoing bias value\
 through a constant excitatory current (GC) or bias weights (BWT) to the unit, and initial\
 weight values.  These establish a distributed representation that represents the given .val\n\
 - A self connection using the FourDValSelfPrjnSpec can be made, which provides a bias\
 for neighboring units to have similar values.  It should usually have a fairly small wt_scale.rel\
 parameter (e.g., .1)";
  cerr << help << endl << flush;
  taMisc::Confirm(help);
}

bool FourDValLayerSpec::CheckConfig_Layer(Layer* ly, bool quiet) {
  LeabraLayer* lay = (LeabraLayer*)ly;
  bool rval = inherited::CheckConfig_Layer(lay, quiet);

  if(lay->CheckError(lay->un_geom.n < 3, quiet, rval,
		"coarse-coded fourd representation requires at least 3 units, I just set un_geom.n")) {
    if(fourd.rep == FourDValSpec::LOCALIST) {
      lay->un_geom.n = 9;
      lay->un_geom.x = 3;
      lay->un_geom.y = 3;
    }
    else if(fourd.rep == FourDValSpec::GAUSSIAN) {
      lay->un_geom.n = 121;
      lay->un_geom.x = 11;
      lay->un_geom.y = 11;
    }
  }

  if(lay->InheritsFrom(&TA_FourDValLeabraLayer)) { // inh will be flagged above
    ((FourDValLeabraLayer*)lay)->UpdateFourDValsGeom();
  }

  if(fourd.rep == FourDValSpec::LOCALIST) {
    kwta.k = 1;		// localist means 1 unit active!!
    gp_kwta.k = 1;
  }

  if(bias_val.un == FourDValBias::GC) {
    LeabraUnitSpec* us = (LeabraUnitSpec*)lay->unit_spec.SPtr();
    if(lay->CheckError(us->hyst.init, quiet, rval,
		  "bias_val.un = GCH requires UnitSpec hyst.init = false, I just set it for you in spec:", us->name, "(make sure this is appropriate for all layers that use this spec!)")) {
      us->SetUnique("hyst", true);
      us->hyst.init = false;
    }
    if(lay->CheckError(us->acc.init, quiet, rval,
		  "bias_val.un = GC requires UnitSpec acc.init = false, I just set it for you in spec:", us->name, "(make sure this is appropriate for all layers that use this spec!)")) {
      us->SetUnique("acc", true);
      us->acc.init = false;
    }
  }

  // check for conspecs with correct params
  LeabraUnit* u = (LeabraUnit*)lay->units.Leaf(0);	// taking 1st unit as representative
  if(lay->CheckError(u == NULL, quiet, rval,
		"fourd val layer doesn't have any units:", lay->name)) {
    return false;		// fatal
  }
    
  for(int g=0; g<u->recv.size; g++) {
    LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
    if((recv_gp->prjn == NULL) || (recv_gp->prjn->spec.SPtr() == NULL)) continue;
    LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
    if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec)) {
      if(lay->CheckError(cs->wt_scale.rel > 0.5f, quiet, rval,
		    "fourd val self connections should have wt_scale < .5, I just set it to .1 for you (make sure this is appropriate for all connections that use this spec!)")) {
	cs->SetUnique("wt_scale", true);
	cs->wt_scale.rel = 0.1f;
      }
      if(lay->CheckError(cs->lrate > 0.0f, quiet, rval,
		    "fourd val self connections should have lrate = 0, I just set it for you in spec:", cs->name, "(make sure this is appropriate for all layers that use this spec!)")) {
	cs->SetUnique("lrate", true);
	cs->lrate = 0.0f;
      }
    }
    else if(cs->InheritsFrom(TA_MarkerConSpec)) {
      continue;
    }
  }
  return rval;
}

void FourDValLayerSpec::ReConfig(Network* net, int n_units) {
  LeabraLayer* lay;
  taLeafItr li;
  FOR_ITR_EL(LeabraLayer, lay, net->layers., li) {
    if(lay->spec.SPtr() != this) continue;
    
    if(n_units > 0) {
      lay->SetNUnits(n_units);
    }

    LeabraUnitSpec* us = (LeabraUnitSpec*)lay->unit_spec.SPtr();
    LeabraUnit* u = (LeabraUnit*)lay->units.Leaf(0);	// taking 1st unit as representative
    
    if(fourd.rep == FourDValSpec::LOCALIST) {
      fourd.min_sum_act = .2f;
      kwta.k = 1;
      inhib.type = LeabraInhibSpec::KWTA_AVG_INHIB;
      inhib.kwta_pt = 0.9f;
      us->g_bar.h = .03f; us->g_bar.a = .09f;
      us->act_fun = LeabraUnitSpec::NOISY_LINEAR;
      us->act.thr = .17f;
      us->act.gain = 220.0f;
      us->act.nvar = .01f;
      us->dt.vm = .05f;
      bias_val.un = FourDValBias::GC; bias_val.wt = FourDValBias::NO_WT;
      x_range.min = 0.0f; x_range.max = 1.0f;
      y_range.min = 0.0f; y_range.max = 1.0f;

      for(int g=0; g<u->recv.size; g++) {
	LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
	if((recv_gp->prjn == NULL) || (recv_gp->prjn->spec.SPtr() == NULL)) continue;
	LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
	if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec) ||
	   cs->InheritsFrom(TA_MarkerConSpec)) {
	  continue;
	}
	cs->lmix.err_sb = false; // false: this is critical for linear mapping of vals..
	cs->rnd.mean = 0.1f;
	cs->rnd.var = 0.0f;
	cs->wt_sig.gain = 1.0; cs->wt_sig.off = 1.0; 
      }
    }
    else if(fourd.rep == FourDValSpec::GAUSSIAN) {
      inhib.type = LeabraInhibSpec::KWTA_INHIB;
      inhib.kwta_pt = 0.25f;
      us->g_bar.h = .015f; us->g_bar.a = .045f;
      us->act_fun = LeabraUnitSpec::NOISY_XX1;
      us->act.thr = .25f;
      us->act.gain = 600.0f;
      us->act.nvar = .005f;
      us->dt.vm = .2f;
      bias_val.un = FourDValBias::GC;  bias_val.wt = FourDValBias::NO_WT;
      x_range.min = -.5f; x_range.max = 1.5f;
      y_range.min = -.5f; y_range.max = 1.5f;

      for(int g=0; g<u->recv.size; g++) {
	LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
	if((recv_gp->prjn == NULL) || (recv_gp->prjn->spec.SPtr() == NULL)) continue;
	LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
	if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec) ||
	   cs->InheritsFrom(TA_MarkerConSpec)) {
	  continue;
	}
	cs->lmix.err_sb = true;
	cs->rnd.mean = 0.1f;
	cs->rnd.var = 0.0f;
	cs->wt_sig.gain = 1.0; cs->wt_sig.off = 1.0; 
      }
    }
    us->UpdateAfterEdit();
  }
  UpdateAfterEdit();
}

// todo: deal with lesion flag in lots of special purpose code like this!!!

void FourDValLayerSpec::Compute_WtBias_Val(Unit_Group* ugp, float x_val, float y_val) {
  if(ugp->size < 3) return;	// must be at least a few units..
  Layer* lay = ugp->own_lay;
  fourd.InitVal(x_val, y_val, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
  for(int i=0;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float act = .03f * bias_val.wt_gain * fourd.GetUnitAct(i);
    for(int g=0; g<u->recv.size; g++) {
      LeabraRecvCons* recv_gp = (LeabraRecvCons*)u->recv.FastEl(g);
      LeabraConSpec* cs = (LeabraConSpec*)recv_gp->GetConSpec();
      if(recv_gp->prjn->spec.SPtr()->InheritsFrom(TA_ScalarValSelfPrjnSpec) ||
	 cs->InheritsFrom(TA_MarkerConSpec)) continue;
      for(int ci=0;ci<recv_gp->size;ci++) {
	LeabraCon* cn = (LeabraCon*)recv_gp->Cn(ci);
	cn->wt += act;
	if(cn->wt < cs->wt_limits.min) cn->wt = cs->wt_limits.min;
	if(cn->wt > cs->wt_limits.max) cn->wt = cs->wt_limits.max;
      }
      recv_gp->Init_Weights_post(u);
    }
  }
}

void FourDValLayerSpec::Compute_UnBias_Val(Unit_Group* ugp, float x_val, float y_val) {
  if(ugp->size < 3) return;	// must be at least a few units..
  Layer* lay = ugp->own_lay;
  fourd.InitVal(x_val, y_val, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
  for(int i=0;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float act = bias_val.un_gain * fourd.GetUnitAct(i);
    if(bias_val.un == FourDValBias::GC)
      u->vcb.g_h = act;
    else if(bias_val.un == FourDValBias::BWT)
      u->bias.OwnCn(0)->wt = act;
  }
}

void FourDValLayerSpec::Compute_BiasVal(LeabraLayer* lay, LeabraNetwork* net) {
  if(bias_val.un != FourDValBias::NO_UN) {
    UNIT_GP_ITR(lay, Compute_UnBias_Val(ugp, bias_val.x_val, bias_val.y_val););
  }
  if(bias_val.wt == FourDValBias::WT) {
    UNIT_GP_ITR(lay, Compute_WtBias_Val(ugp, bias_val.x_val, bias_val.y_val););
  }
}

void FourDValLayerSpec::Init_Weights(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Init_Weights(lay, net);
  Compute_BiasVal(lay, net);
}

void FourDValLayerSpec::ClampValue_ugp(Unit_Group* ugp, LeabraNetwork*, float rescale) {
  if(ugp->size < 3) return;	// must be at least a few units..
  FourDValLeabraLayer* lay = (FourDValLeabraLayer*)ugp->own_lay;
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
  // first initialize to zero
  LeabraUnitSpec* us = (LeabraUnitSpec*)ugp->FastEl(0)->GetUnitSpec();
  for(int i=0;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    u->SetExtFlag(Unit::EXT);
    u->ext = 0.0;
  }
  for(int k=0;k<fourd.n_vals;k++) {
    float x_val = lay->GetFourDVal(FourDValLeabraLayer::FOURD_X, FourDValLeabraLayer::FOURD_EXT,
				  k, gp_geom_pos.x, gp_geom_pos.y);
    float y_val = lay->GetFourDVal(FourDValLeabraLayer::FOURD_Y, FourDValLeabraLayer::FOURD_EXT,
				  k, gp_geom_pos.x, gp_geom_pos.y);
    if(fourd.clip_val) {
      x_val = x_val_range.Clip(x_val);
      y_val = y_val_range.Clip(y_val);
    }
    fourd.InitVal(x_val, y_val, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
    for(int i=0;i<ugp->size;i++) {
      LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
      float act = rescale * fourd.GetUnitAct(i);
      if(act < us->opt_thresh.send)
	act = 0.0f;
      u->ext += act;
    }
  }
}

void FourDValLayerSpec::ReadValue(FourDValLeabraLayer* lay, LeabraNetwork* net) {
  UNIT_GP_ITR(lay, ReadValue_ugp(lay, ugp, net); );
}

void FourDValLayerSpec::ReadValue_ugp(FourDValLeabraLayer* lay, Unit_Group* ugp, LeabraNetwork* net) {
  if(ugp->size < 3) return;	// must be at least a few units..
  fourd.InitVal(0.0f, 0.0f, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
  if(fourd.n_vals == 1) {	// special case
    float x_avg = 0.0f; float y_avg = 0.0f;
    float sum_act = 0.0f;
    for(int i=0;i<ugp->size;i++) {
      LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
      LeabraUnitSpec* us = (LeabraUnitSpec*)u->GetUnitSpec();
      float x_cur, y_cur;  fourd.GetUnitVal(i, x_cur, y_cur);
      float act_val = us->clamp_range.Clip(u->act_eq) / us->clamp_range.max; // clipped & normalized!
      x_avg += x_cur * act_val;
      y_avg += y_cur * act_val;
      sum_act += act_val;
    }
    sum_act = MAX(sum_act, fourd.min_sum_act);
    if(sum_act > 0.0f) {
      x_avg /= sum_act; y_avg /= sum_act;
    }
    // encode the value
    lay->SetFourDVal(x_avg, FourDValLeabraLayer::FOURD_X, FourDValLeabraLayer::FOURD_ACT,
		    0, gp_geom_pos.x, gp_geom_pos.y);
    lay->SetFourDVal(y_avg, FourDValLeabraLayer::FOURD_Y, FourDValLeabraLayer::FOURD_ACT,
		    0, gp_geom_pos.x, gp_geom_pos.y);
  }
  else {			// multiple items
    // first find the max values, using sum of -1..+1 region
    static ValIdx_Array sort_ary;
    sort_ary.Reset();
    for(int i=0;i<ugp->size;i++) {
      float sum = 0.0f;
      float nsum = 0.0f;
      for(int x=-1;x<=1;x++) {
	for(int y=-1;y<=1;y++) {
	  int idx = i + y * lay->un_geom.x + x;
	  if(idx < 0 || idx >= ugp->size) continue;
	  LeabraUnit* u = (LeabraUnit*)ugp->FastEl(idx);
	  LeabraUnitSpec* us = (LeabraUnitSpec*)u->GetUnitSpec();
	  float act_val = us->clamp_range.Clip(u->act_eq) / us->clamp_range.max; // clipped & normalized!
	  nsum += 1.0f;
	  sum += act_val;
	}
      }
      if(nsum > 0.0f) sum /= nsum;
      ValIdx vi(sum, i);
      sort_ary.Add(vi);
    }
    sort_ary.Sort();
    float mn_x = fourd.mn_dst * fourd.un_width_x * x_range.Range();
    float mn_y = fourd.mn_dst * fourd.un_width_y * y_range.Range();
    float mn_dist = mn_x * mn_x + mn_y * mn_y;
    int outi = 0;  int j = 0;
    while((outi < fourd.n_vals) && (j < sort_ary.size)) {
      ValIdx& vi = sort_ary[sort_ary.size - j - 1]; // going backward through sort_ary
      float x_cur, y_cur;  fourd.GetUnitVal(vi.idx, x_cur, y_cur);
      // check distance from all previous!
      float my_mn = x_range.Range() + y_range.Range();
      for(int k=0; k<j; k++) {
	ValIdx& vo = sort_ary[sort_ary.size - k - 1];
	if(vo.val == -1.0f) continue; // guy we skipped over before
	float x_prv, y_prv;  fourd.GetUnitVal(vo.idx, x_prv, y_prv);
	float x_d = x_cur - x_prv; float y_d = y_cur - y_prv; 
	float dist = x_d * x_d + y_d * y_d;
	my_mn = MIN(dist, my_mn);
      }
      if(my_mn < mn_dist) { vi.val = -1.0f; j++; continue; } // mark with -1 so we know we skipped it

      // encode the value
      lay->SetFourDVal(x_cur, FourDValLeabraLayer::FOURD_X, FourDValLeabraLayer::FOURD_ACT,
		      0, gp_geom_pos.x, gp_geom_pos.y);
      lay->SetFourDVal(y_cur, FourDValLeabraLayer::FOURD_Y, FourDValLeabraLayer::FOURD_ACT,
		      0, gp_geom_pos.x, gp_geom_pos.y);
      j++; outi++;
    }
  }
}

void FourDValLayerSpec::LabelUnits_ugp(Unit_Group* ugp) {
  if(ugp->size < 3) return;	// must be at least a few units..
  Layer* lay = ugp->own_lay;
  fourd.InitVal(0.0f, 0.0f, lay->un_geom.x, lay->un_geom.y, x_range.min, x_range.range, y_range.min, y_range.range);
  for(int i=0;i<ugp->size;i++) {
    LeabraUnit* u = (LeabraUnit*)ugp->FastEl(i);
    float x_cur, y_cur; fourd.GetUnitVal(i, x_cur, y_cur);
    u->name = (String)x_cur + "," + String(y_cur);
  }
}

void FourDValLayerSpec::LabelUnits(LeabraLayer* lay, LeabraNetwork* net) {
  UNIT_GP_ITR(lay, LabelUnits_ugp(ugp); );
}

void FourDValLayerSpec::LabelUnitsNet(LeabraNetwork* net) {
  LeabraLayer* l;
  taLeafItr li;
  FOR_ITR_EL(LeabraLayer, l, net->layers., li) {
    if(l->spec.SPtr() == this)
      LabelUnits(l, net);
  }
}

void FourDValLayerSpec::HardClampExt(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Compute_HardClamp(lay, net);
}

void FourDValLayerSpec::Settle_Init_TargFlags_Layer(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Settle_Init_TargFlags_Layer(lay, net);
  // need to actually copy over targ to ext vals!
  FourDValLeabraLayer* tdlay = (FourDValLeabraLayer*)lay;
  if(lay->ext_flag & Unit::TARG) {	// only process target layers..
    if(net->phase == LeabraNetwork::PLUS_PHASE) {
      UNIT_GP_ITR(tdlay, 
		  for(int k=0;k<fourd.n_vals;k++) {
		    TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
		    float x_val = tdlay->GetFourDVal(FourDValLeabraLayer::FOURD_X,
						    FourDValLeabraLayer::FOURD_TARG,
						    k, gp_geom_pos.x, gp_geom_pos.y);
		    float y_val = tdlay->GetFourDVal(FourDValLeabraLayer::FOURD_Y,
						    FourDValLeabraLayer::FOURD_TARG,
						    k, gp_geom_pos.x, gp_geom_pos.y);
		    tdlay->SetFourDVal(x_val, FourDValLeabraLayer::FOURD_X,
				      FourDValLeabraLayer::FOURD_EXT,
				      k, gp_geom_pos.x, gp_geom_pos.y);
		    tdlay->SetFourDVal(y_val, FourDValLeabraLayer::FOURD_Y,
				      FourDValLeabraLayer::FOURD_EXT,
				      k, gp_geom_pos.x, gp_geom_pos.y);
		  }
		  );
    }
  }
}


void FourDValLayerSpec::Settle_Init_Layer(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Settle_Init_Layer(lay, net);

  FourDValLeabraLayer* tdlay = (FourDValLeabraLayer*)lay;
  tdlay->UpdateFourDValsGeom();	// quick, make sure no mismatch

  if(lay->hard_clamped) return;

  LeabraUnit* u;
  taLeafItr i;
  FOR_ITR_EL(LeabraUnit, u, lay->units., i) {
    LeabraConSpec* bspec = (LeabraConSpec*)u->GetUnitSpec()->bias_spec.SPtr();
    u->bias_scale = bspec->wt_scale.abs;  // still have absolute scaling if wanted..
    u->bias_scale /= 100.0f; 		  // keep a constant scaling so it doesn't depend on network size!
  }
}

void FourDValLayerSpec::Compute_HardClamp(LeabraLayer* lay, LeabraNetwork* net) {
  if(fourd.clamp_pat) {
    inherited::Compute_HardClamp(lay, net);
    return;
  }
  if(!(lay->ext_flag & Unit::EXT)) {
    lay->hard_clamped = false;
    return;
  }
  // allow for soft-clamping: translates pattern into exts first
  UNIT_GP_ITR(lay, if(ugp->size > 2) { ClampValue_ugp(ugp, net); } );
  // now check for actual hard clamping
  if(!clamp.hard) {
    lay->hard_clamped = false;
    return;
  }
  HardClampExt(lay, net);
}

void FourDValLayerSpec::Compute_CycleStats(LeabraLayer* lay, LeabraNetwork* net) {
  inherited::Compute_CycleStats(lay, net);
  ReadValue((FourDValLeabraLayer*)lay, net);		// always read out the value
}

void FourDValLayerSpec::PostSettle(LeabraLayer* ly, LeabraNetwork* net) {
  inherited::PostSettle(ly, net);
  FourDValLeabraLayer* lay = (FourDValLeabraLayer*)ly;
  UNIT_GP_ITR(lay, PostSettle_ugp(lay, ugp, net); );
}

void FourDValLayerSpec::PostSettle_ugp(FourDValLeabraLayer* lay, Unit_Group* ugp, LeabraNetwork* net) {
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();

  bool no_plus_testing = false;
  if(net->no_plus_test && (net->train_mode == LeabraNetwork::TEST)) {
    no_plus_testing = true;
  }

  for(int k=0;k<fourd.n_vals;k++) {
    float x_val, y_val, x_m, y_m, x_p, y_p;
    lay->GetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT,
		      k, gp_geom_pos.x, gp_geom_pos.y);
    lay->GetFourDVals(x_m, y_m, FourDValLeabraLayer::FOURD_ACT_M,
		      k, gp_geom_pos.x, gp_geom_pos.y);
    lay->GetFourDVals(x_p, y_p, FourDValLeabraLayer::FOURD_ACT_P,
		      k, gp_geom_pos.x, gp_geom_pos.y);

    switch(net->phase_order) {
    case LeabraNetwork::MINUS_PLUS:
      if(no_plus_testing) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
			 k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(0.0f, 0.0f, FourDValLeabraLayer::FOURD_ACT_DIF,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	if(net->phase == LeabraNetwork::MINUS_PHASE) {
	  lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	}
	else {
	  lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	  lay->SetFourDVals(x_val - x_m, y_val - y_m, FourDValLeabraLayer::FOURD_ACT_DIF,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	}
      }
      break;
    case LeabraNetwork::PLUS_MINUS:
      if(no_plus_testing) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
			 k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P,
			 k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(0.0f, 0.0f, FourDValLeabraLayer::FOURD_ACT_DIF,
			 k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	if(net->phase == LeabraNetwork::MINUS_PHASE) {
	  lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
			   k, gp_geom_pos.x, gp_geom_pos.y);
	  lay->SetFourDVals(x_p - x_val, y_p - y_val, FourDValLeabraLayer::FOURD_ACT_DIF,
			   k, gp_geom_pos.x, gp_geom_pos.y);
	}
	else {
	  lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	}
      }
      break;
    case LeabraNetwork::PLUS_ONLY:
      lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
		      k, gp_geom_pos.x, gp_geom_pos.y);
      lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P,
		      k, gp_geom_pos.x, gp_geom_pos.y);
      lay->SetFourDVals(0.0f, 0.0f, FourDValLeabraLayer::FOURD_ACT_DIF,
		      k, gp_geom_pos.x, gp_geom_pos.y);
      break;
    case LeabraNetwork::MINUS_PLUS_NOTHING:
    case LeabraNetwork::MINUS_PLUS_MINUS:
      // don't use actual phase values because pluses might be minuses with testing
      if(net->phase_no == 0) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
			 k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else if(net->phase_no == 1) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(x_val - x_m, y_val - y_m, FourDValLeabraLayer::FOURD_ACT_DIF,
			 k, gp_geom_pos.x, gp_geom_pos.y);
	if(no_plus_testing) {
	  // update act_m because it is actually another test case!
	  lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
			   k, gp_geom_pos.x, gp_geom_pos.y);
	}
      }
      else {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M2,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(x_p - x_val, y_p - y_val, FourDValLeabraLayer::FOURD_ACT_DIF,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      break;
    case LeabraNetwork::PLUS_NOTHING:
      // don't use actual phase values because pluses might be minuses with testing
      if(net->phase_no == 0) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(x_p - x_val, y_p - y_val, FourDValLeabraLayer::FOURD_ACT_DIF,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      break;
    case LeabraNetwork::MINUS_PLUS_PLUS:
      // don't use actual phase values because pluses might be minuses with testing
      if(net->phase_no == 0) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else if(net->phase_no == 1) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(x_val - x_m, y_val - y_m, FourDValLeabraLayer::FOURD_ACT_DIF,
			 k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P2,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(x_val - x_p, y_val - y_p, FourDValLeabraLayer::FOURD_ACT_DIF2,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      break;
    case LeabraNetwork::MINUS_PLUS_PLUS_NOTHING:
    case LeabraNetwork::MINUS_PLUS_PLUS_MINUS:
      // don't use actual phase values because pluses might be minuses with testing
      if(net->phase_no == 0) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else if(net->phase_no == 1) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(x_val - x_m, y_val - y_m, FourDValLeabraLayer::FOURD_ACT_DIF,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else if(net->phase_no == 2) {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_P2,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(x_val - x_p, y_val - y_p, FourDValLeabraLayer::FOURD_ACT_DIF2,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      else {
	lay->SetFourDVals(x_val, y_val, FourDValLeabraLayer::FOURD_ACT_M2,
			k, gp_geom_pos.x, gp_geom_pos.y);
	lay->SetFourDVals(x_p - x_val, y_p - y_val, FourDValLeabraLayer::FOURD_ACT_DIF2,
			k, gp_geom_pos.x, gp_geom_pos.y);
      }
      break;
    }
  }
}

float FourDValLayerSpec::Compute_SSE_ugp(Unit_Group* ugp, LeabraLayer* ly, int& n_vals) {
  FourDValLeabraLayer* lay = (FourDValLeabraLayer*)ly;
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
  LeabraUnitSpec* us = (LeabraUnitSpec*)ugp->FastEl(0)->GetUnitSpec();
  float rval = 0.0f;
  for(int k=0;k<fourd.n_vals;k++) { // first loop over and find potential target values
    float x_targ = lay->GetFourDVal(FourDValLeabraLayer::FOURD_X, FourDValLeabraLayer::FOURD_TARG, k,
				   gp_geom_pos.x, gp_geom_pos.y);
    float y_targ = lay->GetFourDVal(FourDValLeabraLayer::FOURD_Y, FourDValLeabraLayer::FOURD_TARG, k,
				   gp_geom_pos.x, gp_geom_pos.y);
    // only count if target value is within range -- otherwise considered a non-target
    if(x_val_range.RangeTestEq(x_targ) && y_val_range.RangeTestEq(y_targ)) {
      n_vals++;
      // now find minimum dist actual activations
      float mn_dist = taMath::flt_max;
      for(int j=0;j<fourd.n_vals;j++) {
	float x_act_m = lay->GetFourDVal(FourDValLeabraLayer::FOURD_X, FourDValLeabraLayer::FOURD_ACT_M,
					j, gp_geom_pos.x, gp_geom_pos.y);
	float y_act_m = lay->GetFourDVal(FourDValLeabraLayer::FOURD_Y, FourDValLeabraLayer::FOURD_ACT_M,
					j, gp_geom_pos.x, gp_geom_pos.y);
	float dx = x_targ - x_act_m;
	float dy = y_targ - y_act_m;
	if(fabsf(dx) < us->sse_tol) dx = 0.0f;
	if(fabsf(dy) < us->sse_tol) dy = 0.0f;
	float dist = dx * dx + dy * dy;
	if(dist < mn_dist) {
	  mn_dist = dist;
	  lay->SetFourDVal(dx, FourDValLeabraLayer::FOURD_X, FourDValLeabraLayer::FOURD_ERR,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	  lay->SetFourDVal(dy, FourDValLeabraLayer::FOURD_Y, FourDValLeabraLayer::FOURD_ERR,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	  lay->SetFourDVal(dx*dx, FourDValLeabraLayer::FOURD_X, FourDValLeabraLayer::FOURD_SQERR,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	  lay->SetFourDVal(dy*dy, FourDValLeabraLayer::FOURD_Y, FourDValLeabraLayer::FOURD_SQERR,
			  k, gp_geom_pos.x, gp_geom_pos.y);
	}
      }
      rval += mn_dist;
    }
  }
  return rval;
}

float FourDValLayerSpec::Compute_SSE(LeabraLayer* lay, LeabraNetwork*, 
				    int& n_vals, bool unit_avg, bool sqrt) {
  n_vals = 0;
  if(!(lay->ext_flag & (Unit::TARG | Unit::COMP))) return 0.0f;
  lay->sse = 0.0f;
  UNIT_GP_ITR(lay, 
	      lay->sse += Compute_SSE_ugp(ugp, lay, n_vals);
	      );
  float rval = lay->sse;
  if(unit_avg && n_vals > 0)
    lay->sse /= (float)n_vals;
  if(sqrt)
    lay->sse = sqrtf(lay->sse);
  if(lay->HasLayerFlag(Layer::NO_ADD_SSE) ||
     ((lay->ext_flag & Unit::COMP) && lay->HasLayerFlag(Layer::NO_ADD_COMP_SSE))) {
    rval = 0.0f;
    n_vals = 0;
  }
  return rval;
}

float FourDValLayerSpec::Compute_NormErr_ugp(LeabraLayer* ly, Unit_Group* ugp,
					   LeabraInhib* thr, LeabraNetwork* net) {
  FourDValLeabraLayer* lay = (FourDValLeabraLayer*)ly;
  TwoDCoord gp_geom_pos = ugp->GetGpGeomPos();
  LeabraUnitSpec* us = (LeabraUnitSpec*)ugp->FastEl(0)->GetUnitSpec();
  float rval = 0.0f;
  for(int k=0;k<fourd.n_vals;k++) { // first loop over and find potential target values
    float x_targ = lay->GetFourDVal(FourDValLeabraLayer::FOURD_X, FourDValLeabraLayer::FOURD_TARG,
				   k, gp_geom_pos.x, gp_geom_pos.y);
    float y_targ = lay->GetFourDVal(FourDValLeabraLayer::FOURD_Y, FourDValLeabraLayer::FOURD_TARG,
				   k, gp_geom_pos.x, gp_geom_pos.y);
    // only count if target value is within range -- otherwise considered a non-target
    if(x_val_range.RangeTestEq(x_targ) && y_val_range.RangeTestEq(y_targ)) {
      // now find minimum dist actual activations
      float mn_dist = taMath::flt_max;
      for(int j=0;j<fourd.n_vals;j++) {
	float x_act_m = lay->GetFourDVal(FourDValLeabraLayer::FOURD_X, FourDValLeabraLayer::FOURD_ACT_M,
					j, gp_geom_pos.x, gp_geom_pos.y);
	float y_act_m = lay->GetFourDVal(FourDValLeabraLayer::FOURD_Y, FourDValLeabraLayer::FOURD_ACT_M,
					j, gp_geom_pos.x, gp_geom_pos.y);
	float dx = x_targ - x_act_m;
	float dy = y_targ - y_act_m;
	if(fabsf(dx) < us->sse_tol) dx = 0.0f;
	if(fabsf(dy) < us->sse_tol) dy = 0.0f;
	float dist = fabsf(dx) + fabsf(dy); // only diff from sse!
	if(dist < mn_dist)
	  mn_dist = dist;
      }
      rval += mn_dist;
    }
  }
  return rval;
}

float FourDValLayerSpec::Compute_NormErr(LeabraLayer* lay, LeabraNetwork* net) {
  lay->norm_err = -1.0f;					 // assume not contributing
  if(!(lay->ext_flag & (Unit::TARG | Unit::COMP))) return -1.0f; // indicates not applicable

  float nerr = 0.0f;
  float ntot = 0;
  if((inhib_group != ENTIRE_LAYER) && (lay->units.gp.size > 0)) {
    for(int g=0; g<lay->units.gp.size; g++) {
      LeabraUnit_Group* rugp = (LeabraUnit_Group*)lay->units.gp[g];
      nerr += Compute_NormErr_ugp(lay, rugp, (LeabraInhib*)rugp, net);
      ntot += x_range.range + y_range.range;
    }
  }
  else {
    nerr += Compute_NormErr_ugp(lay, &(lay->units), (LeabraInhib*)lay, net);
    ntot += x_range.range + y_range.range;
  }
  if(ntot == 0.0f) return -1.0f;

  lay->norm_err = nerr / ntot;
  if(lay->norm_err > 1.0f) lay->norm_err = 1.0f;

  if(lay->HasLayerFlag(Layer::NO_ADD_SSE) ||
     ((lay->ext_flag & Unit::COMP) && lay->HasLayerFlag(Layer::NO_ADD_COMP_SSE)))
    return -1.0f;		// no contributarse

  return lay->norm_err;
}

*/

////////////////////////////////////////////////////////////
//	V1RFPrjnSpec

void V1RFPrjnSpec::Initialize() {
  init_wts = true;
  wrap = false;
  dog_surr_mult = 1.0f;
}

void V1RFPrjnSpec::UpdateAfterEdit_impl() {
  inherited::UpdateAfterEdit_impl();
  rf_spec.name = name + "_rf_spec";
}

void V1RFPrjnSpec::Connect_impl(Projection* prjn) {
  if(!(bool)prjn->from)	return;
  if(prjn->layer->units.leaves == 0) // an empty layer!
    return;
  if(TestWarning(prjn->layer->units.gp.size == 0, "Connect_impl",
		 "requires recv layer to have unit groups!")) {
    return;
  }

  rf_spec.InitFilters();	// this one call initializes all filter info once and for all!
  // renorm the dog net filter to 1 abs max!
  if(rf_spec.filter_type == GaborV1Spec::BLOB) {
    for(int i=0;i<rf_spec.blob_specs.size;i++) {
      DoGFilterSpec* df = (DoGFilterSpec*)rf_spec.blob_specs.FastEl(i);
      taMath_float::vec_norm_abs_max(&(df->net_filter));
    }
  }
  TestWarning(rf_spec.n_filters != prjn->layer->un_geom.n,
	      "number of filters from rf_spec:", (String)rf_spec.n_filters,
	      "does not match layer un_geom.n:", (String)prjn->layer->un_geom.n);

  TwoDCoord rf_width = rf_spec.rf_width;
  int n_cons = rf_width.Product();
  TwoDCoord rf_half_wd = rf_width / 2;
  TwoDCoord ru_geo = prjn->layer->gp_geom;

  TwoDCoord su_geo = prjn->from->un_geom;

  TwoDCoord ruc;
  for(int alloc_loop=1; alloc_loop>=0; alloc_loop--) {
    for(ruc.y = 0; ruc.y < ru_geo.y; ruc.y++) {
      for(ruc.x = 0; ruc.x < ru_geo.x; ruc.x++) {

	Unit_Group* ru_gp = prjn->layer->FindUnitGpFmCoord(ruc);
	if(ru_gp == NULL) continue;

	TwoDCoord su_st;
	if(wrap) {
	  su_st.x = (int)floor((float)ruc.x * rf_move.x) - rf_half_wd.x;
	  su_st.y = (int)floor((float)ruc.y * rf_move.y) - rf_half_wd.y;
	}
	else {
	  su_st.x = (int)floor((float)ruc.x * rf_move.x);
	  su_st.y = (int)floor((float)ruc.y * rf_move.y);
	}

	su_st.WrapClip(wrap, su_geo);
	TwoDCoord su_ed = su_st + rf_width;
	if(wrap) {
	  su_ed.WrapClip(wrap, su_geo); // just wrap ends too
	}
	else {
	  if(su_ed.x > su_geo.x) {
	    su_ed.x = su_geo.x; su_st.x = su_ed.x - rf_width.x;
	  }
	  if(su_ed.y > su_geo.y) {
	    su_ed.y = su_geo.y; su_st.y = su_ed.y - rf_width.y;
	  }
	}

	for(int rui=0;rui<ru_gp->size;rui++) {
	  Unit* ru_u = (Unit*)ru_gp->FastEl(rui);
	  if(!alloc_loop)
	    ru_u->RecvConsPreAlloc(n_cons, prjn);

	  TwoDCoord suc;
	  TwoDCoord suc_wrp;
	  for(suc.y = 0; suc.y < rf_width.y; suc.y++) {
	    for(suc.x = 0; suc.x < rf_width.x; suc.x++) {
	      suc_wrp = su_st + suc;
	      if(suc_wrp.WrapClip(wrap, su_geo) && !wrap)
		continue;
	      Unit* su_u = prjn->from->FindUnitFmCoord(suc_wrp);
	      if(su_u == NULL) continue;
	      if(!self_con && (su_u == ru_u)) continue;
	      ru_u->ConnectFrom(su_u, prjn, alloc_loop); // don't check: saves lots of time!
	    }
	  }
	}
      }
    }
    if(alloc_loop) { // on first pass through alloc loop, do sending allocations
      prjn->from->SendConsPostAlloc(prjn);
    }
  }
}

void V1RFPrjnSpec::C_Init_Weights(Projection* prjn, RecvCons* cg, Unit* ru) {
  Unit_Group* rugp = (Unit_Group*)ru->GetOwner();
  int recv_idx = ru->pos.y * rugp->geom.x + ru->pos.x;
  
  bool on_rf = true;
  if(prjn->from->name.contains("_off"))
    on_rf = false;
  DoGFilterSpec::ColorChannel col_chan = DoGFilterSpec::BLACK_WHITE;
  if(prjn->from->name.contains("_rg_"))
    col_chan = DoGFilterSpec::RED_GREEN;
  else if(prjn->from->name.contains("_by_"))
    col_chan = DoGFilterSpec::BLUE_YELLOW;

  int send_x = rf_spec.rf_width.x;
  if(rf_spec.filter_type == GaborV1Spec::BLOB) {
    // color is outer-most dimension, and if it doesn't match, then bail
    int clr_dx = (recv_idx / (rf_spec.blob_rf.n_sizes * 2) % 2);
    DoGFilterSpec* df = (DoGFilterSpec*)rf_spec.blob_specs.SafeEl(recv_idx);
    if(!df) return;		// oops
    if(col_chan != DoGFilterSpec::BLACK_WHITE) {
      // outer-most mod is color, after phases (2) and sizes (inner)
      if((clr_dx == 0 && col_chan == DoGFilterSpec::BLUE_YELLOW) ||
	 (clr_dx == 1 && col_chan == DoGFilterSpec::RED_GREEN)) {
	for(int i=0; i<cg->size; i++)
	  cg->Cn(i)->wt = 0.0f;
	return;			// bail if not our channel.
      }
    }
    for(int i=0; i<cg->size; i++) {
      int su_x = i % send_x;
      int su_y = i / send_x;
      float val = rf_spec.gabor_rf.amp * df->net_filter.SafeEl(su_x, su_y);
      if(on_rf) {
	if(df->on_sigma > df->off_sigma) val *= dog_surr_mult;
	if(val > 0.0f) cg->Cn(i)->wt = val;
	else	       cg->Cn(i)->wt = 0.0f;
      }
      else {
	if(df->off_sigma > df->on_sigma) val *= dog_surr_mult;
	if(val < 0.0f) 	cg->Cn(i)->wt = -val;
	else		cg->Cn(i)->wt = 0.0f;
      }
    }
  }
  else {			// GABOR
    GaborFilterSpec* gf = (GaborFilterSpec*)rf_spec.gabor_specs.SafeEl(recv_idx);
    if(!gf) return;		// oops
    for(int i=0; i<cg->size; i++) {
      int su_x = i % send_x;
      int su_y = i / send_x;
      float val = gf->filter.SafeEl(su_x, su_y);
      if(on_rf) {
	if(val > 0.0f) cg->Cn(i)->wt = val;
	else	       cg->Cn(i)->wt = 0.0f;
      }
      else {
	if(val < 0.0f) 	cg->Cn(i)->wt = -val;
	else		cg->Cn(i)->wt = 0.0f;
      }
    }
  }
}

bool V1RFPrjnSpec::TrgRecvFmSend(int send_x, int send_y) {
  trg_send_geom.x = send_x;
  trg_send_geom.y = send_y;

  if(wrap)
    trg_recv_geom = (trg_send_geom / rf_move);
  else
    trg_recv_geom = (trg_send_geom / rf_move) - 1;

  // now fix it the other way
  if(wrap)
    trg_send_geom = (trg_recv_geom * rf_move);
  else
    trg_send_geom = ((trg_recv_geom +1) * rf_move);

  DataChanged(DCR_ITEM_UPDATED);
  return (trg_send_geom.x == send_x && trg_send_geom.y == send_y);
}

bool V1RFPrjnSpec::TrgSendFmRecv(int recv_x, int recv_y) {
  trg_recv_geom.x = recv_x;
  trg_recv_geom.y = recv_y;

  if(wrap)
    trg_send_geom = (trg_recv_geom * rf_move);
  else
    trg_send_geom = ((trg_recv_geom+1) * rf_move);

  // now fix it the other way
  if(wrap)
    trg_recv_geom = (trg_send_geom / rf_move);
  else
    trg_recv_geom = (trg_send_geom / rf_move) - 1;

  DataChanged(DCR_ITEM_UPDATED);
  return (trg_recv_geom.x == recv_x && trg_recv_geom.y == recv_y);
}

void V1RFPrjnSpec::GraphFilter(DataTable* graph_data, int recv_unit_no) {
  rf_spec.GraphFilter(graph_data, recv_unit_no);
}

void V1RFPrjnSpec::GridFilter(DataTable* graph_data) {
  rf_spec.GridFilter(graph_data);
}

////////////////////////////////////////////////////////////
//	SaliencyPrjnSpec

void SaliencyPrjnSpec::Initialize() {
  //  init_wts = true;
  convergence = 1;
  reciprocal = false;
  feat_only = true;
  feat_gps = 2;
  dog_wts.color_chan = DoGFilterSpec::BLACK_WHITE;
  dog_wts.filter_width = 3;
  dog_wts.filter_size = 7;
  dog_wts.on_sigma = 1;
  dog_wts.off_sigma = 2;
  wt_mult = 1.0f;
  surr_mult = 1.0f;
  units_per_feat_gp = 4;
}

void SaliencyPrjnSpec::Connect_impl(Projection* prjn) {
  if(!(bool)prjn->from)	return;
  if(prjn->layer->units.leaves == 0) // an empty layer!
    return;
  if(TestWarning(prjn->layer->units.gp.size == 0, "Connect_impl",
		 "requires recv layer to have unit groups!")) {
    return;
  }
  if(TestWarning(prjn->from->units.gp.size == 0, "Connect_impl",
		 "requires sending layer to have unit groups!")) {
    return;
  }
  if(feat_only)
    Connect_feat_only(prjn);
  else
    Connect_full_dog(prjn);
}

void SaliencyPrjnSpec::Connect_feat_only(Projection* prjn) {
  Layer* recv_lay = prjn->layer;
  Layer* send_lay = prjn->from;

  if(reciprocal) {
    recv_lay = prjn->from;
    send_lay = prjn->layer;

    Unit* su;
    taLeafItr su_itr;
    FOR_ITR_EL(Unit, su, send_lay->units., su_itr) {
      su->RecvConsPreAlloc(1, prjn); // only ever have 1!
    }
  }

  TwoDCoord rug_geo = recv_lay->gp_geom;
  TwoDCoord ruu_geo = recv_lay->un_geom;
  TwoDCoord su_geo = send_lay->gp_geom;

  int fltsz = convergence;
  int sg_sz_tot = fltsz * fltsz;

  int feat_no = 0;
  TwoDCoord rug;
  for(int alloc_loop=1; alloc_loop>=0; alloc_loop--) {
    for(rug.y = 0; rug.y < rug_geo.y; rug.y++) {
      for(rug.x = 0; rug.x < rug_geo.x; rug.x++, feat_no++) {
	Unit_Group* ru_gp = recv_lay->FindUnitGpFmCoord(rug);
	if(!ru_gp) continue;

	int rui = 0;
	TwoDCoord ruc;
	for(ruc.y = 0; ruc.y < ruu_geo.y; ruc.y++) {
	  for(ruc.x = 0; ruc.x < ruu_geo.x; ruc.x++, rui++) {

	    TwoDCoord su_st = ruc*convergence;

	    Unit* ru_u = (Unit*)ru_gp->SafeEl(rui);
	    if(!ru_u) break;
	    if(!reciprocal && !alloc_loop)
	      ru_u->RecvConsPreAlloc(sg_sz_tot, prjn);

	    TwoDCoord suc;
	    for(suc.y = 0; suc.y < fltsz; suc.y++) {
	      for(suc.x = 0; suc.x < fltsz; suc.x++) {
		TwoDCoord sugc = su_st + suc;
		Unit_Group* su_gp = send_lay->FindUnitGpFmCoord(sugc);
		if(!su_gp) continue;

		Unit* su_u = (Unit*)su_gp->SafeEl(feat_no);
		if(su_u) {
		  if(reciprocal)
		    su_u->ConnectFrom(ru_u, prjn, alloc_loop);
		  else
		    ru_u->ConnectFrom(su_u, prjn, alloc_loop);
		}
	      }
	    }
	  }
	}
      }
    }
    if(alloc_loop) { // on first pass through alloc loop, do sending allocations
      prjn->from->SendConsPostAlloc(prjn);
    }
  }
}

void SaliencyPrjnSpec::Connect_full_dog(Projection* prjn) {
  dog_wts.UpdateFilter();
  taMath_float::vec_norm_abs_max(&(dog_wts.net_filter)); // renorm to abs max = 1

  if(TestError(reciprocal, "Connect_full_dog", "full DoG connection not supported in reciprocal mode!!!")) return;

  Layer* recv_lay = prjn->layer;
  Layer* send_lay = prjn->from;
  TwoDCoord rug_geo = recv_lay->gp_geom;
  TwoDCoord ruu_geo = recv_lay->un_geom;
  TwoDCoord su_geo = send_lay->gp_geom;

  int fltwd = dog_wts.filter_width; // no convergence..
  int fltsz = dog_wts.filter_size * convergence;

  int sg_sz_tot = fltsz * fltsz;
  Unit_Group* su_gp0 = (Unit_Group*)send_lay->units.gp[0];
  int alloc_no = sg_sz_tot * su_gp0->size;

  units_per_feat_gp = su_gp0->size / feat_gps;

  int feat_no = 0;
  TwoDCoord rug;
  for(rug.y = 0; rug.y < rug_geo.y; rug.y++) {
    for(rug.x = 0; rug.x < rug_geo.x; rug.x++, feat_no++) {
      Unit_Group* ru_gp = recv_lay->FindUnitGpFmCoord(rug);
      if(!ru_gp) continue;

      int rui = 0;
      TwoDCoord ruc;
      for(ruc.y = 0; ruc.y < ruu_geo.y; ruc.y++) {
	for(ruc.x = 0; ruc.x < ruu_geo.x; ruc.x++, rui++) {

	  TwoDCoord su_st = ruc*convergence - convergence*fltwd;

	  Unit* ru_u = (Unit*)ru_gp->SafeEl(rui);
	  if(!ru_u) break;
	  ru_u->RecvConsPreAlloc(alloc_no, prjn);

	  TwoDCoord suc;
	  for(suc.y = 0; suc.y < fltsz; suc.y++) {
	    for(suc.x = 0; suc.x < fltsz; suc.x++) {
	      TwoDCoord sugc = su_st + suc;
	      Unit_Group* su_gp = send_lay->FindUnitGpFmCoord(sugc);
	      if(!su_gp) continue;

	      for(int sui=0;sui<su_gp->size;sui++) {
		Unit* su_u = (Unit*)su_gp->FastEl(sui);
		if(!self_con && (su_u == ru_u)) continue;
		ru_u->ConnectFrom(su_u, prjn);
	      }
	    }
	  }
	}
      }
    }
  }
}

void SaliencyPrjnSpec::C_Init_Weights(Projection* prjn, RecvCons* cg, Unit* ru) {
  if(feat_only) {		// just use regular..
    inherited::C_Init_Weights(prjn, cg, ru);
    return;
  }
  Layer* recv_lay = prjn->layer;
  Layer* send_lay = prjn->from;

  int fltwd = dog_wts.filter_width; // no convergence.
  int fltsz = dog_wts.filter_size * convergence;

  Unit_Group* su_gp0 = (Unit_Group*)send_lay->units.gp[0];
  units_per_feat_gp = su_gp0->size / feat_gps;

  Unit_Group* rugp = (Unit_Group*)ru->GetOwner();

  TwoDCoord rug_geo = recv_lay->gp_geom;
  TwoDCoord rgp_pos = rugp->GetGpGeomPos();
  
  int feat_no = rgp_pos.y * rug_geo.x + rgp_pos.x; // unit group index
  int my_feat_gp = feat_no / units_per_feat_gp;
  int fg_st = my_feat_gp * units_per_feat_gp;
  int fg_ed = fg_st + units_per_feat_gp;

  TwoDCoord ruu_geo = recv_lay->un_geom;
  TwoDCoord su_st;		// su starting (left)
  su_st.x = (ru->idx % ruu_geo.x)*convergence - convergence*fltwd;
  su_st.y = (ru->idx / ruu_geo.x)*convergence - convergence*fltwd;

  TwoDCoord su_geo = send_lay->gp_geom;

  int su_idx = 0;
  TwoDCoord suc;
  for(suc.y = 0; suc.y < fltsz; suc.y++) {
    for(suc.x = 0; suc.x < fltsz; suc.x++) {
      TwoDCoord sugc = su_st + suc;
      Unit_Group* su_gp = send_lay->FindUnitGpFmCoord(sugc);
      if(!su_gp) continue;

      float wt = wt_mult * dog_wts.net_filter.FastEl(suc.x/convergence, 
						     suc.y/convergence);

      if(wt > 0) {
	for(int sui=0;sui<su_gp->size;sui++) {
	  if(sui == feat_no)
	    cg->Cn(su_idx++)->wt = wt; // target feature
	  else
	    cg->Cn(su_idx++)->wt = 0.0f; // everyone else
	}
      }
      else {
	for(int sui=0;sui<su_gp->size;sui++) {
	  if(sui != feat_no && sui >= fg_st && sui < fg_ed) 
	    cg->Cn(su_idx++)->wt = -surr_mult * wt;
	  else
	    cg->Cn(su_idx++)->wt = 0.0f; // not in our group or is guy itself
	}
      }
    }
  }
}

void SaliencyPrjnSpec::GraphFilter(DataTable* graph_data) {
  dog_wts.GraphFilter(graph_data);
}

void SaliencyPrjnSpec::GridFilter(DataTable* graph_data) {
  dog_wts.GridFilter(graph_data);
}

////////////////////////////////////////////////////////////
//	GpAggregatePrjnSpec

void GpAggregatePrjnSpec::Initialize() {
}

void GpAggregatePrjnSpec::Connect_impl(Projection* prjn) {
  if(!(bool)prjn->from)	return;
  if(prjn->layer->units.leaves == 0) // an empty layer!
    return;
  if(TestWarning(prjn->from->units.gp.size == 0, "Connect_impl",
		 "requires sending layer to have unit groups!")) {
    return;
  }
  Layer* recv_lay = prjn->layer;
  Layer* send_lay = prjn->from;
  TwoDCoord su_geo = send_lay->gp_geom;
  int n_su_gps = su_geo.Product();

  int alloc_no = n_su_gps; 	// number of cons per recv unit

  // pre-alloc senders -- only 1
  Unit* su;
  taLeafItr su_itr;
  FOR_ITR_EL(Unit, su, prjn->from->units., su_itr)
    su->SendConsPreAlloc(1, prjn);

  for(int ri = 0; ri<recv_lay->units.leaves; ri++) {
    Unit* ru_u = (Unit*)recv_lay->units.Leaf(ri);
    if(!ru_u) break;
    ru_u->RecvConsPreAlloc(alloc_no, prjn);
    
    TwoDCoord suc;
    for(suc.y = 0; suc.y <= su_geo.y; suc.y++) {
      for(suc.x = 0; suc.x <= su_geo.x; suc.x++) {
	Unit_Group* su_gp = send_lay->FindUnitGpFmCoord(suc);
	if(!su_gp) continue;
	Unit* su_u = (Unit*)su_gp->SafeEl(ri);
	if(su_u) {
	  ru_u->ConnectFrom(su_u, prjn);
	}
      }
    }
  }
}

///////////////////////////////////////////////////////////////
//			VisDisparityPrjnSpec 

void VisDisparityPrjnSpec::Initialize() {
  n_disparities = 2;
  disp_dist = 5;
  gauss_sigma = 1.0f;
}

void VisDisparityPrjnSpec::Connect_impl(Projection* prjn) {
  if(!(bool)prjn->from)	return;
  if(prjn->layer->units.leaves == 0) // an empty layer!
    return;
  if(TestWarning(prjn->layer->units.gp.size == 0, "Connect_impl",
		 "requires sending layer to have unit groups!")) {
    return;
  }

  if(prjn->from->units.gp.size > 0)
    Connect_Gps(prjn);
  else
    Connect_NoGps(prjn);
}

void VisDisparityPrjnSpec::Connect_Gps(Projection* prjn) {
  Layer* recv_lay = prjn->layer;
  Layer* send_lay = prjn->from;
  TwoDCoord ru_gp_geo = recv_lay->gp_geom;
  TwoDCoord su_gp_geo = send_lay->gp_geom;

  if(TestWarning(ru_gp_geo != su_gp_geo, "Connect_Gps",
		 "Recv layer does not have same gp geometry as sending layer -- cannot connect!")) {
    return;
  }
  TwoDCoord ru_un_geo = recv_lay->un_geom;
  TwoDCoord su_un_geo = send_lay->un_geom;

  int su_n = su_un_geo.Product();
  int ru_n = ru_un_geo.Product();

  int tot_disp = n_disparities * 2 + 1;
  if(TestWarning(ru_n != su_n * tot_disp, "Connect_Gps",
		 "Recv layer unit groups must have n_disparities * 2 + 1=",
		 String(tot_disp),"times number of units in send layer unit group=",
		 String(su_n),"  should be:", String(su_n * tot_disp), "is:",
		 String(ru_n))) {
    return;
  }

  int lr_dir;			// direction multiplier on offset for left vs right
  if(prjn->recv_idx == 0)	// left eye is first guy
    lr_dir = -1;
  else
    lr_dir = 1;

  int rf_width = 2 * disp_dist; // total receptive field width

  TwoDCoord ruc;
  for(int alloc_loop=1; alloc_loop >= 0; alloc_loop--) {
    for(ruc.x = 0; ruc.x < ru_gp_geo.x; ruc.x++) { // loop over receiving layer x
      for(ruc.y = 0; ruc.y < ru_gp_geo.y; ruc.y++) { // loop over receiving layer y
	// get each unit group in receiving layer
	Unit_Group* ru_gp = prjn->layer->FindUnitGpFmCoord(ruc);
	if(!ru_gp) continue;	// shouldn't happen

	int rui_ctr = 0;
	for(int disp=-n_disparities; disp <= n_disparities; disp++) {
	  int disp_off = disp_dist * disp;
	  int st_off = disp_off - disp_dist;
	
	  for(int rui=0; rui<su_n; rui++) {
	    Unit* ru_u = (Unit*)ru_gp->FastEl(rui_ctr++);
	    if(!alloc_loop)
	      ru_u->RecvConsPreAlloc(rf_width, prjn);

	    TwoDCoord suc;
	    for(int soff=0; soff < rf_width; soff++) {
	      suc.y = ruc.y;
	      suc.x = ruc.x + lr_dir * (st_off + soff);
	      if(suc.WrapClip(wrap, su_gp_geo) && !wrap)
		continue;
	      Unit_Group* su_gp = prjn->from->FindUnitGpFmCoord(suc);
	      if(!su_gp) continue;	// shouldn't happen
	      Unit* su_u = su_gp->SafeEl(rui);
	      if(!su_u) continue;
	      if(!self_con && (su_u == ru_u)) continue;

	      ru_u->ConnectFrom(su_u, prjn, alloc_loop);
	    }
	  }
	}
      }
    }
    if(alloc_loop) { // on first pass through alloc loop, do sending allocations
      prjn->from->SendConsPostAlloc(prjn);
    }
  }
}

void VisDisparityPrjnSpec::Connect_NoGps(Projection* prjn) {
  Layer* recv_lay = prjn->layer;
  Layer* send_lay = prjn->from;
  TwoDCoord ru_gp_geo = recv_lay->gp_geom;
  TwoDCoord su_geo = send_lay->un_geom;

  if(TestWarning(ru_gp_geo != su_geo, "Connect_Gps",
		 "Recv layer does not have same gp geometry as sending layer -- cannot connect!")) {
    return;
  }
  TwoDCoord ru_un_geo = recv_lay->un_geom;
  int ru_n = ru_un_geo.Product();
  int tot_disp = n_disparities * 2 + 1;
  if(TestWarning(ru_n != tot_disp, "Connect_Gps",
		 "Recv layer unit groups must have n_disparities * 2 + 1=",String(tot_disp),
		 "units in it -- instead has:", String(ru_n))) {
    return;
  }

  int lr_dir;			// multiplier on offset for left vs right
  if(prjn->recv_idx == 0)	// left eye is first guy
    lr_dir = -1;
  else
    lr_dir = 1;

  int rf_width = 2 * disp_dist; // total receptive field width

  TwoDCoord ruc;
  for(int alloc_loop=1; alloc_loop >= 0; alloc_loop--) {
    for(ruc.x = 0; ruc.x < ru_gp_geo.x; ruc.x++) { // loop over receiving layer x
      for(ruc.y = 0; ruc.y < ru_gp_geo.y; ruc.y++) { // loop over receiving layer y
	// get each unit group in receiving layer
	Unit_Group* ru_gp = prjn->layer->FindUnitGpFmCoord(ruc);
	if(!ru_gp) continue;	// shouldn't happen

	int rui_ctr = 0;
	for(int disp=-n_disparities; disp <= n_disparities; disp++) {
	  int disp_off = disp_dist * disp;
	  int st_off = disp_off - disp_dist;
	
	  Unit* ru_u = (Unit*)ru_gp->FastEl(rui_ctr++);
	  if(!alloc_loop)
	    ru_u->RecvConsPreAlloc(rf_width, prjn);

	  TwoDCoord suc;
	  for(int soff=0; soff < rf_width; soff++) {
	    suc.y = ruc.y;
	    suc.x = ruc.x + lr_dir * (st_off + soff);
	    if(suc.WrapClip(wrap, su_geo) && !wrap)
	      continue;
	    Unit* su_u = prjn->from->FindUnitFmCoord(suc);
	    if(!su_u) continue;
	    if(!self_con && (su_u == ru_u)) continue;
	    ru_u->ConnectFrom(su_u, prjn, alloc_loop);
	  }
	}
      }
    }
    if(alloc_loop) { // on first pass through alloc loop, do sending allocations
      prjn->from->SendConsPostAlloc(prjn);
    }
  }
}

void VisDisparityPrjnSpec::C_Init_Weights(Projection* prjn, RecvCons* cg, Unit* ru) {
//  Unit_Group* rugp = (Unit_Group*)ru->GetOwner();
//   int recv_idx = ru->pos.y * rugp->geom.x + ru->pos.x;

  float gaus_nrm = 1.0f / ((float)disp_dist * gauss_sigma);
  float ctr = (float)disp_dist - .5f;	// even so put half way
  for(int i=0; i<cg->size; i++) {
    float dst = gaus_nrm * (i - ctr);
    float wt = expf(-0.5 * dst * dst);
    cg->Cn(i)->wt = wt;
  }
}

///////////////////////////////////////////////////////////////
//			V1 Layer
///////////////////////////////////////////////////////////////

void LeabraV1Layer::Initialize() {
}

void LeabraV1Layer::BuildUnits() {
  inherited::BuildUnits();
  // build feature unit groups
  feat_gps.gp.SetSize(un_geom.n); // one group per unit in unit groups
  for(int fg=0; fg<feat_gps.gp.size; fg++) {
    LeabraUnit_Group* fugp = (LeabraUnit_Group*)feat_gps.gp[fg];
    fugp->Reset();		// start over
    for(int gg=0; gg<units.gp.size; gg++) {
      LeabraUnit_Group* rugp = (LeabraUnit_Group*)units.gp[gg];
      if(rugp->size <= fg) continue;
      LeabraUnit* ru = (LeabraUnit*)rugp->FastEl(fg);
      fugp->Link(ru);
    }
  }
}

void LeabraV1Layer::ResetSortBuf() {
  inherited::ResetSortBuf();
  for(int fg=0; fg<feat_gps.gp.size; fg++) {
    LeabraUnit_Group* fugp = (LeabraUnit_Group*)feat_gps.gp[fg];
    fugp->Inhib_ResetSortBuf();
  }
}

void V1FeatInhibSpec::Initialize() {
  feat_gain = .1f;
  dist_sigma = .5f;
  i_rat_thr = .5f;
}

void LeabraV1LayerSpec::Initialize() {
  min_obj_type = &TA_LeabraV1Layer;
}

bool LeabraV1LayerSpec::CheckConfig_Layer(Layer* ly, bool quiet) {
  LeabraLayer* lay = (LeabraLayer*)ly;
  if(!inherited::CheckConfig_Layer(lay, quiet)) return false;
  LeabraV1Layer* vlay = (LeabraV1Layer*)lay;

  bool rval = true;
  lay->CheckError(lay->units.gp.size == 0, quiet, rval,
		  "does not have unit groups -- MUST have unit groups!");
  vlay->CheckError(vlay->feat_gps.gp.size != vlay->un_geom.n, quiet, rval,
		   "not proper number of feature unit groups for layer size!");
  return rval;
}

void LeabraV1LayerSpec::Compute_FeatGpActive(LeabraLayer* lay, LeabraUnit_Group* fugp, 
					     LeabraNetwork* net) {
  fugp->active_buf.size = 0;
  for(int ui=0; ui<fugp->size; ui++) {
    LeabraUnit* u = (LeabraUnit*)fugp->FastEl(ui);
    LeabraUnit_Group* u_own = (LeabraUnit_Group*)u->owner; // NOT fugp!
    if(u->i_thr >= u_own->i_val.g_i) // compare to their own group's inhib val!
      fugp->active_buf.Add(u);
  }
}


void LeabraV1LayerSpec::Compute_ApplyInhib(LeabraLayer* lay, LeabraNetwork* net) {
  if((net->cycle >= 0) && lay->hard_clamped)
    return;			// don't do this during normal processing

  LeabraV1Layer* vlay = (LeabraV1Layer*)lay;

  float dst_norm_val = 1.0f / (feat_inhib.dist_sigma * (float)MAX(lay->gp_geom.x, lay->gp_geom.y));
  dst_norm_val *= dst_norm_val;	// using sq distances

  for(int fg=0; fg<vlay->feat_gps.gp.size; fg++) {
    LeabraUnit_Group* fugp = (LeabraUnit_Group*)vlay->feat_gps.gp[fg];
    // get active lists for each feature group
    Compute_FeatGpActive(vlay, fugp, net);

    for(int ui=0; ui<fugp->size; ui++) {
      LeabraUnit* u = (LeabraUnit*)fugp->FastEl(ui);
      LeabraUnit_Group* u_own = (LeabraUnit_Group*)u->owner; // NOT fugp!
      TwoDCoord up = u_own->GetGpGeomPos();

      float gp_i = u_own->i_val.g_i;
      if(gp_i <= 0) gp_i = .1f;	// note: should have min_i set!!

      // now compare each unit with all the active units and increase inhib in proportion!
      float sum_cost = 0.0f;
      if((u->i_thr / gp_i) > feat_inhib.i_rat_thr) { // only if this guy is even close to firing
	for(int ai=0; ai<fugp->active_buf.size; ai++) {
	  LeabraUnit* au = (LeabraUnit*)fugp->active_buf.FastEl(ai);
	  LeabraUnit_Group* au_own = (LeabraUnit_Group*)au->owner;
	  TwoDCoord aup = au_own->GetGpGeomPos();

	  float dst_sq = up.SqDist(aup);
	  float cost = taMath_float::exp_fast(-dst_sq * dst_norm_val);
	  sum_cost += cost;
	}
      }
      float ival = gp_i * (1.0f + feat_inhib.feat_gain * sum_cost);
      u->Compute_ApplyInhib(net, ival);
    }    
  }
}


///////////////////////////////////////////////////////////////////
//	Cerebellum-related special guys

void CerebConj2PrjnSpec::Initialize() {
  init_wts = true;
  rf_width = 6;
  rf_move = 3.0f;
  wrap = false;
  gauss_sigma = 1.0f;
}

void CerebConj2PrjnSpec::Connect_impl(Projection* prjn) {
  if(!(bool)prjn->from)	return;
  if(prjn->layer->units.leaves == 0) // an empty layer!
    return;
  if(TestWarning(prjn->layer->units.gp.size == 0, "Connect_impl",
		 "requires recv layer to have unit groups!")) {
    return;
  }
  if(prjn->recv_idx == 0)
    Connect_Outer(prjn);
  else
    Connect_Inner(prjn);
}

void CerebConj2PrjnSpec::Connect_Outer(Projection* prjn) {
  int n_cons = rf_width.Product();
  TwoDCoord rf_half_wd = rf_width / 2;
  TwoDCoord rug_geo = prjn->layer->gp_geom;
  TwoDCoord run_geo = prjn->layer->un_geom;
  TwoDCoord su_geo = prjn->from->flat_geom;

  TwoDCoord ruc;
  for(int alloc_loop=1; alloc_loop>=0; alloc_loop--) {
    for(ruc.y = 0; ruc.y < rug_geo.y; ruc.y++) {
      for(ruc.x = 0; ruc.x < rug_geo.x; ruc.x++) {
	Unit_Group* ru_gp = prjn->layer->FindUnitGpFmCoord(ruc);
	if(!ru_gp) continue;

	TwoDCoord su_st;
	if(wrap) {
	  su_st.x = (int)floor((float)ruc.x * rf_move.x) - rf_half_wd.x;
	  su_st.y = (int)floor((float)ruc.y * rf_move.y) - rf_half_wd.y;
	}
	else {
	  su_st.x = (int)floor((float)ruc.x * rf_move.x);
	  su_st.y = (int)floor((float)ruc.y * rf_move.y);
	}

	su_st.WrapClip(wrap, su_geo);
	TwoDCoord su_ed = su_st + rf_width;
	if(wrap) {
	  su_ed.WrapClip(wrap, su_geo); // just wrap ends too
	}
	else {
	  if(su_ed.x > su_geo.x) {
	    su_ed.x = su_geo.x; su_st.x = su_ed.x - rf_width.x;
	  }
	  if(su_ed.y > su_geo.y) {
	    su_ed.y = su_geo.y; su_st.y = su_ed.y - rf_width.y;
	  }
	}

	for(int rui=0;rui<ru_gp->size;rui++) {
	  Unit* ru_u = (Unit*)ru_gp->FastEl(rui);
	  if(!alloc_loop)
	    ru_u->RecvConsPreAlloc(n_cons, prjn);

	  TwoDCoord suc;
	  TwoDCoord suc_wrp;
	  for(suc.y = 0; suc.y < rf_width.y; suc.y++) {
	    for(suc.x = 0; suc.x < rf_width.x; suc.x++) {
	      suc_wrp = su_st + suc;
	      if(suc_wrp.WrapClip(wrap, su_geo) && !wrap)
		continue;
	      Unit* su_u = prjn->from->FindUnitFmCoord(suc_wrp);
	      if(!su_u) continue;
	      if(!self_con && (su_u == ru_u)) continue;

	      ru_u->ConnectFrom(su_u, prjn, alloc_loop); // don't check: saves lots of time!
	    }
	  }
	}
      }
    }
    if(alloc_loop) { // on first pass through alloc loop, do sending allocations
      prjn->from->SendConsPostAlloc(prjn);
    }
  }
}

void CerebConj2PrjnSpec::Connect_Inner(Projection* prjn) {
  if(!(bool)prjn->from)	return;
  if(prjn->layer->units.leaves == 0) // an empty layer!
    return;
  if(TestWarning(prjn->layer->units.gp.size == 0, "Connect_impl",
		 "requires recv layer to have unit groups!")) {
    return;
  }

  int n_cons = rf_width.Product();
  TwoDCoord rf_half_wd = rf_width / 2;
  TwoDCoord rug_geo = prjn->layer->gp_geom;
  TwoDCoord run_geo = prjn->layer->un_geom;
  TwoDCoord su_geo = prjn->from->flat_geom;

  for(int alloc_loop=1; alloc_loop>=0; alloc_loop--) {
    for(int rug=0;rug<prjn->layer->units.gp.size;rug++) {
      Unit_Group* ru_gp = (Unit_Group*)prjn->layer->units.gp[rug];
      if(!ru_gp) continue;

      TwoDCoord ruc;
      for(ruc.y = 0; ruc.y < run_geo.y; ruc.y++) {
	for(ruc.x = 0; ruc.x < run_geo.x; ruc.x++) {
	  Unit* ru_u = (Unit*)ru_gp->SafeEl(ruc.y * run_geo.x + ruc.x);
	  if(!ru_u) continue;
	  if(!alloc_loop)
	    ru_u->RecvConsPreAlloc(n_cons, prjn);

	  TwoDCoord su_st;
	  if(wrap) {
	    su_st.x = (int)floor((float)ruc.x * rf_move.x) - rf_half_wd.x;
	    su_st.y = (int)floor((float)ruc.y * rf_move.y) - rf_half_wd.y;
	  }
	  else {
	    su_st.x = (int)floor((float)ruc.x * rf_move.x);
	    su_st.y = (int)floor((float)ruc.y * rf_move.y);
	  }

	  su_st.WrapClip(wrap, su_geo);
	  TwoDCoord su_ed = su_st + rf_width;
	  if(wrap) {
	    su_ed.WrapClip(wrap, su_geo); // just wrap ends too
	  }
	  else {
	    if(su_ed.x > su_geo.x) {
	      su_ed.x = su_geo.x; su_st.x = su_ed.x - rf_width.x;
	    }
	    if(su_ed.y > su_geo.y) {
	      su_ed.y = su_geo.y; su_st.y = su_ed.y - rf_width.y;
	    }
	  }

	  TwoDCoord suc;
	  TwoDCoord suc_wrp;
	  for(suc.y = 0; suc.y < rf_width.y; suc.y++) {
	    for(suc.x = 0; suc.x < rf_width.x; suc.x++) {
	      suc_wrp = su_st + suc;
	      if(suc_wrp.WrapClip(wrap, su_geo) && !wrap)
		continue;
	      Unit* su_u = prjn->from->FindUnitFmCoord(suc_wrp);
	      if(su_u == NULL) continue;
	      if(!self_con && (su_u == ru_u)) continue;
	      ru_u->ConnectFrom(su_u, prjn, alloc_loop); // don't check: saves lots of time!
	    }
	  }
	}
      }
    }
    if(alloc_loop) { // on first pass through alloc loop, do sending allocations
      prjn->from->SendConsPostAlloc(prjn);
    }
  }
}

void CerebConj2PrjnSpec::C_Init_Weights(Projection* prjn, RecvCons* cg, Unit* ru) {
//  Unit_Group* rugp = (Unit_Group*)ru->GetOwner();
//  int recv_idx = ru->pos.y * rugp->geom.x + ru->pos.x;
  
  TwoDCoord rf_half_wd = rf_width / 2;
  FloatTwoDCoord rf_ctr = rf_half_wd;
  if(rf_half_wd * 2 == rf_width) // even
    rf_ctr -= .5f;

  for(int i=0; i<cg->size; i++) {
    int su_x = i % rf_width.x;
    int su_y = i / rf_width.x;

    float dst = taMath_float::euc_dist_sq(su_x, su_y, rf_ctr.x, rf_ctr.y);
    float wt = expf(-0.5 * dst / (gauss_sigma * gauss_sigma));

    cg->Cn(i)->wt = wt;
  }
}

bool CerebConj2PrjnSpec::TrgRecvFmSend(int send_x, int send_y) {
  trg_send_geom.x = send_x;
  trg_send_geom.y = send_y;

  if(wrap)
    trg_recv_geom = (trg_send_geom / rf_move);
  else
    trg_recv_geom = (trg_send_geom / rf_move) - 1;

  // now fix it the other way
  if(wrap)
    trg_send_geom = (trg_recv_geom * rf_move);
  else
    trg_send_geom = ((trg_recv_geom +1) * rf_move);

  DataChanged(DCR_ITEM_UPDATED);
  return (trg_send_geom.x == send_x && trg_send_geom.y == send_y);
}

bool CerebConj2PrjnSpec::TrgSendFmRecv(int recv_x, int recv_y) {
  trg_recv_geom.x = recv_x;
  trg_recv_geom.y = recv_y;

  if(wrap)
    trg_send_geom = (trg_recv_geom * rf_move);
  else
    trg_send_geom = ((trg_recv_geom+1) * rf_move);

  // now fix it the other way
  if(wrap)
    trg_recv_geom = (trg_send_geom / rf_move);
  else
    trg_recv_geom = (trg_send_geom / rf_move) - 1;

  DataChanged(DCR_ITEM_UPDATED);
  return (trg_recv_geom.x == recv_x && trg_recv_geom.y == recv_y);
}


///////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////
//		Wizard		//
//////////////////////////////////

bool LeabraWizard::UpdateInputDataFmNet(Network* net, DataTable* data_table) {
  if(TestError(!data_table || !net, "UpdateInputDataFmNet",
	       "must specify both a network and a data table")) return false;
  data_table->StructUpdate(true);
  taLeafItr li;
  LeabraLayer* lay;
  FOR_ITR_EL(LeabraLayer, lay, net->layers., li) {
    if(lay->layer_type == Layer::HIDDEN) continue;
    int lay_idx = 0;

    LeabraLayerSpec* ls = (LeabraLayerSpec*)lay->GetLayerSpec();
    if(ls->InheritsFrom(&TA_ScalarValLayerSpec) && !((ScalarValLayerSpec*)ls)->scalar.clamp_pat) {
      if(lay->unit_groups) {
	data_table->FindMakeColName
	  (lay->name, lay_idx, DataTable::VT_FLOAT, 4, 1, 1,
	   MAX(lay->gp_geom.x,1), MAX(lay->gp_geom.y,1));
      }
      else {
	data_table->FindMakeColName
	  (lay->name, lay_idx, DataTable::VT_FLOAT, 2, 1, 1);
      }
    }
    else if(ls->InheritsFrom(&TA_TwoDValLayerSpec) && !((TwoDValLayerSpec*)ls)->twod.clamp_pat) {
      TwoDValLayerSpec* tdls = (TwoDValLayerSpec*)ls;
      int nx = tdls->twod.n_vals * 2;
      if(lay->unit_groups) {
	data_table->FindMakeColName
	  (lay->name, lay_idx, DataTable::VT_FLOAT, 4, nx, 1,
	   MAX(lay->gp_geom.x,1), MAX(lay->gp_geom.y,1));
      }
      else {
	data_table->FindMakeColName
	  (lay->name, lay_idx, DataTable::VT_FLOAT, 2, nx, 1);
      }
    }
    else {
      if(lay->unit_groups) {
	data_table->FindMakeColName
	  (lay->name, lay_idx, DataTable::VT_FLOAT, 4,
	   MAX(lay->un_geom.x,1), MAX(lay->un_geom.y,1),
	   MAX(lay->gp_geom.x,1), MAX(lay->gp_geom.y,1));
      }
      else {
	data_table->FindMakeColName
	  (lay->name, lay_idx, DataTable::VT_FLOAT, 2,
	   MAX(lay->un_geom.x,1), MAX(lay->un_geom.y,1));
      }
    }
  }
  data_table->StructUpdate(false);
//   if(taMisc::gui_active) {
//     tabMisc::DelayedFunCall_gui(data_table, "BrowserSelectMe");
//   }
  return true;
}

///////////////////////////////////////////////////////////////
//			SRN Context
///////////////////////////////////////////////////////////////

bool LeabraWizard::SRNContext(LeabraNetwork* net) {
  if(TestError(!net, "SRNContext", "must have basic constructed network first")) {
    return false;
  }
  OneToOnePrjnSpec* otop = (OneToOnePrjnSpec*)net->FindMakeSpec("CtxtPrjn", &TA_OneToOnePrjnSpec);
  LeabraContextLayerSpec* ctxts = (LeabraContextLayerSpec*)net->FindMakeSpec("CtxtLayerSpec", &TA_LeabraContextLayerSpec);

  if((otop == NULL) || (ctxts == NULL)) {
    return false;
  }

  LeabraLayer* hidden = (LeabraLayer*)net->FindLayer("Hidden");
  LeabraLayer* ctxt = (LeabraLayer*)net->FindMakeLayer("Context");
  
  if((hidden == NULL) || (ctxt == NULL)) return false;

  ctxt->SetLayerSpec(ctxts);
  ctxt->un_geom = hidden->un_geom;

  net->layers.MoveAfter(hidden, ctxt);
  net->FindMakePrjn(ctxt, hidden, otop); // one-to-one into the ctxt layer
  net->FindMakePrjn(hidden, ctxt); 	 // std prjn back into the hidden from context
  net->Build();
  return true;
}

