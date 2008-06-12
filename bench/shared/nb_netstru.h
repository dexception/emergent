#ifndef NB_NETSTRU_H
#define NB_NETSTRU_H


#include "nb_util.h"

// Now: actual neural network code
// units compute net = sum_j weight_j * act_j for all units that send input into them
// then share this net across nodes
// and then compute activations, and repeat.

class Connection;
class RecvCons;
class ConSpec;
class Unit;
class Layer;

// note: some of these classes are same-name and similar in structure to
// the corresponding Emergent/Leabra classes

class Connection {
  // one connection between units
public:
  float 	wt;	// connection weight value
  float 	dwt;	// delta-weight
  float		pdw;
  float		sravg;	// #NO_SAVE average of sender and receiver activation 
};

typedef taArray<Connection>	ConArray;
typedef taPtrList<Unit>		UnitPtrList;
typedef taList<Unit>		UnitList;

class RecvCons {
public:
  ConArray	cons;
  // #NO_FIND #NO_SAVE #CAT_Structure the array of connections, in index correspondence with units
  UnitPtrList	units;
  // #NO_FIND #NO_SAVE #CAT_Structure pointers to the sending units of this connection (in index correspondence with cons)
  int		send_idx;

  RecvCons() {send_idx = -1;}
  ~RecvCons() {}
};

typedef taList<RecvCons>	RecvCons_List;

typedef taPtrList<Connection>	ConPtrList; // SendCons

class SendCons {
public:
  ConPtrList	cons;
  // #NO_FIND #NO_SAVE #CAT_Structure list of pointers to receiving connections, in index correspondence with units;
  UnitPtrList	units;
  // #NO_FIND #NO_SAVE #CAT_Structure pointers to the receiving units of this connection, in index correspondence with cons
  int		recv_idx;
  
  SendCons() { recv_idx = -1;}
  ~SendCons() {}
};

typedef taList<SendCons>	SendCons_List;

class ConSpec {
  int	dummy[8];
public:
  
//  void C_Send_Netin(void*, Connection* cn, Unit* ru, float su_act_eff);
//  void Send_Netin(Unit* cg, Unit* su);
};


class Unit {
  // a simple unit
public:
#ifdef USE_RECV_SMART
  bool		do_delta; // for recv_smart
#endif
  float 	act;			// activation value
  float 	net;			// net input value
  
  RecvCons_List	recv;
  SendCons_List send;
  
  int		task_id; // which task will process this guy
  int		n_recv_cons;
  ConSpec*	cs;
  char dummy[16]; // bump a bit, to spread these guys out
  
//  Connection* 	Cn(int i) const { return &(send_wts[i]); }
  // #CAT_Structure gets the connection at the given index
//  Unit*		Un(int i) const { return targs.FastEl(i); }
    
  bool		DoDelta(); // returns true if we should do a delta; also sets the do_delta
  
  
  Unit();
  ~Unit();
protected:
  int		my_rand; // random value
};

class Layer {
public:
  int		un_to_idx; // circular guy we use to pick next target unit
  
  UnitList	units;
  
  void		ConnectFrom(Layer* lay_fm); // connect from me to
  
  Layer();
  ~Layer();
};

typedef taList<Layer>	LayerList;

class Network {
public:
  enum Algo { // note: not exhaustive...
    RECV	= 0,
    SEND_CLASH	= 1, // sender, where writes can clash
  };
  
  static int 	algo; // the algorithm number
  static bool	recv_smart; // for recv-based, does the smart recv algo
  
  UnitPtrList 	units_flat;	// all units, flattened
  LayerList	layers;
  
  void		Build(); 
  void 		ComputeNets();
  float 	ComputeActs();
  
  Network();
  ~Network();
protected:
  void		PartitionUnits_RoundRobin(); // for recv, and send-clash
  //void		PartitionUnits_SendClash(); 
};

class NetTask: public Task {
public:
  enum Proc {
    P_Send_Netin,
    P_Recv_Netin,
    P_ComputeAct
  };
  
// All
  int		g_u; 
  int		t_tot; // shared by Xxx_Netin
  void		run();

// Send_Netin
  inline static void Send_Netin_inner_0(float cn_wt, float* ru_net, float su_act_eff);
    // highly optimized inner loop
  static void 	Send_Netin_0(Unit* su); // shared by 0 and N
  virtual void	Send_Netin() = 0; // NetIn
  
// Recv_Netin
  static void 	Recv_Netin_0(Unit* ru); // shared by 0 and N
  virtual void	Recv_Netin(); // default is the _0 version
  
  
// ComputeAct  
  static void ComputeAct_inner(Unit* un);
  float		my_act;
  virtual void	ComputeAct(); // default does it globally
  
  NetTask();
};

void NetTask::Send_Netin_inner_0(float cn_wt, float* ru_net, float su_act_eff) {
  *ru_net += su_act_eff * cn_wt;
  //tru_net = *ru_net + su_act_eff * cn_wt;
}


class NetTask_0: public NetTask {
// for single threaded approach -- optimum for that
public:
  void		Send_Netin();
//  void		ComputeAct();
};

class NetTask_N: public NetTask {
// for N threaded approach -- optimum for that
public:
  void		Send_Netin();
  void		Recv_Netin();
  void		ComputeAct();
};

class Nb { // global catchall
public:
  static bool hdr;
// global net params
  static int n_layers;
  static int n_units;			// number of units per layer
  static int n_cons; // number of cons per unit
  static int n_cycles;			// number of cycles of updating
  static Network net;		// global network 
  static int n_tot; // total units (reality check)
  static float tot_act; // check on tot act
  static int send_act; // send activation, as a fraction of 2^16 
  
  static int this_rand; // assigned a new random value each cycle, to let us randomize unit acts
  
// thread values
  static const int core_max_nprocs = 32; // maximum number of processors!
  static int n_procs;		// total number of processors
  static NetTask* netin_tasks[core_max_nprocs]; // only n_procs created
  static QThread* threads[core_max_nprocs]; // only n_procs-1 created, none for [0] (main thread)
  
  static bool nibble; // setting false disables nibbling and adds sync to loop
  static bool single; // true for single thread mode, to compare against nprocs=1
  static bool calc_act;
  static bool sender; // sender based, else receiver-based
  
  static int	main(int argc, char* argv[]); // must be suplied in the main.cpp
protected:
  static void 	MakeThreads();
  static void 	DeleteThreads();

private:
  Nb();
  ~Nb();
};

#endif

