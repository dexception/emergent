#include "nb_netstru.h"

#include <math.h>
#include <stdio.h>
#include <iostream>

#include <QtCore/QFileInfo>
#if (QT_VERSION >= 0x040400) 
# include <QtCore/QtConcurrentMap>
#endif

using namespace std;

// this is set to a rand() before each cycle -- algos can read and hash it
// with their own key values to create a custom pseudo value, with very little
// overhead
static int g_rand; 

//////////////////////////
//  NetTaskList		//
//////////////////////////

NetTaskList::~NetTaskList() {
  while (count() > 0) {
    NetTask* nt = takeLast();
    delete nt;
  }
}


//////////////////////////
//  Connection		//
//////////////////////////


//////////////////////////
//  ConsBase		//
//////////////////////////

Connection* ConsBase::InitCon(SendCons* send_gp, RecvCons* recv_gp, 
    Unit* un_fm, Unit* un_to, int i_fm, int i_to, float wt)
{
  Connection* cn;
#if (CON_IN == CON_RECV)
  cn = recv_gp->Cn(i_fm);
  send_gp->SetCn(cn, i_to);
  send_gp->units[i_to] = un_to; 
# ifdef UN_IN_CON
  cn->un = un_fm;
# else
  recv_gp->units[i_fm] = un_fm;
# endif
#elif (CON_IN == CON_SEND)
  cn = send_gp->Cn(i_to);
  recv_gp->SetCn(cn, i_fm);
  recv_gp->units[i_fm] = un_fm;
# ifdef UN_IN_CON
  cn->un = un_to;
# else
  send_gp->units[i_to] = un_to; 
# endif
#endif
  cn->wt = wt;
  return cn;
}

ConsBase::ConsBase()
{
#ifndef UN_IN_CON
  units = NULL;
#endif
  size = 0;
}

ConsBase::~ConsBase() {
  size = 0;
#ifndef UN_IN_CON
  delete[] units; // safe if NULL
  units = NULL;
#endif
}

void ConsBase::setSize_impl(int i) {
#ifndef UN_IN_CON
  units = new Unit*[i];
#endif
#ifdef DEBUG
 // units.Fill(-1); // helps detect errors
#endif
}


//////////////////////////
//  RecvCons		//
//////////////////////////

RecvCons::RecvCons() {
  send_idx = -1; 
  send_lay = NULL;
  dwt_mean = 0.0f;
  cons = NULL;
  //note: we don't init the modal guys -- they get written in setSize
}

RecvCons::~RecvCons() {
  free(cons);
  cons = NULL;
#if ((CON_IN == CON_SEND) && defined(UN_IN_CON))
  delete[] units; // safe if NULL
  units = NULL;
#endif
}

void RecvCons::setSize_impl(int i) {
  inherited::setSize_impl(i);
#if (CON_IN == CON_RECV)
  cons = (Connection*)malloc(sizeof(Connection) * i);
#elif (CON_IN == CON_SEND)
  cons = (Connection**)malloc(sizeof(Connection*) * i);
# ifdef UN_IN_CON
  units = new Unit*[i];
# endif
#endif
}

//////////////////////////
//  SendCons		//
//////////////////////////

SendCons::SendCons() {
  recv_idx = -1;
  recv_lay = NULL;
  cons = NULL;
}

SendCons::~SendCons() {
  free(cons);
  cons = NULL;
#if ((CON_IN == CON_RECV) && defined(UN_IN_CON))
  delete[] units; // safe if NULL
  units = NULL;
#endif
}

void SendCons::setSize_impl(int i) {
  inherited::setSize_impl(i);
#if (CON_IN == CON_RECV)
  cons = (Connection**)malloc(sizeof(Connection*) * i);
# ifdef UN_IN_CON
  units = new Unit*[i];
# endif
#elif (CON_IN == CON_SEND)
  cons = (Connection*)malloc(sizeof(Connection) * i);
#endif
}


//////////////////////////
//  ConSpec		//
//////////////////////////

ConSpec:: ConSpec() {
  cur_lrate = 0.01f;
  norm_pct = 1.0f;
  wt_limits.max = 1.0f;
}

#if (CON_IN == CON_RECV)
void ConSpec::Compute_SRAvg(Unit* ru) {
  const float ru_act = ru->act;
  for(int g = 0; g < ru->recv.size; g++) {
    RecvCons* cg = ru->recv.FastEl(g);
    Connection* cons = cg->cons;
    const int cg_size = cg->size;
    Unit** units = cg->units;
    for (int i = 0; i < cg_size; ++i) {
      Connection* cn = &(cons[i]);
      cn->sravg += ru_act * units[i]->act;
    }
  }
}

void ConSpec::Compute_dWt(Unit* ru) {
  for(int g = 0; g < ru->recv.size; g++) {
    RecvCons* cg = ru->recv.FastEl(g);
    const int cg_size = cg->size;
    if (cg_size <= 0) continue;

    const float ru_act = ru->act;
    Unit** units = cg->units;
    Connection* cons = cg->cons; // for speed
    
    for (int i=0; i < cg_size; i++) {
      const float su_act = units[i]->act;
      Connection* cn = &(cons[i]);
      const float& cn_wt = cn->wt;
    // float err = (ru->act_p * su->act_p) - (rlay->sravg_nrm * cn->sravg);
      float err = (ru_act * su_act) - (0.02f * cn->sravg);
      //if(lmix.err_sb) {
        if(err > 0.0f) err *= (1.0f - cn_wt);
        else	     err *= cn_wt;
      //}
      cn->dwt += cur_lrate * err;
      cn->sravg = 0.0f;
    }
  }
}

void ConSpec::Compute_Weights(Unit* ru) {
  // CTLEABRA_CAL, DCAL
  for (int g = 0; g < ru->recv.size; g++) {
    RecvCons* cg = ru->recv.FastEl(g);
    const int cg_size = cg->size;
    if (cg_size <= 0) continue;
    
  //  const float ru_act = ru->act;
    Connection* cons = cg->cons; // for speed
    
    for (int i = 0; i < cg_size; ++i) {
      Connection* cn = &(cons[i]);
      float& cn_wt = cn->wt;
      
      // C_Compute_ActReg_CtLeabraCAL(cn, cg, ru, su, rus);
      float dwinc = ((g_rand^i) & 1) ? -0.2f : 0.2f; //0.0f; hack, just for calc cost parity
    /* if(ru->act_avg <= rus->act_reg.min)
        dwinc = rus->act_reg.inc_wt;
      else if(ru->act_avg >= rus->act_reg.max)
        dwinc = -rus->act_reg.dec_wt;*/
      if (dwinc != 0.0f) {
        cn->dwt += cur_lrate * dwinc * cn_wt; // proportional to current weights
      }
      
      // C_Compute_Weights_Norm_CtLeabraCAL(cn, dwnorm);
      wt_limits.ApplyMinLimit(cn_wt); 
      wt_limits.ApplyMaxLimit(cn_wt);
      cn->pdw = cn->dwt;
      cn->dwt = 0.0f;
    }
  }
}

void ConSpec::Compute_dWt_dnorm(Unit* ru) {
  for(int g = 0; g < ru->recv.size; g++) {
    RecvCons* cg = ru->recv.FastEl(g);
    const int cg_size = cg->size;
    if (cg_size <= 0) continue;

    const float ru_act = ru->act;
    Connection* cons = cg->cons; // for speed
    
    cg->dwt_mean = 0.0f;
    for (int i=0; i < cg_size; i++) {
      const float su_act = cg->Un(i)->act;
      Connection* cn = &(cons[i]);
      const float& cn_wt = cn->wt;
    // float err = (ru->act_p * su->act_p) - (rlay->sravg_nrm * cn->sravg);
      float err = (ru_act * su_act) - (0.02f * cn->sravg);
      //if(lmix.err_sb) {
        if(err > 0.0f) err *= (1.0f - cn_wt);
        else	     err *= cn_wt;
      //}
      cn->dwt += cur_lrate * err;
      cn->sravg = 0.0f;
      cg->dwt_mean += cn->dwt;
    }
    cg->dwt_mean /= (float)cg_size;
  }
}

void ConSpec::Compute_Weights_dnorm(Unit* ru) {
  // CTLEABRA_CAL, DCAL
  for (int g = 0; g < ru->recv.size; g++) {
    RecvCons* cg = ru->recv.FastEl(g);
    const int cg_size = cg->size;
    if (cg_size <= 0) continue;
    
  //  const float ru_act = ru->act;
    Connection* cons = cg->cons; // for speed
    
    float dwnorm = -norm_pct * cg->dwt_mean;
      
    for (int i = 0; i < cg_size; ++i) {
      Connection* cn = &(cons[i]);
      float& cn_wt = cn->wt;
      
      // C_Compute_ActReg_CtLeabraCAL(cn, cg, ru, su, rus);
      float dwinc = ((g_rand^i) & 1) ? -0.2f : 0.2f; //0.0f; hack, just for calc cost parity
    /* if(ru->act_avg <= rus->act_reg.min)
        dwinc = rus->act_reg.inc_wt;
      else if(ru->act_avg >= rus->act_reg.max)
        dwinc = -rus->act_reg.dec_wt;*/
      if (dwinc != 0.0f) {
        cn->dwt += cur_lrate * dwinc * cn_wt; // proportional to current weights
      }
      
      // C_Compute_Weights_Norm_CtLeabraCAL(cn, dwnorm);
      cn_wt += cn->dwt + dwnorm;	// weights always linear
      wt_limits.ApplyMinLimit(cn_wt); 
      wt_limits.ApplyMaxLimit(cn_wt);
      cn->pdw = cn->dwt;
      cn->dwt = 0.0f;
    }
  }
}


#elif (CON_IN == CON_SEND)

void ConSpec::Compute_SRAvg(Unit* su) {
  float su_act = su->act;
  for(int g = 0; g < su->send.size; g++) {
    SendCons* cg = su->send.FastEl(g);
    Unit** units = cg->units;
    Connection* cons = cg->cons;
    const int cg_size = cg->size;
    for (int i = 0; i < cg_size; ++i) {
      Connection* cn = &(cons[i]); 
      cn->sravg += su_act * units[i]->act;
    }
  }
}

void ConSpec::Compute_dWt(Unit* su) {
  for(int g = 0; g < su->send.size; g++) {
    SendCons* cg = su->send.FastEl(g);
    const int cg_size = cg->size;
    if (cg_size <= 0) continue;

    const float su_act = su->act;
    Unit** units = cg->units; // for speed
    Connection* cons = cg->cons; // for speed
    
    for (int i=0; i < cg_size; i++) {
      const float ru_act = units[i]->act;
      Connection* cn = &(cons[i]);
      const float& cn_wt = cn->wt;
    // float err = (ru->act_p * su->act_p) - (rlay->sravg_nrm * cn->sravg);
      float err = (ru_act * su_act) - (0.02f * cn->sravg);
      //if(lmix.err_sb) {
        if(err > 0.0f) err *= (1.0f - cn_wt);
        else	     err *= cn_wt;
      //}
      cn->dwt += cur_lrate * err;
      cn->sravg = 0.0f;
    }
  }
}

void ConSpec::Compute_Weights(Unit* ru) {
  // CTLEABRA_CAL, DCAL
  for (int g = 0; g < ru->recv.size; g++) {
    RecvCons* cg = ru->recv.FastEl(g);
    const int cg_size = cg->size;
    if (cg_size <= 0) continue;
    
    Connection** cons = cg->cons; // for speed
    
    for (int i = 0; i < cg_size; ++i) {
      Connection* cn = cons[i];
      float& cn_wt = cn->wt;
      
      // C_Compute_ActReg_CtLeabraCAL(cn, cg, ru, su, rus);
      float dwinc = ((g_rand^i) & 1) ? -0.2f : 0.2f; //0.0f; hack, just for calc cost parity
    /* if(ru->act_avg <= rus->act_reg.min)
        dwinc = rus->act_reg.inc_wt;
      else if(ru->act_avg >= rus->act_reg.max)
        dwinc = -rus->act_reg.dec_wt;*/
      if (dwinc != 0.0f) {
        cn->dwt += cur_lrate * dwinc * cn_wt; // proportional to current weights
      }
      
      // C_Compute_Weights_Norm_CtLeabraCAL(cn, dwnorm);
      wt_limits.ApplyMinLimit(cn_wt); 
      wt_limits.ApplyMaxLimit(cn_wt);
      cn->pdw = cn->dwt;
      cn->dwt = 0.0f;
    }
  }
}

void ConSpec::Compute_dWt_dnorm(Unit* ru) {
  for(int g = 0; g < ru->recv.size; g++) {
    RecvCons* cg = ru->recv.FastEl(g);
    const int cg_size = cg->size;
    if (cg_size <= 0) continue;

    const float ru_act = ru->act;
    Connection** cons = cg->cons; // for speed
    
    cg->dwt_mean = 0.0f;
    for (int i=0; i < cg_size; i++) {
      const float su_act = cg->Un(i)->act;
      Connection* cn = cons[i];
      const float& cn_wt = cn->wt;
    // float err = (ru->act_p * su->act_p) - (rlay->sravg_nrm * cn->sravg);
      float err = (ru_act * su_act) - (0.02f * cn->sravg);
      //if(lmix.err_sb) {
        if(err > 0.0f) err *= (1.0f - cn_wt);
        else	     err *= cn_wt;
      //}
      cn->dwt += cur_lrate * err;
      cn->sravg = 0.0f;
      cg->dwt_mean += cn->dwt;
    }
    cg->dwt_mean /= (float)cg_size;
  }
}

void ConSpec::Compute_Weights_dnorm(Unit* ru) {
  // CTLEABRA_CAL, DCAL
  for (int g = 0; g < ru->recv.size; g++) {
    RecvCons* cg = ru->recv.FastEl(g);
    const int cg_size = cg->size;
    if (cg_size <= 0) continue;
    
    Connection** cons = cg->cons; // for speed
    
    float dwnorm = -norm_pct * cg->dwt_mean;
      
    for (int i = 0; i < cg_size; ++i) {
      Connection* cn = cons[i];
      float& cn_wt = cn->wt;
      
      // C_Compute_ActReg_CtLeabraCAL(cn, cg, ru, su, rus);
      float dwinc = ((g_rand^i) & 1) ? -0.2f : 0.2f; //0.0f; hack, just for calc cost parity
    /* if(ru->act_avg <= rus->act_reg.min)
        dwinc = rus->act_reg.inc_wt;
      else if(ru->act_avg >= rus->act_reg.max)
        dwinc = -rus->act_reg.dec_wt;*/
      if (dwinc != 0.0f) {
        cn->dwt += cur_lrate * dwinc * cn_wt; // proportional to current weights
      }
      
      // C_Compute_Weights_Norm_CtLeabraCAL(cn, dwnorm);
      cn_wt += cn->dwt + dwnorm;	// weights always linear
      wt_limits.ApplyMinLimit(cn_wt); 
      wt_limits.ApplyMaxLimit(cn_wt);
      cn->pdw = cn->dwt;
      cn->dwt = 0.0f;
    }
  }
}

#endif // modal for CON_IN


//////////////////////////
//  UnitSpec		//
//////////////////////////

UnitSpec::UnitSpec() {
//  act_reg = 0.0f;
}

//////////////////////////
//  Unit		//
//////////////////////////

Unit::Unit()
{
  act = 0.4f; 
  net = 0;
  uni = -1; // set during build
  act_avg = 0.0f;
  cs = NULL; 
  spec = NULL;
  n_recv_cons = 0;
  my_rand = rand();
  do_delta = true; // for first iteration, before it is set in ComputeActs
  n_con_calc = 0;
}

Unit::~Unit() {
}

void Unit::CalcDelta() {
  do_delta = ((Nb::this_rand ^ my_rand) & 0xffff) < Nb::send_act;
} 


//////////////////////////
//  Layer		//
//////////////////////////

Layer::Layer() {
  net = NULL;
  un_to_idx = 0;
}

Layer::~Layer() {
}

void Layer::ConnectFrom(Layer* lay_fm) {
  Nb::n_prjns++;
  const int n_send = lay_fm->units.size;
  if ((n_send == 0) || (units.size <= 0)) return; // shouldn't happen
  // get a unit to determine send and recv gp
  Unit* un_fm = lay_fm->units.FastEl(0);
  const int send_idx = un_fm->send.size; // index of new send gp...
  Unit* un_to = units.FastEl(0);
  const int recv_idx = un_to->recv.size; // index of new recv gp...

  // do ConGps first
  for (int i_fm = 0; i_fm < lay_fm->units.size; ++i_fm) {
    un_fm = lay_fm->units.FastEl(i_fm);
    SendCons* send_gp = un_fm->send.New();
    send_gp->setSize(n_send);
    send_gp->recv_idx = recv_idx;
    send_gp->recv_lay = this;
  }
  for (int i_to = 0; i_to < units.size; ++i_to) {
    un_to = units.FastEl(i_to);
    RecvCons* recv_gp = un_to->recv.New();
    recv_gp->setSize(n_send);
    recv_gp->send_idx = send_idx;
    recv_gp->send_lay = lay_fm;
  }

  // connect, on a receiver-basis
  for (int i_to = 0; i_to < units.size; ++i_to) {
    un_to = units.FastEl(i_to);
    RecvCons* recv_gp = un_to->recv.FastEl(recv_idx);
    
    for (int i_fm = 0; i_fm < lay_fm->units.size; ++i_fm) {
      un_fm = lay_fm->units.FastEl(i_fm);
      
      SendCons* send_gp = un_fm->send.FastEl(send_idx);
      
      // wt references and access
      float wt = (4.0 * (float)rand() / RAND_MAX) - 2.0;
//      Connection* cn = 
      ConsBase::InitCon(send_gp, recv_gp,
        un_fm, un_to, i_fm, i_to, wt);
    }
  }
}


//////////////////////////
//  Network		//
//////////////////////////

bool 	Network::recv_based; 
char 	Network::use_dnorm; 
int 	Network::dwt_rate = Network::trial_rate;
int 	Network::sra_start = 30;
int 	Network::sra_rate = 5;

Network::Network() {
  engine = NULL;
  cycle = 0;
  trial = 0;
  tot_cycle = 0;
  n_units_flat = 0;
  g_units = NULL;
  next_wti = 0;
}

Network::~Network() {
  SetEngine(NULL);
  n_units_flat = 0;
  g_units = NULL;
}

void Network::Initialize() {
  if (engine) engine->Initialize();
}

void Network::Build() {
  n_units_flat = Nb::n_layers * Nb::n_units;
  const int n_prjns = (Nb::n_layers - 1) * 2;
  //note: we validated cons < 2^31 in startup
  const int n_cons_flat = n_prjns * Nb::n_units * Nb::n_cons;
  
  layers.Alloc(Nb::n_layers);
  // global allocs and ptrs
  units_flat.Alloc(n_units_flat);
  g_units = units_flat.Els();
  wts_flat.SetSize(n_cons_flat);
  next_wti = 0;
  
  
  int next_uni = 0;
  ConSpec* cs = new ConSpec; // everyone shares
  UnitSpec* uspec = new UnitSpec; // everyone shares
  // make all layers and units first
  layers.Alloc(Nb::n_layers);
  for (int l = 0; l < Nb::n_layers; l++) {
    Layer* lay = layers.New();
    lay->net = this;
    lay->units.Alloc(Nb::n_units);
    
    for (int i=0; i < Nb::n_units; ++i) {
      Unit* un = lay->units.New();
      un->uni = next_uni++; 
      units_flat.Add(un);
      un->cs = cs;
      un->spec = uspec;
    } 
  }
   
  // then connect all bidirectionally 
  for (int lay_ths = 0; lay_ths < (Nb::n_layers); lay_ths++) {
    int lay_nxt = lay_ths + 1; 
    //if (lay_nxt >= Nb::n_layers) lay_nxt = 0;
    int lay_prv = lay_ths - 1; 
    //if (lay_prv < 0) lay_prv = Nb::n_layers - 1;
    if (lay_prv >= 0) 
      layers[lay_ths]->ConnectFrom(layers[lay_prv]);
    if (lay_nxt < Nb::n_layers) 
      layers[lay_nxt]->ConnectFrom(layers[lay_ths]);
    if (Nb::n_layers == 2) break; // special case
  }
  
  engine->OnBuild();
}

float* Network::AllocWts(int sz) {
  if ((next_wti + sz) > wts_flat.alloc_size)
    return NULL;
  float* rval = &(wts_flat[next_wti]);
  next_wti += sz;
  return rval;
}

void Network::ComputeNets() {
  engine->ComputeNets();
}

float Network::ComputeActs() {
  float rval = engine->ComputeActs();
  Nb::this_rand = rand(); // for next cycle
  return rval;
}

void Network::Compute_dWt() {
  engine->Compute_dWt();
}

void Network::Compute_Weights() {
  engine->Compute_Weights();
}

#ifdef SRAVG_CONCURR
void Network::Cycle(int n_cycles) {
  // this is the key computation loop
  int sra_sync_in = 0;
  for (int i = 0; i < n_cycles; ++i) {
    g_rand = rand();
    ComputeNets();
    if (Nb::calc_act) {
      float tot_act = ComputeActs();
      if (Nb::use_log_file)
        fprintf((FILE*)Nb::inst->act_logfile,"%d\t%g\n", tot_cycle, tot_act);
    }
    // see if we should sync SRAvg from previous cycle
    if (sra_sync_in) {
      if (--sra_sync_in == 0)
        engine->Compute_SRAvg_sync();
    }
    // note: we don't kick off SRAvg on last cycle (wt update)
    // note: add 1 to cycle so we do it after dwt_rate full counts
    int nc1 = cycle + 1;
    if ((dwt_rate > 0) && ((nc1 % dwt_rate) == 0)) {
      Compute_dWt();
      Compute_Weights();
    }
    else if ((sra_rate > 0) && (cycle > (sra_start - sra_rate))){
       if ((cycle % sra_rate) == 0) {
         engine->Compute_SRAvg_start();
         sra_sync_in = sra_rate;
       }
    }
    ++tot_cycle;
    if (++cycle >= trial_rate) {
      ++trial;
      cycle = 0;
    }
    
  }
}
#else   
void Network::Cycle(int n_cycles) {
  // this is the key computation loop
  for (int i = 0; i < n_cycles; ++i) {
    g_rand = rand();
    ComputeNets();
    if (Nb::calc_act) {
      float tot_act = ComputeActs();
      if (Nb::use_log_file)
        fprintf((FILE*)Nb::inst->act_logfile,"%d\t%g\n", tot_cycle, tot_act);
    }
    if ((sra_rate > 0) && (cycle >= sra_start) && ((cycle % sra_rate) == 0)) 
      Compute_SRAvg();
    // note: add 1 to cycle so we do it after dwt_rate full counts
    int nc1 = cycle + 1;
    if ((dwt_rate > 0) && ((nc1 % dwt_rate) == 0)) {
      Compute_dWt();
      Compute_Weights();
    }
    ++tot_cycle;
    if (++cycle >= trial_rate) {
      ++trial;
      cycle = 0;
    }
    
  }
}

void Network::Compute_SRAvg() {
  engine->Compute_SRAvg();
}
#endif    

double Network::GetNTot() {
  double rval = 0;
  for (int i = 0; i < n_units_flat; ++i) {
    Unit* un = g_units[i];
    rval += un->n_con_calc;
  }
  return rval;
}

void Network::SetEngine(NetEngine* engine_) {
  if (engine == engine_) return;
  if (engine) {
    delete engine; // removes ref
  }
  engine = engine_;
  if (engine) {
    engine->net = this;
  }
}



/*void Network::PartitionUnits_Send() {
}*/

//////////////////////////
//  NetEngine		//
//////////////////////////

int NetEngine::algo = 0;
//const int NetEngine::core_max_nprocs; // maximum number of processors!
int NetEngine::n_procs;		// total number of processors
//int NetEngine::n_stride;
NetTaskList NetEngine::net_tasks; // only n_procs created

NetEngine::~NetEngine() {
  if (net) {
    net->engine = NULL;
  }
}

void NetEngine::Initialize() {
  Initialize_impl();
}

void NetEngine::Initialize_impl() {
  NetTask* tsk = new NetTask_0(this->net);
  tsk->task_id = 0;
  net_tasks.append(tsk);
}

void NetEngine::DoProc(int proc_id) {
  NetTask* tsk = net_tasks[0];
  //if (!tsk) return;
  tsk->g_u = 0;
  tsk->proc_id = proc_id;
  tsk->run();
}

void NetEngine::ComputeNets() {
  Nb::n_tot = 0;
  ComputeNets_impl();
//#ifdef DEBUG -- ALWAYS do this! too easy for this to fail!
  if (!(algo & SEND_FLAG) && (Nb::n_tot != net->n_units_flat)) {
    cerr << "ERROR: NetEngine::ComputeNets: n_tot != n_units, was: "
      << Nb::n_tot << "\n";
  }
//#endif
}

void NetEngine::ComputeNets_impl() {
  switch (algo) {
  case RECV:
//  case RECV_SMART:
    DoProc(NetTask::P_Recv_Netin);
    break;
  case SEND_CLASH:
  case SEND_ARRAY: // ignore for single, trapped for N
    DoProc(NetTask::P_Send_Netin_Clash);
    break;
  default: break;
  }
}

float NetEngine::ComputeActs() {
  // compute activations (only order number of units)
  Nb::tot_act = 0.0f;
  DoProc(NetTask::P_ComputeAct);
  for (int t = 0; t < net_tasks.count(); ++t) {
    NetTask* tsk = net_tasks[t];
    Nb::tot_act += tsk->my_act;
  }
  return Nb::tot_act;
}

#ifdef SRAVG_CONCURR
void NetEngine::Compute_SRAvg_start() {
}
void NetEngine::Compute_SRAvg_sync() {
  DoProc(NetTask::P_ComputeSRAvg);
}
#else
void NetEngine::Compute_SRAvg() {
  DoProc(NetTask::P_ComputeSRAvg);
}
#endif

void NetEngine::Compute_dWt() {
  DoProc(NetTask::P_ComputedWt);
}

void NetEngine::Compute_Weights() {
  DoProc(NetTask::P_ComputeWeights);
}

void NetEngine::Log(bool hdr) {
  // note: always make/extend the thread log
  if (true/*use_log_file*/) {
  FILE* logfile = NULL;
    if (hdr) {
      logfile = fopen("ThreadNetEngine.log", "w");
      fprintf(logfile,"thread\tt_tot\tn_run\tstart lat\trun time\tnibble time\tsync time\toverhead\n");
    } else {
      logfile = fopen("ThreadNetEngine.log", "a");
    }
    for (int t = 0; t < net_tasks.count(); t++) {
      NetTask* tsk = net_tasks[t];
      fprintf(logfile,"%d\t%d\t%d\t%g\t%g\t%g\t%g\t%g\n", 
        t, 
        tsk->t_tot,
        tsk->n_run, 
        tsk->start_latency.s_used, 
        tsk->run_time.s_used,
        tsk->nibble_time.s_used,
        tsk->sync_time.s_used,
        tsk->overhead.s_used
      );
    }
    fclose(logfile);
  }
}
//////////////////////////////////
//  ThreadNetEngine		//
//////////////////////////////////

#ifdef SRAVG_CONCURR
QTaskThread* ThreadNetEngine::sra_thread;
SRAvg_Task* ThreadNetEngine::sra_task;
#endif

#ifdef USE_QT_CONCURRENT
void net_task_run(NetTask*& nt) {
  nt->run();
}
#else
QThread* ThreadNetEngine::threads[core_max_nprocs]; // only n_procs-1 created, none for [0] (main thread)

void ThreadNetEngine::DeleteThreads() {
  for (int t = n_procs - 1; t >= 1; t--) {
    QTaskThread* th = (QTaskThread*)threads[t];
    if (th->isActive()) {
      th->terminate();
    }
    while (!th->isFinished());
    delete th;
  }
}
#endif


ThreadNetEngine::~ThreadNetEngine() {
#ifdef USE_QT_CONCURRENT
#else
  DeleteThreads();
#endif
}

void ThreadNetEngine::Initialize_impl() {
#ifdef USE_QT_CONCURRENT
  for (int i = 0; i < n_procs; i++) {
    NetTask* tsk = new NetTask_N(this->net);
    tsk->setAutoDelete(false);
    tsk->task_id = i;
    net_tasks.append(tsk);
  }
#else
  for (int i = 0; i < n_procs; i++) {
    NetTask* tsk = new NetTask_N(this->net);
    tsk->task_id = i;
    net_tasks.append(tsk);
    if (i == 0) continue;
    QTaskThread* th = new QTaskThread;
    threads[i] = th;
    th->setTask(tsk);
    th->start(); // starts paused
  }
#endif
#ifdef SRAVG_CONCURR
  sra_task = new SRAvg_Task(this->net);
  sra_thread = new QTaskThread;
  sra_thread->setTask(sra_task);
//  sra_thread->start(QThread::LowPriority);
  sra_thread->start(QThread::NormalPriority);
#endif
}

void ThreadNetEngine::ComputeNets_impl() {
  if (algo == SEND_ARRAY)
    ComputeNets_SendArray();
  else inherited::ComputeNets_impl();
}

#ifdef SRAVG_CONCURR
void ThreadNetEngine::Compute_SRAvg_start() {
  sra_thread->resume();
}

void ThreadNetEngine::Compute_SRAvg_sync() {
  sra_thread->suspend();
}
#endif


void ThreadNetEngine::DoProc(int proc_id) {
  // start all the other threads first...
  NetTask::g_u = 0;
  
#ifdef USE_QT_CONCURRENT
  for (int t = 0; t < n_procs; ++t) {
    NetTask* tsk = net_tasks[t];
    tsk->proc_id = proc_id;
  }
  NetTask* tsk = net_tasks[0];
  tsk->run_time.Start(false);
  QFuture<void> fv = QtConcurrent::map(net_tasks, net_task_run);
  fv.waitForFinished();
  tsk->run_time.Stop(); 
#else // legacy threads
  for (int t = 1; t < n_procs; ++t) {
    QTaskThread* th = (QTaskThread*)threads[t];
    NetTask* tsk = net_tasks[t];
    tsk->proc_id = proc_id;
    th->resume();
  }
  
  // then do my part
  NetTask* tsk = net_tasks[0];
  tsk->proc_id = proc_id;
  tsk->run_time.Start(false);
  tsk->run();
  tsk->run_time.Stop(); 
  //NOTE: "nibbling" is n/a in this block-based algo
  // we should sync, since threads could still be working
  // on the last bits of their chunks
  for (int t = 1; t < n_procs; ++t) {
    tsk->sync_time.Start(false);
    QTaskThread* th = (QTaskThread*)threads[t];
    th->suspend(); // suspending is syncing with completion of loop
    tsk->sync_time.Stop(); 
  }
#endif
}

void ThreadNetEngine::OnBuild() {
  // we just partition them round-robin
  const int n_units_flat = net->n_units_flat;
/*nn  for (int i = 0; i < n_units_flat; ++i) {
    Unit* un = net->g_units[i];
    un->task_id = i % n_procs;
  }*/
  // Send_Array only
  if (algo == SEND_ARRAY) {
    //note: we alloc to all, but the ASYM version doesn't use nt0.excit
    for (int t = 0; t < net_tasks.count(); t++) {
      NetTask_N* tsk = dynamic_cast<NetTask_N*>(net_tasks[t]);
      tsk->excit = new float[n_units_flat]; // no need to init
    }
  }
}

#ifdef SEND_ARY_ASYM
void ThreadNetEngine::ComputeNets_SendArray() {
  DoProc(NetTask::P_Send_Netin_Array);
  const int tsz = net_tasks.size - 1; // we only do Tasks1-N
  if (tsz <= 0) return; // there were no additional buffs to accum
  // post stuff
//  RollupScratch_Netin();
  // called in Thread 0, so allocate the Task0
  NetTask* nt0 = net_tasks[0];
  nt0->overhead.Start(false);
  const int n_units_flat = net->n_units_flat;
  Unit** g_units = net->g_units; // cache -- this is also task0's guys
  // 2-thread version is optimized
  if (tsz == 1) {
    float* nets = dynamic_cast<NetTask_N*>(net_tasks[1])->excit;  
    for (int i = 0; i < n_units_flat; ++i) {
      g_units[i]->net += nets[i]; // since Task0 was also accumulating
    }
  } else { // tsz > 1, need to rollup
    // make local array pointers, for speed
    float** nets = new float*[tsz];  
    for (int t = 1; t < net_tasks.size; t++) {
      NetTask_N* tsk = dynamic_cast<NetTask_N*>(net_tasks[t]);
      nets[t-1] = tsk->excit; // cache
    }
    
    for (int i = 0; i < n_units_flat; ++i) {
      //TODO: this can be optimized using SSE or equiv compiler commands
      // to rollup 4 floats at once in parallel
      float tnet = 0.0f;
      for (int t = 0; t < tsz; ++t) {
        tnet += nets[t][i];
      }
      g_units[i]->net += tnet; // since Task0 was also accumulating
    }
    delete[] nets;
  }
  nt0->overhead.Stop();
}
#else // SYM version
void ThreadNetEngine::ComputeNets_SendArray() {
  DoProc(NetTask::P_Send_Netin_Array);

  // post stuff
//  RollupScratch_Netin();
  // called in Thread 0, so allocate the Task0
  NetTask_N* nt0 = dynamic_cast<NetTask_N*>(net_tasks[0]);
  nt0->overhead.Start(false);
  const int n_units_flat = net->n_units_flat;
  Unit** g_units = net->g_units; // cache -- this is also task0's guys
  // 1-thread version is optimized, for fair comparison
  const int tsz = net_tasks.count();
  if (tsz == 1) {
    float* nets = nt0->excit;  
    for (int i = 0; i < n_units_flat; ++i) {
      g_units[i]->net = nets[i];
    }
  } else { //  need to rollup
    // make local array pointers, for speed
    float** nets = new float*[tsz];
    for (int t = 0; t < tsz; t++) {
      NetTask_N* tsk = dynamic_cast<NetTask_N*>(net_tasks[t]);
      nets[t] = tsk->excit; // cache
    }
    
    for (int i = 0; i < n_units_flat; ++i) {
      //TODO: this can be optimized using SSE or equiv compiler commands
      // to rollup 4 floats at once in parallel
      float tnet = 0.0f;
      for (int t = 0; t < tsz; ++t) {
        tnet += nets[t][i];
      }
      g_units[i]->net = tnet;
    }
    delete[] nets;
  }
  nt0->overhead.Stop();
}
#endif

#ifdef SRAVG_CONCURR
//////////////////////////////////
//  SRAvg_Task			//
//////////////////////////////////

int SRAvg_Task::g_u;

SRAvg_Task::SRAvg_Task(Network* net_) {
  net = net_;
}

void SRAvg_Task::run() {
  Unit** units = net->g_units; // cache
  const int n_units_flat =  net->n_units_flat;
//note: we only run in one thread
  for (int my_u = 0; my_u < n_units_flat; ++my_u) {
    Unit* un = units[my_u]; //note: accessed flat
    un->cs->Compute_SRAvg(un);
  }
/*  PROC_VAR_LOOP(my_u, g_u, n_units_flat) {
    Unit* un = units[my_u]; //note: accessed flat
//note: CON_IN will automatically compile the correct s or r based routine
    un->cs->Compute_SRAvg(un);
  }*/
}
#endif


//////////////////////////////////
//  NetTask -- mixed classes	//
//////////////////////////////////

int NetTask::g_u;
NetTask::NetTask(Network* net_) {
  net = net_;
  t_tot = 0;
  my_act = 0.0f;
  n_run = 0;
}

void NetTask::run() {
  ++n_run;
  switch (proc_id) {
  case P_Recv_Netin: Recv_Netin(); break;
  case P_Send_Netin_Clash: Send_Netin_Clash(); break;
  case P_Send_Netin_Array: Send_Netin_Array(); break;
  case P_ComputeAct: ComputeAct(); break;
  case P_ComputeSRAvg: Compute_SRAvg(); break;
  case P_ComputedWt: Compute_dWt(); break;
  case P_ComputeWeights: Compute_Weights(); break;
  }
}

void NetTask::Send_Netin_0(Unit* su) {
  const float su_act_eff = su->act;
  for (int j = 0; j < su->send.size; ++j) {
    SendCons* send_gp = su->send[j];
    const int send_sz = send_gp->size;
    su->n_con_calc += send_sz;
#if (CON_IN == CON_RECV)
    Unit** units = send_gp->units; // unit pointer
#   ifdef USE_V_CON
    for (int i=0; i < send_sz; i++) {
      Send_Netin_inner_0(send_gp->V_Cn(i)->wt, units[i]->net, su_act_eff);
    }
#   else
    Connection** cons = send_gp->cons; 
    for (int i=0; i < send_sz; i++) {
      //const int targ_i = unis[i];
      Send_Netin_inner_0(cons[i]->wt, units[i]->net, su_act_eff);
    }
#   endif
#elif (CON_IN == CON_SEND)
# ifdef UN_IN_CON
    //Connection* cons = send_gp->cons; 
    for (int i=0; i < send_gp->size /*send_sz*/; i++) {
      Connection* cn = &(send_gp->cons[i]);
      Send_Netin_inner_0(cn->wt, cn->un->net, su_act_eff);
    }
# else
    Unit** units = send_gp->units; // unit pointer
#   ifdef USE_V_CON
    for (int i=0; i < send_sz; i++) {
      Send_Netin_inner_0(send_gp->V_Cn(i)->wt, units[i]->net, su_act_eff);
    }
#   else
    Connection* cons = send_gp->cons; 
    for (int i=0; i < send_sz; i++) {
      Send_Netin_inner_0(cons[i].wt, units[i]->net, su_act_eff);
    }
#   endif
# endif
#endif
  }
}

void NetTask::Recv_Netin_0(Unit* ru) {
  float ru_net = 0.0f;
//  float* g_acts = Nb::net.g_acts;
  for (int j = 0; j < ru->recv.size; ++j) {
    RecvCons* recv_gp = ru->recv[j];
    const int recv_sz = recv_gp->size;
    ru->n_con_calc += recv_sz;
    
#if (CON_IN == CON_RECV)
# ifdef UN_IN_CON
    Connection* cons = recv_gp->cons; 
    for (int i=0; i < recv_sz; ++i) {
      Connection* cn = &(cons[i]);
      ru_net += cn->un->act * cn->wt;
    }
# else
    Unit** units = recv_gp->units; // unit pointer
#   ifdef USE_V_CON
    for(int i=0; i < recv_sz; ++i)
      ru_net += units[i]->act * recv_gp->V_Cn(i)->wt;
#   else
    Connection* cons = recv_gp->cons; 
    for(int i=0; i < recv_sz; ++i)
      ru_net += units[i]->act * cons[i].wt;
#   endif
#endif
#elif (CON_IN == CON_SEND)
    Unit** units = recv_gp->units; // unit pointer
#   ifdef USE_V_CON
    for(int i=0; i < recv_sz; ++i)
      ru_net += units[i]->act * recv_gp->V_Cn(i)->wt;
#   else
    Connection** cons = recv_gp->cons; // cache
    for(int i=0; i < recv_sz; ++i)
      ru_net += units[i]->act * cons[i]->wt;
#   endif
#endif
  }
  ru->net = ru_net;
}

void NetTask::Recv_Netin() {
  Unit** units = Nb::net.g_units; // cache
  const int n_units_flat =  Nb::net.n_units_flat;
  while (g_u < n_units_flat) {
    Unit* un = units[g_u]; //note: accessed flat
    Recv_Netin_0(un);
    AtomicFetchAdd(&Nb::n_tot, 1); // note: we use this because we have to measure it regardless, don't penalize
    ++g_u;
  }
}

float NetTask::ComputeAct_inner(Unit* un) {
  float& un_net = un->net;
  float& un_act = un->net;
  un_act = 1.0f / (1.0f + expf(-un_net));
  un_net = 0.0f; // only needed for sender-based, but cheaper to just do than test
  //NOTE: following is not used for vanilla RECV, but we include it
  // as shared overhead anyway
  un->CalcDelta(); // sets flag, except for RECV
  return un_act;
}

void NetTask::ComputeAct() {
  // compute activations (only order number of units)
  my_act = 0.0f;
  const int n_units_flat =  net->n_units_flat;
  Unit** g_units = net->g_units;
  while (g_u < n_units_flat) {
    Unit* un = g_units[g_u]; //note: accessed flat
    my_act += ComputeAct_inner(un);
    ++g_u;
  }
}

void NetTask::Compute_SRAvg() {
  Unit** units = net->g_units; // cache
  const int n_units_flat =  net->n_units_flat;
  PROC_VAR_LOOP(my_u, g_u, n_units_flat) {
    Unit* un = units[my_u]; //note: accessed flat
//note: CON_IN will automatically compile the correct s or r based routine
    un->cs->Compute_SRAvg(un);
  }
}

void NetTask::Compute_dWt() {
  Unit** g_units = net->g_units; // cache
  const int n_units_flat =  net->n_units_flat;
  if (Network::use_dnorm) {
    PROC_VAR_LOOP(my_u, g_u, n_units_flat) {
      Unit* un = g_units[my_u]; //note: accessed flat
      un->cs->Compute_dWt_dnorm(un);
    }
  } else {
    PROC_VAR_LOOP(my_u, g_u, n_units_flat) {
      Unit* un = g_units[my_u]; //note: accessed flat
      un->cs->Compute_dWt(un);
    }
  }
}

void NetTask::Compute_Weights() {
  Unit** g_units = net->g_units; // cache
  const int n_units_flat =  net->n_units_flat;
  if (Network::use_dnorm) {
    PROC_VAR_LOOP(my_u, g_u, n_units_flat) {
      Unit* un = g_units[my_u]; //note: accessed flat
      un->cs->Compute_Weights_dnorm(un);
    }
  } else {
    PROC_VAR_LOOP(my_u, g_u, n_units_flat) {
      Unit* un = g_units[my_u]; //note: accessed flat
      un->cs->Compute_Weights(un);
    }
  }
}


//////////////////////////////////
//  NetTask_0			//
//////////////////////////////////

void NetTask_0::Send_Netin_Clash() {
  Unit** units = Nb::net.g_units; // cache
  const int n_units_flat =  Nb::net.n_units_flat;
  while (g_u < n_units_flat) {
    Unit* un = units[g_u]; //note: accessed flat
    if (un->do_delta) {
      Send_Netin_0(un);
      AtomicFetchAdd(&Nb::n_tot, 1); // note: we use this because we have to measure it regardless, don't penalize
    }
    ++g_u;
  }
}


//////////////////////////////////
//  NetTask_N			//
//////////////////////////////////

NetTask_N::~NetTask_N() {
  if (excit) {
    delete[] excit;
    excit = NULL;
  }
}

void NetTask_N::Send_Netin_Clash() {
  Unit** units = net->g_units; // cache
  const int n_units_flat = net->n_units_flat;
  PROC_VAR_LOOP(my_u, g_u, n_units_flat) {
    Unit* un = units[my_u]; //note: accessed flat
    if (un->do_delta) {
      Send_Netin_0(un);
      AtomicFetchAdd(&Nb::n_tot, 1);
    }
  }
}

void NetTask_N::Send_Netin_Array() {
#ifdef SEND_ARY_ASYM
  if (task_id == 0) {
    Send_Netin_Clash();
    return;//
  }
#endif
  overhead.Start(false);
  memset(excit, 0, sizeof(float) * net->n_units_flat);
  overhead.Stop();

  Unit** units = Nb::net.g_units; // cache
  const int n_units_flat = Nb::net.n_units_flat;
  PROC_VAR_LOOP(my_u, g_u, n_units_flat) {
    Unit* su = units[my_u]; //note: accessed flat
    if (su->do_delta) {
      float su_act_eff = su->act;
      for (int j = 0; j < su->send.size; ++j) {
        SendCons* send_gp = su->send[j];
        const int send_sz = send_gp->size;
        su->n_con_calc += send_sz;
#if (CON_IN == CON_RECV)
        Unit** units = send_gp->units; // unit pointer
#   ifdef USE_V_CON
	for (int i=0; i < send_sz; i++) {
	Send_Netin_inner_0(send_gp->V_Cn(i)->wt, excit[units[i]->uni], su_act_eff);
	}
#   else
        Connection** cons = send_gp->cons; 
        for (int i=0; i < send_sz; i++) {
          //const int targ_i = uns[i];
          Send_Netin_inner_0(cons[i]->wt, excit[units[i]->uni], su_act_eff);
        }
#   endif
#elif (CON_IN == CON_SEND)
# ifdef UN_IN_CON
        Connection* cons = send_gp->cons; 
        for (int i=0; i < send_sz; i++) {
          Connection* cn = &(cons[i]);
          Send_Netin_inner_0(cn->wt, excit[cn->un->uni], su_act_eff);
        }
# else
        Unit** units = send_gp->units; // unit pointer
#   ifdef USE_V_CON
	for (int i=0; i < send_sz; i++) {
	Send_Netin_inner_0(send_gp->V_Cn(i)->wt, excit[units[i]->uni], su_act_eff);
	}
#   else
        Connection* cons = send_gp->cons; 
        for (int i=0; i < send_sz; i++) {
          //const int targ_i = uns[i];
          Send_Netin_inner_0(cons[i].wt, excit[units[i]->uni], su_act_eff);
        }
#   endif
# endif
#endif
      }
      AtomicFetchAdd(&Nb::n_tot, 1);
    }
  }
}

void NetTask_N::Recv_Netin() {
  Unit** g_units = Nb::net.g_units; // cache
  PROC_VAR_LOOP(my_u, g_u, Nb::net.n_units_flat) {
    Unit* un = g_units[my_u]; //note: accessed flat
    Recv_Netin_0(un);
    AtomicFetchAdd(&Nb::n_tot, 1); // note: we use this because we have to measure it regardless, don't penalize
  }
}

void NetTask_N::ComputeAct() {
  Unit** g_units = net->g_units;
  const int n_units_flat = Nb::net.n_units_flat;
  my_act = 0.0f;
  PROC_VAR_LOOP(my_u, g_u, n_units_flat) {
    Unit* un = g_units[my_u]; //note: accessed flat
    my_act += ComputeAct_inner(un);
  }
}


Nb* Nb::inst;
bool Nb::hdr = false;

int Nb::n_layers;
int Nb::n_units;			// number of units per layer
int Nb::n_cons; // number of cons per unit
int Nb::n_cycles;			// number of cycles of updating
Network Nb::net;		// global network 
int Nb::n_tot;
int Nb::n_prjns;
float Nb::tot_act; // check on tot act

bool Nb::single = false; // true for single thread mode, to compare against nprocs=1
int Nb::send_act = 0x10000; // send activation, as a fraction of 2^16 
int Nb::tsend_act; 
int Nb::inv_act = 1; // inverse of activation -- can use to divide
int Nb::this_rand; // assigned a new random value each cycle, to let us randomize unit acts
bool Nb::use_log_file = false;
bool Nb::calc_act = true;

int Nb::main() {
  Initialize();
  
  net.Initialize();
  net.Build();

  act_logfile = NULL;
  if (use_log_file) {
    act_logfile = fopen("ptest_core.log", "w");
    fprintf((FILE*)act_logfile,"cyc\ttot_act\n");
  }

  time_used.Start();
  
  net.Cycle(n_cycles);

  time_used.Stop();
  if (use_log_file) {
    fclose((FILE*)act_logfile);
    act_logfile = NULL;
  }

  // note: always make/extend the thread log
  net.engine->Log(hdr);

  PrintResults();

  net.SetEngine(NULL); // controlled delete
  return 0;
}

int Nb::PreInitialize() {
  int rval = 0;
  prompt = 
    "must have min 3 args (you can use \"0\" for def of positionals):\n"
    "\t<n_units>\tnumber of units in each of the layers\n"
    "\t<n_cycles>\tnumber of cycles\n"
    "\t<n_procs>\tnumber of cores or procs (0=fast single-threaded model)\n"
    "optional positional params -- none can be skipped: \n"
    "\t<n_lays>\tnumber of layers (min 2, def=5, max 128)\n"
    "\t<n_cons>\tIGNORED number of cons per unit-projection (def=n_units)\n"
    "\t<send_act>\tpercent avg activation level (1-100, def = 5)\n"
    "optional commands: \n"
    "\t-header\t output a header line\n"
    "\t-log=1/0(def)\tlog/do-not-log optional values to ptest_core.log\n"
    "\t-nibble=n\tnibble: 0(def)=none, 1=on, 2=auto (> .20 left)\n"
    "\t-act=0\tdo not calculate activation\n"
    "\t-sra_rate=5\tcalc sravg every n cycles (after 30), 0=never\n"
    "\t-dwt_rate=71\tcalc dwt every n cycles, 0=never\n"
    "\t-suff=xxx\tlog file suffix\n"
    "-algo=n (def=0) is one of the following:\n"
    "\t 0 receiver-based\n"
    "\t 1 receiver-based, with smart calc (ignore senders below thresh)\n"
    "\t 2 sender-based -- clashes allowed \n"
    "\t 3 sender-based -- array-based, 1 array of nets per thread, then rolled up \n"
    "\t-fast_prjn=0(def)/1\tuse fast prjn mode to directly access target units\n"
  ;
  PreInitialize_impl(rval);
  if (rval != 0) return rval;
  if (argc < 4) {
    printf(prompt.toLatin1());
    return 1;
  }
  ParseCmdLine(rval);
  return rval;
}

void Nb::PreInitialize_impl(int& /*rval*/) {
}

NetEngine* Nb::CreateNetEngine() {
  NetEngine* rval = NULL;
  if (NetEngine::n_procs <= 0) {
    single = true;
    NetEngine::n_procs = 1;
    rval = new NetEngine; // just the default
  } else {
    single = false;
    if (NetEngine::n_procs > NetEngine::core_max_nprocs) 
      NetEngine::n_procs = NetEngine::core_max_nprocs;
    rval = new ThreadNetEngine; // just the default
  }
//nn  NetEngine::n_stride = NetEngine::proc_stride * NetEngine::n_procs;
  return rval;
}  

void Nb::ParseCmdLine(int& rval) {
  n_units = (int)strtol(argv[1], NULL, 0);
  n_cycles = (int)strtol(argv[2], NULL, 0);
  NetEngine::n_procs = (int)strtol(argv[3], NULL, 0);
  
  // optional positional params
  n_layers = 5; // def
  if ((argc > 4) && (*argv[4] != '-')) {
    n_layers = (int)strtol(argv[4], NULL, 0);
    if (n_layers < 2) n_layers = 2;
    if (n_layers > 128) n_layers = 128;
  }
  n_cons = n_units; // def
  if ((argc > 5) && (*argv[5] != '-')) {
    n_cons = (int)strtol(argv[5], NULL, 0);
    if (n_cons <= 0) 
      n_cons = n_units;
  }
  
  tsend_act = 5; // def
  if ((argc > 6) && (*argv[6] != '-')) {
    int ttsend_act = (int)strtol(argv[6], NULL, 0);
    if ((ttsend_act < 0) && (ttsend_act <= 100)) {
      tsend_act = ttsend_act;
    }
  }
  send_act = (int)(0x10000 * (tsend_act / 100.0f));
  inv_act = 100 / tsend_act;
  
  
  // switch params
  for (int arg = 4; arg < argc; arg++) {
    QString targ(argv[arg]);
#ifdef USE_SAFE
    if (targ == "-safe=1")
      safe = true;
    if (targ == "-safe=0")
      safe = false;
#endif
    if ((targ == "-header") || (targ == "-header=1")) {
      hdr = true; continue;}
    if (targ == "-header=0") {
      hdr = false; continue;}
    if (targ.startsWith("-algo=")) {
      NetEngine::algo = targ.remove("-algo=").toInt();
      continue; }
    if (targ.startsWith("-dwt_rate=")) {
      Network::dwt_rate = targ.remove("-dwt_rate=").toInt();
      continue; }
    if (targ.startsWith("-sra_rate=")) {
      Network::sra_rate = targ.remove("-sra_rate=").toInt();
      continue; }
    if (targ == "-dnorm=1") {
      Network::use_dnorm = true; continue;}
    if (targ == "-dnorm=0") {
      Network::use_dnorm = false; continue;}
      
    if (targ == "-log=1") {
      use_log_file = true; continue;}
    if (targ == "-log=0") {
      use_log_file = false; continue;}
    if (targ == "-act=0") {
      calc_act = false; continue;}
    if (targ.startsWith("-suff=")) {
      log_suff = targ.remove("-suff=");
      continue;}
  }
  log_filename = QFileInfo(argv[0]).baseName() + log_suff + ".log"; 

  // confirm sizes are in range
  const int n_prjns = (n_layers - 1) * 2;
  const int64_t n_cons_flat = n_prjns * n_units * n_cons;
  if (n_cons_flat > INT_MAX) {
    cerr << "ERROR: too many connections (max is 2^31)!\n";
    rval = 1;
  }

}
void Nb::Initialize() {
  srand(56);			// always start with the same seed!
  
  Network::recv_based = !(NetEngine::algo & NetEngine::SEND_FLAG);
  if (NetEngine::algo == 1/*NetEngine::RECV_SMART*/)
    cerr << "RECV_SMART is no longer supported\n";

  Initialize_impl();
  
  Nb::net.SetEngine(CreateNetEngine());
}

const char* cn_in_str[] = {"recv", "send", "conn"};

void Nb::PrintResults() {
  tot_time = time_used.s_used;
  n_wts = n_prjns * n_units * n_cons;
  n_con_trav = net.GetNTot();
  // actual con_trav / sec based on total actual cons
  con_trav_sec = (n_con_trav / tot_time) / 1.0e6;
  // but effective is based on total number 
  n_eff_con_trav = n_wts * n_cycles;
  eff_con_trav_sec = ((double)n_eff_con_trav / tot_time) / 1.0e6;
  
  FILE* logfile = NULL;
  if (hdr) {
    logfile = fopen(log_filename.toLatin1(), "w");
    const char* hdr_str = 
    "algo\teMcon\tMcon\tsnd_act\tprocs"
    "\tlayers\tunits\tcons\tKwts\tcycles"
    "\tKcn_tot\tsecs\tQtConc\tsrarate\tdwtrate"
    "\tcn_in\tdnorm\n";
    
    if (logfile) fprintf(logfile, hdr_str);
    printf(hdr_str);
  } else {
    logfile = fopen(log_filename.toLatin1(), "a");
  }
  if (single) NetEngine::n_procs = 0;
  char buf[256]; // s/b enough
  sprintf(buf, 
    "%d\t%.4g\t%.4g\t%d\t%d"
    "\t%d\t%d\t%d\t%g\t%d"
    "\t%g\t%.3g\t%d\t%d\t%d"
    "\t%s\t%d\n",
    NetEngine::algo, eff_con_trav_sec, con_trav_sec, tsend_act, NetEngine::n_procs,
    n_layers, n_units, n_cons, n_wts / 1000, n_cycles,
    n_con_trav / 1000, tot_time, qt_conc, Network::sra_rate, Network::dwt_rate,
    cn_in_str[cn_in], Network::use_dnorm);
  cout << buf;
  if (logfile) {
    fprintf(logfile, buf);
    fclose(logfile);
    logfile = NULL;
  }
}
