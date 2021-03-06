#include "nb_netstru.h"

#include <math.h>
#include <stdio.h>
#include <iostream>

using namespace std;

/*void ConSpec::C_Send_Netin(void*, Connection* cn, Unit* ru, float su_act_eff) {
  ru->net += su_act_eff * cn->wt;
}
void ConSpec::Send_Netin(Unit* cg, Unit* su) {
  float su_act_eff = su->act;
  for(int i=0; i<cg->units.size; i++) \
    C_Send_Netin(NULL, cg->Cn(i), cg->Un(i), su_act_eff);
}*/

//////////////////////////
//  RecvCons		//
//////////////////////////


//////////////////////////
//  Unit		//
//////////////////////////

Unit::Unit() {
  act = 0; 
  cs = NULL; 
  task_id = 0;
  flat_idx = -1;
  n_recv_cons = 0;
  net = 0;
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
  un_to_idx = 0;
}

Layer::~Layer() {
}

#ifdef SEND_CONS
void Layer::ConnectTo(Layer* lay_to) {
  Nb::n_prjns++;
  const int n_recv = lay_to->units.size;
  if (n_recv == 0) return; // shouldn't happen
  const int n_send = this->units.size;  
  // get a unit to determine receiving gp
  Unit* un = lay_to->units.FastEl(0);
  const int recv_idx = un->recv.size; // index of new recv gp...

  for (int i_fm = 0; i_fm < units.size; ++i_fm) {
    Unit* un_fm = units.FastEl(i_fm);
    const int send_idx = un_fm->send.size; // index of new send gp...
    SendCons* send_gp = un_fm->send.New();
    send_gp->cons.SetSize(n_recv);
    send_gp->units.SetSize(n_recv);
    send_gp->recv_idx = recv_idx;
    send_gp->recv_lay = lay_to;
    
    for (int i_to = 0; i_to < lay_to->units.size; ++i_to) {
      Unit* un_to = lay_to->units.FastEl(i_to);
      RecvCons* recv_gp;
      if (i_fm == 0) {
        recv_gp = un_to->recv.New();
        recv_gp->cons.SetSize(n_send);
        recv_gp->units.SetSize(n_send);
        recv_gp->send_idx = send_idx;
        recv_gp->send_lay = this;
      } else {
        recv_gp = un_to->recv.FastEl(recv_idx);
      }
      
      Connection& cn = send_gp->cons[i_to];
      cn.wt = (4.0 * (float)rand() / RAND_MAX) - 2.0;
      send_gp->units.Set(un_to, i_to);
      
      recv_gp->cons.Set(&cn, i_fm);
      recv_gp->units.Set(un_fm, i_fm);
    }
  }
}
#else
void Layer::ConnectFrom(Layer* lay_fm) {
  Nb::n_prjns++;
  const int n_send = lay_fm->units.size;
  if (n_send == 0) return; // shouldn't happen
  const int n_recv = this->units.size;  
  // get a unit to determine sending gp
  Unit* un = lay_fm->units.FastEl(0);
  const int send_idx = un->send.size; // index of new send gp...

  for (int i_to = 0; i_to < units.size; ++i_to) {
    Unit* un_to = units.FastEl(i_to);
    const int recv_idx = un_to->recv.size; // index of new recv gp...
    RecvCons* recv_gp = un_to->recv.New();
    recv_gp->cons.SetSize(n_send);
    recv_gp->units.SetSize(n_send);
    recv_gp->send_idx = send_idx;
    recv_gp->send_lay = lay_fm;
    
    for (int i_fm = 0; i_fm < lay_fm->units.size; ++i_fm) {
      Unit* un_fm = lay_fm->units.FastEl(i_fm);
      SendCons* send_gp;
      if (i_to == 0) {
        send_gp = un_fm->send.New();
        send_gp->cons.SetSize(n_recv);
        send_gp->units.SetSize(n_recv);
        send_gp->recv_idx = recv_idx;
        send_gp->recv_lay = this;
      } else {
        send_gp = un_fm->send.FastEl(send_idx);
      }
      
      Connection& cn = recv_gp->cons[i_fm];
      cn.wt = (4.0 * (float)rand() / RAND_MAX) - 2.0;
      recv_gp->units.Set(un_fm, i_fm);
      
      send_gp->cons.Set(&cn, i_to);
      send_gp->units.Set(un_to, i_to);
    }
  }
}
#endif


//////////////////////////
//  Network		//
//////////////////////////

bool Network::recv_smart; 

Network::Network() {
  engine = NULL;
}

Network::~Network() {
  SetEngine(NULL);
}

void Network::Initialize() {
  if (engine) engine->Initialize();
}

void Network::Build() {
  layers.Alloc(Nb::n_layers);
  const int n_units_flat = Nb::n_layers * Nb::n_units;
  units_flat.Alloc(n_units_flat);
  
  ConSpec* cs = new ConSpec; // everyone shares
  // make all layers and units first
  for (int l = 0; l < Nb::n_layers; l++) {
    Layer* lay = layers.New();
    lay->units.Alloc(Nb::n_units);
    
    for (int i=0;i<Nb::n_units;i++) {
      Unit* un = lay->units.New();
      un->flat_idx = units_flat.size; // next index
      units_flat.Add(un);
      un->cs = cs;
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
/*  // then partition for multi-threading, if applicable
  if (Nb::n_procs > 0) {
    switch (algo) {
    case RECV:
    case SEND_CLASH:
      PartitionUnits_RoundRobin(); break;
    default:  break;
    }
  }*/
}


void Network::ComputeNets() {
  engine->ComputeNets();
}

float Network::ComputeActs() {
  float rval = engine->ComputeActs();
  Nb::this_rand = rand(); // for next cycle
  return rval;
}

double Network::GetNTot() {
  double rval = 0;
  for (int i = 0; i < units_flat.size; ++i) {
    Unit* un = units_flat[i];
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
const int NetEngine::core_max_nprocs; // maximum number of processors!
int NetEngine::n_procs;		// total number of processors
NetTaskList NetEngine::net_tasks; // only n_procs created

NetEngine::~NetEngine() {
  if (net) {
    net->engine = NULL;
  }
}

void NetEngine::Initialize() {
  Initialize_impl();
  switch (algo) {
  case RECV:
    NetTask::Recv_Netin_0 = *NetTask::Recv_Netin_0_Dumb;
    break;
  case RECV_SMART:
    NetTask::Recv_Netin_0 = *NetTask::Recv_Netin_0_Smart;
    break;
  default: break;
  }
}

void NetEngine::Initialize_impl() {
  NetTask* tsk = new NetTask_0;
  tsk->task_id = 0;
  net_tasks.Add(tsk);
}

void NetEngine::DoProc(int proc_id) {
  NetTask* tsk = net_tasks.SafeEl(0);
  //if (!tsk) return;
  tsk->g_u = 0;
  tsk->proc_id = proc_id;
  tsk->run();
}

void NetEngine::ComputeNets() {
  Nb::n_tot = 0;
  ComputeNets_impl();
#ifdef DEBUG
  if (algo < RECV_SMART) && (Nb::n_tot != net->units_flat.size)) {
    cerr << "ERROR: NetEngine::ComputeNets: n_tot != n_units, was: "
      << Nb::n_tot << "\n";
  }
#endif
}

void NetEngine::ComputeNets_impl() {
  switch (algo) {
  case RECV:
  case RECV_SMART:
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
  for (int t = 0; t < net_tasks.size; ++t) {
    NetTask* tsk = net_tasks[t];
    Nb::tot_act += tsk->my_act;
  }
  return Nb::tot_act;
}

//////////////////////////////////
//  ThreadNetEngine		//
//////////////////////////////////

QThread* ThreadNetEngine::threads[core_max_nprocs]; // only n_procs-1 created, none for [0] (main thread)

ThreadNetEngine::~ThreadNetEngine() {
  DeleteThreads();
}

void ThreadNetEngine::Initialize_impl() {
  net_tasks.Alloc(n_procs);
  for (int i = 0; i < n_procs; i++) {
    NetTask* tsk = new NetTask_N;
    tsk->task_id = i;
    net_tasks.Add(tsk);
    if (i == 0) continue;
    QTaskThread* th = new QTaskThread;
    threads[i] = th;
    th->setTask(tsk);
    th->start(); // starts paused
  }
  
}

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
void ThreadNetEngine::ComputeNets_impl() {
  if (algo == SEND_ARRAY)
    ComputeNets_SendArray();
  else inherited::ComputeNets_impl();
}



void ThreadNetEngine::DoProc(int proc_id) {
  const int n_units_flat = net->units_flat.size;
  const int n_nibb_thresh = (int)(n_units_flat * Nb::nibble_thresh);
  // start all the other threads first...
  // have to suspend then resume in case not finished from last time
  for (int t = 1; t < n_procs; ++t) {
    QTaskThread* th = (QTaskThread*)threads[t];
    NetTask* tsk = net_tasks[t];
    th->suspend(); // prob already suspended
    tsk->g_u = t;
    tsk->proc_id = proc_id;
    th->resume();
  }
  
  // then do my part
  NetTask* tsk = net_tasks[0];
  tsk->g_u = 0;
  tsk->proc_id = proc_id;
  tsk->run_time.Start(false);
  tsk->run();
  tsk->run_time.Stop(); 
  // then either sync or "nibble"
  for (int t = 1; t < n_procs; ++t) {
    bool nibble = false;
    switch (Nb::nibble_mode) {
    case 0: nibble = false; break;
    case 1: nibble = true; break;
    default: // 2: auto, only if 
      nibble = (tsk->g_u < n_nibb_thresh); break;
    }
    if (nibble) {
      NetTask* tsk = net_tasks[t];
      // note: its ok if tsk finishes between our test and calling run
      if (tsk->g_u < n_units_flat) {
        tsk->nibble_time.Start(false);
        tsk->run();
        tsk->nibble_time.Stop(); 
      }
    } else { // sync
      tsk->sync_time.Start(false);
      QTaskThread* th = (QTaskThread*)threads[t];
      th->suspend(); // suspending is syncing with completion of loop
      tsk->sync_time.Stop(); 
    }
  }
}

void ThreadNetEngine::Log(bool hdr) {
  // note: always make/extend the thread log
  if (true/*use_log_file*/) {
  FILE* logfile = NULL;
    if (hdr) {
      logfile = fopen("nb_thread.log", "w");
      fprintf(logfile,"thread\tt_tot\tn_run\tstart lat\trun time\tnibble time\tsync time\toverhead\n");
    } else {
      logfile = fopen("nb_thread.log", "a");
    }
    for (int t = 0; t < net_tasks.size; t++) {
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

void ThreadNetEngine::OnBuild() {
  // we just partition them round-robin
  const int n_flat_units = net->units_flat.size;
  for (int i = 0; i < n_flat_units; ++i) {
    Unit* un = net->units_flat[i];
    un->task_id = i % n_procs;
  }
  // Send_Array only
  if (algo == SEND_ARRAY) {
    for (int t = 0; t < net_tasks.size; t++) {
      NetTask_N* tsk = dynamic_cast<NetTask_N*>(net_tasks[t]);
      tsk->excit.SetSize(n_flat_units);
    }
  }
}

void ThreadNetEngine::ComputeNets_SendArray() {
  DoProc(NetTask::P_Send_Netin_Array);

  // post stuff
//  RollupWritebackScratch_Netin();
  // called in Thread 0, so allocate the Task0
  NetTask* nt0 = net_tasks[0];
  nt0->overhead.Start(false);
  const int n_flat_units = net->units_flat.size;
  for (int i = 0; i < n_flat_units; ++i) {
    Unit* un = net->units_flat[i];
    //NOTE: un.net was reset to 0.0 in previous Act calc
    for (int t = 0; t < net_tasks.size; t++) {
      NetTask_N* tsk = dynamic_cast<NetTask_N*>(net_tasks[t]);
      un->net += tsk->excit[i];
    }
  }
  nt0->overhead.Stop();
}

//////////////////////////////////
// NetTask			//
//////////////////////////////////

Recv_Netin_0_t NetTask::Recv_Netin_0 = NULL;

NetTask::NetTask() {
  g_u = 0;
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
  }
}

void NetTask::Send_Netin_0(Unit* su) {
  float su_act_eff = su->act;
  for (int j = 0; j < su->send.size; ++j) {
    SendCons* send_gp = su->send[j];
#ifdef SEND_CONS
    Connection* cns = &(send_gp->cons[0]); // array pointer
#else
    Connection** cns = send_gp->cons.Els(); // conn pointer
#endif
    Unit** uns = (Nb::fast_prjn) ? send_gp->recv_lay->units.Els() :
      send_gp->units.Els(); // unit pointer
    const int send_sz = send_gp->units.size;
    su->n_con_calc += send_sz;
    for (int i=0; i < send_sz; i++)
#ifdef SEND_CONS
      Send_Netin_inner_0(cns[i].wt, &(uns[i]->net), su_act_eff);
#else
      Send_Netin_inner_0(cns[i]->wt, &(uns[i]->net), su_act_eff);
#endif
  }
}

void NetTask::Recv_Netin_0_Dumb(Unit* ru) {
  float ru_net = 0.0f;
  for (int j = 0; j < ru->recv.size; ++j) {
    RecvCons* recv_gp = ru->recv[j];
#ifdef SEND_CONS
    Connection** cns = recv_gp->cons.Els(); // array pointer
#else
    Connection* cns = &(recv_gp->cons[0]); // array pointer
#endif
    Unit** uns = (Nb::fast_prjn) ? recv_gp->send_lay->units.Els() :
      recv_gp->units.Els(); // unit pointer
    const int recv_sz = recv_gp->units.size;
    ru->n_con_calc += recv_sz;
    for(int i=0; i < recv_sz; ++i)
#ifdef SEND_CONS
      ru_net += uns[i]->act * cns[i]->wt;
#else
      ru_net += uns[i]->act * cns[i].wt;
#endif
  }
  ru->net = ru_net;
}

void NetTask::Recv_Netin_0_Smart(Unit* ru) {
  float ru_net = 0.0f;
  for (int j = 0; j < ru->recv.size; ++j) {
    RecvCons* recv_gp = ru->recv[j];
#ifdef SEND_CONS
    Connection** cns = recv_gp->cons.Els(); // array pointer
#else
    Connection* cns = &(recv_gp->cons[0]); // array pointer
#endif
    Unit** uns = (Nb::fast_prjn) ? recv_gp->send_lay->units.Els() :
      recv_gp->units.Els(); // unit pointer
    const int recv_sz = recv_gp->units.size;
    ru->n_con_calc += (recv_sz / Nb::inv_act);
    for(int i=0; i < recv_sz; ++i)
      if (uns[i]->do_delta)
#ifdef SEND_CONS
        ru_net += uns[i]->act * cns[i]->wt;
#else
        ru_net += uns[i]->act * cns[i].wt;
#endif
  }
  ru->net = ru_net;
}


void NetTask::Recv_Netin() {
  Unit** units = Nb::net.units_flat.Els();
  const int n_units_flat =  Nb::net.units_flat.size;
  while (g_u < n_units_flat) {
    Unit* un = units[g_u]; //note: accessed flat
    Recv_Netin_0(un);
    AtomicFetchAdd(&Nb::n_tot, 1); // note: we use this because we have to measure it regardless, don't penalize
    ++g_u;
  }
}

void NetTask::ComputeAct_inner(Unit* un) {
  un->act = 1.0f / (1.0f + expf(-un->net));
  un->net = 0.0f; // only needed for sender-based, but cheaper to just do than test
  //NOTE: following is not used for vanilla RECV, but we include it
  // as shared overhead anyway
  un->CalcDelta(); // sets flag, except for RECV
}

void NetTask::ComputeAct() {
  Unit** units = Nb::net.units_flat.Els();
  // compute activations (only order number of units)
  my_act = 0.0f;
  const int n_units_flat =  Nb::net.units_flat.size;
  while (g_u < n_units_flat) {
    Unit* un = units[g_u]; //note: accessed flat
    ComputeAct_inner(un);
    my_act += un->act;
    ++g_u;
  }
}


void NetTask_0::Send_Netin_Clash() {
  Unit** units = Nb::net.units_flat.Els();
  const int n_units_flat =  Nb::net.units_flat.size;
  while (g_u < n_units_flat) {
    Unit* un = units[g_u]; //note: accessed flat
    if (un->do_delta) {
      Send_Netin_0(un);
      AtomicFetchAdd(&Nb::n_tot, 1); // note: we use this because we have to measure it regardless, don't penalize
    }
    ++g_u;
  }
}

void NetTask_N::Send_Netin_Clash() {
  Unit** units = Nb::net.units_flat.Els();
  int my_u = AtomicFetchAdd(&g_u, ThreadNetEngine::n_procs);
  while (my_u < Nb::net.units_flat.size) {
    Unit* un = units[my_u]; //note: accessed flat
    if (un->do_delta) {
      Send_Netin_0(un);
      AtomicFetchAdd(&Nb::n_tot, 1);
    }
    my_u = AtomicFetchAdd(&g_u, ThreadNetEngine::n_procs);
  }
}

void NetTask_N::Send_Netin_Array() {
  overhead.Start(false);
  memset(excit.el, 0, sizeof(float) * excit.size);
  overhead.Stop();

  Unit** units = Nb::net.units_flat.Els();
  int my_u = AtomicFetchAdd(&g_u, ThreadNetEngine::n_procs);
  while (my_u < Nb::net.units_flat.size) {
    Unit* su = units[my_u]; //note: accessed flat
    if (su->do_delta) {
      float su_act_eff = su->act;
      for (int j = 0; j < su->send.size; ++j) {
        SendCons* send_gp = su->send[j];
#ifdef SEND_CONS
        Connection* cns = &(send_gp->cons[0]); // conn pointer
#else
        Connection** cns = send_gp->cons.Els(); // conn pointer
#endif
        Unit** uns = (Nb::fast_prjn) ? send_gp->recv_lay->units.Els() :
          send_gp->units.Els(); // unit pointer
        const int send_sz = send_gp->units.size;
        su->n_con_calc += send_sz;
        for (int i=0; i < send_sz; i++) {
          int targ_i = uns[i]->flat_idx;
#ifdef SEND_CONS
          Send_Netin_inner_0(cns[i].wt, &(excit.el[targ_i]), su_act_eff);
#else
          Send_Netin_inner_0(cns[i]->wt, &(excit.el[targ_i]), su_act_eff);
#endif
        }
      }
      AtomicFetchAdd(&Nb::n_tot, 1);
    }
    my_u = AtomicFetchAdd(&g_u, ThreadNetEngine::n_procs);
  }
}

void NetTask_N::Recv_Netin() {
  Unit** units = Nb::net.units_flat.Els();
  int my_u = AtomicFetchAdd(&g_u, ThreadNetEngine::n_procs);
  while (my_u < Nb::net.units_flat.size) {
    Unit* un = units[my_u]; //note: accessed flat
    Recv_Netin_0(un);
    AtomicFetchAdd(&Nb::n_tot, 1); // note: we use this because we have to measure it regardless, don't penalize
    my_u = AtomicFetchAdd(&g_u, ThreadNetEngine::n_procs);
  }
}

void NetTask_N::ComputeAct() {
  Unit** units = Nb::net.units_flat.Els();
  int my_u = AtomicFetchAdd(&g_u, ThreadNetEngine::n_procs);
  my_act = 0.0f;
  const int unit_sz = Nb::net.units_flat.size;
  while (my_u < unit_sz) {
    Unit* un = units[my_u];
    ComputeAct_inner(un);
    my_act += un->act;
    my_u = AtomicFetchAdd(&g_u, ThreadNetEngine::n_procs);
  }
}


bool Nb::hdr = false;

int Nb::n_layers;
int Nb::n_units;			// number of units per layer
int Nb::n_cons; // number of cons per unit
int Nb::n_cycles;			// number of cycles of updating
Network Nb::net;		// global network 
int Nb::n_tot;
int Nb::n_prjns;
float Nb::tot_act; // check on tot act
float Nb::nibble_thresh = 0.8f;
signed char Nb::nibble_mode = 0;
signed char Nb::fast_prjn = 0; // fast prjns directly access target unit array
bool Nb::single = false; // true for single thread mode, to compare against nprocs=1
int Nb::send_act = 0x10000; // send activation, as a fraction of 2^16 
int Nb::tsend_act; 
int Nb::inv_act = 1; // inverse of activation -- can use to divide
int Nb::this_rand; // assigned a new random value each cycle, to let us randomize unit acts
bool Nb::use_log_file = false;
bool Nb::calc_act = true;
#ifdef SEND_CONS
signed char Nb::sndcn = 1; 
#else
signed char Nb::sndcn = 0; 
#endif

int Nb::main() {
  Initialize();
  
  net.Initialize();
  net.Build();

  FILE* logfile = NULL;
  if (use_log_file) {
    logfile = fopen("ptest_core.log", "w");
    fprintf(logfile,"cyc\ttot_act\n");
  }

  time_used.Start();

  // this is the key computation loop
  for(int cyc = 0; cyc < n_cycles; cyc++) {
    net.ComputeNets();
    if (calc_act) {
      float tot_act = net.ComputeActs();
      if(use_log_file)
        fprintf(logfile,"%d\t%g\n", cyc, tot_act);
    }
  }

  time_used.Stop();
  if (use_log_file) {
    fclose(logfile);
    logfile = NULL;
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
  return rval;
}

void Nb::PreInitialize_impl(int& /*rval*/) {
}

void Nb::Initialize() {

  srand(56);			// always start with the same seed!

  n_units = (int)strtol(argv[1], NULL, 0);
  n_cycles = (int)strtol(argv[2], NULL, 0);


  
  NetEngine::n_procs = (int)strtol(argv[3], NULL, 0);
  if (NetEngine::n_procs <= 0) {
    single = true;
    NetEngine::n_procs = 1;
    Nb::net.SetEngine(new NetEngine); // just the default
  } else {
    single = false;
    if (NetEngine::n_procs > NetEngine::core_max_nprocs) 
      NetEngine::n_procs = NetEngine::core_max_nprocs;
    Nb::net.SetEngine(new ThreadNetEngine); // just the default
  }
  
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
    if (targ == "-header")
      hdr = true;
    if (targ.startsWith("-algo=")) {
      NetEngine::algo = targ.remove("-algo=").toInt();
      continue;
    }
      
    if (targ == "-log=1")
      use_log_file = true;
    if (targ == "-log=0")
      use_log_file = false;
    if (targ == "-act=0")
      calc_act = false;
    if (targ.startsWith("-nibble=")) {
      nibble_mode = targ.remove("-nibble=").toInt();
      continue;
    if (targ == "-fast_prjn=1")
      fast_prjn = true;
    }
    //TODO: any more
  }
  if (NetEngine::algo == NetEngine::RECV_SMART)
    Network::recv_smart = true;


  Initialize_impl();
}

void Nb::PrintResults() {
  tot_time = time_used.s_used;
  n_wts = n_prjns * n_units * n_cons;
  n_con_trav = net.GetNTot();
  // actual con_trav / sec based on total actual cons
  con_trav_sec = (n_con_trav / tot_time) / 1.0e6;
  // but effective is based on total number 
  n_eff_con_trav = n_wts * n_cycles;
  eff_con_trav_sec = ((double)n_eff_con_trav / tot_time) / 1.0e6;
  
  if (hdr)
    printf("algo\teMcon\tMcon\tsnd_act\tprocs\tlayers\tunits\tcons\tweights\tcycles\tcon_trav\tsecs\tnibble\tsndcn\tfstprjn\n");
  if (single) NetEngine::n_procs = 0;
  printf("%d\t%g\t%g\t%d\t%d\t%d\t%d\t%d\t%g\t%d\t%g\t%g\t%d\t%d\t%d\n",
    NetEngine::algo, eff_con_trav_sec, con_trav_sec, tsend_act, NetEngine::n_procs,
    n_layers, n_units, n_cons, n_wts, n_cycles, n_con_trav, tot_time, nibble_mode,
    sndcn, fast_prjn);
}
