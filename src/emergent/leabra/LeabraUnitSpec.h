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

#ifndef LeabraUnitSpec_h
#define LeabraUnitSpec_h 1

// parent includes:
#include "network_def.h"
#include <UnitSpec>
#include <SpecMemberBase>

// member includes:
#include <MinMaxRange>
#include <RandomSpec>
#include <Schedule>
#include <FunLookup>
#include <taMath_float>
#include <LeabraUnitVars>

// declare all other types mentioned but not required to include:
class Unit; // 
class Network; // 
class LeabraNetwork; // 
class LeabraUnit; // 
class DataTable; // 
class LeabraLayerSpec; //
class LeabraInhib; //

eTypeDef_Of(LeabraActFunSpec);

class E_API LeabraActFunSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra activation function specifications, using the gelin (g_e linear) activation function by default
INHERITED(SpecMemberBase)
public:
  float         thr;                // #DEF_0.5 threshold value Theta (Q) for firing output activation (.5 is more accurate value based on AdEx biological parameters and normalization -- see BioParams button)
  float         gain;                // #DEF_80;100;40 #MIN_0 gain (gamma) of the rate-coded activation functions -- 100 is default, 80 works better for larger models, and 40 is closer to the actual spiking behavior of the AdEx model -- use lower values for more graded signals, generally in lower input/sensory layers of the network
  float         nvar;                // #DEF_0.005;0.01 #MIN_0 variance of the Gaussian noise kernel for convolving with XX1 in NOISY_XX1 and NOISY_LINEAR -- determines the level of curvature of the activation function near the threshold -- increase for more graded responding there -- note that this is not actual stochastic noise, just constant convolved gaussian smoothness to the activation function

  String        GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(LeabraActFunSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(LeabraActMiscSpec);

class E_API LeabraActMiscSpec : public SpecMemberBase {
  //##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra miscellaneous activation computation parameters and specs
INHERITED(SpecMemberBase)
public:
  bool          rec_nd;         // record the act_nd non-depressed activation variable (instead of act_eq) for the act_q* quarter-trial and phase (act_m, act_p) activation state variables -- these are used primarily for statistics, or possibly for specialized learning mechanisms
  bool          avg_nd;         // use the act_nd non-depressed activation variable (instead of act_eq) for the time-average activation values (avg_ss, avg_s, avg_m, avg_l) used in the XCAL learning mechanism -- this is appropriate for action-potential driven learning dynamics, as compared to synaptic efficacy, when short term plasticity is present
  float         net_gain;       // #DEF_1 #MIN_0 multiplier on total synaptic net input -- this multiplies the net_raw, but AFTER the net_raw variable is saved (upon which the netin_raw statistics are computed)

  float         act_max_hz;     // #DEF_100 #MIN_1 for translating rate-code activations into discrete spiking (only used for clamped layers), what is the maximum firing rate associated with a maximum activation value (max act is typically 1.0 -- depends on act_range)
  float         avg_tau;        // #DEF_200 #MIN_1 for integrating activation average (act_avg), time constant in trials (roughly, how long it takes for value to change significantly) -- used mostly for visualization and tracking "hog" units
  float         avg_init;        // #DEF_0.15 #MIN_0 initial activation average value -- used for act_avg, avg_s, avg_m, avg_l
  float         avg_dt;                // #READ_ONLY #EXPERT rate = 1 / tau

  inline int    ActToInterval(const float time_inc, const float integ, const float act)
  { return (int) (1.0f / (time_inc * integ * act * act_max_hz)); }
  // #CAT_ActMisc compute spiking interval based on network time_inc, dt.integ, and unit act -- note that network time_inc is usually .001 = 1 msec per cycle -- this depends on that being accurately set

  TA_SIMPLE_BASEFUNS(LeabraActMiscSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(SpikeFunSpec);

class E_API SpikeFunSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra spiking activation function specs -- conductance is computed postsynaptically using an alpha function based on spike pulses sent presynaptically -- for clamped layers, spiking probability is proportional to external input controlled by the clamp_type and clamp_max_p values -- soft clamping may still be a better option though
INHERITED(SpecMemberBase)
public:
  float         rise;                // #DEF_0 #MIN_0 exponential rise time (in cycles) of the synaptic conductance according to the alpha function 1/(decay - rise) [e^(-t/decay) - e^(-t/rise)] -- set to 0 to only include decay time (1/decay e^(-t/decay)), which is highly optimized (doesn't use window -- just uses recursive exp decay) and thus the default!
  float         decay;                // #DEF_5 #MIN_0 exponential decay time (in cycles) of the synaptic conductance according to the alpha function 1/(decay - rise) [e^(-t/decay) - e^(-t/rise)] -- set to 0 to implement a delta function (not very useful)
  float         g_gain;                // #DEF_9 #MIN_0 multiplier for the spike-generated conductances when using alpha function which is normalized by area under the curve -- needed to recalibrate the alpha-function currents relative to rate code net input which is overall larger -- in general making this the same as the decay constant works well, effectively neutralizing the area normalization (results in consistent peak current, but differential integrated current over time as a function of rise and decay)
  int           window;                // #DEF_3 #MIN_0 spike integration window -- when rise==0, this window is used to smooth out the spike impulses similar to a rise time -- each net contributes over the window in proportion to 1/window -- for rise > 0, this is used for computing the alpha function -- should be long enough to incorporate the bulk of the alpha function, but the longer the window, the greater the computational cost
  float         eq_gain;        // #DEF_8 #MIN_0 gain for computing act_eq relative to actual average: act_eq = eq_gain * (spikes/cycles)
  float         eq_tau;                // #DEF_50 #MIN_0 if non-zero, compute act_eq as a continuous running average instead of explicit spikes / cycles -- this is the time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life)

  float         gg_decay;        // #READ_ONLY #NO_SAVE g_gain/decay
  float         gg_decay_sq;        // #READ_ONLY #NO_SAVE g_gain/decay^2
  float         gg_decay_rise; // #READ_ONLY #NO_SAVE g_gain/(decay-rise)
  float         oneo_decay;        // #READ_ONLY #NO_SAVE 1.0/decay
  float         oneo_rise;        // #READ_ONLY #NO_SAVE 1.0/rise
  float         eq_dt;          // #READ_ONLY #EXPERT rate = 1 / tau

  float        ComputeAlpha(float t) {
    if(decay == 0.0f) return (t == 0.0f) ? g_gain : 0.0f; // delta function
    if(rise == 0.0f) return gg_decay * taMath_float::exp_fast(-t * oneo_decay);         // exponential
    if(rise == decay) return t * gg_decay_sq * taMath_float::exp_fast(-t * oneo_decay); // symmetric alpha
    return gg_decay_rise * (taMath_float::exp_fast(-t * oneo_decay) - taMath_float::exp_fast(-t * oneo_rise)); // full alpha
  }

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(SpikeFunSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init() { Initialize(); }
};

eTypeDef_Of(SpikeMiscSpec);

class E_API SpikeMiscSpec : public SpecMemberBase {
  // ##INLINE ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra extra misc spiking parameters 
INHERITED(SpecMemberBase)
public:
  enum ClampType {                // how to generate spikes during hard clamp conditions
    POISSON,                        // generate spikes according to Poisson distribution with probability = clamp_max_p * u->ext
    UNIFORM,                        // generate spikes according to Uniform distribution with probability = clamp_max_p * u->ext
    REGULAR,                        // generate spikes every 1 / (clamp_max_p * u->ext) cycles -- this works the best, at least in smaller networks, due to the lack of additional noise, and the synchrony of the inputs for driving synchrony elsewhere
    CLAMPED,                        // just use the straight clamped activation value -- do not do any further modifications
  };

  bool          ex;             // #DEF_false turn on exponential excitatory current that drives v_m rapidly upward for spiking as it gets past its nominal firing threshold (act.thr) -- nicely captures the Hodgkin Huxley dynamics of Na and K channels -- uses Brette & Gurstner 2005 AdEx formulation -- this mechanism has an unfortunate interaction with the continuous inhibitory currents generated by the standard FF_FB inhibitory function, which cause this mechanism to desensitize and fail to spike
  float         exp_slope;        // #CONDSHOW_ON_ex #DEF_0.02 slope in v_m (2 mV = .02 in normalized units) for extra exponential excitatory current that drives v_m rapidly upward for spiking as it gets past its nominal firing threshold (act.thr) -- nicely captures the Hodgkin Huxley dynamics of Na and K channels -- uses Brette & Gurstner 2005 AdEx formulation -- a value of 0 disables this mechanism
  float         spk_thr;        // #CONDSHOW_ON_ex #DEF_1.2 membrane potential threshold for actually triggering a spike when using the exponential mechanism -- the nominal threshold in act.thr enters into the exponential mechanism, but this value is actually used for spike thresholding 
  float         vm_r;                // #DEF_0;0.15;0.3 #AKA_v_m_r post-spiking membrane potential to reset to, produces refractory effect if lower than vm_init -- 0.30 is apropriate biologically-based value for AdEx (Brette & Gurstner, 2005) parameters
  int           t_r;                // #DEF_3 post-spiking explicit refractory period, in cycles -- prevents v_m updating for this number of cycles post firing
  float         clamp_max_p;        // #DEF_0.12 #MIN_0 #MAX_1 maximum probability of spike rate firing for hard-clamped external inputs -- multiply ext value times this to get overall probability of firing a spike -- distribution is determined by clamp_type
  ClampType     clamp_type;        // how to generate spikes when layer is hard clamped -- in many cases soft clamping may work better

  float         eff_spk_thr;    // #HIDDEN #READ_ONLY effective spiking threshold -- depends on whether exponential mechanism is being used (= act.thr if not ex, else spk_thr)

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(SpikeMiscSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

#ifdef SUGP_NETIN
eTypeDef_Of(LeabraNetinSpec);

class E_API LeabraNetinSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra how to compute excitatory net input to unit
INHERITED(SpecMemberBase)
public:
  bool          max_on;             // enable computation of netinput as a max over sending unit groups, for projections that have sending unit groups -- the max is computed within each projection, and then summed across projections -- parameters allow for a mix of max and sum netin
  bool          max_ff;             // #CONDSHOW_ON_max_on apply max operation to feedforward (from input) projections only
  float         max_mix;            // #CONDSHOW_ON_max_on #MIN_0 #MAX_1 mix between max over sending unit group netin, and sum over all sending unit groups -- 1 = all max, 0 = all sum, .5 = half of each
  float         max_gain;           // #CONDSHOW_ON_max_on #DEF_0.4 multiplier applied to number of sending unit groups, to bring max_net into same range as netin summed across all unit groups

  float         sum_mix;            // #READ_ONLY #HIDDEN 1 - max_mix
  float         max_mult;           // #READ_ONLY #HIDDEN max_mix * max_gain
  
  String        GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(LeabraNetinSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};
#endif // SUGP_NETIN

eTypeDef_Of(OptThreshSpec);

class E_API OptThreshSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra optimization thresholds for faster processing
INHERITED(SpecMemberBase)
public:
  float         send;                // #DEF_0.1 don't send activation when act <= send -- greatly speeds processing
  float         delta;                // #DEF_0.005 don't send activation changes until they exceed this threshold: only for when LeabraNetwork::send_delta is on!
  float         xcal_lrn;       // #DEF_0.01 xcal learning threshold -- don't learn when sending unit activation is below this value in both phases -- due to the nature of the learning function being 0 when the sr coproduct is 0, it should not affect learning in any substantial way -- this is applied in the LeabraConSpec, so other learning algorithms that have different properties should ignore it

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(OptThreshSpec);
protected:
  SPEC_DEFAULTS;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init() { Initialize(); }
};

eTypeDef_Of(LeabraInitSpec);

class E_API LeabraInitSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra initial values for key network state variables -- initialized at start of trial with Init_Acts or DecayState
INHERITED(SpecMemberBase)
public:
  float       v_m;        // #DEF_0.4 initial membrane potential -- see e_rev.l for the resting potential (typically .3) -- often works better to have a somewhat elevated initial membrane potential relative to that
  float       act;        // #DEF_0 initial activation value -- typically 0
  float       netin;      // #DEF_0 baseline level of excitatory net input -- netin is initialized to this value, and it is added in as a constant background level of excitatory input -- captures all the other inputs not represented in the model, and intrinsic excitability, etc

  TA_SIMPLE_BASEFUNS(LeabraInitSpec);
protected:
  SPEC_DEFAULTS;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(LeabraDtSpec);

class E_API LeabraDtSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra time and rate constants for temporal derivatives in Leabra (Vm, net input)
INHERITED(SpecMemberBase)
public:
  float         integ;                // #DEF_1;0.5 #MIN_0 overall rate constant for numerical integration, for all equations at the unit level -- all time constants are specified in millisecond units, with one cycle = 1 msec -- if you instead want to make one cycle = 2 msec, you can do this globaly by setting this integ value to 2 (etc).  However, stability issues will likely arise if you go too high.  For improved numerical stability, you may even need to reduce this value to 0.5 or possibly even lower (typically however this is not necessary).  MUST also coordinate this with network.time_inc variable to ensure that global network.time reflects simulated time accurately
  float         vm_tau;                // #AKA_vm_time #DEF_2.81:10 [3.3 std for rate code, 2.81 for spiking] #MIN_1 membrane potential and rate-code activation time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life) -- reflects the capacitance of the neuron in principle -- biological default for AeEx spiking model C = 281 pF = 2.81 normalized -- for rate-code activation, this also determines how fast to integrate computed activation values over time
  float         net_tau;        // #AKA_net_time #DEF_1.4 #MIN_1 net input time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life) -- this is important for damping oscillations -- generally reflects time constants associated with synaptic channels which are not modeled in the most abstract rate code models (set to 1 for detailed spiking models with more realistic synaptic currents)
  int           fast_cyc;        // #AKA_vm_eq_cyc #DEF_0 number of cycles at start of a trial to run units in a fast integration mode -- the rate-code activations have no effective time constant and change immediately to the new computed value (vm_time is ignored) and vm is computed as an equilibirium potential given current inputs: set to 1 to quickly activate soft-clamped input layers (primary use); set to 100 to always use this computation

  float         vm_dt;                // #READ_ONLY #EXPERT rate = 1 / tau
  float         net_dt;                // #READ_ONLY #EXPERT rate = 1 / tau

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(LeabraDtSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(LeabraActAvgSpec);

class E_API LeabraActAvgSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS ##CAT_Leabra rate constants for averaging over activations -- only used in XCAL learning rules
INHERITED(SpecMemberBase)
public:
  float         ss_tau;                // #DEF_2;20 #MIN_1 time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life), for continuously updating the super-short time-scale avg_ss value -- this is provides a pre-integration step before integrating into the avg_s short time scale
  float         s_tau;                // #DEF_2;20 #MIN_1 time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life), for continuously updating the short time-scale avg_s value from the super-short avg_ss value (cascade mode) -- avg_s represents the plus phase learning signal that reflects the most recent past information
  float         m_tau;                // #DEF_10;100 #MIN_1 time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life), for continuously updating the medium time-scale avg_m value from the short avg_s value (cascade mode) -- avg_m represents the minus phase learning signal that reflects the expectation representation prior to experiencing the outcome (in addition to the outcome)
  float         m_in_s;                // #DEF_0.1 #MIN_0 #MAX_1 how much of the medium term average activation to include at the short (plus phase) avg_s_eff variable that is actually used in learning -- important to ensure that when unit turns off in plus phase (short time scale), enough medium-phase trace remains so that learning signal doesn't just go all the way to 0, at which point no learning would take place -- typically need faster time constant for updating s such that this trace of the m signal is lost

  float         s_in_s;              // #READ_ONLY #EXPERT 1-m_in_s
  float         ss_dt;               // #READ_ONLY #EXPERT rate = 1 / tau
  float         s_dt;                // #READ_ONLY #EXPERT rate = 1 / tau
  float         m_dt;                // #READ_ONLY #EXPERT rate = 1 / tau

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(LeabraActAvgSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(LeabraAvgLSpec);

class E_API LeabraAvgLSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS ##CAT_Leabra parameters for computing the long-term floating average value, avg_l, which is used for driving BCM-style hebbian learning in XCAL -- this form of learning increases contrast of weights and generally decreases overall activity of neuron, to prevent "hog" units
INHERITED(SpecMemberBase)
public:
  float         init;           // #DEF_0.4 #MIN_0 #MAX_1 initial avg_l value at start of training
  float         max;            // #DEF_1.5 #MIN_0 maximum avg_l value -- when unit activation is greater than act_thr, then we increase avg_l in a soft-bounded way toward this max value -- higher values up to 3.0 can be used when "hog" unit problem is particularly severe, but in general this does not fix the problem unfortunately -- just treating the symptoms, not the underlying cause -- the 1.5 default generally provides a beneficial nudge and works well for most models
  float         min;            // #DEF_0.2 #MIN_0 miniumum avg_l value -- when unit activation is less than act_thr, then we decrease avg_l in a soft-bounded way toward this min value -- the default 0.2 value seems to work well for most models
  float         tau;            // #DEF_10 #MIN_1 time constant for updating avg_l -- rate of approaching soft exponential bound to max / min -- longer time constants can also work fine, but the default of 10 allows for quicker reaction to beneficial weight changes
  float         lrn_max;        // #DEF_0.05 #MIN_0 maximum avg_l_lrn value -- if avg_l is at its maximum value, then avg_l_lrn will be at this maximum value -- used to increase the amount of self-organizing learning, which will then bring down average activity of units -- the default of 0.05, in combination with the err_mod flag, works well for most models
  float         lrn_min;        // #DEF_0.005 #MIN_0 miniumum avg_l_lrn -- if avg_l is at its minimum value, then avg_l_lrn will be at this minimum value -- neurons that are not overly active may not need to increase the contrast of their weights as much -- the default of 0.005 works well for most models
  
  float         dt;             // #READ_ONLY #EXPERT rate = 1 / tau
  float         lrn_fact;       // #READ_ONLY #EXPERT (lrn_max - lrn_min) / (max - min)

  inline float  GetLrn(const float avg_l) {
    return lrn_min + lrn_fact * (avg_l - min);
  }
  // get the avg_l_lrn value for given avg_l value
  
  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(LeabraAvgLSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(LeabraAvgL2Spec);

class E_API LeabraAvgL2Spec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS ##CAT_Leabra additional parameters for computing the long-term floating average value, avg_l, which is used for driving BCM-style hebbian learning in XCAL -- this form of learning increases contrast of weights and generally decreases overall activity of neuron, to prevent "hog" units
INHERITED(SpecMemberBase)
public:
  bool          err_mod;        // #DEF_true if true, then we multiply avg_l_lrn factor by layer.cos_diff_avg_lrn to make hebbian term roughly proportional to amount of error driven learning signal across layers -- cos_diff_avg computes the running average of the cos diff value between act_m and act_p (no diff is 1, max diff is 0), and cos_diff_avg_lrn = 1 - cos_diff_avg (and 0 for non-HIDDEN layers), so the effective lrn value is high when there are large error signals (differences) in a layer, and low when error signals are low, producing a more consistent mix overall -- typically this error level tends to be stable for a given layer, so this is really just a quick shortcut for setting layer-specific mixes by hand (which the brain can do) -- cos_diff_avg_tau rate constant is in LayerSpec.decay settings
  float         err_min;        // #DEF_0.01:0.1 #CONDSHOW_ON_err_mod minimum layer.cos_diff_avg_lrn value (for non-zero cases, i.e., not for target or input layers) -- ensures a minimum amount of self-organizing learning even for layers that have a very small level of error signal -- this may need to be increased for problematic hog layers in deep5aed auto-encoder networks, which tend to have small error values but also significant hog unit problems
  float         act_thr;        // #DEF_0.2 threshold of minus-phase activation value for whether to increase or decrease the avg_l value -- generally don't need to change this from the default value
  float         lay_act_thr;    // #DEF_0.01 threshold of layer average activation on this trial, in order to update avg_l values -- setting to 0 disables this check
  
  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(LeabraAvgL2Spec);
protected:
  SPEC_DEFAULTS;

private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};


eTypeDef_Of(AdaptLeakSpec);

class E_API AdaptLeakSpec : public SpecMemberBase {
  // ##INLINE ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra adapting leak value -- a homeostatic adaptation using the bias weight values to keep units in their sensitive range of activation -- works along with the BCM function but operates at the whole-neuron level -- uses the avg_l value that reflects the long-term running average activation -- bias weight reflects *addition* to default g_bar.l value -- need to turn bias weight learning OFF!
INHERITED(SpecMemberBase)
public:
  bool          on;             // perform leak adaptation?  if so, adapts bias weight as an increment to default g_bar.l leak value as function of computed avg_l values which reflect longer-term average activation of unit -- be sure to turn learning off when using this!
  float         tau;           // #CONDSHOW_ON_on #DEF_100 time constant for adapting the leak as a function of avg_l value
  float         hi_thr;        // #CONDSHOW_ON_on #DEF_1:1.2 high-side threshold on avg_l value -- avg_l must exceed this value before increasing bias weight offset to leak value
  float         lo_thr;        // #CONDSHOW_ON_on #DEF_0.4 low-side threshold on avg_l value -- avg_l must be below this value before decreasing bias weight offset to leak value
  float         min_bwt;      // #CONDSHOW_ON_on #DEF_0;-0.05 minimum leak offset (i.e., bias weight or bwt) value -- don't want too decrease leak too much below default parameter in general
  float         max_bwt;      // #CONDSHOW_ON_on #DEF_0.2 maximum leak offset (i.e., bias weight or bwt) value -- prevents excessive modification for situations where units must be active

  float         dt;            // #CONDSHOW_ON_on #READ_ONLY #EXPERT rate =  1 / tau
  float         mid_thr;       // #CONDSHOW_ON_on #READ_ONLY #EXPERT middle point between hi and low threshold

  inline bool   AdaptLeak(float& bwt, float& dwt, const float avg_l) {
    if(avg_l >= hi_thr || avg_l <= lo_thr) {
      dwt = dt * (avg_l - mid_thr);
      bwt += dwt;
      if(bwt < min_bwt)        bwt = min_bwt;
      else if(bwt > max_bwt)   bwt = max_bwt;
      return true;
    }
    return false;
  }
  // adadpt leak offset as bias weight values, given current avg_l value
  
  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(AdaptLeakSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(LeabraChannels);

class E_API LeabraChannels : public taOBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra channels used in Leabra
INHERITED(taOBase)
public:
  float         e;                // excitatory (sodium (Na) channel), synaptic glutamate AMPA activated
  float         l;                // constant leak (potassium, K+) channel 
  float         i;                // inhibitory (chloride, Cl-) channel, synaptic GABA activated

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  void         Copy_(const LeabraChannels& cp); // used in units, so optimized copy needed
  TA_BASEFUNS(LeabraChannels);
private:
  void        Initialize();
  void        Destroy()        { };
};

eTypeDef_Of(ActAdaptSpec);

class E_API ActAdaptSpec : public SpecMemberBase {
  // ##INLINE ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra activation-driven adaptation dynamics -- negative feedback on v_m based on sub- and super-threshold activation -- relatively rapid time-scale and especially relevant for spike-based models -- drives the adapt variable on the unit
INHERITED(SpecMemberBase)
public:
  bool          on;                // apply adaptation?
  float         tau;                // #CONDSHOW_ON_on #DEF_144 adaptation dynamics time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life)
  float         vm_gain;        // #CONDSHOW_ON_on #MIN_0 #DEF_0.04 gain on the membrane potential v_m driving the adapt adaptation variable -- default of 0.04 reflects 4nS biological value converted into normalized units
  float         spike_gain;        // #CONDSHOW_ON_on #DEF_0.00805;0.004;0.002 value to add to the adapt adaptation variable after spiking -- default of 0.00805 is normalized version of .0805 nA in biological values -- weaker levels often work better (e.g., 0.004)
  bool          Ei_dyn;         // #CONDSHOW_ON_on does the inhibitory reversal potential (E_i) update dynamically over time in response to activation of the receiving neuron (backpropagating action potentials), or is it static -- dynamics are important when using adaptation, as this compensates for adaptation and allows active neurons to remain active -- only enabeled when activation is on, because it is bad when adaptation is not on
  float         Ei_gain;        // #CONDSHOW_ON_Ei_dyn&&on #MIN_0 #DEF_0.001 multiplier on postsynaptic cell activation (act_eq), driving increases in E_i reversal potential for Cl- -- this factor determines how strong the e_rev change effect is
  float         Ei_tau;         // #CONDSHOW_ON_Ei_dyn&&on #MIN_1 #DEF_50 decay time constant for decay of inhibitory reversal potential -- active neurons raise their inhibitory reversal potential, giving them an advantage over inactive neurons
  float         dt;                // #READ_ONLY #EXPERT rate = 1 / tau
  float         Ei_dt;          // #READ_ONLY #EXPERT rate = 1 / tau

  float        Compute_dAdapt(float vm, float e_rev_l, float adapt)
  { return dt * (vm_gain * (vm - e_rev_l) - adapt); }
  // compute the change in adapt given vm, resting reversal potential (leak reversal), and adapt inputs

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(ActAdaptSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};


eTypeDef_Of(ShortPlastSpec);

class E_API ShortPlastSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra short-term plasticity specifications -- uses standard equations summarized in Hennig, 2013 (eq 6) to capture both facilitation and depression dynamics as a function of presynaptic firing -- models interactions between number of vesicles available to release, and probability of release, and a time-varying recovery rate -- rate code uses generated spike var to drive this
INHERITED(SpecMemberBase)
public:
  bool          on;                // synaptic depression is in effect: multiplies normal activation computed by current activation function in effect
  float         f_r_ratio;      // #CONDSHOW_ON_on #DEF_0.01:3 ratio of facilitating (t_fac) to depression recovery (t_rec) time constants -- influences overall nature of response balance (ratio = 1 is balanced, > 1 is facilitating, < 1 is depressing).  Wang et al 2006 found: ~2.5 for strongly facilitating PFC neurons (E1), ~0.02 for strongly depressing PFC and visual cortex (E2), and ~1.0 for balanced PFC (E3)
  float         rec_tau;        // #CONDSHOW_ON_on #DEF_100:1000 #MIN_1 [200 std] time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life) for the constant form of the recovery of number of available vesicles to release at each action potential -- one factor influencing how strong and long-lasting depression is: nr += (1-nr)/rec_tau.  Wang et al 2006 found: ~200ms for strongly depressing in visual cortex and facilitating PFC (E1), 600ms for depressing PFC (E2), and between 200-600 for balanced (E3)
  float         p0;             // #CONDSHOW_ON_on #DEF_0.1:0.4 [0.2 std] baseline probability of release -- lower values around .1 produce more strongly facilitating dynamics, while .4 makes depression dominant -- interacts with f_r_ratio time constants as well.  Tuning advice: keeping all other params at their default values, and focusing on depressing dynamics, this value relative to p0_norm = 0.2 can give different degrees of depression: 0.2 = strong depression, 0.15 = weaker, and 0.1 = very weak depression dynamics
  float         p0_norm;        // #CONDSHOW_ON_on #DEF_0.1:1 [0.2 std] baseline probability of release to use for normalizing the overall net synaptic transmitter release (syn_tr) -- for depressing synapses, this should be = p0, but for facilitating, it make sense to normalize at a somewhat higher level, so that the syn_tr starts out lower and rises to a max -- it maxes out at 1.0 so you don't want to lose dynamic range
  float         kre_tau;        // #CONDSHOW_ON_on #DEF_100 time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life) on dynamic enhancement of time constant of recovery due to activation -- recovery time constant increases as a function of activity, helping to linearize response (reduce level of depression) at higher frequencies -- supported by multiple sources of biological data (Hennig, 2013)
  float         kre;            // #CONDSHOW_ON_on #DEF_0.002;0 how much the dynamic enhancement of recovery time constant increases for each action potential -- determines how strong this dynamic component is -- set to 0 to turn off this extra adaptation
  float         fac_tau;          // #CONDSHOW_ON_on #READ_ONLY #SHOW auto computed from f_r_ratio and rec_tau: time constant in cycles, which should be milliseconds typically (roughly, how long it takes for value to change significantly -- 1.4x the half-life) for the dynamics of facilitation of release probability: pr += (p0 - pr) / fac_tau. Wang et al 2006 found: 6ms for visual cortex, 10-20ms strongly depressing PFC (E2), ~500ms for strongly facilitating (E1), and between 200-600 for balanced (E3)
  float         fac;            // #CONDSHOW_ON_on #DEF_0.2:0.5 #MIN_0 strength of facilitation effect -- how much each action potential facilitates the probability of release toward a maximum of one: pr += fac (1-pr) -- typically right around 0.3 in Wang et al, 2006

  float         rec_dt;                // #CONDSHOW_ON_on #READ_ONLY #EXPERT rate constant for recovery = 1 / rec_tau 
  float         fac_dt;         // #CONDSHOW_ON_on #READ_ONLY #EXPERT rate constant for facilitation =  1 / fac_tau
  float         kre_dt;         // #CONDSHOW_ON_on #READ_ONLY #EXPERT rate constant for recovery enhancement = 1 / kre_tau
  float         oneo_p0_norm;   // #CONDSHOW_ON_on #READ_ONLY #EXPERT 1 / p0_norm

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(ShortPlastSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(SynDelaySpec);

class E_API SynDelaySpec : public SpecMemberBase {
  // ##INLINE ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra synaptic delay -- activation sent to other units is delayed by a given number of cycles
INHERITED(SpecMemberBase)
public:
  bool          on;                // is synaptic delay active?
  int           delay;             // #CONDSHOW_ON_on #MIN_0 number of cycles to delay for

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(SynDelaySpec);
protected:
  SPEC_DEFAULTS;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init() { }; // note: does NOT do any init -- these vals are not really subject to defaults in the usual way, so don't mess with them
};

eTypeDef_Of(RLrateSpec);

class E_API RLrateSpec : public SpecMemberBase {
  // ##INLINE ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra receiving unit learning rate spec: learning rate dynamics based on activity profile of the receiving unit -- implements trace-like learning to support development of invariant representations -- high lrate for units that have above-threshold activation for two trials in a row
INHERITED(SpecMemberBase)
public:
  bool          on;                // turn on recv learning rate dynamics
  float         base;              // #CONDSHOW_ON_on #MIN_0 baseline learning rate -- for units that fail to meet the high lrate criterion of two active trials in a row -- this value multiplies the overall learning rate for this unit across all of its synapses
  float         act_thr;           // #CONDSHOW_ON_on #MIN_0 activation threshold for units -- only those that are active above this threshold for two sequential trials get the full normal learning rate level -- others get the base level learning rate multiplier

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(RLrateSpec);
protected:
  SPEC_DEFAULTS;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};


eTypeDef_Of(DeepSpec);

class E_API DeepSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra specs for DeepLeabra deep neocortical layer dynamics, which capture attentional, thalamic auto-encoder, and temporal integration mechanisms 
INHERITED(SpecMemberBase)
public:
  enum DeepRole {          // what role do these neurons play in the deep layer network dynamics -- determines what function the deep_net plays
    SUPER,                 // superficial layer cortical neurons -- generate a deep_raw activation based on thresholds, and receive a deep_net signal from ongoing deep layer neuron activations via SendDeepModConSpec connections, which they then turn into a deep_mod activation value that multiplies activations in this layer
    DEEP,                  // deep layer cortical neurons -- receive deep_net inputs via DeepCtxtConSpec from superficial layer neurons to drive deep_ctxt values that are added into net input along with other inputs to support predictive learning of thalamic relay cell layer activations, which these should project to and receive from -- they should also receive direct top-down projections from other deep layer neurons for top-down attentional signals
    TRC,                   // thalamic relay cell neurons -- receive a SendDeepRawConSpec topographic projection from lower-layer superficial neurons (sending their deep_raw values into deep_net) which is all that is used for net input in the plus phase -- minus phase activation is driven from projections from deep layer predictive learning neurons
  };

  bool       on;         // enable the DeepLeabra mechanisms, including temporal integration via deep_ctxt context connections, thalamic-based auto-encoder driven by deep_raw projections, and attentional modulation by deep_mod
  DeepRole   role;       // #CONDSHOW_ON_on what role do these neurons play in overall deep layer network dynamics -- determines what function the deep_net plays, among other things
  float      raw_thr_rel;    // #CONDSHOW_ON_on #MAX_1 #DEF_0.1;0.2;0.5 #AKA_thr_rel relative threshold on act_raw value (distance between average and maximum act_raw values within layer, e.g., 0 = average, 1 = max) for deep_raw neurons to fire -- neurons below this level have deep_raw = 0 -- above this level, deep_raw = act_raw
  float      raw_thr_abs;    // #CONDSHOW_ON_on #MIN_0 #MAX_1 #DEF_0.1;0.2;0.5 #AKA_thr_abs absolute threshold on act_raw value for deep_raw neurons to fire -- see thr_rel for relative threshold and activation value -- effective threshold is MAX of relative and absolute thresholds
  float      mod_min;     // #CONDSHOW_ON_on&&role:SUPER #MIN_0 #MAX_1 minimum deep_mod value -- provides a non-zero baseline for deep-layer modulation
  bool       trc_p_only_m; // #CONDSHOW_ON_on&&role:TRC TRC plus-phase (clamping) for TRC units only occurs if the minus phase max activation for given unit group is above .1
  bool       trc_thal_gate; // #CONDSHOW_ON_on&&role:TRC apply thalamic gating to TRC activations -- multiply netin by current thal parameter
  bool       trc_trace;   // #CONDSHOW_ON_on&&role:TRC TRC plus-phase activation is MAX  of prior alpha trial's deep clamp activation and whatever is computed for this trial

  float      mod_range;  // #READ_ONLY #EXPERT 1 - mod_min -- range for the netinput to modulate value of deep_mod, between min and 1 value

  inline bool   IsSuper()
  { return on && role == SUPER; }
  // are we SUPER?
  inline bool   IsDeep()
  { return on && role == DEEP; }
  // are we DEEP?
  inline bool   IsTRC()
  { return on && role == TRC; }
  // are we thalamic relay cell units?

  inline bool   ApplyDeepMod()
  { return on && role == SUPER; }
  // should deep modulation be applied to these units?

  inline bool   SendDeepMod()
  { return on && role == DEEP; }
  // should we send our activation into deep_net of other (superficial) units via SendDeepModConSpec connections?

  inline bool   ApplyDeepCtxt()
  { return on && role == DEEP; }
  // should we apply deep context netinput?  only for deep guys

  String     GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(DeepSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;

private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(DaModSpec);

class E_API DaModSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra specs for effects of da-based modulation: plus-phase = learning effects
INHERITED(SpecMemberBase)
public:
  bool          on;               // whether to add dopamine factor to net input
  float         minus;            // #CONDSHOW_ON_on how much to multiply da_p in the minus phase to add to netinput -- use negative values for NoGo/indirect pathway/D2 type neurons
  float         plus;             // #CONDSHOW_ON_on #AKA_gain how much to multiply da_p in the plus phase to add to netinput -- use negative values for NoGo/indirect pathway/D2 type neurons

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(DaModSpec);
protected:
  SPEC_DEFAULTS;
private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(NoiseAdaptSpec);

class E_API NoiseAdaptSpec : public SpecMemberBase {
  // ##INLINE ##INLINE_DUMP ##NO_TOKENS #NO_UPDATE_AFTER ##CAT_Leabra specs for adapting the noise variance over time as a function of different variables
INHERITED(SpecMemberBase)
public:
  enum AdaptMode {
    FIXED_NOISE,                // no adaptation of noise: remains fixed at the noise.var value in the unit spec
    SCHED_CYCLES,                // use noise_sched over cycles of settling
    SCHED_EPOCHS,                // use noise_sched over epochs of learning
  };

  bool          trial_fixed;        // keep the same noise value over the entire trial -- prevents noise from being washed out and produces a stable effect that can be better used for learning -- this is strongly recommended for most learning situations
  float         drop_thr;       // probability of dropping units for DROPOUT_NOISE
  AdaptMode     mode;                // how to adapt noise variance over time

  String       GetTypeDecoKey() const override { return "UnitSpec"; }

  TA_SIMPLE_BASEFUNS(NoiseAdaptSpec);
protected:
  SPEC_DEFAULTS;
  void        UpdateAfterEdit_impl() override;

private:
  void        Initialize();
  void        Destroy()        { };
  void        Defaults_init();
};

eTypeDef_Of(LeabraUnitSpec);

class E_API LeabraUnitSpec : public UnitSpec {
  // #STEM_BASE ##CAT_Leabra Leabra unit specifications, point-neuron approximation
INHERITED(UnitSpec)
public:
  enum ActFun {
    NOISY_XX1,                        // x over x plus 1 convolved with Gaussian noise (noise is nvar)
    SPIKE,                        // discrete spiking activations (spike when > thr) -- default params produce adaptive exponential (AdEx) model
  };

  enum NoiseType {
    NO_NOISE,                        // no noise added to processing
    VM_NOISE,                        // noise in the value of v_m (membrane potential) -- IMPORTANT: this should NOT be used for rate-code (NXX1) activations, because they do not depend directly on the vm -- this then has no effect
    NETIN_NOISE,                // noise in the net input (g_e) -- this should be used for rate coded activations (NXX1)
    ACT_NOISE,                        // noise in the activations
    NET_MULT_NOISE,             // multiplicative net-input noise: multiply net input by the noise term
  };

  enum Quarters {               // #BITS specifies gamma frequency quarters within an alpha-frequency trial on which to do things
    QNULL = 0x00,              // #NO_BIT no quarter (yeah..)
    Q1 = 0x01,                 // first quarter
    Q2 = 0x02,                 // second quarter
    Q3 = 0x04,                 // third quarter -- posterior cortical minus phase
    Q4 = 0x08,                 // fourth quarter -- posterior cortical plus phase
    Q2_Q4 = Q2 | Q4,           // #NO_BIT standard beta frequency option, for bg, pfc
    QALL = Q1 | Q2 | Q3 | Q4,  // #NO_BIT all quarters
  };

  ActFun            act_fun;        // #CAT_Activation activation function to use -- typically NOISY_XX1 or SPIKE -- others are for special purposes or testing
  LeabraActFunSpec  act;            // #CAT_Activation activation function parameters -- very important for determining the shape of the selected act_fun
  LeabraActMiscSpec act_misc;       // #CAT_Activation miscellaneous activation parameters
  SpikeFunSpec      spike;          // #CONDSHOW_ON_act_fun:SPIKE #CAT_Activation spiking function specs (only for act_fun = SPIKE)
  SpikeMiscSpec    spike_misc;      // #CAT_Activation misc extra spiking function specs (only for act_fun = SPIKE)
#ifdef SUGP_NETIN
  LeabraNetinSpec  netin;           // #CAT_Activation how to compute the excitatory net input to units
#endif // SUGP_NETIN
  OptThreshSpec    opt_thresh;      // #CAT_Learning optimization thresholds for speeding up processing when units are basically inactive
  MinMaxRange      clamp_range;     // #CAT_Activation range of clamped activation values (min, max, 0, .95 std), don't clamp to 1 because acts can't reach, so .95 instead
  MinMaxRange      vm_range;        // #CAT_Activation membrane potential range (min, max, 0-2 for normalized)
  LeabraInitSpec   init;            // #CAT_Activation initial starting values for various key neural parameters
  LeabraDtSpec     dt;              // #CAT_Activation time constants (rate of updating): membrane potential (vm) and net input (net)
  LeabraActAvgSpec act_avg;         // #CAT_Learning time constants (rate of updating) for computing activation averages -- used in XCAL learning rules
  LeabraAvgLSpec   avg_l;           // #CAT_Learning parameters for computing the avg_l long-term floating average that drives BCM-style hebbian learning
  LeabraAvgL2Spec  avg_l_2;         // #CAT_Learning additional parameters for computing the avg_l long-term floating average that drives BCM-style hebbian learning
  AdaptLeakSpec    adapt_leak;     // #CAT_Learning adapting leak value -- a homeostatic adaptation using the bias weight values to keep units in their sensitive range of activation -- works along with the BCM function but operates at the whole-neuron level -- based on the avg_l value as computed in specs above -- bias weight reflects *addition* to default g_bar.l value
  LeabraChannels   g_bar;           // #CAT_Activation [Defaults: 1, .1, 1] maximal conductances for channels
  LeabraChannels   e_rev;           // #CAT_Activation [Defaults: 1, .3, .25] reversal potentials for each channel
  ActAdaptSpec     adapt;           // #CAT_Activation activation-driven adaptation factor that drives spike rate adaptation dynamics based on both sub- and supra-threshold membrane potentials
  ShortPlastSpec   stp;             // #CAT_Activation short term presynaptic plasticity specs -- can implement full range between facilitating vs. depresssion
  SynDelaySpec     syn_delay;       // #CAT_Activation synaptic delay -- if active, activation sent to other units is delayed by a given amount
  RLrateSpec       r_lrate;         // #CAT_Learning receiving-unit based learning rate specs -- sets effective learning rate multiplier based on activation profile of receiving unit, to promote greater translation invariance
  Quarters         deep_raw_qtr;    // #CAT_Learning #AKA_deep_qtr quarter(s) during which deep_raw layer 5 intrinsic bursting activations should be updated -- deep_raw is updated and sent to deep_raw_net during this quarter, and deep_ctxt is updated right after this quarter (wrapping around to the first quarter for the 4th quarter)
  DeepSpec         deep;            // #CAT_Learning specs for DeepLeabra deep neocortical layer dynamics, which capture attentional, thalamic auto-encoder, and temporal integration mechanisms 
  DaModSpec        da_mod;          // #CAT_Learning da modulation of activations (for da-based learning, and other effects)
  NoiseType        noise_type;      // #CAT_Activation where to add random noise in the processing (if at all)
  RandomSpec       noise;           // #CONDSHOW_OFF_noise_type:NO_NOISE #CAT_Activation distribution parameters for random added noise
  NoiseAdaptSpec   noise_adapt;     // #CONDSHOW_OFF_noise_type:NO_NOISE #CAT_Activation how to adapt the noise variance (var) value
  Schedule         noise_sched;     // #CONDSHOW_OFF_noise_type:NO_NOISE #CAT_Activation schedule of noise variance -- time scale depends on noise_adapt parameter (cycles, epochs, etc)

  FunLookup        nxx1_fun;        // #HIDDEN #NO_SAVE #NO_INHERIT #CAT_Activation convolved gaussian and x/x+1 function as lookup table
  FunLookup        noise_conv;      // #HIDDEN #NO_SAVE #NO_INHERIT #CAT_Activation gaussian for convolution


  inline void  TestWrite(float& var, const float val) {
    if(var != val) var = val;
  }
  // #IGNORE only write a value to a variable if it is not already set to that value -- may save on cache churn -- use for cases where it is likely that the same value is present

  ///////////////////////////////////////////////////////////////////////
  //        General Init functions

  void         LoadBiasWtVal(float bwt, UnitVars* u, Network* net) override;
  void         Init_Vars(UnitVars* uv, Network* net, int thr_no) override;
  void         Init_Weights(UnitVars* uv, Network* net, int thr_no) override;
  void         Init_Acts(UnitVars* uv, Network* net, int thr_no) override;
  virtual void Init_ActAvg(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation initialize average activation values, used to control learning

  virtual void Init_Netins(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation initialize netinput computation variables (delta-based requires several intermediate variables)
  virtual void DecayState(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no,
                           float decay);
  // #CAT_Activation decay activation states towards initial values by given amount (0 = no decay, 1 = full decay)

  virtual void Init_SpikeBuff(LeabraUnitVars* uv);
  // #CAT_Activation initialize spike buffers based on whether they are needed
  virtual void Init_ActBuff(LeabraUnitVars* uv);
  // #CAT_Activation initialize activation buffer based on whether they are needed

  ///////////////////////////////////////////////////////////////////////
  //        TrialInit -- at start of trial

  virtual void Trial_Init_Specs(LeabraNetwork* net);
  // #CAT_Learning initialize specs and specs update network flags 

  virtual void Trial_Init_Unit(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation trial unit-level initialization functions: Trial_Init_SRAvg, DecayState, NoiseInit

    virtual void Trial_Init_PrvVals(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Learning save previous trial values at start of new trial -- allow values at end of trial to be valid for visualization..
    virtual void Trial_Init_SRAvg(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Learning reset the sender-receiver coproduct average -- call at start of trial
    virtual void Trial_DecayState(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation decay activation states towards initial values: at trial-level boundary
    virtual void Trial_NoiseInit(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation init trial-level noise -- ONLY called if noise_adapt.trial_fixed is set

  ///////////////////////////////////////////////////////////////////////
  //        QuarterInit -- at start of new gamma-quarter

  inline  bool Quarter_DeepRawNow(int qtr)
  { return deep_raw_qtr & (1 << qtr); }
  // #CAT_Activation test whether to compute deep_raw activations and send deep_raw_net netintput at given quarter (pass net->quarter as arg)
  inline  bool Quarter_DeepRawPrevQtr(int qtr)
  { if(qtr == 0) qtr = 3; else qtr--; return deep_raw_qtr & (1 << qtr); }
  // #CAT_Activation test whether the previous quarter was when deep_raw was updated
  inline  bool Quarter_DeepRawNextQtr(int qtr)
  { return deep_raw_qtr & (1 << (qtr + 1)); }
  // #CAT_Activation test whether the next quarter will be when deep_raw is updated

  virtual void Quarter_Init_Unit(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation quarter unit-level initialization functions: Init_TargFlags, Init_PrvNet, NetinScale
    virtual void Quarter_Init_TargFlags(LeabraUnitVars* uv, LeabraNetwork* net,
                                        int thr_no);
    // #CAT_Activation initialize external input flags based on phase
    virtual void Quarter_Init_PrvVals(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation update the previous values: e.g., netinput variables (prv_net_q) based on current counters
    virtual void Compute_NetinScale(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation compute net input scaling values -- call at start of quarter just to be sure

    virtual void Quarter_Init_Deep(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no) { };
    // #CAT_Deep first entry point into quarter init deep for deep -- needed in subclasses (PFCUnitSpec) for updates prior to sending DeepCtxtNetin
    virtual void Send_DeepCtxtNetin(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Deep send deep_raw to deep_ctxt netinput, using deepraw netin temp buffer -- not delta based
    virtual void Compute_DeepCtxt(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Deep integrate deep_ctxt from deepraw netin temp sent previously by Send_DeepCtxtNetin
    virtual void Compute_DeepStateUpdt(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Deep state update for deep leabra -- typically at start of new alpha trial --

  void  Init_InputData(UnitVars* u, Network* net, int thr_no) override;
  
  virtual void ApplyInputData_post(LeabraUnitVars* uv);
  // #CAT_Activation post-apply input data -- cache the ext value b/c it might get overwritten in transforms of the input data, as in ScalarValLayerSpec
  virtual void Compute_HardClamp(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation force units to external values provided by environment
  virtual void Compute_HardClampNoClip(LeabraUnitVars* uv, LeabraNetwork* net,
                                        int thr_no);
  // #CAT_Activation hard-clamp units without clipping values to clamp_range (use for freezing activation states for example, e.g., in second plus phase)

  virtual void ExtToComp(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation change external inputs to comparisons (remove input)
  virtual void TargExtToComp(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation change target & external inputs to comparisons (remove targ & input)


  ///////////////////////////////////////////////////////////////////////
  //        Cycle Step 1: Netinput 

  virtual void Send_NetinDelta(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation send netinput; sender based and only when act changes above a threshold -- only this delta form is supported
  virtual void Compute_NetinInteg(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation integrate newly-computed netinput delta values into a resulting complete netinput value for the network (does both excitatory and inhibitory)
    virtual void  Compute_NetinRaw(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #IGNORE called by Compute_NetinInteg -- roll up the deltas into net_raw and gi_syn values (or compute net_raw by some other means for special algorithms)
    virtual void DeepModNetin_Integ(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #IGNORE integrate deep_mod_net values
    virtual float Compute_NetinExtras(LeabraUnitVars* uv, LeabraNetwork* net,
                                      int thr_no, float& net_syn);
    // #IGNORE called by Compute_NetinInteg -- get extra excitatory net input factors to add on top of regular synapticaly-generated net inputs, passed as net_syn -- standard items include: bias weights, external soft-clamp input, TI extras (ti_ctxt, d5b_net), CIFER extras: thal (which multiplies net_syn), and da_mod (which multiplies net_syn) -- specialized algorithms can even overwrite net_syn if they need too..
    virtual void Compute_NetinInteg_Spike_e(LeabraUnitVars* uv, LeabraNetwork* net,
                                            int thr_no);
    // #IGNORE called by Compute_NetinInteg for spiking units: compute actual excitatory netin conductance value for spiking units by integrating over spike
    virtual void Compute_NetinInteg_Spike_i(LeabraUnitVars* uv, LeabraNetwork* net,
                                            int thr_no);
    // #IGNORE called by Compute_NetinInteg for spiking units: compute actual inhibitory netin conductance value for spiking units by integrating over spike

  inline float Compute_EThresh(LeabraUnitVars* uv);
  // #CAT_Activation #IGNORE compute excitatory value that would place unit directly at threshold

  ///////////////////////////////////////////////////////////////////////
  //        Cycle Step 2: Inhibition: these are actually called by Compute_Act to integrate
  //            inhibition computed at the layer level

  inline LeabraInhib* GetInhib(LeabraUnit* u);
  // #CAT_Activation #IGNORE get the inhib that applies to this unit (either unit group or entire layer, depending on layer spec setting)

  inline void  Compute_SelfInhib(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no,
                                  LeabraLayerSpec* lspec);
  // #CAT_Activation #IGNORE compute self inhibition value
  virtual void        Compute_ApplyInhib
    (LeabraUnitVars* uv, LeabraNetwork* net, int thr_no, LeabraLayerSpec* lspec,
     LeabraInhib* thr, float ival);
  // #CAT_Activation #IGNORE apply computed inhibition value to unit inhibitory conductance


  ///////////////////////////////////////////////////////////////////////
  //        Cycle Step 3: Activation

  // main function is Compute_Act_Rate or _Spike which calls all the various sub-functions
  // below derived types that send activation directly to special unit variables (e.g.,
  // VTAUnitSpec -> da_p) should do this here, so they can be processed in Compute_Act_Post

  // IMPORTANT: the following function is NOT called -- Network calls _Rate or _Spike
  // directly!!
  inline void  Compute_Act(UnitVars* uv, Network* net, int thr_no) override {
    if(act_fun == SPIKE)
      Compute_Act_Spike((LeabraUnitVars*)uv, (LeabraNetwork*)net, thr_no);
    else
      Compute_Act_Rate((LeabraUnitVars*)uv, (LeabraNetwork*)net, thr_no);
  }

  virtual void Compute_Act_Rate(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation Rate coded activation

    virtual void Compute_DeepMod(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation compute deep_lrn and deep_mod values
    virtual void Compute_ActFun_Rate(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation compute the activation from g_e vs. threshold -- rate code functions
    virtual float Compute_ActFun_Rate_impl(float val_sub_thr);
    // #CAT_Activation raw activation function: computes an activation value from given value subtracted from its relevant threshold value

    virtual void Compute_RateCodeSpike(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation compute spiking activation (u->spike) based off of rate-code activation value

  virtual void Compute_Act_Spike(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation Spiking activation

    virtual void Compute_ActFun_Spike(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation compute the activation from membrane potential -- discrete spiking
    virtual void Compute_ClampSpike(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no,
                                    float spike_p);
    // #CAT_Activation compute spiking activation according to spike.clamp_type with given probability (typically spike.clamp_max_p * u->ext) -- includes depression and other active factors as done in Compute_ActFun_spike -- used for hard clamped inputs in spiking nets

  virtual void Compute_Vm(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation Act Step 2: compute the membrane potential from input conductances
    inline float Compute_EqVm(LeabraUnitVars* uv);
    // #CAT_Activation #IGNORE compute the equilibrium (asymptotic) membrante potential from input conductances (assuming they remain fixed as they are)

  virtual void Compute_SelfReg_Cycle(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation Act Step 3: compute self-regulatory dynamics at the cycle time scale -- adapt, etc
    virtual void Compute_ActAdapt_Cycle(LeabraUnitVars* uv, LeabraNetwork* net, 
                                        int thr_no);
    // #CAT_Activation compute the activation-based adaptation value based on spiking and membrane potential
    virtual void Compute_ShortPlast_Cycle(LeabraUnitVars* uv, LeabraNetwork* net,
                                            int thr_no);
    // #CAT_Activation compute whole-neuron (presynaptic) short-term plasticity at the cycle level, using the stp parameters -- updates the syn_* unit variables

    virtual float Compute_Noise(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation utility fun to generate and return the noise value based on current settings -- will set unit->noise value as appropriate (generally excludes effect of noise_sched schedule)

  ///////////////////////////////////////////////////////////////////////
  //        Post Activation Step

  virtual void Compute_Act_Post(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation post-processing step after activations are computed -- calls  Compute_SRAvg by default
    virtual void Compute_SRAvg(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Learning compute sending-receiving running activation averages (avg_ss, avg_s, avg_m) -- only for this unit (SR name is a hold-over from connection-level averaging that is no longer used) -- unit level only, used for XCAL -- called by Compute_Act_Post

  ///////////////////////////////////////////////////////////////////////
  //        Cycle Stats


  ///////////////////////////////////////////////////////////////////////
  //        Deep Leabra Computations -- after superifical acts updated

  virtual void Compute_DeepRaw(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation update the deep_raw activations -- assumes checks have already been done
  virtual void Send_DeepRawNetin(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Deep send deep5b netinputs through SendDeepRawConSpec connections
  virtual void DeepRawNetin_Integ(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Deep send context netinputs through SendDeepRawConSpec connections -- post processing rollup -- checks deeptest

  virtual void ClearDeepActs(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Deep clear all the deep lamina variables -- can be useful to do at discontinuities of experience

  ///////////////////////////////////////////////////////////////////////
  //        Quarter Final

  virtual void Quarter_Final(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
  // #CAT_Activation record state variables after each gamma-frequency quarter-trial of processing
    virtual void Quarter_Final_RecVals(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation record state variables after each gamma-frequency quarter-trial of processing
    virtual void Compute_ActTimeAvg(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no);
    // #CAT_Activation compute time-averaged activation of unit (using act.avg_dt time constant), typically done at end of settling in Quarter_Final function

  ///////////////////////////////////////////////////////////////////////
  //        Stats

  float Compute_SSE(UnitVars* uv, Network* net, int thr_no, bool& has_targ) override;
  bool  Compute_PRerr
    (UnitVars* uv, Network* net, int thr_no, float& true_pos, float& false_pos,
     float& false_neg, float& true_neg) override;
  virtual float  Compute_NormErr(LeabraUnitVars* uv, LeabraNetwork* net, int thr_no,
                                 bool& targ_active);
  // #CAT_Statistic compute normalized binary error (0-1 as function of bits off of act_m vs target) according to settings on the network (returns a 1 or 0) -- if (net->lstats.on_errs && act_m > .5 && targ < .5) return 1; if (net->lstats.off_errs && act_m < .5 && targ > .5) return 1; else return 0

  ///////////////////////////////////////////////////////////////////////
  //        Misc Housekeeping, non Compute functions

  virtual void        CreateNXX1Fun(LeabraActFunSpec& act_spec, FunLookup& nxx1_fl,
                              FunLookup& noise_fl);
  // #CAT_Activation create convolved gaussian and x/x+1 function lookup tables

  virtual void         BioParams
    (float norm_sec=0.001f, float norm_volt=0.1f, float volt_off=-0.1f,
     float norm_amp=1.0e-8f, float C_pF=281.0f, float gbar_l_nS=10.0f,
     float gbar_e_nS=100.0f, float gbar_i_nS=100.0f,
     float erev_l_mV=-70.0f, float erev_e_mV=0.0f, float erev_i_mV=-75.0f,
     float act_thr_mV=-50.0f, float spk_thr_mV=20.0f, float exp_slope_mV=2.0f,
     float adapt_tau_ms=144.0f, float adapt_vm_gain_nS=4.0f,
     float adapt_spk_gain_nA=0.0805);
  // #BUTTON set parameters based on biologically-based values, using normalization scaling to convert into typical Leabra standard parameters.  norm_x are normalization values to convert from SI units to normalized values (defaults are 1ms = .001 s, 100mV with -100 mV offset to bring into 0-1 range between -100..0 mV, 1e-8 amps (makes g_bar, C, etc params nice).  other defaults are based on the AdEx model of Brette & Gurstner (2005), which the SPIKE mode implements exactly with these default parameters -- last bit of name indicates the units in which this value must be provided (mV = millivolts, ms = milliseconds, pF = picofarads, nS = nanosiemens, nA = nanoamps)

  virtual void        GraphVmFun(DataTable* graph_data, float g_i = .5, float min = 0.0,
                           float max = 1.0, float incr = .01);
  // #MENU_BUTTON #MENU_ON_Graph #NULL_OK #NULL_TEXT_NewGraphData graph membrane potential (v_m) as a function of excitatory net input (net) for given inhib conductance (g_i) (NULL = new graph data)
  virtual void        GraphActFmNetFun(DataTable* graph_data, float g_i = .5, float min = 0.0,
                                 float max = 1.0, float incr = .001, float g_e_thr = 0.5,
                                 float lin_gain = 10);
  // #MENU_BUTTON #MENU_ON_Graph #NULL_OK #NULL_TEXT_NewGraphData graph the activation function as a function of net input -- this is the direct activation function, computed relative to the g_e_thr threshold value provided -- a linear comparison with lin_gain slope is also provided for reference -- always computed as lin_gain * (net - g_e_thr) (NULL = new graph data)
  virtual void        GraphSpikeAlphaFun(DataTable* graph_data, bool force_alpha=false);
  // #MENU_BUTTON #MENU_ON_Graph #NULL_OK #NULL_TEXT_NewGraphData graph the spike alpha function for conductance integration over time window given in spike parameters -- last data point is the sum over the whole window (total conductance of a single spike) -- force_alpha means use explicit alpha function even when rise=0 (otherewise it simulates actual recursive exp decay used in optimized code)
//   virtual void        GraphSLNoiseAdaptFun(DataTable* graph_data, float incr = 0.05f);
//   // #MENU_BUTTON #MENU_ON_Graph #NULL_OK #NULL_TEXT_NewGraphData graph the short and long-term noise adaptation function, which integrates both short-term and long-term performance values
  virtual void        TimeExp(int mode, int nreps=100000000);
  // #EXPERT time how long it takes to compute various forms of exp() function: mode=0 = double sum ctrl (baseline), mode=1 = std double exp(), mode=2 = taMath_double::exp_fast, mode=3 = float sum ctrl (float baseline), mode=4 = expf, mode=5 = taMath_float::exp_fast -- this is the dominant cost in spike alpha function computation, so we're interested in optimizing it..

  bool          CheckConfig_Unit(Layer* lay, bool quiet=false) override;

  String        GetToolbarName() const override { return "unit spec"; }

  void          InitLinks() override;
  SIMPLE_COPY(LeabraUnitSpec);
  TA_BASEFUNS(LeabraUnitSpec);
protected:
  SPEC_DEFAULTS;
  void         UpdateAfterEdit_impl() override;        // to set _impl sig
  void         CheckThisConfig_impl(bool quiet, bool& rval) override;

  LeabraChannels e_rev_sub_thr;        // #CAT_Activation #READ_ONLY #NO_SAVE #HIDDEN e_rev - act.thr for each item -- used for compute_ithresh
  float          thr_sub_e_rev_i;// #CAT_Activation #READ_ONLY #NO_SAVE #HIDDEN g_bar.i * (act.thr - e_rev.i) used for compute_ithresh
  float          thr_sub_e_rev_e;// #CAT_Activation #READ_ONLY #NO_SAVE #HIDDEN g_bar.e * (act.thr - e_rev.e) used for compute_ethresh

private:
  void        Initialize();
  void        Destroy()                { };
  void        Defaults_init();
};

TA_SMART_PTRS(E_API, LeabraUnitSpec);

#endif // LeabraUnitSpec_h
