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

#include "leabra_td.h"

#ifndef leabra_pvlv_h
#define leabra_pvlv_h

//////////////////////////////////////////////////////////////////////////////////////////
//      Pavlovian (PVLV): Primary Value and Learned Value Reward Learning System        //
//////////////////////////////////////////////////////////////////////////////////////////

// PV primary value: learns continuously about primary rewards (present or absent)
//   PVe = excitatory: primary reward (just uses ExtRewLayerSpec -- nothing specific for pvlv)
//   PVi = inhibitory: cancelling expected primary rewards
//   PVr = detector of time when rewards are present (fast to learn, slow to extinguish,
//         learns based on 1/.5 values instead of actual reward magnitudes)
// LV learned value: learns only at the time of primary (expected) rewards,
//         free to fire at time CS's come on
//   LVe = excitatory: rapidly learns excitatory CS assocs
//   LVi = inhibitory: slowly learns to cancel CS assocs (adaptive baseline for LVe)
// PVLVDa (VTA/SNc) computes DA signal as: IF PV present, PVe - PVi, else LVe - LVi
// delta version (5/07) uses temporal derivative of LV & PV signals, not synaptic depression


TypeDef_Of(PVLVLayerSpec);

class LEABRA_API PVLVLayerSpec : public ScalarValLayerSpec {
  // #VIRT_BASE -- generic PVLV layer spec -- all PVLV layer specs inherit from this
INHERITED(ScalarValLayerSpec)
public:
  TA_BASEFUNS_NOCOPY(PVLVLayerSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize()            { }
  void  Destroy()               { };
  void  Defaults_init()         { };
};

//////////////////////////////////////////
//      PV: Primary Value Layer         //
//////////////////////////////////////////

TypeDef_Of(PVConSpec);

class LEABRA_API PVConSpec : public LeabraConSpec {
  // pvlv connection spec: learns using delta rule from act_p - act_m values -- does not use hebb or err_sb parameters
INHERITED(LeabraConSpec)
public:
  enum SendActVal {
    ACT_P,                      // plus phase activation state
    ACT_M2,                     // mid-minus activation state (for PBWM)
  };

  SendActVal    send_act;       // what to use for the sending activation value

  inline void C_Compute_dWt_Delta(LeabraCon* cn, float lin_wt, LeabraUnit* ru, float su_act) {
    float dwt = (ru->act_p - ru->act_m) * su_act; // basic delta rule
    if(lmix.err_sb) {
      if(dwt > 0.0f)    dwt *= (1.0f - lin_wt);
      else              dwt *= lin_wt;
    }
    cn->dwt += cur_lrate * dwt;
  }

  inline override void Compute_dWt_LeabraCHL(LeabraSendCons* cg, LeabraUnit* su) {
    float su_act;
    if(send_act == ACT_P)
      su_act = su->act_p;
    else                        // ACT_M2
      su_act = su->act_m2;

    for(int i=0; i<cg->size; i++) {
      LeabraUnit* ru = (LeabraUnit*)cg->Un(i);
      LeabraCon* cn = (LeabraCon*)cg->OwnCn(i);
      C_Compute_dWt_Delta(cn, LinFmSigWt(cn->wt), ru, su_act);
    }
  }

  inline override void Compute_dWt_CtLeabraXCAL(LeabraSendCons* cg, LeabraUnit* su) {
    Compute_dWt_LeabraCHL(cg, su);
  }

  inline override void Compute_dWt_CtLeabraCAL(LeabraSendCons* cg, LeabraUnit* su) {
    Compute_dWt_LeabraCHL(cg, su);
  }

  inline override void Compute_Weights_CtLeabraXCAL(LeabraSendCons* cg, LeabraUnit* su) {
    // just run chl version through-and-through
    CON_GROUP_LOOP(cg, C_Compute_Weights_LeabraCHL((LeabraCon*)cg->OwnCn(i)));
    //  ApplyLimits(cg, ru); limits are automatically enforced anyway
  }
  inline override void Compute_Weights_CtLeabraCAL(LeabraSendCons* cg, LeabraUnit* su) {
    // just run chl version through-and-through
    CON_GROUP_LOOP(cg, C_Compute_Weights_LeabraCHL((LeabraCon*)cg->OwnCn(i)));
    //  ApplyLimits(cg, ru); limits are automatically enforced anyway
  }

  TA_SIMPLE_BASEFUNS(PVConSpec);
protected:
  SPEC_DEFAULTS;
  void  UpdateAfterEdit_impl();
private:
  void  Initialize();
  void  Destroy()               { };
  void  Defaults_init();
};


TypeDef_Of(PVMiscSpec);

class LEABRA_API PVMiscSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra specs for PV layer spec
INHERITED(SpecMemberBase)
public:
  float         min_pvi;        // #DEF_0.4 minimum pvi value -- PVi is not allowed to go below this value for the purposes of computing the PV delta value: pvd = PVe - MAX(PVi,min_pvi)
  bool          pvi_scale_min;  // if both the PVe and PVi values are below min_pvi, then scale the result by (PVi/min_pvi) -- as PVi gets lower, meaning that it expects to be doing poorly, then punish the system less (but still punish it)
  float         prior_gain;     // #DEF_1 #MIN_0 #MAX_1 #EXPERT #AKA_prior_discount how much of the prior PV delta value (pvd = PVe - MAX(PVi,min_pvi)) to subtract away in computing the net PV dopamine signal (PV DA = pvd_t - prior_gain * pvd_t-1)
  bool          er_reset_prior; // #EXPERT #DEF_true reset prior delta value (pvd_t-1) when external rewards are received (akin to absorbing rewards in TD)
  bool		no_y_dot; // #DEF_false if true do not use y-dot for phasic DA calculation (PVi)

  override String       GetTypeDecoKey() const { return "LayerSpec"; }

  TA_SIMPLE_BASEFUNS(PVMiscSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()       { };
  void  Defaults_init() { Initialize(); }
};

TypeDef_Of(PViLayerSpec);

class LEABRA_API PViLayerSpec : public PVLVLayerSpec {
  // primary value inhibitory (PVi) layer: continously learns to expect primary reward values, contribute to overall dopamine with PV delta pvd = PVe - PVi; PV DA = pvd_t - pvd_t-1
INHERITED(PVLVLayerSpec)
public:
  PVMiscSpec    pv;             // misc parameters for the PV computation

  virtual void  Compute_PVPlusPhaseDwt(LeabraLayer* lay, LeabraNetwork* net);
  // compute plus phase activations as external rewards and change weights

  virtual float Compute_PVDa(LeabraLayer* lay, LeabraNetwork* net);
  // compute da contribution from PV
    virtual float Compute_PVDa_ugp(LeabraLayer* lay, Layer::AccessMode acc_md, int gpidx,
                                   float pve_val, LeabraNetwork* net);
    // #IGNORE
  virtual void  Update_PVPrior(LeabraLayer* lay, LeabraNetwork* net);
  // update the prior PV value, stored in pv unit misc_1 values -- at very end of trial
    virtual void Update_PVPrior_ugp(LeabraLayer* lay, Layer::AccessMode acc_md, int gpidx,
                                    bool er_avail);
    // #IGNORE

  // overrides:
  override void Compute_CycleStats(LeabraLayer* lay, LeabraNetwork* net);
  override void PostSettle(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_SRAvg_Test(LeabraLayer*, LeabraNetwork*) { return false; }

  override void Compute_dWt_Layer_pre(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_dWt_FirstPlus_Test(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_dWt_Nothing_Test(LeabraLayer* lay, LeabraNetwork* net);

  void  HelpConfig();   // #BUTTON get help message for configuring this spec
  bool  CheckConfig_Layer(Layer* lay, bool quiet=false);

  TA_SIMPLE_BASEFUNS(PViLayerSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()               { };
  void  Defaults_init()         { };
};

//////////////////////////////////////////////////////////////////////
//      PVr = PV reward detection system (habenula?)

TypeDef_Of(PVrConSpec);

class LEABRA_API PVrConSpec : public PVConSpec {
  // primary value connection spec with asymmetrical learning rates -- used for reward detection connections -- have asymmetric weight decrease to lock in expectations for longer
INHERITED(PVConSpec)
public:
  float         wt_dec_mult;   // multiplier for weight decrease rate relative to basic lrate used for weight increases

  inline void C_Compute_dWt_Delta(LeabraCon* cn, float lin_wt, LeabraUnit* ru, LeabraUnit* su) {
    float dwt = (ru->act_p - ru->act_m) * su->act_p; // basic delta rule
    if(dwt < 0.0f)      dwt *= wt_dec_mult;
    if(lmix.err_sb) {
      if(dwt > 0.0f)    dwt *= (1.0f - lin_wt);
      else              dwt *= lin_wt;
    }
    cn->dwt += cur_lrate * dwt;
  }

  inline override void Compute_dWt_LeabraCHL(LeabraSendCons* cg, LeabraUnit* su) {
    for(int i=0; i<cg->size; i++) {
      LeabraUnit* ru = (LeabraUnit*)cg->Un(i);
      LeabraCon* cn = (LeabraCon*)cg->OwnCn(i);
      C_Compute_dWt_Delta(cn, LinFmSigWt(cn->wt), ru, su);
    }
  }

  inline override void Compute_dWt_CtLeabraXCAL(LeabraSendCons* cg, LeabraUnit* su) {
    Compute_dWt_LeabraCHL(cg, su);
  }

  inline override void Compute_dWt_CtLeabraCAL(LeabraSendCons* cg, LeabraUnit* su) {
    Compute_dWt_LeabraCHL(cg, su);
  }

  inline override void Compute_Weights_CtLeabraXCAL(LeabraSendCons* cg, LeabraUnit* su) {
    // just run chl version through-and-through
    CON_GROUP_LOOP(cg, C_Compute_Weights_LeabraCHL((LeabraCon*)cg->OwnCn(i)));
    //  ApplyLimits(cg, ru); limits are automatically enforced anyway
  }
  inline override void Compute_Weights_CtLeabraCAL(LeabraSendCons* cg, LeabraUnit* su) {
    // just run chl version through-and-through
    CON_GROUP_LOOP(cg, C_Compute_Weights_LeabraCHL((LeabraCon*)cg->OwnCn(i)));
    //  ApplyLimits(cg, ru); limits are automatically enforced anyway
  }


  TA_SIMPLE_BASEFUNS(PVrConSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()               { };
  void  Defaults_init() { Initialize(); }
};

TypeDef_Of(PVDetectSpec);

class LEABRA_API PVDetectSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra specs for detecting if a primary value is present or expected -- just learns with value 1 for PV present, .5 for absent
INHERITED(SpecMemberBase)
public:
  float         thr;    // #DEF_0.7 threshold on PVr value, above which PV is considered present (i.e., reward) -- PVr learns a 1 for all reward-valence cases, regardless of value, and .5 for reward absent

  override String       GetTypeDecoKey() const { return "LayerSpec"; }

  TA_SIMPLE_BASEFUNS(PVDetectSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()       { };
  void  Defaults_init() { Initialize(); }
};

TypeDef_Of(PVrLayerSpec);

class LEABRA_API PVrLayerSpec : public PVLVLayerSpec {
  // primary value reward detection layer: learns when rewards are expected to occur -- gets a 1 for any primary value feedback (reward or punishment), and .5 otherwise
INHERITED(PVLVLayerSpec)
public:
  PVDetectSpec  pv_detect;      // primary reward value detection spec: detect if a primary reward is expected based on PVr value

  virtual void  Compute_PVPlusPhaseDwt(LeabraLayer* lay, LeabraNetwork* net);
  // compute plus phase activations as external rewards and change weights

  virtual bool  Compute_PVDetect(LeabraLayer* lay, LeabraNetwork* net);
  // detect PV expectation based on PVr value -- happens at end of minus phase, based on unit activations then

  // overrides:
  override bool Compute_SRAvg_Test(LeabraLayer*, LeabraNetwork*) { return false; }
  override void Compute_CycleStats(LeabraLayer* lay, LeabraNetwork* net);

  override void Compute_dWt_Layer_pre(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_dWt_FirstPlus_Test(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_dWt_Nothing_Test(LeabraLayer* lay, LeabraNetwork* net);

  void  HelpConfig();   // #BUTTON get help message for configuring this spec
  bool  CheckConfig_Layer(Layer* lay, bool quiet=false);

  TA_BASEFUNS_NOCOPY(PVrLayerSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()               { };
  void  Defaults_init()         { };
};


////////////////////////////////////////////////////////////////////////
//              LV System: Learned Value

TypeDef_Of(LVMiscSpec);

class LEABRA_API LVMiscSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra specs for PV layer spec
INHERITED(SpecMemberBase)
public:
  float         min_lvi;        // #DEF_0.1;0.4 minimum lvi value -- LVi is not allowed to go below this value for the purposes of computing the LV delta value: lvd = LVe - MAX(LVi,min_lvi)
  bool          lvi_scale_min;  // if both the LVe and LVi values are below min_lvi, then scale the result by (LVi/min_lvi) -- as LVi gets lower, meaning that it expects to be doing poorly, then punish the system less (but still punish it)
  bool          lrn_pv_only;    // #DEF_true only compute weight changes on trials where primary rewards are expected or actually received -- the target PV value is only presented on such trials, but if this flag is off, it actually learns on other trials, but with whatever plus phase activation state happens to arise
  float		nopv_val;	// #CONDSHOW_ON_lrn_pv_only:false value to apply for learning on non-pv trials -- simulates a baseline effort cost for non-reward trials.  only works when lrn_pv_only is false.  see nopv_lrate for lrate multiplier for these trials, to independently manipulate how rapidly learning takes place
  float		nopv_lrate;	// #CONDSHOW_ON_lrn_pv_only:false learning rate for learning on non-pv trials -- see nopv_val for value that is clamped.  this can be used to simulate a baseline effort cost for non-reward trials.  only works when lrn_pv_only is false.
  float         prior_gain;     // #DEF_1 #MIN_0 #MAX_1 #EXPERT #AKA_prior_discount how much of the the prior time step LV delta value (lvd = LVe - MAX(LVi,min_lvi)) to subtract away in computing the net LV dopamine signal (LV DA = lvd_t - prior_gain * lvd_t-1)
  bool          er_reset_prior; // #EXPERT #DEF_true reset prior delta value (lvd_t-1) when external rewards are received (akin to absorbing rewards in TD)
  bool		no_y_dot; 	// #DEF_false don't use y-dot temporal derivative at all in computing LVe phasic DA 
  bool		pos_y_dot_only; // #DEF_false use only positive deviations for computing LVe phasic DA -- mutex with no_y_dot

  override String       GetTypeDecoKey() const { return "LayerSpec"; }

  TA_SIMPLE_BASEFUNS(LVMiscSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()       { };
  void  Defaults_init() { Initialize(); }
};

TypeDef_Of(LVeLayerSpec);

class LEABRA_API LVeLayerSpec : public PVLVLayerSpec {
  // learns value based on inputs that are associated with rewards, only learns at time of primary rewards (filtered by PV system). This is excitatory version LVe.  LV contribution to dopamine is based on LV delta lvd = LVe - LVi; LV DA = lvd_t - lvd_t-1
INHERITED(PVLVLayerSpec)
public:
  LVMiscSpec    lv;             // misc parameters controlling the LV computation (note: only the LVe instance of these parameters are used)

  virtual void  Compute_LVPlusPhaseDwt(LeabraLayer* lay, LeabraNetwork* net);
  // if primary value detected (present/expected), compute plus phase activations for learning, and actually change weights
    virtual void  Compute_LVCurLrate(LeabraLayer* lay, LeabraNetwork* net, LeabraUnit* u,
				     float lrate_mult);
    // set the cur_lrate for con specs coming into LV layer, using lrate_mult multiplier -- used for the nopv_lrate param

  virtual float Compute_LVDa(LeabraLayer* lve_lay, LeabraLayer* lvi_lay, LeabraNetwork* net);
  // compute da contribution from Lv, based on lve_layer and lvi_layer activations (multiple subgroups allowed)
  virtual float Compute_LVDa_ugp(LeabraLayer* lve_lay, LeabraLayer* lvi_lay,
                                 Layer::AccessMode lve_acc_md, int lve_gpidx,
                                 Layer::AccessMode lvi_acc_md, int lvi_gpidx,
                                 LeabraNetwork* net);
    // #IGNORE

  virtual void  Update_LVPrior(LeabraLayer* lay, LeabraNetwork* net);
  // update the prior Lv value, stored in lv unit misc_1 values
    virtual void Update_LVPrior_ugp(LeabraLayer* lay, Layer::AccessMode acc_md, int gpidx,
                                    bool er_avail);
    // #IGNORE

  override void Compute_CycleStats(LeabraLayer* lay, LeabraNetwork* net);
  override void PostSettle(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_SRAvg_Test(LeabraLayer*, LeabraNetwork*) { return false; }

  override void Compute_dWt_Layer_pre(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_dWt_FirstPlus_Test(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_dWt_Nothing_Test(LeabraLayer* lay, LeabraNetwork* net);

  void  HelpConfig();   // #BUTTON get help message for configuring this spec
  bool  CheckConfig_Layer(Layer* lay, bool quiet=false);

  TA_SIMPLE_BASEFUNS(LVeLayerSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()               { };
  void  Defaults_init()         { };
};

TypeDef_Of(LViLayerSpec);

class LEABRA_API LViLayerSpec : public LVeLayerSpec {
  // inhibitory/slow version of LV layer spec: (just a marker for layer; same functionality as LVeLayerSpec)
INHERITED(LVeLayerSpec)
public:
  override void Compute_CycleStats(LeabraLayer* lay, LeabraNetwork* net);

  TA_BASEFUNS_NOCOPY(LViLayerSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize()            { };
  void  Destroy()               { };
  void  Defaults_init()         { };
};

//////////////////////////////////////////
//        Novelty Value Layer (NV)      //
//////////////////////////////////////////

TypeDef_Of(NVConSpec);

class LEABRA_API NVConSpec : public PVConSpec {
  // novelty value connection spec: learns using delta rule from act_p - act_m values -- does not use hebb or err_sb parameters -- has decay to refresh novelty if not seen for a while..
INHERITED(PVConSpec)
public:
  float         decay;          // #MIN_0 amount to decay weight values every time they are updated, restoring some novelty (also multiplied by lrate to compute weight change, so it automtically takes that into account -- think of as a pct to decay)

  inline void C_Compute_Weights_LeabraCHL(LeabraCon* cn, float dkfact) {
    float lin_wt = LinFmSigWt(cn->wt);
    cn->dwt -= dkfact * lin_wt;
    if(cn->dwt != 0.0f) {
      cn->wt = SigFmLinWt(lin_wt + cn->dwt);
    }
    cn->pdw = cn->dwt;
    cn->dwt = 0.0f;
  }

  inline override void Compute_Weights_LeabraCHL(LeabraSendCons* cg, LeabraUnit* su) {
    float dkfact = cur_lrate * decay;
    CON_GROUP_LOOP(cg, C_Compute_Weights_LeabraCHL((LeabraCon*)cg->OwnCn(i), dkfact));
    //  ApplyLimits(cg, ru); limits are automatically enforced anyway
  }

  inline override void Compute_Weights_CtLeabraXCAL(LeabraSendCons* cg, LeabraUnit* su) {
    float dkfact = cur_lrate * decay;
    CON_GROUP_LOOP(cg, C_Compute_Weights_LeabraCHL((LeabraCon*)cg->OwnCn(i), dkfact));
    //  ApplyLimits(cg, ru); limits are automatically enforced anyway
  }

  inline override void Compute_Weights_CtLeabraCAL(LeabraSendCons* cg, LeabraUnit* su) {
    float dkfact = cur_lrate * decay;
    CON_GROUP_LOOP(cg, C_Compute_Weights_LeabraCHL((LeabraCon*)cg->OwnCn(i), dkfact));
    //  ApplyLimits(cg, ru); limits are automatically enforced anyway
  }

  TA_SIMPLE_BASEFUNS(NVConSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()               { };
  void  Defaults_init() { Initialize(); }
};

TypeDef_Of(NVSpec);

class LEABRA_API NVSpec : public SpecMemberBase {
  // ##INLINE ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra specs for novelty value learning
INHERITED(SpecMemberBase)
public:
  float         da_gain;        // #DEF_0:1 #MIN_0 gain for novelty value dopamine signal
  float         val_thr;        // #DEF_0.1 #MIN_0 threshold for value (training value is 0) -- value is zero below this threshold
  float         prior_gain;     // #DEF_1 #MIN_0 #MAX_1 #EXPERT #AKA_prior_discount how much of the prior NV delta value (nvd = NV - val_thr) to subtract away in computing the net NV dopamine signal (NV DA = nvd_t - prior_gain * nvd_t-1)
  bool          er_reset_prior; // #EXPERT #DEF_true reset prior delta value (nvd_t-1) when external rewards are received (akin to absorbing rewards in TD)

  override String       GetTypeDecoKey() const { return "LayerSpec"; }

  TA_SIMPLE_BASEFUNS(NVSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()       { };
  void  Defaults_init() { Initialize(); }
};

TypeDef_Of(NVLayerSpec);

class LEABRA_API NVLayerSpec : public PVLVLayerSpec {
  // novelty value (NV) layer: starts with a bias of 1.0, and learns to activate 0.0 value -- value signal is how novel the stimulus is: NV delta nvd = NV - val_thr; NV DA = nvd_t - nvd_t-1
INHERITED(PVLVLayerSpec)
public:
  NVSpec        nv;     // novelty value specs

  virtual float Compute_NVDa_raw(LeabraLayer* lay, LeabraNetwork* net);
  // compute raw novelty value da value -- no gain factor
  virtual float Compute_NVDa(LeabraLayer* lay, LeabraNetwork* net);
  // compute novelty value da value -- with gain factor applied
  virtual void  Compute_NVPlusPhaseDwt(LeabraLayer* lay, LeabraNetwork* net);
  // compute plus phase activations as train target value and change weights
  virtual void  Update_NVPrior(LeabraLayer* lay, LeabraNetwork* net);
  // update the prior Nv value, stored in nv unit misc_1 values

  override void Compute_CycleStats(LeabraLayer* lay, LeabraNetwork* net);
  override void PostSettle(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_SRAvg_Test(LeabraLayer*, LeabraNetwork*) { return false; }
  override void Compute_dWt_Layer_pre(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_dWt_FirstPlus_Test(LeabraLayer* lay, LeabraNetwork* net);
  override bool Compute_dWt_Nothing_Test(LeabraLayer* lay, LeabraNetwork* net);

  void  HelpConfig();   // #BUTTON get help message for configuring this spec
  bool  CheckConfig_Layer(Layer* lay, bool quiet=false);

  TA_SIMPLE_BASEFUNS(NVLayerSpec);
protected:
  SPEC_DEFAULTS;
  void  UpdateAfterEdit_impl();
private:
  void  Initialize();
  void  Destroy()               { };
  void  Defaults_init()         { };
};

//////////////////////////
//        DaLayer       //
//////////////////////////

TypeDef_Of(PVLVDaSpec);

class LEABRA_API PVLVDaSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra specs for PVLV da parameters
INHERITED(SpecMemberBase)
public:
  float         da_gain;        // #DEF_0:2 #MIN_0 multiplier for dopamine values
  float         tonic_da;       // #DEF_0 set a tonic 'dopamine' (DA) level (offset to add to da values)
  float         pv_gain;        // #DEF_1;0.1;0.5 #MIN_0 extra gain modulation of PV generated DA -- it can be much larger in general than lv so sometimes it is useful to turn it down (e.g., in new version of PBWM)
  bool          add_pv_lv;      // #DEF_false for cases where reward is expected/delivered, add PV and LV dopamine signals (otherwise, only use PV signal)

  override String       GetTypeDecoKey() const { return "LayerSpec"; }

  TA_SIMPLE_BASEFUNS(PVLVDaSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()       { };
  void  Defaults_init() { Initialize(); }
};

TypeDef_Of(PVLVDaLayerSpec);

class LEABRA_API PVLVDaLayerSpec : public LeabraLayerSpec {
  // computes PVLV dopamine (Da) signal: typically if(ER), da = PVe-PVi, else LVe - LVi
INHERITED(LeabraLayerSpec)
public:
  PVLVDaSpec    da;             // parameters for the pvlv da computation

  virtual void  Send_Da(LeabraLayer* lay, LeabraNetwork* net);
  // send the da value to sending projections: every cycle
  virtual void  Compute_Da(LeabraLayer* lay, LeabraNetwork* net);
  // compute the da value based on recv projections: every cycle in 1+ phases (delta version)

  override void BuildUnits_Threads(LeabraLayer* lay, LeabraNetwork* net);

  override void Compute_HardClamp(LeabraLayer* lay, LeabraNetwork* net);
  override void Compute_NetinStats(LeabraLayer* lay, LeabraNetwork* net) { };
  override void Compute_Inhib(LeabraLayer* lay, LeabraNetwork* net) { };
  override void Compute_ApplyInhib(LeabraLayer* lay, LeabraNetwork* net);

  // never learn
  override bool Compute_SRAvg_Test(LeabraLayer* lay, LeabraNetwork* net)  { return false; }
  override bool Compute_dWt_FirstPlus_Test(LeabraLayer* lay, LeabraNetwork* net) { return false; }
  override bool Compute_dWt_Nothing_Test(LeabraLayer* lay, LeabraNetwork* net) { return false; }

  void  HelpConfig();   // #BUTTON get help message for configuring this spec
  bool  CheckConfig_Layer(Layer* lay, bool quiet=false);

  TA_SIMPLE_BASEFUNS(PVLVDaLayerSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()               { };
  void  Defaults_init() { Initialize(); }
};

TypeDef_Of(PVLVTonicDaLayerSpec);

class LEABRA_API PVLVTonicDaLayerSpec : public PVLVLayerSpec {
  // display of tonic dopamine level -- just clamps the current value which is always stored in network->pvlv_tonic_da and is the definitive value (which can be manipulated by other layers) that is just reflected in this layer -- does not do any actual computation
INHERITED(PVLVLayerSpec)
public:
  virtual void  Clamp_Da(LeabraLayer* lay, LeabraNetwork* net);
  // clamp the current Da value on the layer activations

  override void BuildUnits_Threads(LeabraLayer* lay, LeabraNetwork* net);
  override void Compute_NetinStats(LeabraLayer* lay, LeabraNetwork* net) { };
  override void Compute_Inhib(LeabraLayer* lay, LeabraNetwork* net) { };
  override void Compute_ApplyInhib(LeabraLayer* lay, LeabraNetwork* net);

  // never learn
  override bool	Compute_SRAvg_Test(LeabraLayer* lay, LeabraNetwork* net)  { return false; }
  override bool	Compute_dWt_FirstPlus_Test(LeabraLayer* lay, LeabraNetwork* net) { return false; }
  override bool	Compute_dWt_Nothing_Test(LeabraLayer* lay, LeabraNetwork* net) { return false; }

  void	HelpConfig();	// #BUTTON get help message for configuring this spec
  bool  CheckConfig_Layer(Layer* lay, bool quiet=false);

  TA_SIMPLE_BASEFUNS(PVLVTonicDaLayerSpec);
protected:
  SPEC_DEFAULTS;
private:
  void  Initialize();
  void  Destroy()               { };
  void  Defaults_init()         { };
};


#endif // leabra_pvlv_h
