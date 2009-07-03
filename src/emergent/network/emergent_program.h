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

// emergent_program.h -- pdp extensions to program

#ifndef EMERGENT_PROGRAM_H
#define EMERGENT_PROGRAM_H

#include "ta_program_els.h"
// #include "ta_datatable.h"
#include "ta_dataproc.h"
#include "ta_dmem.h"

class Network;
class Layer;

#include "emergent_base.h"

// note: the motivation for supporting dmem within the basic data loops is so that the
// same project can be run transparently in dmem or non-dmem mode without modification

class EMERGENT_API NetBaseProgEl: public ProgEl { 
  // #VIRT_BASE #NO_INSTANCE base type for network-oriented prog els (filter function, etc)
INHERITED(ProgEl)
public:
  static bool		NetProgVarFilter(void* base, void* var); // Network* progvar filter -- only shows Network* items -- use in ITEM_FILTER comment directive
  TA_BASEFUNS_NOCOPY(NetBaseProgEl);
private:
 void	Initialize() { };
 void	Destroy() { };
};


class EMERGENT_API NetDataLoop: public DataLoop { 
  // For network input data: loops over items in a DataTable, in different basic orderings, using index to select current data table item using ReadItem(index) call, so that later processes will access this row of data. Note: assumes that there is a 'network' object pointer variable and an int 'trial' counter variable defined in the program!!
INHERITED(DataLoop)
public:
  bool		update_after;	// call update-after-edit on the network object after changing the trial counter -- this is necessary to update control panels that monitor information at the trial level
  int		dmem_nprocs;	// #READ_ONLY number of processors to use for distributed memory processing (input data distributed over nodes) -- computed automatically if dmem is active; else set to 1
  int		dmem_this_proc;	// #READ_ONLY processor rank for this processor relative to communicator group

  virtual void	DMem_Initialize(Network* net);
  // configure the dmem communicator stuff: depends on dmem setup of network

  override String GetDisplayName() const;
  override String	GetToolbarName() const { return "data loop"; }

  PROGEL_SIMPLE_BASEFUNS(NetDataLoop);
protected:
  override const String	GenCssPre_impl(int indent_level); 

private:
  void	Initialize();
  void	Destroy() { CutLinks(); }
};

class EMERGENT_API NetGroupedDataLoop: public Loop { 
  // loops over items in a DataTable, in different basic orderings, using index to select current data table item using ReadItem(index) call, so that later processes will access this row of data.  Note: assumes that there is a 'network' variable defined in program!!
INHERITED(Loop)
public:
  enum Order {
    SEQUENTIAL,			// present events (input data rows) in sequential order
    PERMUTED,			// permute the order of event (input data row) presentation
    RANDOM, 			// pick an event (input data row) at random (with replacement)
  };

  ProgVarRef	data_var;	// #ITEM_FILTER_DataProgVarFilter program variable pointing to the data table to use
  ProgVarRef	group_index_var; // #ITEM_FILTER_StdProgVarFilter program variable for the group index used in the loop -- goes from 0 to number of groups in data table-1
  ProgVarRef	item_index_var; // #ITEM_FILTER_StdProgVarFilter program variable for the item index used in the loop -- goes from 0 to number of items in current group
  ProgVarRef	group_order_var; // #ITEM_FILTER_StdProgVarFilter variable that contains the order to process data groups in -- is automatically created if not set
  ProgVarRef	item_order_var; // #ITEM_FILTER_StdProgVarFilter variable that contains the order to process data items in -- is automatically created if not set
  Order		group_order;	// #READ_ONLY #SHOW order to process data groups in -- set from group_order_var
  Order		item_order;	// #READ_ONLY #SHOW order to process data items in -- set from item_order_var
  int		group_col;	// column in the data table that contains the group names
  bool		update_after;	// call update-after-edit on the network object after changing the trial counter -- this is necessary to update control panels that monitor information at the trial level
  int_Array	group_idx_list;	// #READ_ONLY list of group starting indicies
  int_Array	item_idx_list;	// #READ_ONLY list of item indicies within group

  override String	GetDisplayName() const;

  virtual void	GetOrderVals();
  // get order values from order_var variables
  virtual void	GetGroupList();
  // initialize the group_idx_list from the data: idx's are where group name changes
  virtual void  GetItemList(int group_idx); // 
  override String	GetToolbarName() const { return "gp data lp"; }

  PROGEL_SIMPLE_BASEFUNS(NetGroupedDataLoop);
protected:
  virtual void	GetOrderVars(); // make order variables in program
  virtual void	GetIndexVars(); // make index variables in program if not already set
  override void	UpdateAfterEdit_impl();
  override void	CheckThisConfig_impl(bool quiet, bool& rval);
  override const String	GenCssPre_impl(int indent_level); 
  override const String	GenCssBody_impl(int indent_level); 
  override const String	GenCssPost_impl(int indent_level); 

private:
  void	Initialize();
  void	Destroy() { CutLinks(); }
};

class EMERGENT_API NetCounterBase: public NetBaseProgEl { 
  // #VIRT_BASE #NO_INSTANCE base for network counter manip prog els
INHERITED(NetBaseProgEl)
public:
  ProgVarRef	network_var;	// #ITEM_FILTER_NetProgVarFilter variable that points to the network 
  TypeDef*	network_type;	// #HIDDEN #NO_SAVE just to anchor the memberdef*
  ProgVarRef 	local_ctr_var;	// #ITEM_FILTER_StdProgVarFilter local version of the counter variable, maintained by the program -- must have same name as the counter!  automatically created if not set
  MemberDef*	counter;	// #TYPE_ON_network_type #DEFCAT_Counter counter variable on network to operate on
  bool		update_after;	// call UpdateAfterEdit on network after updating counter value -- not necessary except to trigger updating of various displays, etc
  
  override String 	GetTypeDecoKey() const { return "ProgVar"; }

  PROGEL_SIMPLE_BASEFUNS(NetCounterBase);
protected:
  override void	UpdateAfterEdit_impl();
  override void	CheckThisConfig_impl(bool quiet, bool& rval);
  virtual void	GetLocalCtrVar(); // if counter is not empty and local_ctr_var == NULL, then get a local ctr var for it

private:
  void	Initialize();
  void	Destroy();
};

class EMERGENT_API NetCounterInit: public NetCounterBase { 
  // initialize a network counter: program keeps a local version of the counter, and updates both this and the network's copy
INHERITED(NetCounterBase)
public:
  override String	GetDisplayName() const;
  override String	GetToolbarName() const { return "net ctr init"; }

  PROGEL_SIMPLE_BASEFUNS(NetCounterInit);
protected:
  override const String	GenCssBody_impl(int indent_level);

private:
  void	Initialize() { };
  void	Destroy() { };
};

class EMERGENT_API NetCounterIncr: public NetCounterBase { 
  // increment a network counter: program keeps a local version of the counter, and updates both this and the network's copy
INHERITED(NetCounterBase)
public:
  override String	GetDisplayName() const;
  override String	GetToolbarName() const { return "net ctr inc"; }

  PROGEL_SIMPLE_BASEFUNS(NetCounterIncr);
protected:
  override const String	GenCssBody_impl(int indent_level);

private:
  void	Initialize() { };
  void	Destroy() { };
};

class EMERGENT_API NetUpdateView: public NetBaseProgEl { 
  // update the network view, conditional on an update_net_view variable that is created by this progam element
INHERITED(NetBaseProgEl)
public:
  ProgVarRef	network_var;	// #ITEM_FILTER_NetProgVarFilter variable that points to the network
  ProgVarRef	update_var;	// #ITEM_FILTER_StdProgVarFilter variable that controls whether we update the display or not
  
  override String	GetDisplayName() const;
  override String 	GetTypeDecoKey() const { return "Function"; }
  override String	GetToolbarName() const { return "net updt view"; }

  PROGEL_SIMPLE_BASEFUNS(NetUpdateView);
protected:
  override void	UpdateAfterEdit_impl();
  override void	CheckThisConfig_impl(bool quiet, bool& rval);
  virtual void	GetUpdateVar(); // get the update_var variable

  override const String	GenCssBody_impl(int indent_level);

private:
  void	Initialize();
  void	Destroy();
};

////////////////////////////////////////////////////
//		Named Units Framework
////////////////////////////////////////////////////

class EMERGENT_API InitNamedUnits: public NetBaseProgEl { 
  // Initialize named units system -- put this in the Init code of the program and it will configure everything based on the input_data datatable (which should be the first datatable in the args or vars -- Set Unit guys will look for it there)
INHERITED(NetBaseProgEl)
public:
  ProgVarRef	input_data_var;	// #ITEM_FILTER_DataProgVarFilter program variable pointing to the input data table -- finds the first one in the program by default (and makes one if not found)
  ProgVarRef	unit_names_var;	// #ITEM_FILTER_DataProgVarFilter program variable pointing to the unit_names data table, which is created if it does not exist -- contains the name labels for each of the units
  ProgVarRef	network_var;	// #ITEM_FILTER_NetProgVarFilter variable that points to the network (optional; for labeling network units if desired)
  int		n_lay_name_chars; // number of layer-name chars to prepend to the enum values
  int		max_unit_chars; // max number of characters to use in unit label names (-1 = all)

  static bool	InitUnitNamesFmInputData(DataTable* unit_names, const DataTable* input_data);
  // intialize unit names data table from input data table
  static bool	InitDynEnumFmUnitNames(DynEnumType* dyn_enum, const DataCol* unit_names_col,
				       const String& prefix);
  // initialize a dynamic enum with names from unit names table colum (string matrix with one row)
  virtual bool	InitNamesTable();
  // #BUTTON #CONFIRM intialize (and update) the unit names table (will auto-create if not set) -- must have set the input_data_var to point to an input data table already!
  virtual bool	InitDynEnums();
  // #BUTTON #CONFIRM intialize the dynamic enums from names table -- do this after you have entered the names in the unit_names table, in order to then refer to the names using enum values (avoiding having to use quotes!)
  virtual bool	LabelNetwork(bool propagate_names = false);
  // #BUTTON label units in the network based on unit names table -- also sets the unit_names matrix in the layer so they are persistent -- network_var must be set -- if propagate_names is set, then names will be propagated along one-to-one projections to other layers that do not have names in the table
  virtual bool	ViewDataLegend();
  // #BUTTON #CONFIRM create a new grid view display of the input data with the unit names as alegend
  
  override String	GetDisplayName() const;
  override String 	GetTypeDecoKey() const { return "ProgCtrl"; }
  override String	GetToolbarName() const { return "init nm units"; }

  PROGEL_SIMPLE_BASEFUNS(InitNamedUnits);
protected:
  override void	UpdateAfterEdit_impl();
  override void	CheckThisConfig_impl(bool quiet, bool& rval);

  virtual bool	GetInputDataVar();
  virtual bool	GetUnitNamesVar();
  virtual bool	GetNetworkVar();

  override const String	GenCssBody_impl(int indent_level);

private:
  void	Initialize();
  void	Destroy();
};

class EMERGENT_API SetUnitsLit: public ProgEl { 
  // set units in input_data table to present to the network based on dynamic enum values where the type name of the dynamic enum corresponds to the layer name in the input data: values supplied as literal items
INHERITED(ProgEl)
public:
  ProgVarRef	input_data_var;	// #ITEM_FILTER_DataProgVarFilter program variable pointing to the input data table
  bool		set_nm;		// set trial name based on unit names here
  int		offset;		// add this additional offset to unit indicies -- useful for unit groups with same sets of units
  DynEnum	unit_1; 	// unit to activate -- order doesn't matter -- can be any unit
  DynEnum	unit_2; 	// unit to activate -- order doesn't matter -- can be any unit
  DynEnum	unit_3; 	// unit to activate -- order doesn't matter -- can be any unit
  DynEnum	unit_4; 	// unit to activate -- order doesn't matter -- can be any unit

  override String	GetDisplayName() const;
  override String 	GetTypeDecoKey() const { return "Function"; }
  override String	GetToolbarName() const { return "set units lit"; }

  PROGEL_SIMPLE_BASEFUNS(SetUnitsLit);
protected:
  override void	UpdateAfterEdit_impl();
  override void	CheckThisConfig_impl(bool quiet, bool& rval);
  virtual bool	GetInputDataVar();

  override const String	GenCssBody_impl(int indent_level);
  virtual bool	GenCss_OneUnit(String& rval, DynEnum& un, const String& idnm, 
			       DataTable* idat, const String& il);

private:
  void	Initialize();
  void	Destroy();
};


class EMERGENT_API SetUnitsVar: public ProgEl { 
  // set units in input_data table to present to the network based on dynamic enum variables where the type name of the dynamic enum corresponds to the layer name in the input data: values supplied as variables
INHERITED(ProgEl)
public:
  ProgVarRef	input_data_var;	// #ITEM_FILTER_DataProgVarFilter program variable pointing to the input data table
  
  bool		set_nm;		// set trial name based on unit names here
  ProgVarRef	offset;		// #ITEM_FILTER_StdProgVarFilter add this additional offset to unit indicies -- useful for unit groups with same sets of units
  ProgVarRef	unit_1;		// #ITEM_FILTER_DynEnumProgVarFilter unit to activate -- order doesn't matter -- can be any unit
  ProgVarRef	unit_2;		// #ITEM_FILTER_DynEnumProgVarFilter unit to activate -- order doesn't matter -- can be any unit
  ProgVarRef	unit_3;		// #ITEM_FILTER_DynEnumProgVarFilter unit to activate -- order doesn't matter -- can be any unit
  ProgVarRef	unit_4;		// #ITEM_FILTER_DynEnumProgVarFilter unit to activate -- order doesn't matter -- can be any unit
  
  override String	GetDisplayName() const;
  override String 	GetTypeDecoKey() const { return "Function"; }
  override String	GetToolbarName() const { return "set units var"; }

  PROGEL_SIMPLE_BASEFUNS(SetUnitsVar);
protected:
  override void	UpdateAfterEdit_impl();
  override void	CheckThisConfig_impl(bool quiet, bool& rval);
  virtual bool	GetInputDataVar();

  override const String	GenCssBody_impl(int indent_level);
  virtual bool	GenCss_OneUnit(String& rval, ProgVarRef& un, const String& idnm, 
			       DataTable* idat, const String& il);

private:
  void	Initialize();
  void	Destroy();
};

class EMERGENT_API WtInitPrompt: public IfGuiPrompt { 
  // special program element for prompting whether to initialize network weights -- only prompts if network has been trained (epoch > 0) -- requires a variable named: network -- will complain if not found!
INHERITED(IfGuiPrompt)
public:
  override String	GetToolbarName() const { return "wt init prmt"; }
  TA_BASEFUNS_NOCOPY(WtInitPrompt);
protected:
  override const String	GenCssPre_impl(int indent_level); 

private:
  void	Initialize();
  void	Destroy()	{ } //
};



#endif
