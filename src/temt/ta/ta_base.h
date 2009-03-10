// Copyright, 1995-2007, Regents of the University of Colorado,
// Carnegie Mellon University, Princeton University.
//
// This file is part of The Emergent Toolkit
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

#ifndef ta_base_h
#define ta_base_h 1

#include "ta_type.h"
#include "icolor.h"
//#include "ta_qtbrowse_def.h"
#include "ta_TA_type.h"

#ifndef __MAKETA__
#ifdef TA_GUI
# include <qobject.h>
#endif
#endif


// externals
class SelectEdit;
class taFiler;
class taDoc;
class taRootBase; //

#ifdef TA_GUI
class taiMimeSource; //
#endif

// forwards
class taBase;
class taSmartRef;
class taSmartPtr;
class taOBase;
class taOABase;
class taDataView;
class taNBase;
class taList_impl;
class taBase_List;
class taBase_PtrList;
class taBase_RefList;
class String_Array;
class UserDataItem;
class UserDataItem_List;
class NameVar_Array;
class taBase_FunCallList; //

class TA_API tabMisc {
  // #NO_TOKENS #INSTANCE miscellaneous useful stuff for taBase
friend class taBase;
friend class taList_impl;
public:
  static taRootBase*	root;
  // root of the structural object hierarchy

  static taBase_RefList	delayed_updateafteredit;
  // list of objs to be update-after-edit'd in the wait process
  static taBase_FunCallList  delayed_funcalls;
  // functions to call during the waiting process -- variant value is the object, and name is the function
  static ContextFlag	in_wait_proc; // context -- don't do WaitProc

  static void		WaitProc();
  // wait process function: process all the delayed stuff

  static void		DelayedUpdateAfterEdit(TAPtr obj);
  // call update-after-edit on object in wait process (in case this does other kinds of damage..)
  static void		DelayedFunCall_gui(taBase* obj, const String& fun_name);
  // perform a delayed function call on this object of given function name (using CallFun) -- if args required they will be prompted for, but that is probably not a great idea from the user's perspective.. best for void functions -- gui version for gui feedback events -- checks for gui_active
  static void		DelayedFunCall_nogui(taBase* obj, const String& fun_name);
  // perform a delayed function call on this object of given function name (using CallFun) -- if args required they will be prompted for, but that is probably not a great idea from the user's perspective.. best for void functions -- nogui version -- doesn't check for gui

  static void		DeleteRoot(); // get rid of root, if not nuked already

protected:
  static taBase_RefList	delayed_close;
  // list of objs to be removed in the wait process (e.g. when objs delete themselves)
  
  static void		DelayedClose(taBase* obj);
  // close this object during the wait process (after other events have been processed and we are outside of any functions within the to-be-closed object) -- USE taBase APIs for this!
  
  static bool		DoDelayedCloses();
  static bool		DoDelayedUpdateAfterEdits();
  static bool		DoDelayedFunCalls();
};

// common defs used by ALL taBase types: Type and Copy guys
#define TA_BASEFUNS_MAIN_(y) \
private: \
  inline void Copy__(const y& cp) { \
    SetBaseFlag(COPYING); \
      Copy_(cp); \
    ClearBaseFlag(COPYING);} \
protected: \
  void Copy_impl(const y& cp) { \
    StructUpdate(true); \
      inherited::Copy_impl(cp); \
      Copy__(cp); \
    StructUpdate(false);} \
  void  UnSafeCopy(const taBase* cp) { if(cp->InheritsFrom(&TA_##y)) Copy_impl(*((y*)cp)); \
    else if(InheritsFrom(cp->GetTypeDef())) cp->CastCopyTo(this);} \
  void  CastCopyTo(taBase* cp) const { y& rf = *((y*)cp); rf.Copy_impl(*this); } \
public: \
  static TypeDef* StatTypeDef(int) { return &TA_##y; } \
  TypeDef* GetTypeDef() const { return &TA_##y; } \
  inline bool Copy(const taBase* cp) {return taBase::Copy(cp);} \
  void Copy(const y& cp) { Copy_impl(cp);} \
  y& operator=(const y& cp) { Copy(cp); return *this;}

#define TA_TMPLT_BASEFUNS_MAIN_(y,T) \
private: \
  inline void Copy__(const y<T>& cp) { \
    SetBaseFlag(COPYING); \
      Copy_(cp); \
    ClearBaseFlag(COPYING);} \
protected: \
  void Copy_impl(const y<T>& cp) { \
    StructUpdate(true); \
      inherited::Copy_impl(cp); \
      Copy__(cp); \
    StructUpdate(false);} \
  void  UnSafeCopy(const taBase* cp) { if(cp->InheritsFrom(&TA_##y)) Copy_impl(*((y<T>*)cp)); \
    else if(InheritsFrom(cp->GetTypeDef())) cp->CastCopyTo(this); } \
  void  CastCopyTo(taBase* cp) const { y<T>& rf = *((y<T>*)cp); rf.Copy_impl(*this); } \
public: \
  static TypeDef* StatTypeDef(int) { return &TA_##y; } \
  TypeDef* GetTypeDef() const { return &TA_##y; } \
  inline bool Copy(const taBase* cp) {return taBase::Copy(cp);} \
  void Copy(const y<T>& cp) { Copy_impl(cp);} \
  y<T>& operator=(const y<T>& cp) { Copy(cp); return *this; }

// common defs used to make instances: Cloning and Tokens
#define TA_BASEFUNS_INST_(y) \
  TAPtr Clone() { return new y(*this); }  \
  TAPtr MakeToken(){ return (TAPtr)(new y); } \
  TAPtr MakeTokenAry(int n){ return (TAPtr)(new y[n]); }

#define TA_TMPLT_BASEFUNS_INST_(y,T) \
  TAPtr Clone() { return new y<T>(*this); } \
  TAPtr MakeToken(){ return (TAPtr)(new y<T>); }  \
  TAPtr MakeTokenAry(int n){ return (TAPtr)(new y<T>[n]); }  

// ctors -- one size fits all (where used) thanks to Initialize__
#ifndef __MAKETA__
#define TA_BASEFUNS_CTORS_(y) \
  y () { Initialize__(); } \
  y (const y& cp):inherited(cp) { Initialize__(); Copy__(cp);}
  
#define TA_TMPLT_BASEFUNS_CTORS_(y,T) \
  y () { Initialize__(); } \
  y (const y<T>& cp):inherited(cp) { Initialize__(); Copy__(cp); }

#else

#define TA_BASEFUNS_CTORS_(y) \
  y () { Initialize__(); } \
  y (const y& cp) { Initialize__(); Copy__(cp);}
  
#define TA_TMPLT_BASEFUNS_CTORS_(y,T) \
  y () { Initialize__(); } \
  y (const y<T>& cp) { Initialize__(); Copy__(cp); }

#endif

// common dtor/init, when using tokens (same for TMPLT)
#define TA_BASEFUNS_TOK_(y) \
  private: \
  inline void Initialize__() {Register(); Initialize(); \
    if (!(taMisc::is_loading || taMisc::is_duplicating)) SetDefaultName();} \
  public: \
  ~y () { CheckDestroyed(); unRegister(); Destroying(); Destroy(); }

#define TA_TMPLT_BASEFUNS_TOK_(y,T) TA_BASEFUNS_TOK_(y)

// common dtor/init when not using tokens (the LITE guys)
#define TA_BASEFUNS_NTOK_(y) \
  private: \
  inline void Initialize__() {Initialize();}  \
  public: \
  ~y () { CheckDestroyed(); Destroying(); Destroy(); } 

#define TA_TMPLT_BASEFUNS_NTOK_(y,T) TA_BASEFUNS_NTOK_(y)

// normal set of funs, for tokens, except ctors; you can use this yourself
// when you have consts in your class and can't use the generic ctors
#define TA_BASEFUNS_TOK_NCTORS(y) \
  TA_BASEFUNS_TOK_(y) \
  TA_BASEFUNS_MAIN_(y) \
  TA_BASEFUNS_INST_(y)

#define TA_TMPLT_BASEFUNS_TOK_NCTORS(y,T) \
  TA_TMPLT_BASEFUNS_TOK_(y,T) \
  TA_TMPLT_BASEFUNS_MAIN_(y,T) \
  TA_TMPLT_BASEFUNS_INST_(y,T)

// this is the typical guy to use for most classes, esp if they keep Tokens
// the 2 versions bake in the inherited guy, so you don't need to do that
// the ncopy version includes a dummy Copy_ func (typical for template instances)
#define TA_BASEFUNS(y) \
  TA_BASEFUNS_CTORS_(y) \
  TA_BASEFUNS_TOK_NCTORS(y)
  
#define TA_BASEFUNS_NOCOPY(y) \
  private: NOCOPY(y) public: \
  TA_BASEFUNS(y)

#define TA_BASEFUNS2(y,x) \
  private: INHERITED(x) public: \
  TA_BASEFUNS(y)
  
#define TA_BASEFUNS2_NOCOPY(y,x) \
  private: INHERITED(x) NOCOPY(y) public: \
  TA_BASEFUNS(y)


// for templates (single parameter)
#define TA_TMPLT_BASEFUNS(y,T) \
  TA_TMPLT_BASEFUNS_CTORS_(y,T) \
  TA_TMPLT_BASEFUNS_TOK_NCTORS(y,T)

#define TA_TMPLT_BASEFUNS_NOCOPY(y,T) \
  private: TMPLT_NOCOPY(y,T) public: \
  TA_TMPLT_BASEFUNS(y,T) 

#define TA_TMPLT_BASEFUNS2(y,T,x) \
  private: INHERITED(x) public: \
  private: TMPLT_NOCOPY(y,T) public: \
  TA_TMPLT_BASEFUNS(y,T) 

#define TA_TMPLT_BASEFUNS2_NOCOPY(y,T,x) \
  private: INHERITED(x) TMPLT_NOCOPY(y,T) public: \
  TA_TMPLT_BASEFUNS(y,T) 


// this is the typical guy to use for "helper" or "value" classes
// that do not keep Tokens -- it avoids registration overhead
#define TA_BASEFUNS_LITE(y) \
  TA_BASEFUNS_CTORS_(y) \
  TA_BASEFUNS_MAIN_(y) \
  TA_BASEFUNS_INST_(y) \
  TA_BASEFUNS_NTOK_(y) 

#define TA_BASEFUNS_LITE_NOCOPY(y) \
  private: NOCOPY(y) public: \
  TA_BASEFUNS_LITE(y)

#define TA_BASEFUNS2_LITE(y,x) \
  private: INHERITED(x) public: \
  TA_BASEFUNS_LITE(y)

#define TA_BASEFUNS2_LITE_NOCOPY(y,x) \
  private: INHERITED(x) NOCOPY(y) public: \
  TA_BASEFUNS_LITE(y)

// template versions, ex. for smart ptrs, and similar class with no reg
#define TA_TMPLT_BASEFUNS_LITE(y,T) \
  TA_TMPLT_BASEFUNS_CTORS_(y,T) \
  TA_TMPLT_BASEFUNS_MAIN_(y,T) \
  TA_TMPLT_BASEFUNS_INST_(y,T) \
  TA_TMPLT_BASEFUNS_NTOK_(y,T) 

// macro for abstract base classes (with pure virtual methods, and no instance)
#define TA_ABSTRACT_BASEFUNS(y) \
  TA_BASEFUNS_CTORS_(y) \
  TA_BASEFUNS_MAIN_(y) \
  TA_BASEFUNS_TOK_(y)
  
#define TA_ABSTRACT_BASEFUNS_NOCOPY(y) \
  private: NOCOPY(y) public: \
  TA_ABSTRACT_BASEFUNS(y)


#define TA_TMPLT_ABSTRACT_BASEFUNS(y,T) \
  TA_TMPLT_BASEFUNS_CTORS_(y,T) \
  TA_TMPLT_BASEFUNS_MAIN_(y,T) \
  TA_TMPLT_BASEFUNS_TOK_(y,T)

// for use with templates
#define TA_TMPLT_TYPEFUNS(y,T) \
  static TypeDef* StatTypeDef(int) {  return &TA_##y##_##T; } \
  TypeDef* GetTypeDef() const { return &TA_##y##_##T; }

// this guy is your friend for most simple classes! esp good in plugins
#define SIMPLE_COPY(T) \
  void Copy_(const T& cp) {T::StatTypeDef(0)->CopyOnlySameType((void*)this, (void*)&cp); }

// for when you need to give it a diff name
#define SIMPLE_COPY_EX(T,NAME) \
  void NAME(const T& cp) {T::StatTypeDef(0)->CopyOnlySameType((void*)this, (void*)&cp); }

// dummy, for when nothing to copy in this class
#define NOCOPY(y) \
  void Copy_(const y& cp){}

#define TMPLT_NOCOPY(y,T) \
  void Copy_(const y<T>& cp){}

// this calls UpdatePointers_NewPar based on a major scoping owning parent, only if that
// parent is not already copying, and the parent is different than the copy parent
#define SIMPLE_COPY_UPDT_PTR_PAR(T,P) \
  void Copy_(const T& cp) {T::StatTypeDef(0)->CopyOnlySameType((void*)this, (void*)&cp); \
    UpdatePointers_NewPar_IfParNotCp(&cp, &TA_##P); }

// automated Init/Cut links guys -- esp good for code in Plugins
#define	SIMPLE_INITLINKS(T) \
  void InitLinks() { inherited::InitLinks(); InitLinks_taAuto(&TA_##T); }
  
#define	SIMPLE_CUTLINKS(T) \
  void CutLinks() { CutLinks_taAuto(&TA_##T); inherited::CutLinks(); }
  
#define SIMPLE_LINKS(T) \
  SIMPLE_INITLINKS(T); \
  SIMPLE_CUTLINKS(T)

// this is Sweetness and Light1(TM) -- everything is automatic and simple!
#define TA_BASEFUNS_SC(T) \
  SIMPLE_COPY(T); \
  TA_BASEFUNS(T)

// this is Sweetness and Light2(TM) -- everything is automatic and simple!
#define TA_SIMPLE_BASEFUNS(T) \
  SIMPLE_COPY(T); \
  SIMPLE_LINKS(T); \
  TA_BASEFUNS(T)

// for guys that have pointers to outside objects -- need to update if 
// not within PAR scope
#define TA_SIMPLE_BASEFUNS_UPDT_PTR_PAR(T,P) \
  SIMPLE_COPY_UPDT_PTR_PAR(T,P); \
  SIMPLE_LINKS(T); \
  TA_BASEFUNS(T)

#define TA_SIMPLE_BASEFUNS2(y,x) \
  SIMPLE_COPY(y); \
  SIMPLE_LINKS(y); \
  TA_BASEFUNS2(y,x)

#define TA_SIMPLE_BASEFUNS_LITE(y) \
  SIMPLE_COPY(y); \
  SIMPLE_LINKS(y); \
  TA_BASEFUNS_LITE(y)
  
#define TA_SIMPLE_BASEFUNS2_LITE(y,x) \
  SIMPLE_COPY(y); \
  SIMPLE_LINKS(y); \
  TA_BASEFUNS2_LITE(y,x)

// simplified Get owner functions B = ta_base object, T = class name
#define GET_MY_OWNER(T) (T *) GetOwner(&TA_##T)
#define GET_OWNER(B,T)  (T *) B ->GetOwner(T::StatTypeDef(0))

#define SET_POINTER(var,obj) (taBase::SetPointer((TAPtr*)&(var), (TAPtr)(obj)))
#define DEL_POINTER(var) (taBase::DelPointer((TAPtr*)&(var)))

// standard smart refs and ptrs -- you should use this for every class
#define TA_SMART_PTRS(y) \
  SmartPtr_Of(y) \
  SmartRef_Of(y, TA_ ## y);

/* Clipboard (Edit) operation summary

   Clipboard operations are of two basic types:
     Src -- source operations: Cut, Copy, Delete
     Dst -- destination ops: Paste, Link, etc.

   Clipboard API calls are of two types, and several subtypes:
     Query -- determines allowed operations (ex., to control UI enabling of Cut, Copy, etc.)
     Action -- perform the indicated action

   Src ops support both single and multi selected items.
   Dst ops support a single selected item, and single or multi items on the clipboard.

   There must always be at least one item selected in the UI to allow calling of clipboard functions.

   Data that is already on the clipboard is passed using a taiMimeSource iterator object
   defined in ta_qtclipdata.h -- this object supports both single and multi-item data.

   For all ops below, ms=NULL indicates a Src-op (CUT, COPY, etc.).

*/

////////////////////////////////////////////////////////////////////////////////////
//		ta Base	  --- 	The Base of all type-aware classes

class TA_API taBase {
  // #NO_TOKENS #INSTANCE #NO_UPDATE_AFTER Base type for all type-aware classes
  // has auto instances for all taBases unless NO_INSTANCE
friend class taSmartRef;
friend class taDataView;
friend class taBase_PtrList;
friend class taList_impl;

  ///////////////////////////////////////////////////////////////////////////
  //	Types
public:

  enum Orientation { // must be same values as Qt::Orientation
    Horizontal = 0x1,
    Vertical = 0x2
  };

  enum ValType { // the basic data types widely supported by data-handling api's, esp. matrices
    VT_STRING,		// #LABEL_String an ANSI string of any length
    VT_DOUBLE,		// #LABEL_double a 8-byte floating point value (aprox 15 sig decimal digits)
    VT_FLOAT,		// #LABEL_float a 4-byte floating point value (aprox 7 sig decimal digits)
    VT_INT,		// #LABEL_int a 32-bit signed integer
    VT_BYTE,		// #LABEL_byte an unsigned 8-bit integer; used mostly for image components (rgb)
    VT_VARIANT		// #LABEL_Variant a Variant, which can hold scalars, strings, matrices, and objects
  };
  
  enum BaseFlags { // #BITS control flags 
    THIS_INVALID	= 0x0001, // CheckThisConfig_impl has detected a problem
    CHILD_INVALID	= 0x0002, // CheckChildConfig_impl returns issue with a child
    COPYING		= 0x0004, // this object is currently within a Copy function
    USE_STALE		= 0x0008, // calls setStale on appropriate changes; usually set in Initialize
    BF_READ_ONLY	= 0x0010, // this object should be considered readonly by most code (except controlling objs) and by CSS -- note that ro is a property -- use that to query the ro status
    BF_GUI_READ_ONLY	= 0x0020, // a less restrictive form of ro intended to prevent users from modifying an object, but still permit programmatic access; RO ==> GRO
    DESTROYING		= 0x0040, // Set in Destroying at the very beginning of destroy
    DESTROYED		= 0x0080,  // set in base destroy (DEBUG only); lets us detect multi destroys
    NAME_READONLY	= 0x0100  // set to disable editing of name
#ifndef __MAKETA__
    ,INVALID_MASK	= THIS_INVALID | CHILD_INVALID
    ,COPY_MASK		= THIS_INVALID | CHILD_INVALID | NAME_READONLY // flags to copy when doing an object copy
    ,EDITABLE_MASK	= BF_READ_ONLY | BF_GUI_READ_ONLY // flags in the Editable group 
#endif
  };
  
  enum DumpQueryResult { // #IGNORE Dump_QuerySaveMember response
    DQR_NO_SAVE,	// definitely do not save
    DQR_SAVE,		// definitely save
    DQR_DEFAULT		// do default for this member (this is the base result)
  };

  ///////////////////////////////////////////////////////////////////////////
  // 	Reference counting mechanisms, all static just for consistency..
public:

  static int		GetRefn(TAPtr it)	{ return it->refn; } // #IGNORE
#ifdef DEBUG
  static void  		Ref(taBase& it);	     // #IGNORE
  static void  		Ref(taBase* it);	     // #IGNORE
#else
  static void  		Ref(taBase& it)		{ it.refn++; }	     // #IGNORE
  static void  		Ref(taBase* it) 	{ it->refn++; }	     // #IGNORE
#endif
  static void		UnRef(taBase* it) {unRef(it); Done(it);} // #IGNORE
  static void		Own(taBase& it, TAPtr onr);	// #IGNORE note: also does a RefStatic() on first ownership
  static void		Own(taBase* it, TAPtr onr);	// #IGNORE note: also does a Ref() on new ownership
  static void		Own(taSmartRef& it, TAPtr onr);	// #IGNORE for semantic compat with other Owns
protected:
  // legacy ref counting routines, for compatability -- do not use for new code
  // note that guards/tests in these are "defensive", not "by design"
#ifdef DEBUG
  static void   	unRef(taBase* it); // #IGNORE
  static void   	Done(taBase* it); // #IGNORE
#else
  static void   	unRef(taBase* it) 
    { if (it->refn > 0) it->refn--; }  // #IGNORE -- don't keep writing if <=0 since this could be a zombie/deleted object in some obscure scenarios
  static void   	Done(taBase* it) 
    { if ((it->refn == 0) && (!it->HasBaseFlag(DESTROYED))) delete it;} 
    // #IGNORE -- we check the flag for similar reasons as given in unRef
#endif
  static void		unRefDone(taBase* it) 	{unRef(it); Done(it);}	 // #IGNORE

  ///////////////////////////////////////////////////////////////////////////
  // 	Pointer management routines (all pointers should be ref'd!!)
public:

  static void 		InitPointer(taBase** ptr) { *ptr = NULL; } // #IGNORE
  static void 		SetPointer(taBase** ptr, taBase* new_val);	 // #IGNORE
  static void 		OwnPointer(taBase** ptr, taBase* new_val, taBase* onr); // #IGNORE
  static void 		DelPointer(taBase** ptr);				  // #IGNORE

  ///////////////////////////////////////////////////////////////////////////
  //	Basic constructor/destructor ownership/initlink etc interface
public:
  
  virtual void		InitLinks()		{ };
  // #IGNORE initialize links to other objs and do more elaborate object initialization, called after construction & SetOwner (added to object hierarchy).  ALWAYS CALL PARENT InitLinks!!!
  virtual void		CutLinks();
  // #IGNORE cut any links to other objs, called upon removal from a group or owner.  ALWAYS CALL PARENT CutLinks!!!  MIGHT BE CALLED MULTIPLE TIMES
  virtual void		InitLinks_taAuto(TypeDef* td);
  // #IGNORE automatic TA-based initlinks: calls inherited and goes through only my members & owns them
  virtual void		CutLinks_taAuto(TypeDef* td);
  // #IGNORE automatic TA-based cutlinks: goes through only my members & calls cutlinks and calls inherited

  void 			Register()
  { if(!taMisc::not_constr) GetTypeDef()->Register((void*)this); }
  // #IGNORE non-virtual, called in constructors to register token in token list
  void 			unRegister()
  { CheckDestroyed(); if(!taMisc::not_constr) GetTypeDef()->unRegister((void*)this); }
  // #IGNORE non-virtual, called in destructors to unregister token in token list
  virtual void		SetTypeDefaults();
  // #IGNORE initialize modifiable default initial values stored with the typedef -- see TypeDefault object in ta_defaults.  currently not used; was called in taBase::Own
  virtual void		SetTypeDefaults_impl(TypeDef* ttd, TAPtr scope); // #IGNORE
  virtual void		SetTypeDefaults_parents(TypeDef* ttd, TAPtr scope); // #IGNORE


protected:  // Impl
#ifdef DEBUG
  void			CheckDestroyed();// issues error msg or assertion if destroyed
#else
  inline void		CheckDestroyed() {} // should get optimized out
#endif
  void			Destroying(); // non-virtual called at beginning of destroy

  ///////////////////////////////////////////////////////////////////////////
  // actual constructors/destructors and related: defined in TA_BASEFUNS for derived classes
public:
  static  TypeDef*	StatTypeDef(int);	// #IGNORE
  static TAPtr 		MakeToken(TypeDef* td);
  // #IGNORE static version to make a token of the given type
  static TAPtr 		MakeTokenAry(TypeDef* td, int no);
  // #IGNORE static version to make an array of tokens of the given type

  taBase()			{ Register(); Initialize(); }
  taBase(const taBase& cp)		{ Register(); Initialize(); Copy_impl(cp); }
  virtual ~taBase() 		{ Destroy(); } //

  virtual TAPtr		Clone()			{ return new taBase(*this); } // #IGNORE
  virtual TAPtr 	MakeToken()		{ return new taBase; }	// #IGNORE
  virtual TAPtr 	MakeTokenAry(int no)	{ return new taBase[no]; } // #IGNORE
//  taBase&		operator=(const taBase& cp) { Copy(cp); return *this;}
  virtual TypeDef*	GetTypeDef() const;	// #IGNORE
  taBase*		New(int n_objs=1, TypeDef* type=NULL,
    const String& name = "(default name)")
    { return New_impl(n_objs, type, name);}
  // #CAT_ObjectMgmt Create n_objs objects of given type (type is optional)
protected:
  virtual taBase*	New_impl(int n_objs, TypeDef* type,
    const String& nm) { return NULL; }

  ////////////////////////////////////////////////////////////////////,T///////
  //	Object managment flags (taBase supports up to 8 flags for basic object mgmt purposes)
public:

  bool			HasBaseFlag(int flag) const;
  // #CAT_ObjectMgmt true if flag set, or if multiple, any set
  void			SetBaseFlag(int flag);
  // #CAT_ObjectMgmt sets the flag(s)
  void			ClearBaseFlag(int flag);
  // #CAT_ObjectMgmt clears the flag(s)
  void			ChangeBaseFlag(int flag, bool set)
    {if (set) SetBaseFlag(flag); else ClearBaseFlag(flag);}
  // #CAT_ObjectMgmt sets or clears the flag(s)
  int			baseFlags() const {return base_flags;}
  // #IGNORE flag values; see also HasBaseFlag
  inline bool		useStale() const {return HasBaseFlag(USE_STALE);}
    // #IGNORE
  inline void		setUseStale(bool val) 
    {if (val) SetBaseFlag(USE_STALE); else ClearBaseFlag(USE_STALE);}
    // #IGNORE
  int			GetEditableState(int mask) const; 
  // #IGNORE returns READ_ONLY and GUI_READ_ONLY, which also (in default behavior) factors in the owner's state supercursively until/unless not found; WARNING: result may include other flags, so you must &
  inline bool		isDestroying() const  {return HasBaseFlag(DESTROYING);}
    // #IGNORE
  bool			isReadOnly() const 
    {return (GetEditableState(BF_READ_ONLY) & BF_READ_ONLY);}
    // #IGNORE true if the object is (supercursively) strongly read-only
  bool			isGuiReadOnly() const 
    {return (GetEditableState((BF_READ_ONLY | BF_GUI_READ_ONLY))
       & (BF_READ_ONLY | BF_GUI_READ_ONLY));}
    // #IGNORE true if the object is (supercursively) read-only in the gui
protected:
  virtual int		GetThisEditableState_impl(int mask) const;
    // extend this guy to factor in special purpose flags or runtime conditions
  virtual int		GetOwnerEditableState_impl(int mask) const;
   // you can stub this one out to prevent supercursively searching for the flag
  
  ///////////////////////////////////////////////////////////////////////////
  //	Basic object properties: index in list, owner, name, description, etc
public:

  virtual int		GetIndex() const {return -1;}
  // #CAT_ObjectMgmt object's index within an owner list.  cached by some objs.
  virtual void		SetIndex(int value) {};
  // #IGNORE set the objects index value.  note: typically don't do a notify, because list itself will take care of notifying gui clients
  virtual int		GetEnabled() const {return -1;}
  // #IGNORE for items that support an enabled/disabled state; -1=n/a, 0=disabled, 1=enabled (note: (bool)-1 = true)
  virtual void		SetEnabled(bool value) {};
  // #IGNORE
  virtual bool		SetName(const String& nm) {return false;} 
  // #CAT_ObjectMgmt #SET_name Set the object's name
  virtual String	GetName() const 	{ return _nilString; }
  // #CAT_ObjectMgmt #GET_name Get the name of the object
  virtual String	GetDisplayName() const;
  // #IGNORE can be overridden to provide a more elaborate or cleaned-up user-visible name for display purposes (default is just GetName())
  virtual String	GetUniqueName() const; // #IGNORE the name, possibly with dotted parent, to help globally identify an item, mostly for token lists
  virtual String	GetDesc() const {return _nilString;}
  // #IGNORE a type-specific description of the specific functionality/properties of the object
  virtual void 		SetDefaultName() {} // #IGNORE note: called non-virtually in every constructor
  void 			SetDefaultName_(); // #IGNORE default behavior for >=taNBase -- you can call this manually for taOBase (or others that implement Name)
  virtual void 		SetDefaultName_impl(int idx); // #IGNORE called from within, or by list -- NAME_TYPE will determine what we do with idx
  virtual String	GetTypeDecoKey() const { return _nilString; }
  // #IGNORE lookup key for visual decoration of an item reflecting its overall type information, used for font colors in the gui browser, for example
  virtual bool		GetQuiet() const {return false;} 
  // #IGNORE general-purpose fuzzy flag for suppressing Warning messages from an object, maybe because it is special, user shuts them off, etc.
  virtual String	GetStateDecoKey() const;
  // #IGNORE lookup key for visual decoration of an item reflecting current state information, used for backgroundt colors in the gui browser, for example

  virtual void* 	GetTA_Element(Variant idx, TypeDef*& eltd) 
  { eltd = NULL; return NULL; } // #IGNORE a bracket operator (e.g., owner[i])
  virtual taList_impl*  children_() {return NULL;} 
  // #IGNORE for lists, and for taOBase w/ default children
  const taList_impl*  	children_() const 
  { return const_cast<const taList_impl*>(const_cast<taBase*>(this)->children_());} 
  // #IGNORE mostly for testing if has children
  virtual TAPtr 	SetOwner(TAPtr)		{ return(NULL); } // #IGNORE
  virtual TAPtr 	GetOwner() const	{ return(NULL); } // #CAT_ObjectMgmt 
  virtual TAPtr		GetOwner(TypeDef* td) const; // #CAT_ObjectMgmt 
  virtual TAPtr		GetThisOrOwner(TypeDef* td); // #CAT_ObjectMgmt get this obj or first owner that is of type td
  virtual TAPtr 	GetParent() const; 
    // #CAT_ObjectMgmt typically the first non-list/group owner above this one
  bool 			IsParentOf(const taBase* obj) const; // #CAT_ObjectMgmt true if this object is a direct or indirect parent of the obj (or is the obj)
  bool			IsChildOf(const taBase* obj) const; // #CAT_ObjectMgmt true if this object is a direct or indirect child of the obj (or is the obj)
  ///////////////////////////////////////////////////////////////////////////
  //	Paths in the structural hierarchy
public:

  virtual String 	GetPath_Long(TAPtr ta=NULL, TAPtr par_stop=NULL) const;
  // #IGNORE get path from root (default), but stop at par_stop if non-null
  virtual String	GetPath(TAPtr ta=NULL, TAPtr par_stop=NULL) const;
  // #CAT_ObjectMgmt get path without name informtation, stop at par_stop if non-null
  virtual taBase*	FindFromPath(const String& path, MemberDef*& ret_md, int start=0) const;
  // #CAT_ObjectMgmt find object from path (starting from this, and position start of the path -- ret_md is return member def: if NULL and return is !NULL, then it is a member of a list or group, not a member in object
  virtual Variant	GetValFromPath(const String& path, MemberDef*& ret_md, bool warn_not_found=false) const;
  // #CAT_ObjectMgmt get a member value from given path -- only follows direct members (of members) of this object -- does not look into items in lists or groups

  // utility functions for doing path stuff
  static int		GetNextPathDelimPos(const String& path, int start);
  // #IGNORE get the next delimiter ('.' or '[') position in the path
  static int		GetLastPathDelimPos(const String& path);
  // #IGNORE get the last delimiter ('.' or '[') position in the path

  virtual TypeDef*	GetScopeType();
  // #IGNORE gets my scope type (if NULL, it means no scoping, or root)
  virtual TAPtr		GetScopeObj(TypeDef* scp_tp=NULL);
  // #IGNORE gets the object that is at the scope type above me (uses GetScopeType() or scp_tp)
  virtual bool		SameScope(TAPtr ref_obj, TypeDef* scp_tp=NULL);
  // #IGNORE determine if this is in the same scope as given ref_obj (uses my scope type)
  static int		NTokensInScope(TypeDef* type, TAPtr ref_obj, TypeDef* scp_tp=NULL);
  // #IGNORE number of tokens of taBase objects of given type in same scope as ref_obj

protected: // Impl
  
  ////////////////////////////////////////////////////////////////////// 
  // 	Saving and Loading to/from files
public:
  virtual bool		SetFileName(const String& val)  {return false;}
  // #CAT_File set file name for object
  virtual String	GetFileName() const 	{ return _nilString; }
  // #CAT_File get file name object was last saved with
  virtual String	GetFileNameFmProject(const String& ext, const String& tag = "", const String& subdir = "", bool dmem_proc_no = false);
  // #CAT_File get file name from project file name -- useful for saving files associated with the project; ext = extension; tag = additional tag; subdir = additional directory after any existing in project name; fname = proj->base_name (subdir) + tag + ext; if dmem_proc_no, add dmem proc no to file name.  empty if project not found

  static taFiler*	StatGetFiler(TypeItem* td, String exts= _nilString,
    int compress=-1, String filetypes =_nilString);
  // #IGNORE gets file dialog for the TypeItem -- clients must ref/unrefdone; ext is for non-default extension (otherwise looks up EXT_); compress -1=default, 0=none, 1=yes
  taFiler*		GetFiler(TypeItem* td = NULL, const String& exts = _nilString,
    int compress=-1, const String& filetypes = _nilString);
  // #IGNORE gets filer for this object (or TypeItem if non-null) -- clients must ref/unrefdone; ext is for non-default extension (otherwise looks up EXT_); compress -1=default, 0=none, 1=yes; exts/ft's must match, and are ,-separated lists

  virtual int	 	Load_strm(istream& strm, TAPtr par=NULL, taBase** loaded_obj_ptr = NULL);
  // #CAT_XpertFile Load object data from a file -- sets pointer to loaded obj if non-null: could actually load a different object than this (e.g. if this is a list or group)
  virtual taFiler* 	GetLoadFiler(const String& fname, String exts = _nilString,
    int compress=-1, String filetypes = _nilString);
  // #IGNORE get filer with istrm opened for loading for file fname; if empty, prompts user with filer chooser.  NOTE: must unRefDone the filer when done with it in calling function!
  virtual int	 	Load(const String& fname="", taBase** loaded_obj_ptr = NULL);
  // #MENU #MENU_ON_Object #ARGC_0 #CAT_File Load object data from given file name (if empty, prompt user for a name) -- sets pointer to loaded obj if non-null: could actually load a different object than this (e.g. if this is a list or group)
  virtual int 		Load_cvt(taFiler*& flr);
  // #IGNORE convert stream from old to new format (if needed)

  virtual int 		Save_strm(ostream& strm, TAPtr par=NULL, int indent=0);
  // #CAT_XpertFile Save object data to a file stream
  virtual taFiler* 	GetSaveFiler(const String& fname, String ext = _nilString,
    int compress=-1, String filetypes=_nilString);
  // #IGNORE get filer with ostrm opened for saving for file fname; if empty, prompts user with filer chooser.  NOTE: must unRefDone the filer when done with it in calling function!
  virtual taFiler* 	GetAppendFiler(const String& fname, const String& ext="",
    int compress=-1, String filetypes=_nilString);
  // #IGNORE get filer with ostrm opened for appending for file fname; if empty, prompts user with filer chooser.  NOTE: must unRefDone the filer when done with it in calling function!
  virtual int		Save(); 
  // #MENU #MENU_ON_Object #ARGC_0 #CAT_File saves the object to a file using current file name (from GetFileName() function); if context="" then default is used
  virtual int		SaveAs(const String& fname = ""); 
  // #MENU #ARGC_0 #CAT_File Saves object data to a new file -- if fname is empty, it prompts the user; if context="" then default is used

  virtual String	GetValStr(void* par = NULL, MemberDef* md = NULL,
				  TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
				  bool force_inline = false) const;
  // #IGNORE get a value string for this object (ptr=0) -- called by TypeDef GetValStr -- default for inline is just to iterate over members and output values just as in TypeDef code -- can overload for more complex classes for inlines
  static String		GetValStr_ptr(const TypeDef* td, const void* base, void* par = NULL, 
				      MemberDef* md = NULL,
				      TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
				      bool force_inline = false);
  // #IGNORE get a value string for pointer to ta base object (ptr=1) -- called by TypeDef GetValStr
  virtual bool		SetValStr(const String& val, void* par = NULL, MemberDef* md = NULL,
				  TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
				  bool force_inline = false);
  // #IGNORE set value from a string for this object (ptr=0) -- called by TypeDef SetValStr -- default for inline is just to iterate over members and output values just as in TypeDef code -- can overload for more complex classes that might still be inlinable
  static bool		SetValStr_ptr(const String& val, TypeDef* td, void* base,
				      void* par = NULL, MemberDef* md = NULL, 
				      TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
				      bool force_inline = false);
  // #IGNORE set value from a string for ptr to taBase (ptr=1) -- called by TypeDef SetValStr

  ////////////////////////////////////////////////////////////////////// 
  // 	Low-level dump load/save
public:  
  virtual int	 	Dump_Load_impl(istream& strm, TAPtr par=NULL) // #IGNORE
  { return GetTypeDef()->Dump_Load_impl(strm, (void*)this, par); }
  virtual void		Dump_Load_pre() {};
  // #IGNORE -- called just before single-object Load_strm -- use to reset stuff prior to loading
  virtual int	 	Dump_Load_Value(istream& strm, TAPtr par=NULL) // #IGNORE
  { return GetTypeDef()->Dump_Load_Value(strm, (void*)this, par); }
  virtual void 		Dump_Load_post() {} 
  // #IGNORE called after load, in normal (non loading) context if has DUMP_LOAD_POST directive
  
  virtual void 		Dump_Save_GetPluginDeps();
  // #IGNORE called prior to saving, to build the plugin deps in .plugin_deps
  // note: this routine (and overrides) is in ta_dump.cpp
  virtual void 		Dump_Save_pre() {}
  // #IGNORE called before _Path, enables jit updating before save
  virtual int 		Dump_Save_impl(ostream& strm, TAPtr par=NULL, int indent=0)
  { Dump_Save_pre(); 
    return GetTypeDef()->Dump_Save_impl(strm, (void*)this, par, indent); } // #IGNORE
  virtual int 		Dump_Save_inline(ostream& strm, TAPtr par=NULL, int indent=0)
  { Dump_Save_pre(); 
    return GetTypeDef()->Dump_Save_inline(strm, (void*)this, par, indent); } // #IGNORE
  virtual int 		Dump_Save_Path(ostream& strm, TAPtr par=NULL, int indent=0)
  { return GetTypeDef()->Dump_Save_Path(strm, (void*)this, par, indent); } // #IGNORE
  virtual int 		Dump_Save_Value(ostream& strm, TAPtr par=NULL, int indent=0)
  { return GetTypeDef()->Dump_Save_Value(strm, (void*)this, par, indent); } // #IGNORE

  virtual int		Dump_SaveR(ostream& strm, TAPtr par=NULL, int indent=0)
  { return GetTypeDef()->Dump_SaveR(strm, (void*)this, par, indent); } 	// #IGNORE
  virtual int 		Dump_Save_PathR(ostream& strm, TAPtr par=NULL, int indent=0)
  { return GetTypeDef()->Dump_Save_PathR(strm, (void*)this, par, indent); } // #IGNORE
  virtual DumpQueryResult Dump_QuerySaveMember(MemberDef* md); 
  // #IGNORE default checks NO_SAVE directive; override to make save decision at runtime
  virtual bool		Dump_QuerySaveChildren() 
  {return true;} // #IGNORE override to make save decision at runtime


  ///////////////////////////////////////////////////////////////////////////
  // 	Updating of object properties
public:

  virtual void		UpdateAfterEdit();
  // #CAT_ObjectMgmt called after editing, or any user change to members (eg. in the interface, script)
  virtual void		ChildUpdateAfterEdit(TAPtr child, bool& handled);
  // #IGNORE called by a child in its UAE routine; provides child notifications  NOTE: only member objects are detected; subclasses that want to notify on owned TAPtr members must override and check for those instances manually
  virtual void		UpdateAfterMove(taBase* old_owner);
  // #IGNORE called after object has been moved from one location to another in the object hierarchy (i.e., list Transfer fun) -- actual functions should be put in the _impl version which should call inherited:: etc just as for UAE -- use for updating pointers etc
  virtual void		UpdateAllViews();
  // #CAT_Display called after data changes, to update views
  virtual void		RebuildAllViews();
  // #CAT_Display call after data changes, to rebuild views, typically when a child is added
  virtual void 		DataChanged(int dcr, void* op1 = NULL, void* op2 = NULL);
  // #IGNORE sends the indicated notification to all datalink clients, if any; virtual so we can override to trap/monitor
  void			StructUpdate(bool begin) { BatchUpdate(begin, true); }
  // #CAT_ObjectMgmt bracket structural changes with (nestable) true/false calls;
  void			DataUpdate(bool begin) { BatchUpdate(begin, false); }
  // #CAT_ObjectMgmt bracket data value changes with (nestable) true/false calls;

  virtual bool		isDirty() const {return false;}
  // #IGNORE implemented by very few, esp. Project -- Dirty is used to indicate the need to resave an object
  virtual void 		setDirty(bool value);
  // #CAT_ObjectMgmt set the dirty flag indicating a change in object values; 'true' gets forwarded up; 'false' does nothing
  
  virtual bool		isStale() const {return false;}
  // #IGNORE implemented by few, esp. Program and Network -- Stale indicates the need to "rebuild" an object with such semantics (ex regen script, rebuild a net, etc.)
  virtual void		setStale();
  // #CAT_ObjectMgmt set the stale flag indicating a change in object values; gets forwarded up ("true" is implied, only the impl obj defines when it is cleared)

protected:  // Impl
  virtual void		UpdateAfterEdit_impl() {}
  // this is the preferred place to put all UAE actions, so they all take place before the notify
  virtual void		UpdateAfterMove_impl(taBase* old_owner) {}
  // for actions that should be performed after object has been moved from one location to another in the structure hierarchy

  ///////////////////////////////////////////////////////////////////////////
  //	Data Links -- notify other guys when you change
public:

  virtual taDataLink* 	data_link() {return NULL;} // #IGNORE link for viewer system created when needed, deleted when 0 clients -- all delegated functions must be of form: if(data_link()) data_link->SomeFunc(); NOT autocreated by call to this func -- call GetDataLink() to force creation
  virtual taDataLink* 	GetDataLink(); // #IGNORE forces creation; can still be NULL if the type doesn't support datalinks
  bool			AddDataClient(IDataLinkClient* dlc); // #IGNORE note: only applicable for classes that implement datalinks
  bool			RemoveDataClient(IDataLinkClient* dlc); // #IGNORE WARNING: link is undefined after this 
protected:	// Impl
  virtual void		BatchUpdate(bool begin, bool struc);
  // #IGNORE bracket changes with (nestable) true/false calls; data clients can use it to supress change updates
  virtual void		SmartRef_DataDestroying(taSmartRef* ref, taBase* obj);
  // #IGNORE the obj (to which we had a ref) is about to destroy (the ref has already been NULL'ed)
  virtual void		SmartRef_DataChanged(taSmartRef* ref, taBase* obj,
    int dcr, void* op1_, void* op2_) {}
  // #IGNORE the obj (to which we have a ref) has signalled the indicated data change
  virtual void		SmartRef_DataRefChanging(taSmartRef* ref, 
    taBase* obj, bool setting) {}
  // #IGNORE the obj ref has either been removed (smartref now null) or added (smartref already set to that object)

  
  ///////////////////////////////////////////////////////////////////////////
  //	Checking the configuration of objects prior to using them
public:

  inline bool 	TestError(bool test, const char* fun_name,
			  const char* a, const char* b=0, const char* c=0,
			  const char* d=0, const char* e=0, const char* f=0,
			  const char* g=0, const char* h=0) const {
    if(!test) return false;
    return taMisc::TestError(this, test, fun_name, a, b, c, d, e, f, g, h);
  }
  // #CAT_ObjectMgmt if test, then report error, including object name, type, and path information; returns test -- use e.g. if(TestError((condition), "fun", "msg")) return false;
  inline bool 	TestWarning(bool test, const char* fun_name, 
			    const char* a, const char* b=0, const char* c=0,
			    const char* d=0, const char* e=0, const char* f=0,
			    const char* g=0, const char* h=0) const {
    if(!test) return false;
    return taMisc::TestWarning(this, test, fun_name, a, b, c, d, e, f, g, h);
  }
  // #CAT_ObjectMgmt if test, then report warning, including object name, type, and path information; returns test -- use e.g. if(TestWarning((condition), "fun", "msg")) return false;

#ifndef __MAKETA__
  void			CheckConfig(bool quiet, bool& rval)
    {if (!CheckConfig_impl(quiet)) rval = false;}
    // this one is typically used in CheckXxx_impl routines; we don't do gui wrap stuff
#endif
  bool			CheckConfig(bool quiet = false)
  { return CheckConfig_Gui(false, quiet);}
  // #CAT_ObjectMgmt check the configuration of this object and all its children (defaults to no confirm of success)
  bool			CheckConfig_Gui(bool confirm_success = true, bool quiet = false);
  // #MENU #MENU_ON_Object #CAT_ObjectMgmt #ARGC_0 #LABEL_CheckConfig check the configuration of this object and all its children -- failed items highlighted in red, items with failed children in yellow
  void			ClearCheckConfig(); // #IGNORE this can be called when a CheckConfig_impl routine blindly assert ok, ex. for an object that has an "off" or "disable" state; this routine updates the gui if the state has now changed

  virtual void 	CheckError_msg(const char* a, const char* b=0, const char* c=0,
			       const char* d=0, const char* e=0, const char* f=0,
			       const char* g=0, const char* h=0) const;
  // #IGNORE generate error message

  inline bool 	CheckError(bool test, bool quiet, bool& rval,
			   const char* a, const char* b=0, const char* c=0,
			   const char* d=0, const char* e=0, const char* f=0,
			   const char* g=0, const char* h=0) const {
    if(!test) return false;
    rval = false;
    if(!quiet) CheckError_msg(a,b,c,d,e,f,g,h);
    return test;
  }
  // #CAT_ObjectMgmt for CheckConfig routines: if test, then report config error, including object name, type, and path information; returns test & sets rval to false if test is true -- use e.g. CheckError((condition), quiet, rval, "msg"));

protected: // impl
  virtual bool		CheckConfig_impl(bool quiet);
  // #IGNORE usually not overridden, see Check[This/Child]_impl
  virtual void		CheckThisConfig_impl(bool quiet, bool& ok) {}
  // impl for us; can include embedded objects (but don't incl them in Child check); only clear ok (if invalid), don't set
  virtual void		CheckChildConfig_impl(bool quiet, bool& ok) {}
  // impl for checking children; only clear ok (if invalid), don't set

  ///////////////////////////////////////////////////////////////////////////
  //	Copying and changing type 
public:

  bool 			CanCopy(const taBase* cp, bool quiet = true) const;
    // #IGNORE the retail version
  void 			CanCopy(const taBase* cp, bool quiet, bool& ok) const 
    {if (CanCopy(cp, quiet)) return; ok = false;} 
    // #IGNORE convenience, for nested calls

//  void			Copy(const taBase& cp) { 
//    if (CanCopy(&cp)) Copy_impl(cp);}
  // #IGNORE the copy (=) operator FOR JUST THIS CLASS -- CALL PARENT Copy() for its functions; note: base version is so trivial, we don't do any of the stuff in BASEFUNS macro, but you should imagine it does
  virtual bool		Copy(const taBase* cp);
  // #IGNORE this is a generic copy, that enables common-subclass copying, or even copying from disparate clases that might have a sensible copy semantic
  virtual void		CopyFromSameType(void* src_base)
  { GetTypeDef()->CopyFromSameType((void*)this, src_base); }
  // #IGNORE copy values from object of same type
  virtual void		CopyOnlySameType(void* src_base)
  { GetTypeDef()->CopyOnlySameType((void*)this, src_base); }
  // #IGNORE copy only those members from same type (no inherited members)
  virtual void		MemberCopyFrom(int memb_no, void* src_base)
  { GetTypeDef()->MemberCopyFrom(memb_no, (void*)this, src_base); }
   // #IGNORE copy given member index no from source object of same type
//note: CopyFrom/To should NOT be virtual -- specials should be handled in the impl routines, or the Copy_ routines
  bool			CopyFrom(TAPtr cpy_from);
  // #MENU #MENU_ON_Object #MENU_SEP_BEFORE #TYPE_ON_this #NO_SCOPE #CAT_ObjectMgmt Copy from given object into this object (this is a safe interface to UnSafeCopy)
  bool			CopyTo(TAPtr cpy_to);
  // #MENU #TYPE_ON_this #NO_SCOPE #CAT_ObjectMgmt Copy to given object from this object
  // need both directions to more easily handle scoping of types on menus
  virtual bool		ChildCanDuplicate(const taBase* chld, bool quiet = true) const;
    // #IGNORE
  virtual taBase*	ChildDuplicate(const taBase* chld);
    // #IGNORE duplicate given child, returning the new one (NULL if can't do it)

  bool			DuplicateMe();
  // #MENU #CONFIRM #CAT_ObjectMgmt Make another copy of myself (done through owner)
  virtual bool		ChangeMyType(TypeDef* new_type);
  // #MENU #TYPE_this #CAT_ObjectMgmt #ARG_VAL_FM_FUN Change me into a different type of object, copying current info (done through owner)
  virtual void		UnSafeCopy(const taBase*) {} // #IGNORE custom version made for each class: if cp->Inherits(me) Copy(cp); else if me->Inherits(cp) cp.CastCopyTo(me) else CopyOther_impl(cp)
  virtual void		CastCopyTo(taBase*) const {}; // #IGNORE custom version made for every class, casts
  void			CopyToCustom(taBase* src) const; // #IGNORE DO NOT CALL -- this is only a public, static wrapper for the _impl
  void			CopyFromCustom(const taBase* cp); // #IGNORE DO NOT CALL -- this is only a public, static wrapper for the _impl

protected: // impl
  void			Copy_impl(const taBase& cp);
  virtual bool		CanDoCopy_impl(const taBase* cp, bool quiet, bool copy);
    // intertwines the checks and copy, so it can be used for checking, or copying 
  virtual void		CanCopy_impl(const taBase* cpy_from, bool quiet,
    bool& ok, bool virt) const; 
    // basic query interface impl, only passed frm >= our class; may get called repeatedly, so subs are allowed to add an empty stub
  virtual void		CopyFromCustom_impl(const taBase* cp) {} // this is the generic copy, that enables common subclass or disparate class copying; follow pattern of Copy_impl, except we are always called in a Struct bracket
  virtual void		CopyToCustom_impl(taBase* targ) const {} // this is a fairly rarely used one for the case where the src actually does the copy; follow pattern of Copy_impl, except we are always called in a Struct bracket
  
  virtual void		CanCopyCustom_impl(bool to, const taBase* cp,
    bool quiet, bool& allowed, bool& forbidden) const {}
    // we need an allowed/forbidden paradigm here, so we can always call inherited -- only issue msg on forbidden; caller will supply msg if not allowed -- this routine is called for self (to=0), and we also call the proposed buddy (to=1) -- either one can forbid; us forbidding trumps cp allowing; since cp-controlled is so unusual, it is given priority
  bool			CanCopy_ctor(const taBase* cpy_from) const
    {bool ok = true; CanCopy_impl(cpy_from, true, ok, false); return ok;}
    // this is the guy used in the Copy ctor to guard the Copy_ for that level
  
  ///////////////////////////////////////////////////////////////////////////
  //	Type information
public:

  bool		InheritsFrom(const TypeDef& it) const
  { return GetTypeDef()->InheritsFrom(it); }
  bool 		InheritsFrom(TypeDef* it) const
  { return GetTypeDef()->InheritsFrom(it); }
  // #CAT_ObjectMgmt does this inherit from given type 
  bool 		InheritsFromName(const char* nm) const
  { return GetTypeDef()->InheritsFromName(nm); }
  // #CAT_ObjectMgmt does this inherit from given type name?

  bool		InheritsFormal(TypeDef* it) const	// #IGNORE
  { return GetTypeDef()->InheritsFormal(it); }
  bool		InheritsFormal(const TypeDef& it) const	// #IGNORE
  { return GetTypeDef()->InheritsFormal(it); }

  TypeDef*	GetStemBase() const;
  // #IGNORE get first (from me) parent with STEM_BASE directive -- defines equivalence class -- if not found, then taBase is returned

  virtual MemberDef*	FindMember(const String& nm) const // #IGNORE
  { return GetTypeDef()->members.FindName(nm); }
  virtual MemberDef*	FindMember(TypeDef* it) const 	// #IGNORE
  { return GetTypeDef()->members.FindType(it); }
  virtual MemberDef* 	FindMember(void* mbr) const 	// #IGNORE
  { int idx; return GetTypeDef()->members.FindAddr((void*)this, mbr, idx); }
  virtual MemberDef* 	FindMemberPtr(void* mbr) const	// #IGNORE
  { int idx; return GetTypeDef()->members.FindAddrPtr((void*)this, mbr, idx); }	// #IGNORE

  virtual void* 	FindMembeR(const String& nm, MemberDef*& ret_md) const;
  // #CAT_ObjectMgmt find member based on name or type, recursive -- does breadth-first then depth search -- returns pointer of member item, and ret_md is filled in if avail -- if NULL it indicates that it is an item in a list of type taBase and not a proper member

  virtual bool		FindCheck(const String& nm) const // #IGNORE check this for the name
  { return (GetName() == nm); }

  virtual void		Search(const String& srch, taBase_PtrList& items,
			       taBase_PtrList* owners = NULL, 
			       bool contains = true, bool case_sensitive = false,
			       bool obj_name = true, bool obj_type = true,
			       bool obj_desc = true, bool obj_val = true,
			       bool mbr_name = true, bool type_desc = false);
  // #CAT_ObjectMgmt search for objects using srch string, from this point down the structural hierarchy (my members, and their members and objects in lists, etc).  items are linked into items list, and all owners of items found are linked into owners list (if present -- can be used as a lookup table for expanding owners to browse found items).  contains = use "contains" for all matches instead of exact match, rest are values to search in (obj_desc includes DisplayName as well as any explicit description), obj_val is only for value members and inline members

  virtual void		Search_impl(const String& srch, taBase_PtrList& items,
				    taBase_PtrList* owners = NULL, 
				    bool contains = true, bool case_sensitive = false,
				    bool obj_name = true, bool obj_type = true,
				    bool obj_desc = true, bool obj_val = true,
				    bool mbr_name = true, bool type_desc = false);
  // #IGNORE implementation -- only first Search() is externally called

  virtual bool		SearchTestStr_impl(const String& srch,  String tst,
					   bool contains, bool case_sensitive);
  // #IGNORE Search test string according to searching criteria

  virtual bool		SearchTestItem_impl(taBase* obj, const String& srch,  
					    bool contains, bool case_sensitive,
					    bool obj_name, bool obj_type,
					    bool obj_desc, bool obj_val,
					    bool mbr_name, bool type_desc);
  // #IGNORE Search test for just this one taBase item according to criteria

  virtual void		CompareSameTypeR(Member_List& mds, TypeSpace& base_types, 
					 void_PArray& trg_bases, void_PArray& src_bases,
					 taBase* cp_base, 
					 int show_forbidden = taMisc::NO_HIDDEN,
					 int show_allowed = taMisc::SHOW_CHECK_MASK,
					 bool no_ptrs = true);
  // #IGNORE compare all member values from object of the same type as me, adding ones that are different to the mds, trg_bases, src_bases lists -- recursive -- will also check members of lists/groups that I own

  virtual String	GetEnumString(const String& enum_tp_nm, int enum_val) const
  { return GetTypeDef()->GetEnumString(enum_tp_nm, enum_val); }
  // #CAT_ObjectMgmt get the name corresponding to given enum value in enum type enum_tp_nm
  virtual int		GetEnumVal(const String& enum_nm, String& enum_tp_nm) const
  { return GetTypeDef()->GetEnumVal(enum_nm, enum_tp_nm); }
  // #CAT_ObjectMgmt get the enum value corresponding to the given enum name (-1 if not found), and sets enum_tp_nm to name of type this enum belongs in (empty if not found)
  virtual uint		GetSize() const		{ return GetTypeDef()->size; }  // #IGNORE

  virtual String	GetTypeName() const 			// #IGNORE 
  { return GetTypeDef()->name; }
  virtual ostream&  	OutputType(ostream& strm) const		// #IGNORE
  { return GetTypeDef()->OutputType(strm); }
  virtual ostream&  	OutputInherit(ostream& strm) const 	// #IGNORE
  { return GetTypeDef()->OutputInherit(strm); }
  virtual ostream&  	OutputTokens(ostream& strm) const	// #IGNORE
  { GetTypeDef()->tokens.List(strm); return strm; }

  static const String 	ValTypeToStr(ValType vt);
  // #IGNORE get the value type as a standard type string
  static ValType	ValTypeForType(TypeDef* td);
  // #IGNORE return the appropriate ValType for given typedef

  ///////////////////////////////////////////////////////////////////////////
  //	Printing out object state values
public:

  virtual ostream& 	Output(ostream& strm, int indent = 0) const // #IGNORE
  { return GetTypeDef()->Output(strm, (void*)this, indent); }
  virtual ostream& 	OutputR(ostream& strm, int indent = 0) const // #IGNORE
  { return GetTypeDef()->OutputR(strm, (void*)this, indent); }

  static String		GetStringRep(const taBase& it) {return it.GetStringRep_impl();}
   // #IGNORE string representation
  static String		GetStringRep(TAPtr it); 
  // #IGNORE string representation; ok if null, calls ->GetStringRep_impl

protected:  // Impl
  virtual String	GetStringRep_impl() const;
  // #IGNORE string representation, ex. for variants; default is typename:fullpath

  ///////////////////////////////////////////////////////////////////////////
  //	User Data: optional configuration settings for objects
public:
  virtual UserDataItem_List* GetUserDataList(bool force_create = false) const
    {return NULL;}
  // #CAT_UserData #EXPERT gets the userdatalist for this class
  bool			HasUserDataList() const
    {return (GetUserDataList(false) != NULL);}
  // #CAT_UserData #EXPERT returns true if UserData exists at all
  bool			HasUserData(const String& key) const;
  // #CAT_UserData returns true if UserData exists for this key (case sens)
  const Variant		GetUserData(const String& key) const;
  // #CAT_UserData get specified user data; returns class default value if not present, or nilVariant if no default user data or class doesn't support UserData
  const Variant 	GetUserDataDef(const String& key, const Variant& def)
    {if (HasUserData(key)) return GetUserData(key); else return def;}
  // #CAT_UserData #EXPERT return value if exists, or default if doesn't
  UserDataItemBase* 	GetUserDataOfType(TypeDef* typ, const String& key,
					  bool force_create);
  // #CAT_UserData #EXPERT #ARGC_2 gets specified user data of given type, making one if doesn't exist and fc=true
  UserDataItemBase* 	GetUserDataOfTypeC(TypeDef* typ, const String& key) const;
  // #IGNORE const non-forced version, for convenience
  inline bool		GetUserDataAsBool(const String& key) const
    {return GetUserData(key).toBool();} // #CAT_UserData #EXPERT get specified user data as bool (see GetUserData)
  inline int		GetUserDataAsInt(const String& key) const
    {return GetUserData(key).toInt();} // #CAT_UserData #EXPERT get specified user data as int (see GetUserData)
  inline float		GetUserDataAsFloat(const String& key) const
    {return GetUserData(key).toFloat();} // #CAT_UserData #EXPERT get specified user data as float (see GetUserData)
  inline double		GetUserDataAsDouble(const String& key) const
    {return GetUserData(key).toDouble();} // #CAT_UserData #EXPERT get specified user data as double (see GetUserData)
  inline const String	GetUserDataAsString(const String& key) const
    {return GetUserData(key).toString();} // #CAT_UserData #EXPERT get specified user data as String (see GetUserData)
  UserDataItem*		SetUserData(const String& key, const Variant& value);
  // #CAT_UserData make new (or change existing) simple user data entry with given name and value; returns item, which can be ignored
  void			SetUserData_Gui(const String& key, const Variant& value,
    const String& desc);
  // #CAT_UserData #MENU #MENU_CONTEXT #LABEL_SetUserData make new (or change existing) simple user data entry with given name and value (desc optional) -- this is how to get User Data editor panel to show up the first time
  bool			RemoveUserData(const String& key);
  // #CAT_UserData removes data for indicated key; returns true if it existed
  taDoc*		GetDocLink() const; // #CAT_UserData #EXPERT gets a linked Doc, if any; you can use this to test for existence
  void			SetDocLink(taDoc* doc); // #CAT_UserData #MENU #MENU_CONTEXT #DROP1 #NULL_OK set a link to a doc from the .docs collection -- the doc will then show up automatically in a panel for this obj -- set to NULL to remove it
  
  bool		HasOption(const char* op) const
  { return GetTypeDef()->HasOption(op); }
  // #IGNORE hard-coded options for this type
  bool		CheckList(const String_PArray& lst) const
  { return GetTypeDef()->CheckList(lst); }
  // #IGNORE 

  ///////////////////////////////////////////////////////////////////////////
  //   Clipboard Queries and Edit Actions (for drag-n-drop, cut/paste etc)
  //   ms=NULL indicates SRC ops/context, else DST ops
  //  child="item here" for item, else NULL for "into" parent
#ifdef TA_GUI
public:
  void			QueryEditActions(taiMimeSource* ms, int& allowed, int& forbidden);
  // #IGNORE
  int			EditAction(taiMimeSource* ms, int ea); // #IGNORE 
  
  void			ChildQueryEditActions(const MemberDef* md, const taBase* child,
	taiMimeSource* ms, int& allowed, int& forbidden);
  // #IGNORE gives ops allowed on child, md valid if obj is a member of query parent, o/w NULL; child is sel item, or NULL if querying parent only; ms is clipboard or drop contents
  virtual int		ChildEditAction(const MemberDef* md, taBase* child,
        taiMimeSource* ms, int ea);
  // #IGNORE note: multi source ops will have child=NULL
  
protected: //  note: all impl code is in ta_qtclipdata.cpp
  virtual void		QueryEditActionsS_impl(int& allowed, int& forbidden);
    // called once per src item, by controller
  virtual void		QueryEditActionsD_impl(taiMimeSource* ms,
    int& allowed, int& forbidden);
    // gives ops allowed on child, with ms being clipboard or drop contents, md valid if we are a member, o/w NULL
  virtual int		EditActionS_impl(int ea);
    // called once per src item, by controller
  virtual int		EditActionD_impl(taiMimeSource* ms, int ea);
    // called once per ms item, in 0:N order, by ourself
  
  virtual void		ChildQueryEditActions_impl(const MemberDef* md,
    const taBase* child, const taiMimeSource* ms,
    int& allowed, int& forbidden);
    // called in 
  virtual int		ChildEditAction_impl(const MemberDef* md, taBase* child,
	taiMimeSource* ms, int ea);
    // op implementation (non-list/grp)
#endif // TA_USE_GUI
  ///////////////////////////////////////////////////////////////////////////
  // 	Browser gui
public:
  static const KeyString key_name; // #IGNORE "name" -- Name, note: can easily be empty
  static const KeyString key_type; // #IGNORE "type" -- def to typename, but some like progvar append their own subtype
  static const KeyString key_type_desc; // #IGNORE "type_desc" -- static type description
  static const KeyString key_desc; // #IGNORE "desc" -- per-instance desc if available (def to type)
  static const KeyString key_disp_name; // #IGNORE "disp_name" -- DisplayName, never empty
  static const KeyString key_unique_name; // #IGNORE "unique_name" -- DottedName, for token lists, DisplayName if empty
  
  virtual const String	statusTip(const KeyString& key = "") const;
  // #IGNORE the default text returned for StatusTipRole (key usually not needed)
  virtual const String	GetToolTip(const KeyString& key) const;
  // #IGNORE the default text returned for ToolTipRole
  virtual String	GetColText(const KeyString& key, int itm_idx = -1) const;
  // #IGNORE default keys are: name, type, desc, disp_name
  virtual const QVariant GetColData(const KeyString& key, int role) const;
  // #IGNORE typ roles are ToolTipRole, StatusTipRole; key can be blank if not col-specific
  
protected:
  virtual String 	ChildGetColText_impl(taBase* child, const KeyString& key, 
    int itm_idx = -1) const {return _nilKeyString;}

  ///////////////////////////////////////////////////////////////////////////
  //	Edit Dialog gui
public:  
  // NOTE: this plain Edit seems weird and vestigial and should be nuked:
  virtual bool		Edit();
  // #CAT_Display Edit this object using the gui -- this will be an edit dialog or an edit panel depending on ...???
  virtual bool		EditDialog(bool modal = false);
  // #MENU #ARGC_0 #MENU_ON_Object #MENU_CONTEXT #NO_SCRIPT #CAT_Display Edit this object in a popup dialog using the gui (if modal == true, the edit dialog blocks all other gui operations until the user closes it)
  virtual bool		EditPanel(bool new_tab = false, bool pin_tab = false);
  // #MENU #ARGC_0 #MENU_ON_Object #MENU_CONTEXT #NO_SCRIPT #CAT_Display Edit this object in a panel in the gui browser (if new_tab == true, then a new edit panel tab is opened for it, if pin_tab == true then the new tab is pinned in place (option ignored for new_tab == false))
  virtual bool		BrowserSelectMe();
  // #CAT_Display select this item in the main project browser (only works if gui is active, etc) -- returns success
  virtual bool		BrowserExpandAll();
  // #CAT_Display expand all sub-leaves under this item in the browser
  virtual bool		BrowserCollapseAll();
  // #CAT_Display collapse all sub-leaves under this item in the browser
  virtual void		BrowseMe();
  // #MENU #MENU_ON_Object #MENU_SEP_AFTER #MENU_CONTEXT #CAT_Display show this object in its own browser 
//obs  virtual bool		ReShowEdit(bool force = false);
  virtual bool		GuiFindFromMe(const String& find_str="");
  // #CAT_Display activate the gui find dialog starting from this object, with given find string
  // #CAT_Display reshows any open edit dialogs for this object
  virtual const iColor GetEditColor(bool& ok); // #IGNORE background color for edit dialog
  virtual const iColor GetEditColorInherit(bool& ok);
  // #IGNORE background color for edit dialog, include inherited colors from parents
#if defined(TA_GUI) && !defined(__MAKETA__) 
  virtual const QPixmap* GetDataNodeBitmap(int, int& flags_supported) const
    {return NULL; } // #IGNORE gets the NodeBitmapFlags for the tree or list node -- see ta_qtbrowse_def.h
#endif
  virtual String	StringFieldLookupFun(const String& cur_txt, int cur_pos,
					     const String& mbr_name, int& new_pos)
  { return _nilString; } 
  // #IGNORE special lookup function called when Ctrl-L is pressed for string members -- is passed current text and position of cursor, and name of member, and it must return the replacement text for the entire edit (if rval is empty, nothing happens)

  virtual void		CallFun(const String& fun_name);
  // #CAT_ObjectMgmt call function (method) of given name on this object, prompting for args using gui interface

  virtual Variant	GetGuiArgVal(const String& fun_name, int arg_idx);
  // #IGNORE overload this to get default initial arg values for given function and arg index -- function must be marked with ARG_VAL_FM_FUN[_n] comment directive, and _nilVariant rval will be ignored (NOTE: definitely call inherited:: because this is used for ChangeMyType!
  
  virtual bool		SelectForEdit(MemberDef* member, SelectEdit* editor,
      const String& extra_label = "", const String& sub_gp_nm = "");
  // #MENU #MENU_ON_SelectEdit #CAT_Display #NULL_OK_1 #NULL_TEXT_1_NewEditor select a given member for editing in an edit dialog that collects selected members and methods from different objects (if editor is NULL, a new one is created in .edits).  returns false if member was already selected.  extra_label is prepended to item name, and if sub_gp_nm is specified, item will be put in this sub-group (new one will be made if it does not yet exist)
  virtual bool		SelectForEditNm(const String& memb_nm, SelectEdit* editor,
	const String& extra_label = _nilString, const String& sub_gp_nm = _nilString);
  // #CAT_Display select a given member (by name) for editing in an edit dialog that collects selected members from different objects (if editor is NULL, a new one is created in .edits).  returns false if member was already selected.  extra_label is prepended to item name, and if sub_gp_nm is specified, item will be put in this sub-group (new one will be made if it does not yet exist)
  virtual int		SelectForEditSearch(const String& memb_contains, SelectEdit*& editor);
  // #MENU #NULL_OK_1 #NULL_TEXT_1_NewEditor #CAT_Display search among this object and any sub-objects for members containing given string, and add to given select editor (if NULL, a new one is created in .edits).  returns number found
  virtual int		SelectForEditCompare(taBase* cmp_obj, SelectEdit*& editor, bool no_ptrs = true);
  // #MENU #NULL_OK_1  #NULL_TEXT_1_NewEditor  #CAT_Display #TYPE_ON_0_this #NO_SCOPE compare this object with selected comparison object, adding any differences to given select editor (if NULL, a new one is created in .edits).  returns number of differences.  no_ptrs = ignore differences in pointer fields
  virtual bool		SelectFunForEdit(MethodDef* function, SelectEdit* editor,
	 const String& extra_label = "", const String& sub_gp_nm = "");
  // #MENU #NULL_OK_1  #NULL_TEXT_1_NewEditor  #CAT_Display select a given function (method) for calling in a select edit dialog that collects selected members and methods from different objects (if editor is NULL, a new one is created in .edits). returns false if method was already selected.  extra_label is prepended to item name, and if sub_gp_nm is specified, item will be put in this sub-group (new one will be made if it does not yet exist)
  virtual bool		SelectFunForEditNm(const String& function_nm, SelectEdit* editor,
	   const String& extra_label = _nilString, const String& sub_gp_nm = _nilString);
  // #CAT_Display select a given method (by name) for editing in an edit dialog that collects selected members from different objects (if editor is NULL, a new one is created in .edits)  returns false if method was already selected.   extra_label is prepended to item name, and if sub_gp_nm is specified, item will be put in this sub-group (new one will be made if it does not yet exist)
  virtual void		GetSelectText(MemberDef* mbr, String extra_label,
    String& lbl, String& desc) const; // #IGNORE supply extra_label (optional); provides the canonical lbl and (if empty) desc -- NOTE: routine is in ta_seledit.cpp
  
  ///////////////////////////////////////////////////////////////////////////
  //	Closing 

  virtual void		CloseLater();
  // #MENU #MENU_ON_Object #CONFIRM #NO_REVERT_AFTER #LABEL_Close_(Destroy) #NO_MENU_CONTEXT #CAT_ObjectMgmt PERMANENTLY Destroy this object!  This is not Iconify or close window..
  virtual void		Close();
  // #IGNORE an immediate version of Close for use in code (no waitproc delay)
  virtual bool		Close_Child(TAPtr obj);
  // #IGNORE actually closes a child object (should be immediate child)
  virtual bool		CloseLater_Child(TAPtr obj);
  // #IGNORE actually closes a child object (should be immediate child) but defers deletion to loop

  virtual void		Help();
  // #MENU #CAT_Display get help on using this object

  ///////////////////////////////////////////////////////////////////////////
  //	Updating pointers (when objects change type or are copied)

  virtual taBase* UpdatePointers_NewPar_FindNew(taBase* old_guy, taBase* old_par,
						taBase* new_par);
  // #IGNORE find a new pointer to replace old_guy in new_par
  virtual bool	UpdatePointers_NewPar_Ptr(taBase** ptr, taBase* old_par, taBase* new_par,
					  bool null_not_found = true);
  // #IGNORE update pointer if it used to point to an object under old_par parent, have it point to the corresponding object under new_par (based on path) -- set to null if not found if option set.  used for updating after a copy operation: returns true if updated
  virtual bool	UpdatePointers_NewPar_PtrNoSet(taBase** ptr, taBase* old_par, taBase* new_par,
					       bool null_not_found = true);
  // #IGNORE update pointer if it used to point to an object under old_par parent, have it point to the corresponding object under new_par (based on path) -- set to null if not found if option set.  used for updating after a copy operation: returns true if updated (NO_SET_POINTER version)
  virtual bool	UpdatePointers_NewPar_SmPtr(taSmartPtr& ptr, taBase* old_par, taBase* new_par,
					    bool null_not_found = true);
  // #IGNORE update pointer if it used to point to an object under old_par parent, have it point to the corresponding object under new_par (based on path) -- set to null if not found if option set.  used for updating after a copy operation: returns true if updated
  virtual bool	UpdatePointers_NewPar_Ref(taSmartRef& ref, taBase* old_par,
					  taBase* new_par, bool null_not_found = true);
  // #IGNORE update reference if it used to point to an object under old_par parent, have it point to the corresponding object under new_par (based on path) -- set to null if not found if option set.  used for updating after a copy operation: returns true if updated

  virtual int	UpdatePointers_NewPar(taBase* old_par, taBase* new_par);
  // #IGNORE update pointers for a new parent (e.g., after a copy operation): and anything that lives under old_par and points to something else that lives under old_par is updated to point to new_par: this default impl uses TA info to walk the members and find the guys to change; returns number changed
  virtual int	UpdatePointers_NewPar_IfParNotCp(const taBase* cp, TypeDef* par_type);
  // #IGNORE for use during a copy operation: call UpdatePointers_NewPar with parent (GET_OWNER) of given par_type, only if that parent is not already COPYING (according to base flag)

  virtual bool	UpdatePointers_NewParType_Ptr(taBase** ptr, TypeDef* par_typ, taBase* new_par,
					  bool null_not_found = false);
  // #IGNORE update pointer if it used to point to an object under par_typ parent, have it point to the corresponding object under new_par (based on path) -- set to null if not found if option set.
  virtual bool	UpdatePointers_NewParType_PtrNoSet(taBase** ptr, TypeDef* par_typ, taBase* new_par,
					  bool null_not_found = false);
  // #IGNORE update pointer if it used to point to an object under par_typ parent, have it point to the corresponding object under new_par (based on path) -- set to null if not found if option set. (NO_SET_POINTER version)
  virtual bool	UpdatePointers_NewParType_SmPtr(taSmartPtr& ptr, TypeDef* par_typ,
						taBase* new_par, bool null_not_found = false);
  // #IGNORE update pointer if it used to point to an object under par_typ parent, have it point to the corresponding object under new_par (based on path) -- set to null if not found if option set.
  virtual bool	UpdatePointers_NewParType_Ref(taSmartRef& ref, TypeDef* par_typ,
					  taBase* new_par, bool null_not_found = false);
  // #IGNORE update reference if it used to point to an object under par_typ parent, have it point to the corresponding object under new_par (based on path) -- set to null if not found if option set.

  virtual int	UpdatePointers_NewParType(TypeDef* par_typ, taBase* new_par);
  // #IGNORE update pointers for a new parent of given type -- any pointer under this object that points to an object that lives under an object of par_typ (type of new_par): and anything that lives under old_par and points to something else that lives under old_par is updated to point to new_par: this default impl uses TA info to walk the members and find the guys to change; returns number changed

  static bool	UpdatePointers_NewObj_Ptr(taBase** ptr, taBase* ptr_owner, 
					  taBase* old_ptr, taBase* new_ptr);
  // #IGNORE update pointer to new_ptr if it used to point to old_ptr; call UAE on ptr_owner
  static bool	UpdatePointers_NewObj_PtrNoSet(taBase** ptr, taBase* ptr_owner, 
					  taBase* old_ptr, taBase* new_ptr);
  // #IGNORE update pointer to new_ptr if it used to point to old_ptr; call UAE on ptr_owner -- NO_SET_POINTER version
  static bool	UpdatePointers_NewObj_SmPtr(taSmartPtr& ptr, taBase* ptr_owner, 
					    taBase* old_ptr, taBase* new_ptr);
  // #IGNORE update pointer to new_ptr if it used to point to old_ptr; call UAE on ptr_owner
  static bool	UpdatePointers_NewObj_Ref(taSmartRef& ref, taBase* ptr_owner, 
					  taBase* old_ptr, taBase* new_ptr);
  // #IGNORE update pointer to new_ptr if it used to point to old_ptr; call UAE on ptr_owner

  virtual int	UpdatePointersToMe(taBase* new_ptr);
  // #IGNORE replace all pointers that might have pointed to old version of this object (and all of its kids!) to new_ptr -- called during ChangeType: default is to call UpdatePointers_NewObj on taMisc::default_scope; returns number changed
  virtual int	UpdatePointersToMe_impl(taBase* scope_obj, taBase* new_ptr);
  // #IGNORE actually does the work: above just passes in the scoper
  virtual int	UpdatePointersToMyKids_impl(taBase* scope_obj, taBase* new_ptr);
  // #IGNORE override this for list/group guys that contain sub-objs -- update all the sub objs that we own!

  virtual int	UpdatePointers_NewObj(taBase* old_ptr, taBase* new_ptr);
  // #IGNORE replace all pointers to old_ptr with new_ptr: walks the entire structure (members, lists, etc) and iteratively calls; returns number changed

  ///////////////////////////////////////////////////////////////////////////
  //	DMem -- distributed memory (MPI)	

#ifdef DMEM_COMPILE
  virtual bool DMem_IsLocal()      // #IGNORE check if local given stored "this process" number
  { return true; }
  virtual bool DMem_IsLocalProc(int proc)  // #IGNORE check if local given external process number
  { return true; }
  virtual int  DMem_GetLocalProc()  // #IGNORE get local processor number for this object
  { return 0; }
  virtual void DMem_SetLocalProc(int lproc) // #IGNORE set the local process number for this object
  { };
  virtual void DMem_SetThisProc(int proc) // #IGNORE set the local processor number RELATIVE to relevant communicator
  { }
#endif

public:
  ///////////////////////////////////////////////////////////////////////////
  //		Misc Impl stuff

#ifdef __MAKETA__
  BaseFlags		base_flags; // #NO_SHOW #NO_SAVE #READ_ONLY fake base_flags for ta system, flags are actually in the lower short
#else
# if    (TA_BYTE_ORDER == TA_BIG_ENDIAN)
  protected: short	refn;	// number of references to this object
  public: mutable short	base_flags;
# define refn_base refn
# elif (TA_BYTE_ORDER == TA_LITTLE_ENDIAN)
  mutable short		base_flags;
  protected: short	refn;	// number of references to this object
# define refn_base base_flags
# else
#   error "undefined byte order"
# endif
#endif
private: 
// Initialize and Destroy are always private because they should only be called in ctor/dtor
  void			Initialize() { *reinterpret_cast<int*>(&refn_base) = 0; }
  // #IGNORE constructor implementation to initialize members of class.  every class should define this initializer.  cannot refer to anything outside of the object itself (use InitLinks for when it gets added into the object hierarchy)
  void			Destroy();
  // #IGNORE destructor implementation -- free any allocated memory and reset pointers to null, etc. -- set to null and check for null!

};

inline istream& operator>>(istream &strm, taBase &obj)
{ obj.Load_strm(strm); return strm; }
inline ostream& operator<<(ostream &strm, taBase &obj)
{ obj.Save_strm(strm); return strm; }



///////////////////////////////////////////////////////////////////////////
//	taSmartPtr / Ref

class TA_API taSmartPtr {
  // ##NO_INSTANCE ##NO_TOKENS ##SMART_POINTER "safe" ptr for taBase objects -- automatically does ref counts; designed to be binary compatible with taBase*
public:
  static TypeDef*	GetBaseType(TypeDef* this_typ);
    // returns a best-guess min type -- hacks by looking at the name
    
  inline taBase*	ptr() const {return m_ptr;}
  inline void		set(taBase* src) {taBase::SetPointer(&m_ptr, src);}
  
  inline		operator bool() const {return (m_ptr);}
  inline bool		operator !() const {return !(m_ptr);}
    // needed to avoid ambiguities when we have derived T* operators
  inline 		operator taBase*() const {return m_ptr;}
  inline taBase* 	operator->() const {return m_ptr;}
  inline TAPtr* 	operator&() const {return &m_ptr;}
    //note: operator& is *usually* verbotten but we are binary compat -- it simplifies low-level compat
  inline taBase* 	operator=(const taSmartPtr& src) 
    {set(src.m_ptr); return m_ptr;} 
  inline taBase* 	operator=(taBase* src) {set(src); return m_ptr;} 
  
  taSmartPtr(taBase* val) {m_ptr = NULL; set(val);}
  taSmartPtr() {m_ptr = NULL;}
  ~taSmartPtr() {set(NULL);} //note: DO NOT change to be virtual!
protected:
  mutable taBase*	m_ptr;
private:
  taSmartPtr(const taSmartPtr& src); // not defined 
};


template<class T>
class taSmartPtrT: public taSmartPtr { 
public:
  inline T*	ptr() const {return (T*)m_ptr;} // typed alias for the base version

  inline 	operator T*() const {return (T*)m_ptr;}
  inline T* 	operator->() const {return (T*)m_ptr;}
  inline T** 	operator&() const {return (T**)(&m_ptr);}
    //note: operator& is *usually* verbotten but we are binary compat -- it simplifies low-level compat
  T* 		operator=(const taSmartPtrT<T>& src) 
    {set((T*)src.m_ptr); return (T*)m_ptr;} 
    //NOTE: copy only implies ptr, NOT the owner!
  T* 		operator=(T* src) {set(src); return (T*)m_ptr;}
  T& 		operator*() const {return *(T*)m_ptr;}
   
  friend bool	operator==(const taSmartPtrT<T>& a, const taSmartPtrT<T>& b)
    {return (a.m_ptr == b.m_ptr);} 
  
  taSmartPtrT(T* val): taSmartPtr(val) {}
  taSmartPtrT() {} 
  
private:
  taSmartPtrT(const taSmartPtrT<T>& src); // not defined 
};

// macro for creating smart ptrs of taBase classes

#define SmartPtr_Of(T)  typedef taSmartPtrT<T> T ## Ptr;

// compatibility macro 
#define taPtr_Of(T)  SmartPtr_Of(T)


class TA_API taSmartRef: protected IDataLinkClient {
  // ##NO_INSTANCE ##NO_TOKENS ##SMART_POINTER safe reference for taBase objects -- does not ref count, but is a data link client so it tracks changes and automatically sets ptr to NULL when object dies
friend class taBase;
friend class TypeDef; // for various
friend class MemberDef; // for streaming
friend class taDataView; // for access to link
public:
  inline taBase*	ptr() const {return m_ptr;}
  void			set(taBase* src) {if (src == m_ptr) return;
    if (m_ptr) {m_ptr->RemoveDataClient(this); 
      //note: important to wait to get mptr in case RDC indirectly deleted it
      taBase* t = m_ptr; m_ptr = NULL; DataRefChanging(t, false);}
    if (src && src->AddDataClient(this)) 
      {m_ptr = src; DataRefChanging(m_ptr, true);} }
  
  virtual TypeDef*	GetBaseType() const {return &TA_taBase;}
  taBase* 		GetOwner() const { return m_own; }
  
  inline		operator bool() const {return (m_ptr);}
    // needed to avoid ambiguities when we have derived T* operators
  inline bool		operator !() const {return !(m_ptr);}
  inline 		operator taBase*() const {return m_ptr;}
  inline taBase* 	operator->() const {return m_ptr;}
  taBase* 		operator=(const taSmartRef& src) 
    {set(src.m_ptr); return m_ptr;} 
    //NOTE: copy only implies ptr, NOT the owner!
  taBase* 		operator=(taBase* src) {set(src); return m_ptr;} 
  
  inline void 		Init(taBase* own_) {m_own = own_;} // call in owner's Initialize or InitLinks
  inline void		CutLinks() {set(NULL); m_own = NULL;}
  taSmartRef() {m_own = NULL; m_ptr = NULL;}
  ~taSmartRef() {CutLinks();}
  
protected:
  taBase*		m_own; 
  mutable taBase*	m_ptr;
  
  void			DataRefChanging(taBase* obj, bool setting);
  
private:
  taSmartRef(const taSmartRef& src); // not defined 
  
public: // ITypedObject interface
  override void*	This() {return (void*)this;} //
  override TypeDef*	GetTypeDef() const {return &TA_taSmartRef;} //note: only one typedef for all

public: // IDataLinkClient interface
  override TypeDef*	GetDataTypeDef() const 
    {return (m_ptr) ? m_ptr->GetTypeDef() : &TA_taBase;} // TypeDef of the data
  override void		DataDataChanged(taDataLink*, int dcr, void* op1, void* op2);
  override void		DataLinkDestroying(taDataLink* dl);
};

template<class T>
class taSmartRefT: public taSmartRef { 
public:
  inline T*		ptr() const {return (T*)m_ptr;} // typed alias for the base version
  
  inline 		operator T*() const {return (T*)m_ptr;}
  inline T* 		operator->() const {return (T*)m_ptr;}
  T* 			operator=(const taSmartRefT<T>& src) {set((T*)src.m_ptr); return (T*)m_ptr;}
  T* 			operator=(T* src) {set(src); return (T*)m_ptr;}
  override TypeDef*	GetBaseType() const {return T::StatTypeDef(0);}
  TypeDef*		GetDataTypeDef() const 
    {return (m_ptr) ? m_ptr->GetTypeDef() : T::StatTypeDef(0);}
  taSmartRefT() {}  //
  
#ifndef __MAKETA__
  template <class U>
  inline friend bool	operator==(U* a, const taSmartRefT<T>& b) 
    {return (a == b.m_ptr);} 
  template <class U>
  inline friend bool	operator!=(U* a, const taSmartRefT<T>& b) 
    {return (a != b.m_ptr);} 
  template <class U>
  friend bool		operator==(const taSmartRefT<T>& a, U* b) 
    {return (a.m_ptr == b);} 
  template <class U>
  friend bool		operator!=(const taSmartRefT<T>& a, U* b) 
    {return (a.m_ptr != b);} 
  template <class U>
  friend bool		operator==(const taSmartRefT<T>& a, const taSmartRefT<U>& b) 
    {return (a.m_ptr == b.m_ptr);} 
  template <class U>
  friend bool		operator!=(const taSmartRefT<T>& a, const taSmartRefT<U>& b) 
    {return (a.m_ptr != b.m_ptr);} 
private:
  taSmartRefT(const taSmartRefT<T>& src); // not defined 
#endif
};

// macro for creating smart refs of taBase classes
#define SmartRef_Of(T,td)  typedef taSmartRefT<T> T ## Ref

SmartRef_Of(taBase,);		// basic ref if you don't know the type


class TA_API taOBase : public taBase {
  // #NO_TOKENS #NO_UPDATE_AFTER owned base class of taBase
INHERITED(taBase)
public:
  taBase*		owner;	// #NO_SHOW #READ_ONLY #NO_SAVE #NO_SET_POINTER pointer to owner

  mutable UserDataItem_List* user_data_; // #OWN_POINTER #NO_SHOW_EDIT #HIDDEN_TREE #NO_SAVE_EMPTY storage for user data (created if needed)

  taDataLink**		addr_data_link() {return &m_data_link;} // #IGNORE
  override taDataLink*	data_link() {return m_data_link;}	// #IGNORE
//  override void		set_data_link(taDataLink* dl) {m_data_link = dl;}

  TAPtr 		GetOwner() const	{ return owner; }
  USING(inherited::GetOwner)
  TAPtr 		SetOwner(TAPtr ta)	{ owner = ta; return ta; }
  UserDataItem_List* 	GetUserDataList(bool force = false) const;  
  void	CutLinks();
  TA_BASEFUNS(taOBase); //
protected:
  taDataLink*		m_data_link; //
  override void		CanCopy_impl(const taBase* cp_fm, bool quiet,
    bool& ok, bool virt) const 
    {if (virt) inherited::CanCopy_impl(cp_fm, quiet, ok, virt);}
  
#ifdef TA_GUI
protected: // all related to taList or DEF_CHILD children_
  override void	ChildQueryEditActions_impl(const MemberDef* md, const taBase* child,
    const taiMimeSource* ms, int& allowed, int& forbidden);
     // gives the src ops allowed on child (ex CUT)
  virtual void	ChildQueryEditActionsL_impl(const MemberDef* md, const taBase* lst_itm,
    const taiMimeSource* ms, int& allowed, int& forbidden);
    // returns the operations allowed for list items (ex Paste)
  
  virtual int	ChildEditAction_impl(const MemberDef* md, taBase* child,
        taiMimeSource* ms, int ea);
  virtual int	ChildEditActionLS_impl(const MemberDef* md, taBase* lst_itm, int ea);
  virtual int	ChildEditActionLD_impl_inproc(const MemberDef* md,
    taBase* lst_itm, taiMimeSource* ms, int ea);
  virtual int	ChildEditActionLD_impl_ext(const MemberDef* md,
    taBase* lst_itm, taiMimeSource* ms, int ea);
#endif

private:
  void	Copy_(const taOBase& cp);
  void 	Initialize()	{ owner = NULL; user_data_ = NULL; m_data_link = NULL; }
  void	Destroy();
};


#ifdef TA_USE_QT
/*
 * taBaseAdapter enables a taOBase object to handle Qt events, via a
 * proxy(taBaseAdapter)/stub(taBase) approach. Note that dual-parenting a taBase object
 * with QObject is not workable, because QObject must come first, and then all the (TAPtr)(void*)
 * casts in the system break...

 * To use, subclass taBaseAdapter when events need to be handled. Create the instance in
 * the Initialize() call and set with SetAdapter. The adapter object does not participate
 * in copying/cloning/etc. (it has no state information).

 * Since classes can have subclasses, there may be successive calls to Initialize with subclasses
 * of an adapter. The adapters will be chained, with the new adapter becoming the QObject parent of
 * the previous adapter. Therefore, when the current adapter is destroyed, child adapters will also
 * get destroyed. It will not matter whether a base class hooks to its own adapter, or to a subclass
 * or to a combination.
*/

class TA_API taBaseAdapter: public QObject {
  // ##IGNORE QObject for attaching events/signals for its taBase owner
friend class taOABase;
public:
  taBaseAdapter(taOABase* owner_): QObject(NULL) {owner = owner_;}
  ~taBaseAdapter();
protected:
  taOABase* owner; // #IGNORE
};
#endif // TA_USE_QT


//NOTE: the taOABase object is not used very much...
class TA_API taOABase : public taOBase {
  // #NO_TOKENS #NO_UPDATE_AFTER owned base class with QObject adapter for signals/slots
INHERITED(taOBase)
friend class taBaseAdapter;
public:
#ifdef TA_USE_QT
  taBaseAdapter* 	adapter; // #IGNORE
  void			SetAdapter(taBaseAdapter* adapter_);
  void 	Initialize()	{adapter = NULL;}
#else
  void 	Initialize()	{}
#endif
  void	CutLinks();
  TA_BASEFUNS_NOCOPY(taOABase); //
  
private:
  void	Destroy() {CutLinks();}
};


class TA_API taNBase : public taOBase { // #NO_TOKENS Named, owned base class of taBase
INHERITED(taOBase)
public:
  String		name; // #CONDEDIT_OFF_base_flags:NAME_READONLY name of the object

  bool 		SetName(const String& nm)    	{ name = nm; return true; }
  String	GetName() const			{ return name; }
  override void	SetDefaultName();

  TA_BASEFUNS(taNBase);
protected:
  override void		UpdateAfterEdit_impl();

private:
  NOCOPY(taNBase); //note: we don't copy name, because it causes too many issues
  void 	Initialize()	{ }
  void  Destroy()       { } 
};


class TA_API taFBase: public taNBase {
  // #NO_TOKENS #NO_UPDATE_AFTER named/owned base class of taBase, with filename
public:
  String		desc;	   // #EDIT_DIALOG description of this object: what does it do, how should it be used, etc
  String		file_name; // #READ_ONLY #NO_SAVE #SHOW #FILE_DIALOG_LOAD the current filename for this object

  override String	GetDesc() const { return desc; }

  override bool		SetFileName(const String& val); // #IGNORE note: we canonicalize name first
  override String	GetFileName() const { return file_name; } // #IGNORE
  TA_BASEFUNS2(taFBase, taNBase)
private:
  void			Copy_(const taFBase& cp) { desc = cp.desc; file_name = cp.file_name; }
  void 			Initialize() {}
  void			Destroy() {}
};



class TA_API taBase_PtrList: public taPtrList<taBase> { // a primitive taBase list type, used for global lists that manage taBase objects
public:
  TypeDef*	El_GetType_(void* it) const 
    {return ((TAPtr)it)->GetTypeDef();} // #IGNORE
protected:
  String	El_GetName_(void* it) const { return ((TAPtr)it)->GetName(); }

  void*		El_Ref_(void* it)	{ taBase::Ref((TAPtr)it); return it; }
  void*		El_unRef_(void* it)	{ taBase::unRef((TAPtr)it); return it; }
  void		El_Done_(void* it)	{ taBase::Done((TAPtr)it); }
};

typedef taPtrList_base<taBase>  taPtrList_ta_base; // this comment needed for maketa parser

class taBase_RefList;

class TA_API IRefListClient: virtual public ITypedObject { // #NO_CSS
// optional interface so disparate classes can use RefList to get notifies
public:
  virtual void		DataDestroying_Ref(taBase_RefList* src, taBase* ta) = 0;
    // note: item will already have been removed from list
  virtual void		DataChanged_Ref(taBase_RefList* src, taBase* ta,
    int dcr, void* op1, void* op2) = 0;

};

class TA_API taBase_RefList: public taPtrList<taBase>,
   virtual public IMultiDataLinkClient { // #IGNORE a primitive taBase list type, that uses SmartRef semantics to manage the items -- note: this list does NOT manage ownership/lifetimes
public:
  void			setOwner(IRefListClient* own_);

  taBase_RefList(IRefListClient* own_ = NULL) {Initialize(); setOwner(own_);}
  ~taBase_RefList();
  
public: // IDataLinkClient i/f 
  void*			This() {return this;}  // #IGNORE
  override TypeDef*	GetTypeDef() const {return &TA_taBase_RefList;} // #IGNORE
protected: // we actually protect these
  override void		DataLinkDestroying(taDataLink* dl); // #IGNORE
  override void		DataDataChanged(taDataLink* dl, int dcr, void* op1, void* op2);
     // #IGNORE

protected:
  IRefListClient*	m_own; // optional owner
  
  String	El_GetName_(void* it) const { return ((TAPtr)it)->GetName(); }
  TypeDef*	El_GetType_(void* it) const {return ((TAPtr)it)->GetTypeDef();}
  void*		El_Ref_(void* it);
  void*		El_unRef_(void* it);
private:
  void Initialize();
};

class taList_impl;
typedef taList_impl* TABLPtr; // this comment needed for maketa parser


////////////////////////////////////////////////////////////////////////
//		taList_impl -- base ta list impl

class TA_API taList_impl : public taOBase, public taPtrList_ta_base {
  // #INSTANCE #NO_TOKENS #STEM_BASE #NO_UPDATE_AFTER ##MEMB_HIDDEN_EDIT ##HIDDEN_INLINE implementation for a taBase list class
#ifndef __MAKETA__
private:
typedef taOBase inherited; // for the boilerplate code
typedef taOBase inherited_taBase;
typedef taPtrList_ta_base inherited_taPtrList;
#endif
public:
  String        name;           // #CONDEDIT_OFF_base_flags:NAME_READONLY name of the object 
  TypeDef*	el_base;	// #EXPERT #NO_SHOW_TREE #READ_ONLY_GUI #NO_SAVE Base type for objects in group
  TypeDef* 	el_typ;		// #TYPE_ON_el_base #NO_SHOW_TREE Default type for objects in group
  int		el_def;		// #EXPERT Index of default element in group

  // stuff for the taBase 
  bool          SetName(const String& nm)       { name = nm; return true; }
  String        GetName() const         	{ return name; } 
  void          SetDefaultName();

  override TypeDef* 	GetElType() const {return el_typ;}
  // #IGNORE Default type for objects in group
  override void*        GetTA_Element(Variant i, TypeDef*& eltd)
    { return taPtrList_ta_base::GetTA_Element_(i, eltd); }
  override TypeDef*	El_GetType_(void* it) const
    {return ((TAPtr)it)->GetTypeDef();} // #IGNORE 
  override taList_impl* children_() {return this;}


  String 	GetPath_Long(TAPtr ta=NULL, TAPtr par_stop = NULL) const;
  String 	GetPath(TAPtr ta=NULL, TAPtr par_stop = NULL) const;

  void* 	FindMembeR(const String& nm, MemberDef*& ret_md) const;

  override void	Close();
  override bool	Close_Child(TAPtr obj);
  override bool	CloseLater_Child(TAPtr obj);
  override void	ChildUpdateAfterEdit(TAPtr child, bool& handled); 
  override void	DataChanged(int dcr, void* op1 = NULL, void* op2 = NULL); 

  ostream& 	OutputR(ostream& strm, int indent = 0) const;

  override String GetValStr(void* par = NULL, MemberDef* md = NULL,
			    TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
			    bool force_inline = false) const;
  override bool  SetValStr(const String& val, void* par = NULL, MemberDef* md = NULL,
			   TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
			   bool force_inline = false);

  override void Dump_Save_GetPluginDeps();
  override int	Dump_SaveR(ostream& strm, TAPtr par=NULL, int indent=0);
  override int	Dump_Save_PathR(ostream& strm, TAPtr par=NULL, int indent=0);
  virtual int 	Dump_Save_PathR_impl(ostream& strm, TAPtr par, int indent); // #IGNORE
  override void	Dump_Load_pre();
  override int	Dump_Load_Value(istream& strm, TAPtr par=NULL);

  override void	Search_impl(const String& srch, taBase_PtrList& items,
			    taBase_PtrList* owners = NULL, 
			    bool contains = true, bool case_sensitive = false,
			    bool obj_name = true, bool obj_type = true,
			    bool obj_desc = true, bool obj_val = true,
			    bool mbr_name = true, bool type_desc = false);
  override void	CompareSameTypeR(Member_List& mds, TypeSpace& base_types, 
				 void_PArray& trg_bases, void_PArray& src_bases,
				 taBase* cp_base,
				 int show_forbidden = taMisc::NO_HIDDEN,
				 int show_allowed = taMisc::SHOW_CHECK_MASK,
				 bool no_ptrs = true);
  override int	UpdatePointers_NewPar(taBase* old_par, taBase* new_par);
  override int	UpdatePointers_NewParType(TypeDef* par_typ, taBase* new_par);
  override int	UpdatePointers_NewObj(taBase* old_ptr, taBase* new_ptr);
  override int	UpdatePointersToMyKids_impl(taBase* scope_obj, taBase* new_ptr);

  TAPtr		DefaultEl_() const	{ return (TAPtr)SafeEl_(el_def); } // #IGNORE

  virtual int	SetDefaultElType(TypeDef* it);
  // #CAT_Access set the default element to be item with given type
  virtual int	SetDefaultElName(const String& nm);
  // #CAT_Access set the default element to be item with given name
  virtual int	SetDefaultEl(TAPtr it);
  // #CAT_Access set the default element to be given item

  taBase* 	New_gui(int n_objs=1, TypeDef* typ=NULL,
    const String& name="(default name)");
  // #BUTTON #MENU_CONTEXT #NO_SAVE_ARG_VAL #TYPE_ON_1_el_base #INIT_ARGVAL_ON_1_el_typ #LABEL_New #CAT_Modify create n_objs new objects of given type in list (typ=NULL: default type, el_typ;)
  virtual void	SetSize(int sz);
  // #MENU #MENU_ON_Edit #CAT_Modify add or remove elements to force list to be of given size

  override bool	RemoveIdx(int idx);
  override bool Transfer(taBase* item);

  virtual void	EnforceType();
  // #CAT_Modify enforce current type (all elements have to be of this type)
  void	EnforceSameStru(const taList_impl& cp);
  // #CAT_Modify make the two lists identical in terms of size and types of objects

  virtual bool	ChangeType(int idx, TypeDef* new_type);
  // change type of item at index
  virtual bool	ChangeType(TAPtr itm, TypeDef* new_type);
  // #MENU #ARG_ON_OBJ #CAT_Modify #TYPE_ON_el_base change type of item to new type, copying current info
  virtual int	ReplaceType(TypeDef* old_type, TypeDef* new_type);
  // #MENU #USE_RVAL #CAT_Modify #TYPE_ON_el_base replace all items of old type with new type (returns number changed)

  virtual int	FindTypeIdx(TypeDef* item_tp) const;
  // #CAT_Access find index of (first) element that inherits from given type (-1 = not found)
  virtual int	FindNameContainsIdx(const String& item_nm) const;
  // #CAT_Access Find index of (first) element whose name contains given name sub-string (-1 = nonot found)
  virtual int 	FindNameTypeIdx(const String& item_nm) const;
  // #CAT_Access Find index of (first) element with given object name or type name (item_nm) (-1 if not found)

  virtual taBase* FindType_(TypeDef* item_tp) const; 	// #IGNORE
  virtual taBase* FindNameContains_(const String& item_nm) const;
  // #IGNORE
  virtual taBase* FindNameType_(const String& item_nm) const;
  // #IGNORE

  void	SetBaseType(TypeDef* it); // #CAT_Modify set base (and default) type to given td

#if defined(TA_GUI) && !defined(__MAKETA__) 
  override const QPixmap* GetDataNodeBitmap(int bmf, int& flags_supported) const;
#endif
  override int		NumListCols() const {return 3;}
  // #IGNORE number of columns in a default list view for this list type
  override const 	KeyString GetListColKey(int col) const;
  // #IGNORE get the key for the default list view
  override String	GetColHeading(const KeyString& key) const;
  // #IGNORE header text for the indicated column
  override String	GetColText(const KeyString& key, int itm_idx = -1) const;
  override String	ChildGetColText(void* child, TypeDef* typ, const KeyString& key, 
					int itm_idx = -1) const;	// #IGNORE
  override int		SelectForEditSearch(const String& memb_contains, SelectEdit*& editor);

  void 	CutLinks();
  void	UpdateAfterEdit(); // we skip the taOBase version, and inherit only taBase
  TA_BASEFUNS(taList_impl);

protected:
  String	GetListName_() const	{ return name; }
  void		El_SetIndex_(void* it, int idx) {((TAPtr)it)->SetIndex(idx);}
  void		El_SetDefaultName_(void*, int idx); // sets default name if child has DEF_NAME_LIST 
  String	El_GetName_(void* it) const { return ((TAPtr)it)->GetName(); }
  void 		El_SetName_(void* it, const String& nm)  {((TAPtr)it)->SetName(nm);}
  TALPtr	El_GetOwnerList_(void* it) const 
  { return dynamic_cast<TABLPtr>(((TAPtr)it)->GetOwner()); }
  void*		El_GetOwnerObj_(void* it) const { return ((TAPtr)it)->GetOwner(); }
  void*		El_SetOwner_(void* it)	{ ((TAPtr)it)->SetOwner(this); return it; }
  bool		El_FindCheck_(void* it, const String& nm) const
  { if (((TAPtr)it)->FindCheck(nm)) {TALPtr own = El_GetOwnerList_(it);
    return ((!own) || (own == (TALPtr)this));} return false; }

  void*		El_Ref_(void* it)	{ taBase::Ref((TAPtr)it); return it; }
  void*		El_unRef_(void* it)	{ taBase::unRef((TAPtr)it); return it; }
  void		El_Done_(void* it)	{ taBase::Done((TAPtr)it); }
  void*		El_Own_(void* it)	{ taBase::Own((TAPtr)it,this); return it; }
  void		El_disOwn_(void* it)
  { if(El_GetOwnerList_(it) == this) {((TAPtr)it)->Destroying(); ((TAPtr)it)->CutLinks();}
    El_Done_(El_unRef_(it)); }
  // cut links to other objects when removed from owner group

  void*		El_MakeToken_(void* it) { return (void*)((TAPtr)it)->MakeToken(); }
  void*		El_Copy_(void* trg, void* src)
  { ((TAPtr)trg)->UnSafeCopy((TAPtr)src); return trg; }
  void* 	El_CopyN_(void* to, void* fm); // wrap in an update bracket
  
protected:
  override void CanCopy_impl(const taBase* cp_fm, bool quiet,
    bool& ok, bool virt) const;
  override void	CheckChildConfig_impl(bool quiet, bool& rval);
  override String ChildGetColText_impl(taBase* child, const KeyString& key, int itm_idx = -1) const;
  override taBase* New_impl(int n_objs, TypeDef* typ, const String& nm);
private:
  void 	Copy_(const taList_impl& cp);
  void 	Initialize();
  void	Destroy();
};

template<class T> 
class taList: public taList_impl {
  // #NO_TOKENS #INSTANCE #NO_UPDATE_AFTER a base list template
INHERITED(taList_impl)
public:
  T*		SafeEl(int idx) const		{ return (T*)SafeEl_(idx); }
  // #CAT_Access get element at index
  T*		PosSafeEl(int idx) const		{ return (T*)PosSafeEl_(idx); }
  // #IGNORE positive only, internal use
  T*		FastEl(int i) const		{ return (T*)el[i]; }
  // #CAT_Access fast element (no range checking)
  T* 		operator[](int i) const	{ return (T*)el[i]; }

  T*		DefaultEl() const		{ return (T*)DefaultEl_(); }
  // #CAT_Access returns the element specified as the default for this list

  T*		Edit_El(T* item) const		{ return SafeEl(FindEl((TAPtr)item)); }
  // #MENU #MENU_ON_Edit #USE_RVAL #ARG_ON_OBJ #CAT_Access Edit given list item

  T*		FindName(const String& item_nm) const
  { return (T*)FindName_(item_nm); }
  // #MENU #USE_RVAL #ARGC_1 #CAT_Access Find element with given name (item_nm)
  T*		FindNameContains(const String& item_nm) const
  { return (T*)FindNameContains_(item_nm); }
  // #MENU #USE_RVAL #ARGC_1 #CAT_Access Find element whose name contains given name sub-string
  T* 		FindType(TypeDef* item_tp) const
  { return (T*)FindType_(item_tp); }
  // #CAT_Access find given type element (NULL = not here), sets idx
  T*		FindNameType(const String& item_nm) const
  { return (T*)FindNameType_(item_nm); }
  // #CAT_Access Find element with given object name or type name (item_nm)

  T*		First() const			{ return (T*)First_(); }
  // #CAT_Access look at the first element; NULL if none
  T*		Pop()				{ return (T*)Pop_(); }
  // #CAT_Modify pop the last element off the stack
  T*		Peek() const			{ return (T*)Peek_(); }
  // #CAT_Access peek at the last element on the stack, if any

  virtual T*	AddUniqNameOld(T* item)		{ return (T*)AddUniqNameOld_((void*)item); }
  // #CAT_Modify add so that name is unique, old used if dupl, returns one used
  virtual T*	LinkUniqNameOld(T* item)	{ return (T*)LinkUniqNameOld_((void*)item); }
  // #CAT_Modify link so that name is unique, old used if dupl, returns one used

  virtual bool	MoveBefore(T* trg, T* item) { return MoveBefore_((void*)trg, (void*)item); }
  // #CAT_Modify move item so that it appears just before the target item trg in the list
  virtual bool	MoveAfter(T* trg, T* item) { return MoveAfter_((void*)trg, (void*)item); }
  // #CAT_Modify move item so that it appears just after the target item trg in the list

  TA_TMPLT_BASEFUNS(taList,T);
private:
  TMPLT_NOCOPY(taList,T);
  void	Initialize()			{ SetBaseType(StatTypeDef(0)); }
  void	Destroy()			{ };
};

// do not use this macro, since you typically will want ##NO_TOKENS, #NO_UPDATE_AFTER
// which cannot be put inside the macro!
//
// #define taList_of(T)
// class T ## _List : public taList<T> {
// public:
//   void Initialize()	{ };
//   void Destroy()	{ };
//   TA_BASEFUNS(T ## _List);
// }

// use the following as a template instead..

// define default base list to not keep tokens
class TA_API taBase_List : public taList<taBase> {
  // #NO_TOKENS ##NO_UPDATE_AFTER list of objects
INHERITED(taList<taBase>)
public:
  TA_BASEFUNS(taBase_List);
private:
  NOCOPY(taBase_List)
  void	Initialize() 		{ SetBaseType(&TA_taBase); }
  void 	Destroy()		{ };
};


/* taDataView -- exemplar base class of a view of an object, of class taOBase or later

   A view is a high-level depiction of an object, ex. "Network", "Graph", etc.

  The IDataViews list object does not own the view object -- some outer controller is
  responsible for lifetime management of dataview objects.

  However, if a dataobject is destroying, it will destroy all its views
  
  Most DataView objs are strictly managed by a parent, and so user should not
  be able to cut/paste/del them -- this is overridden for top-level view objs
  by overidding canCutPasteDel()

*/
class TA_API taDataView: public taOBase, public virtual IDataLinkClient {
  // #NO_TOKENS ##CAT_Display base class for views of an object
INHERITED(taOBase)
friend class DataView_List;
public:
  enum DataViewAction { // #BITS enum used to (safely) manually invoke one or more _impl actions
    CONSTR_POST		= 0x001, // #BIT (only used by DataViewer)
    CLEAR_IMPL		= 0x002, // #BIT (only used by T3DataView)
    RENDER_PRE		= 0x004, // #BIT
    RENDER_IMPL		= 0x008, // #BIT
    RENDER_POST		= 0x010, // #BIT
    CLOSE_WIN_IMPL	= 0x020,  // #BIT (only used by DataViewer)
    RESET_IMPL		= 0x040, // #BIT
    UNBIND_IMPL		= 0x080, // #BIT disconnect everyone from a data source
    SHOWING_IMPL	= 0x100, // #BIT
    HIDING_IMPL		= 0x200 // #BIT
#ifndef __MAKETA__
    ,CLEAR_ACTS		= CLEAR_IMPL | CLOSE_WIN_IMPL // for Clear
    ,RENDER_ACTS	= CLEAR_IMPL | RENDER_PRE | RENDER_IMPL | RENDER_POST // for Render
    ,REFRESH_ACTS	= RENDER_IMPL // for Refresh
    ,RESET_ACTS		= CLEAR_IMPL | CLOSE_WIN_IMPL | RESET_IMPL // for Reset
    ,UNBIND_ACTS	= UNBIND_IMPL
    
    ,SHOWING_HIDING_MASK = SHOWING_IMPL | HIDING_IMPL
    ,CONSTR_MASK	= CONSTR_POST | RENDER_PRE | RENDER_IMPL | RENDER_POST |
      SHOWING_HIDING_MASK
      // mask for doing child delegations in forward order
    ,DESTR_MASK		= CLEAR_IMPL | CLOSE_WIN_IMPL | RESET_IMPL | UNBIND_IMPL
      // mask for doing child delegations in reverse order
#endif
  };
  
  taBase*		m_data;		// #READ_ONLY #NO_SET_POINTER data -- referent of the item (not ref'ed)
  TypeDef*		data_base;	// #READ_ONLY #NO_SAVE Minimum type for data object

  taBase*		data() const {return m_data;} // subclasses usually redefine a strongly typed version
  void 			SetData(taBase* ta); // #MENU set the data to which this points -- must be subclass of data_base
  int			dbuCnt() {return m_dbu_cnt;} // batch update: -ve:data, 0:none, +ve:struct
  inline int		index() const {return m_index;} // convenience accessor
  virtual bool		isMapped() const {return true;} // for DataView classes, or anything w/ separate gui classes that get created distinct from view hierarchy
  virtual MemberDef*	md() const {return NULL;} // ISelectable property member stub
  virtual int		parDbuCnt(); // dbu of parent(s); note: only sign is accurate, not necessarily value (optimized)
  inline bool		hasParent() const {return (m_parent);} // encapsulated way to check for a par
  taDataView*		parent() const {return m_parent;} // typically lex override with strong type
  virtual TypeDef*	parentType() const {return &TA_taDataView;} // the controlling parent -- note that when in a list, this is the list owner, not the list; overrride for strong check in SetOwner
  virtual bool		isRootLevelView() const {return false;} // #IGNORE controls the default clip behavior, whereby root = allow child ops (cut, dup, etc.); not = do almost nothing
  virtual bool		isTopLevelView() const {return false;} // #IGNORE controls the default clip behavior, whereby top = do most stuff; not = do almost nothing

  virtual MemberDef*	GetDataMemberDef() {return NULL;} // returns md if known and/or knowable (ex. NULL for list members)
  virtual String	GetLabel() const; // returns a label suitable for tabview tabs, etc.
  virtual void		DataUpdateAfterEdit(); // note: normally overrride the _impl
  virtual void		DataUpdateAfterEdit_Child(taDataView* chld) 
    {DataUpdateAfterEdit_Child_impl(chld);}
    // optionally called by child in its DUAE routine; must be added manually
  virtual void 		ChildAdding(taDataView* child); // #IGNORE called from list;
  virtual void 		ChildRemoving(taDataView* child) {} // #IGNORE called from list; 
  virtual void		ChildClearing(taDataView* child) {} // override to implement par's portion of clear
  virtual void		ChildRendered(taDataView* child) {} // override to implement par's portion of render
  virtual void		CloseChild(taDataView* child) {}
  virtual void		SetVisible(bool showing); // called recursively when a view ctrl shows or hides
  virtual void		Render() {DoActions(RENDER_ACTS);} 
    // renders the visible contents (usually override the _impls) -- MUST BE DEFINED IN SUB
  virtual void		Clear(taDataView* par = NULL) {DoActions(CLEAR_ACTS);} // clears the view (but doesn't delete any components) (usually override _impl)
  virtual void		Reset() {DoActions(RESET_ACTS);} 
    // clears, and deletes any components (usually override _impls)
  virtual void		Refresh(){DoActions(REFRESH_ACTS);} // for manual refreshes -- just the impl stuff, not structural stuff
  virtual void		Unbind() {DoActions(UNBIND_ACTS);} 
    // clears, and deletes any components (usually override _impls)
  virtual void		DoActions(DataViewAction acts); // do the indicated action(s) if safe in this context (ex loading, whether gui exists, etc.)
  
  virtual void		ItemRemoving(taDataView* item) {} // items call this on the root item -- usually used by a viewer to insure item removed from things like sel lists
  virtual void		DataDestroying() {} // called when data is destroying (m_data will already be NULL)
  
  // special clip op queries, including Child_xx that gets forwarded from owned lists -- most view objs are managed by an owner, exception is very top-level objs; so defaults of following basically disallow things like Cut, Paste, and Delete (Copy and Paste Assign are allowed by default) -- all in ta_qtclipdata.cpp
  virtual void		DV_QueryEditActionsS(int& allowed, int& forbidden); // #IGNORE 
  virtual void		DV_QueryEditActionsD(taiMimeSource* ms,
    int& allowed, int& forbidden); // #IGNORE
  virtual void		DV_ChildQueryEditActions(const MemberDef* md,
    const taBase* child, const taiMimeSource* ms,
    int& allowed, int& forbidden); // #IGNORE note: this includes children of owned lists--md=NULL
protected: // the following just call inherited then insert the DV_ version 
  override void		QueryEditActionsS_impl(int& allowed, int& forbidden);
    // called once per src item, by controller
  override void		QueryEditActionsD_impl(taiMimeSource* ms,
    int& allowed, int& forbidden);
  override void		ChildQueryEditActions_impl(const MemberDef* md,
    const taBase* child, const taiMimeSource* ms,
    int& allowed, int& forbidden);

public:
  int	GetIndex() const {return m_index;}
  void	SetIndex(int value) {m_index = value;}
  TAPtr	SetOwner(TAPtr own); // update the parent; nulls it if not of parentType
  void	CutLinks();
  void	UpdateAfterEdit();
  TA_BASEFUNS(taDataView)

public: // IDataLinkCLient
  override void*	This() {return (void*)this;}
//in taBase  virtual TypeDef*	GetTypeDef() const;
  override TypeDef*	GetDataTypeDef() const 
    {return (m_data) ? m_data->GetTypeDef() : &TA_taBase;} // TypeDef of the data
  override bool		ignoreDataChanged() const {return (m_vis_cnt <= 0);}
  override bool		isDataView() const {return true;}
  override void		DataDataChanged(taDataLink* dl, int dcr, void* op1, void* op2);
  override void		IgnoredDataChanged(taDataLink* dl, int dcr,
    void* op1, void* op2);
  override void		DataLinkDestroying(taDataLink* dl); // called by DataLink when destroying; it will remove 

protected:
  int			m_dbu_cnt; // data batch update count; +ve is Structural, -ve is Parameteric only
  int			m_index; // for when in a list
  // NOTE: all Dataxxx_impl are supressed if dbuCnt or parDbuCnt <> 0 -- see ta_type.h for detailed rules
  mutable taDataView* 	m_parent; // autoset on SetOwner, type checked as well
  mutable short		m_vis_cnt; // visible count -- when >0, is visible, so refresh
  mutable signed char	m_defer_refresh; // while hidden: +1, struct; -1, data	
  
  override void		UpdateAfterEdit_impl();
  virtual void		DataDataChanged_impl(int dcr, void* op1, void* op2) {}
   // called when the data item has changed, esp. ex lists and groups, *except* UAE -- we also forward the last end of a batch update
  virtual void		DataUpdateAfterEdit_impl() {} // called by data for an UAE, i.e., after editing etc.
  virtual void		DataUpdateAfterEdit_Child_impl(taDataView* chld) {}
  virtual void		DataUpdateView_impl() { if(taMisc::gui_active) Render_impl(); } // called for Update All Views, and at end of a DataUpdate batch
  virtual void		DataRebuildView_impl() {} // called for Rebuild All Views, clients usually do beg/end both
  virtual void		DataStructUpdateEnd_impl() {} // called ONLY at end of a struct update -- derived classes usually do some kind of rebuild or render
  virtual void		DataChanged_Child(TAPtr child, int dcr, void* op1, void* op2) {} 
   // typically from an owned list
  virtual void		DoActionChildren_impl(DataViewAction acts) {} // only one action called at a time, if CONSTR do children in order, if DESTR do in reverse order; call child.DoActions(act)
  virtual void 		SetVisible_impl(DataViewAction act);
    // called when a viewer hides/shows (act is one of SHOWING or HIDING)
  virtual void		Constr_post() {DoActionChildren_impl(CONSTR_POST);} 
    // extend to implement post-constr
  virtual void		Clear_impl() {DoActionChildren_impl(CLEAR_IMPL);} 
    // extend to implement clear
  virtual void		CloseWindow_impl() {DoActionChildren_impl(CLOSE_WIN_IMPL);} 
    // extend to implement clear
  virtual void		Render_pre() {DoActionChildren_impl(RENDER_PRE);} 
    // extend with pre-rendering code, if needed
  virtual void		Render_impl() {DoActionChildren_impl(RENDER_IMPL);}
    // extend with code that renders the window contents
  virtual void		Render_post() {DoActionChildren_impl(RENDER_POST);}
    // extend with post-rendering code, if needed
  virtual void		Reset_impl() {DoActionChildren_impl(RESET_IMPL);}
    // extend to implement reset
  virtual void		Unbind_impl() {DoActionChildren_impl(UNBIND_IMPL);}
    // extend to implement unbind
private:
  void	Copy_(const taDataView& cp);
  void	Initialize();
  void	Destroy() {CutLinks();}
};
TA_SMART_PTRS(taDataView);

// for explicit lifetime management
#define TA_DATAVIEWFUNS(b,i) \
  TA_BASEFUNS(b)

// convenience, for declaring the safe strong-typed parent
#define DATAVIEW_PARENT(T) \
  T* parent() const {return (T*)m_parent;} \
  TypeDef* parentType() const {return &TA_ ## T;}
  
// for strongly-typed lists
#define TA_DATAVIEWLISTFUNS(B,I,T) \
  T* SafeEl(int i) const {return (T*)SafeEl_(i);} \
  T* FastEl(int i) const {return (T*)FastEl_(i);} \
  TA_BASEFUNS(B);

class TA_API DataView_List: public taList<taDataView> {
  // #NO_TOKENS ##CAT_Display ##NO_UPDATE_AFTER
INHERITED(taList<taDataView>)
public:
  override void 	DataChanged(int dcr, void* op1 = NULL, void* op2 = NULL);
    // we send to an owner DataView DataChanged_Child
  
  virtual void		DoAction(taDataView::DataViewAction act); 
   // do a single action on all items; we also do self->Reset on Reset_impl
  
  override TAPtr SetOwner(TAPtr); // #IGNORE
  TA_DATAVIEWLISTFUNS(DataView_List, taList<taDataView>, taDataView) //
  
protected:
  taDataView*		data_view; // #IGNORE our owner, when owned by a taDataView, for efficiency

  override void*	El_Own_(void* it);
  override void		El_disOwn_(void* it);
  override void		ChildQueryEditActionsL_impl(const MemberDef* md,
    const taBase* lst_itm, const taiMimeSource* ms,
    int& allowed, int& forbidden); // also forwards to dv owner; in ta_qtclipdata.cpp
  virtual void 		DV_ChildQueryEditActionsL_impl(const MemberDef* md,
    const taBase* lst_itm, const taiMimeSource* ms,
    int& allowed, int& forbidden); // specialized guys for DV -- can be replaced

private:
  NOCOPY(DataView_List)
  void 	Initialize() { SetBaseType(&TA_taDataView); data_view = NULL;}
  void	Destroy() {}
};


class TA_API taArray_base : public taOBase, public taArray_impl {
  // #VIRT_BASE #NO_TOKENS #NO_UPDATE_AFTER ##CAT_Data base for arrays (from taBase)
INHERITED(taOBase)
public:
  ostream& 	Output(ostream& strm, int indent = 0) const;
  ostream& 	OutputR(ostream& strm, int indent = 0) const
  { return Output(strm, indent); }

  override String GetValStr(void* par = NULL, MemberDef* md = NULL,
			    TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
			    bool force_inline = false) const;
  override bool  SetValStr(const String& val, void* par = NULL, MemberDef* md = NULL,
			   TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
			   bool force_inline = false);
  int		Dump_Save_Value(ostream& strm, TAPtr par=NULL, int indent = 0);
  int		Dump_Load_Value(istream& strm, TAPtr par=NULL);
  
  override void	DataChanged(int dcr, void* op1 = NULL, void* op2 = NULL); 

  void	CutLinks();
  TA_ABSTRACT_BASEFUNS(taArray_base);
private:
  void	Copy_(const taArray_base& cp) {taArray_impl::Copy_Duplicate(cp);}
    //WARNING: Copy_Duplicate is not a true copy, but we retain the behavior for compatibility
  void 	Initialize()	{ }
  void 	Destroy()	{ CutLinks(); }
};


#define TA_ARRAY_FUNS(y,T) \
public: \
  explicit y(int init_size) {Initialize(); SetSize(init_size); } \
  T&		operator[](int i) { return el[i]; } \
  const T&	operator[](int i) const	{ return el[i]; } \
protected: \
  override const void*	El_GetBlank_() const	{ return (const void*)&blank; }

#define TA_ARRAY_OPS(y) \
  inline bool operator ==(const y& a, const y& b) {return a.Equal_(b);} \
  inline bool operator !=(const y& a, const y& b) {return !(a.Equal_(b));} \
  TA_SMART_PTRS(y)

template<class T>
class taArray : public taArray_base {
  // #VIRT_BASE #NO_TOKENS #NO_UPDATE_AFTER
INHERITED(taArray_base)
public:
  T*		el;		// #NO_SHOW #NO_SAVE Pointer to actual array memory
  T		err;		// #NO_SHOW what is returned when out of range; MUST INIT IN CONSTRUCTOR
  
  ////////////////////////////////////////////////
  // 	functions that return the type		//
  ////////////////////////////////////////////////

  const T&	SafeEl(int i) const {return *(T*)SafeEl_(i);}
  // #MENU #MENU_ON_Edit #USE_RVAL the element at the given index
  T&		FastEl(int i)  {return el[i]; }
  // fast element (no range checking)
  const T&	FastEl(int i) const	{return el[i]; }
  // fast element (no range checking)
  const T&	RevEl(int idx) const	{ return SafeEl(size - idx - 1); }
  // reverse (index) element (ie. get from the back of the list first)

  const T	Pop() 
    {if (size == 0) return *(static_cast<const T*>(El_GetErr_()));
     else return el[--size]; }
  // pop the last item in the array off
  const T& Peek() const {return SafeEl(size - 1);}
  // peek at the last item on the array

  ////////////////////////////////////////////////
  // 	functions that are passed el of type	//
  ////////////////////////////////////////////////

  void	Set(int i, const T& item) 	
    { if (InRange(i)) el[i] = item; }
  // use this for safely assigning values to items in the array (Set should update if needed)
  void	Add(const T& item)		{ Add_((void*)&item); }
  // #MENU add the item to the array
  bool	AddUnique(const T& item)	{ return AddUnique_((void*)&item); }
  // add the item to the array if it isn't already on it, returns true if unique
  void	Push(const T& item)		{ Add(item); }
  // #CAT_Modify push the item on the end of the array (same as add)
  void	Insert(const T& item, int indx, int n_els=1)	{ Insert_((void*)&item, indx, n_els); }
  // #MENU Insert (n_els) item(s) at indx (-1 for end) in the array
  int	FindEl(const T& item, int indx=0) const { return FindEl_((void*)&item, indx); }
  // #MENU #USE_RVAL Find item starting from indx in the array (-1 if not there)
//  virtual bool	Remove(const T& item)		{ return Remove_((void*)&item); }
//  virtual bool	Remove(uint indx, int n_els=1)	{ return taArray_impl::Remove(indx,n_els); }
  // Remove (n_els) item(s) at indx, returns success
  virtual bool	RemoveEl(const T& item)		{ return RemoveEl_((void*)&item); }
  // remove given item, returns success
  virtual void	InitVals(const T& item, int start=0, int end=-1) { InitVals_((void*)&item, start, end); }
  // set array elements to specified value starting at start through end (-1 = size)

/*  taArray()				{ el = NULL; } // no_tokens is assumed
  ~taArray()				{ SetArray_(NULL); }
  
  void  UnSafeCopy(TAPtr cp) { 
    if(cp->InheritsFrom(taArray::StatTypeDef(0))) Copy(*((taArray<T>*)cp));
    else if(InheritsFrom(cp->GetTypeDef())) cp->CastCopyTo(this); }
  void  CastCopyTo(TAPtr cp)            { taArray<T>& rf = *((taArray<T>*)cp); rf.Copy(*this); } //*/
  TA_TMPLT_BASEFUNS_NOCOPY(taArray,T); //
public:
  void*		FastEl_(int i)			{ return &(el[i]); }// #IGNORE
protected:
  mutable T		tmp; // #IGNORE temporary item
  
  override void*	MakeArray_(int n) const	{ return new T[n]; }
  override void		SetArray_(void* nw) {if (el) delete [] el; el = (T*)nw;}
  void		El_Copy_(void* to, const void* fm) { *((T*)to) = *((T*)fm); }
  uint		El_SizeOf_() const		{ return sizeof(T); }
  const void*	El_GetErr_() const		{ return (void*)&err; }
  void*		El_GetTmp_() const		{ return (void*)&tmp; } //
private:
  void 	Initialize() { el = NULL; } 
  void 	Destroy() { SetArray_(NULL); }
};

class TA_API int_Array : public taArray<int> {
  // #NO_UPDATE_AFTER #NO_TOKENS array of ints
INHERITED(taArray<int>)
public:
  STATIC_CONST int blank; // #HIDDEN #READ_ONLY 
  virtual void	FillSeq(int start=0, int inc=1);
  // fill array with sequential values starting at start, incrementing by inc
  
  override void*	GetTA_Element(Variant i, TypeDef*& eltd) 
  { eltd = &TA_int; int dx = i.toInt(); if(InRange(dx)) return FastEl_(dx); return NULL; }
  NOCOPY(int_Array)
  void Initialize()	{err = 0; };
  void Destroy()	{ }; //
  //note: Register() is not necessary for arrays, so we omit in these convenience constructors
  int_Array(int num, int i0) {Initialize(); SetSize(1); el[0] = i0;}
  int_Array(int num, int i0, int i1) {Initialize(); SetSize(2); el[0] = i0; el[1] = i1;}
  int_Array(int num, int i0, int i1, int i2) 
    {Initialize(); SetSize(3); el[0] = i0; el[1] = i1; el[2] = i2;}
  int_Array(int num, int i0, int i1, int i2, int i3) 
    {Initialize(); SetSize(4); el[0] = i0; el[1] = i1; el[2] = i2; el[3] = i3;}
  TA_BASEFUNS(int_Array);
  TA_ARRAY_FUNS(int_Array, int)
protected:
  int		El_Compare_(const void* a, const void* b) const
  { int rval=-1; if(*((int*)a) > *((int*)b)) rval=1; else if(*((int*)a) == *((int*)b)) rval=0; return rval; }
  bool		El_Equal_(const void* a, const void* b) const
    { return (*((int*)a) == *((int*)b)); }
  String	El_GetStr_(const void* it) const { return (*((int*)it)); }
  void		El_SetFmStr_(void* it, const String& val)
  { int tmp = (int)val; *((int*)it) = tmp; }
};
TA_ARRAY_OPS(int_Array)


class TA_API float_Array : public taArray<float> {
  // #NO_UPDATE_AFTER #NO_TOKENS array of floats
INHERITED(taArray<float>)
public:
  STATIC_CONST float blank; // #HIDDEN #READ_ONLY 
  override void*	GetTA_Element(Variant i, TypeDef*& eltd) 
  { eltd = &TA_float; int dx = i.toInt(); if(InRange(dx)) return FastEl_(dx); return NULL; }
  TA_BASEFUNS_NOCOPY(float_Array);
  TA_ARRAY_FUNS(float_Array, float)
protected:
  int		El_Compare_(const void* a, const void* b) const
  { int rval=-1; if(*((float*)a) > *((float*)b)) rval=1; else if(*((float*)a) == *((float*)b)) rval=0; return rval; }
  bool		El_Equal_(const void* a, const void* b) const
    { return (*((float*)a) == *((float*)b)); }
  String	El_GetStr_(const void* it) const { return (*((float*)it)); }
  void		El_SetFmStr_(void* it, const String& val)
  { float tmp = (float)val; *((float*)it) = tmp; }
private:
  void Initialize()	{err = 0.0f; };
  void Destroy()	{ };
};
TA_ARRAY_OPS(float_Array)

class TA_API double_Array : public taArray<double> {
  // #NO_UPDATE_AFTER #NO_TOKENS array of doubles
INHERITED(taArray<double>)
public:
  STATIC_CONST double blank; // #HIDDEN #READ_ONLY 
  override void*	GetTA_Element(Variant i, TypeDef*& eltd) 
  { eltd = &TA_double; int dx = i.toInt(); if(InRange(dx)) return FastEl_(dx); return NULL; }
  TA_BASEFUNS_NOCOPY(double_Array);
  TA_ARRAY_FUNS(double_Array, double)
protected:
  int		El_Compare_(const void* a, const void* b) const
  { int rval=-1; if(*((double*)a) > *((double*)b)) rval=1; else if(*((double*)a) == *((double*)b)) rval=0; return rval; }
  bool		El_Equal_(const void* a, const void* b) const
    { return (*((double*)a) == *((double*)b)); }
  String	El_GetStr_(const void* it) const { return (*((double*)it)); }
  void		El_SetFmStr_(void* it, const String& val)
  { double tmp = (double)val; *((double*)it) = tmp; }
private:
  void Initialize()	{err = 0.0;};
  void Destroy()	{ };
};
TA_ARRAY_OPS(double_Array)

class TA_API char_Array : public taArray<char> {
  // #NO_UPDATE_AFTER #NO_TOKENS array of chars (bytes)
INHERITED(taArray<char>)
public:
  STATIC_CONST char blank; // #HIDDEN #READ_ONLY 
  override void*	GetTA_Element(Variant i, TypeDef*& eltd) 
  { eltd = &TA_char; int dx = i.toInt(); if(InRange(dx)) return FastEl_(dx); return NULL; }
  TA_BASEFUNS(char_Array);
  TA_ARRAY_FUNS(char_Array, char)
protected:
  int		El_Compare_(const void* a, const void* b) const
  { int rval=-1; if(*((char*)a) > *((char*)b)) rval=1; else if(*((char*)a) == *((char*)b)) rval=0; return rval; }
  bool		El_Equal_(const void* a, const void* b) const
    { return (*((char*)a) == *((char*)b)); }
  String	El_GetStr_(const void* it) const { return (*((char*)it)); }
  void		El_SetFmStr_(void* it, const String& val)
  { char tmp = val[0]; *((char*)it) = tmp; }
private:
  NOCOPY(char_Array)
  void Initialize()	{err = ' ';};
  void Destroy()	{ };
};
TA_ARRAY_OPS(char_Array)


class TA_API String_Array : public taArray<String> {
  // #NO_UPDATE_AFTER #NO_TOKENS array of Strings
INHERITED(taArray<String>)
public:
  STATIC_CONST String blank; // #HIDDEN #READ_ONLY 
  override void*	GetTA_Element(Variant i, TypeDef*& eltd) 
  { eltd = &TA_taString; int dx = i.toInt(); if(InRange(dx)) return FastEl_(dx); return NULL; }
#ifdef TA_USE_QT
  void			ToQStringList(QStringList& sl); // #IGNORE fills a QStringList
#endif
  TA_BASEFUNS(String_Array);
  TA_ARRAY_FUNS(String_Array, String)
protected:
  int		El_Compare_(const void* a, const void* b) const
  { int rval=-1; if(*((String*)a) > *((String*)b)) rval=1; else if(*((String*)a) == *((String*)b)) rval=0; return rval; }
  bool		El_Equal_(const void* a, const void* b) const
    { return (*((String*)a) == *((String*)b)); }
  String	El_GetStr_(const void* it) const { return (*((String*)it)); }
  void		El_SetFmStr_(void* it, const String& val)
  {*((String*)it) = val; }
private:
  void	Copy_(const String_Array&){}
  void Initialize()	{ };
  void Destroy()	{ };
};
TA_ARRAY_OPS(String_Array)


class TA_API SArg_Array : public String_Array {
  // ##CAT_Program string argument array: has labels for each argument to make it easier in the interface
INHERITED(String_Array)
public:
  String_Array	labels;		// #HIDDEN labels for each argument

  // note: all following key-based api routines are case sensitive
  bool		HasValue(const String& key) const;
    // returns true if there is an entry for the key
  String	GetValue(const String& key) const;
    // return the value for the key, or nil if none
  void		SetValue(const String& key, const String& value);
    // set or update the value for the key
    
  int		Dump_Save_Value(ostream& strm, TAPtr par=NULL, int indent = 0);
  int		Dump_Load_Value(istream& strm, TAPtr par=NULL);
  void	UpdateAfterEdit();
  void	InitLinks();
  TA_BASEFUNS(SArg_Array);
private:
  void	Copy_(const SArg_Array& cp);
  void	Initialize();
  void	Destroy()	{ };
};

class TA_API Variant_Array : public taArray<Variant> {
  // #NO_UPDATE_AFTER #NO_TOKENS array of variants
INHERITED(taArray<Variant>)
public:
  STATIC_CONST Variant blank; // #HIDDEN #READ_ONLY 
  override void*	GetTA_Element(Variant i, TypeDef*& eltd) 
  { eltd = &TA_Variant; int dx = i.toInt(); if(InRange(dx)) return FastEl_(dx); return NULL; }
  TA_BASEFUNS_NOCOPY(Variant_Array);
  TA_ARRAY_FUNS(Variant_Array, Variant)
protected:
  int		El_Compare_(const void* a, const void* b) const
  { int rval=-1; if(*((Variant*)a) > *((Variant*)b)) rval=1; else if(*((Variant*)a) == *((Variant*)b)) rval=0; return rval; }
  bool		El_Equal_(const void* a, const void* b) const
    { return (*((Variant*)a) == *((Variant*)b)); }
  String	El_GetStr_(const void* it) const { return ((Variant*)it)->toString(); }
  void		El_SetFmStr_(void* it, const String& val)
  { Variant tmp = (Variant)val; *((Variant*)it) = tmp; }
private:
  void Initialize()	{err = 0.0;};
  void Destroy()	{ };
};
TA_ARRAY_OPS(Variant_Array)


class TA_API voidptr_Array : public taArray<voidptr> {
  // #NO_UPDATE_AFTER #NO_TOKENS array of void pointers
INHERITED(taArray<voidptr>)
public:
  STATIC_CONST voidptr blank; // #HIDDEN #READ_ONLY 

  override void*	GetTA_Element(Variant i, TypeDef*& eltd) 
  { eltd = &TA_voidptr; int dx = i.toInt(); if(InRange(dx)) return FastEl_(dx); return NULL; }
  TA_BASEFUNS_NOCOPY(voidptr_Array);
  TA_ARRAY_FUNS(voidptr_Array, voidptr)
protected:
  bool		El_Equal_(const void* a, const void* b) const
    { return (*((voidptr*)a) == *((voidptr*)b)); }
private:
  void Initialize()	{err = 0; };
  void Destroy()	{ };
};

class TA_API NameVar_Array : public taArray<NameVar> {
  // #NO_TOKENS an array of name value (variant) items
INHERITED(taArray<NameVar>)
public:
  STATIC_CONST String	def_sep; // ", " default separator
  STATIC_CONST NameVar blank; // #HIDDEN #READ_ONLY 

  int	FindName(const String& nm, int start=0) const;
  // find by name  (start < 0 = from end)
  int	FindNameContains(const String& nm, int start=0) const;
  // find by name containing nm (start < 0 = from end)
  int	FindValue(const Variant& var, int start=0) const;
  // find by value (start < 0 = from end)  
  int	FindValueContains(const String& vl, int start=0) const;
  // find by value.toString() containing vl (start < 0 = from end)

  Variant	GetVal(const String& nm);
  // get value from name; isNull if not found
  bool		GetAllVals(const String& nm, String_Array& vals);
  // get all values having given name (converts to strings)
  bool		SetVal(const String& nm, const Variant& vl);
  // set value by name; if name already on list, it is updated (rval = true); else new item added
  override void*	GetTA_Element(Variant i, TypeDef*& eltd) 
  { eltd = &TA_NameVar; int dx = i.toInt(); if(InRange(dx)) return FastEl_(dx); return NULL; }
  TA_BASEFUNS_NOCOPY(NameVar_Array);
  TA_ARRAY_FUNS(NameVar_Array, NameVar)
protected:
  int		El_Compare_(const void* a, const void* b) const
  { int rval=-1; if(((NameVar*)a)->value > ((NameVar*)b)->value) rval=1; else if(((NameVar*)a)->value == ((NameVar*)b)->value) rval=0; return rval; }
  bool		El_Equal_(const void* a, const void* b) const
  { return (((NameVar*)a)->value == ((NameVar*)b)->value); }
  String	El_GetStr_(const void* it) const { return ((NameVar*)it)->GetStr(); }
  void		El_SetFmStr_(void* it, const String& val) { ((NameVar*)it)->SetFmStr(val); }
private:
  void Initialize()	{ };
  void Destroy()	{ };
};
TA_ARRAY_OPS(NameVar_Array)


class TA_API FunCallItem {
// #NO_INSTANCE
public:
  taBase*		it;
  String		fun_name;
  FunCallItem(taBase* it_, const String& fn) {it = it_; fun_name = fn;}
};

class TA_API taBase_FunCallList: public taPtrList<FunCallItem>, public IMultiDataLinkClient {
  // #INSTANCE function call list manager
INHERITED(taPtrList<FunCallItem>)
public:

  bool	AddBaseFun(taBase* obj, const String& fun_name); // add base + function -- no check for unique on base_funs
  
  taBase_FunCallList() {}
  ~taBase_FunCallList() {Reset();}

public: // ITypedObject interface
  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_taBase_FunCallList;}

public: // IDataLinkClient interface
  override void		DataLinkDestroying(taDataLink* dl);
  override void		DataDataChanged(taDataLink* dl, int dcr, void* op1, void* op2) {}

protected:
  override void	El_Done_(void* it); // unref link

private:
  void Initialize();
  void Destroy();
};

class TA_API UserDataItemBase: public taNBase {
  // ##INLINE ##NO_TOKENS base class for all simple user data -- name is key
INHERITED(taNBase)
public://
//note: we hide the name, because we don't want it inline since we have a dedicated editor
#ifdef __MAKETA__
  String		name; // #HIDDEN #READ_ONLY
#endif

  virtual bool		canDelete() const {return false;}
    // whether item can be manually deleted by user
  virtual bool		canRename() const {return false;}
    // but rename system-created items at your own peril!!!
  virtual bool		isSimple() const {return false;}
    // only true for UserDataItem class
  virtual bool		isVisible() const {return false;}
    // in general, custom guys are hidden on UserData page, unless they override
    
  virtual const Variant valueAsVariant() const {return _nilVariant;}
  virtual bool		setValueAsVariant(const Variant& value) {return false;}
  virtual bool		SetDesc(const String& desc) {return false;}
  
  TA_BASEFUNS(UserDataItemBase)
protected:
  UserDataItemBase(const String& type_name, const String& key); // for schema constructors
private:
  NOCOPY(UserDataItemBase)
  void Initialize() {}
  void Destroy() {}
};
TA_SMART_PTRS(UserDataItemBase);

class TA_API UserDataItem: public UserDataItemBase {
  // an item of simple user data
INHERITED(UserDataItemBase)
public:
  Variant		value;
  String		desc; // #NO_SAVE_EMPTY optional description (typ. used for schema, not items)
  
  override bool		canDelete() const {return true;}
  override bool		canRename() const {return true;}
  override bool		isSimple() const {return true;}
  override bool		isVisible() const {return true;}
  
  override const Variant valueAsVariant() const {return value;}
  override bool		setValueAsVariant(const Variant& v) {value = v; return true;}
  
  override String	GetDesc() const {return desc;}
  override bool		SetDesc(const String& d) {desc = d; return true;}
  TA_BASEFUNS(UserDataItem)
  UserDataItem(const String& type_name, const String& key, const Variant& value,
	       const String& desc = _nilString);
  // #IGNORE constructor for creating static (compile-time) schema instances
private:
  void Copy_(const UserDataItem& cp){value = cp.value; desc = cp.desc;}
  void Initialize() {}
  void Destroy() {}
};
TA_SMART_PTRS(UserDataItem);

#endif // ta_base_h
