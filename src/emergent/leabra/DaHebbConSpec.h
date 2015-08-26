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

#ifndef DaHebbConSpec_h
#define DaHebbConSpec_h 1

// parent includes:
#include <LeabraConSpec>

// member includes:
#include <LeabraNetwork>

// declare all other types mentioned but not required to include:

eTypeDef_Of(DaHebbConSpec);

class E_API DaHebbConSpec : public LeabraConSpec {
  // basic dopamine-modulated hebbian learning -- dwt = da * ru_act * su_act
INHERITED(LeabraConSpec)
public:
  enum LearnActVal {            // activation value to use for learning
    PREV_TRIAL,                 // previous trial
    ACT_P,                      // act_p from current trial
    ACT_M,                      // act_m from current trial
  };
  
  LearnActVal         su_act_var;     // what variable to use for sending unit activation
  LearnActVal         ru_act_var;     // what variable to use for receiving unit activation

  inline float GetActVal(LeabraUnitVars* u, const LearnActVal& val) {
    switch(val) {
    case PREV_TRIAL:
      return u->act_q0;
      break;
    case ACT_P:
      return u->act_p;
      break;
    case ACT_M:
      return u->act_m;
      break;
    }
    return 0.0f;
  }    
  
  inline void C_Compute_dWt_Hebb_Da(float& dwt, const float ru_act, const float su_act,
                                    const float da_p) {
    dwt += cur_lrate * da_p * ru_act * su_act;
  }
  // #IGNORE dopamine multiplication

  inline void Compute_dWt(ConGroup* rcg, Network* rnet, int thr_no) {
    LeabraNetwork* net = (LeabraNetwork*)rnet;
    if(!learn || (ignore_unlearnable && net->unlearnable_trial)) return;
    LeabraConGroup* cg = (LeabraConGroup*)rcg;
    LeabraUnitVars* su = (LeabraUnitVars*)cg->ThrOwnUnVars(net, thr_no);
    float* dwts = cg->OwnCnVar(DWT);

    float su_act = GetActVal(su, su_act_var);

    const int sz = cg->size;
    for(int i=0; i<sz; i++) {
      LeabraUnitVars* ru = (LeabraUnitVars*)cg->UnVars(i, net);
      float ru_act = GetActVal(ru, ru_act_var);
      C_Compute_dWt_Hebb_Da(dwts[i], ru_act, su_act, ru->da_p);
    }
  }

  TA_SIMPLE_BASEFUNS(DaHebbConSpec);
protected:
  SPEC_DEFAULTS;
  // void	UpdateAfterEdit_impl();
private:
  void 	Initialize();
  void	Destroy()		{ };
  void	Defaults_init();
};

#endif // DaHebbConSpec_h
