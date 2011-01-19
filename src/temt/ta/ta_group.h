// Copyright, 1995-2007, Regents of the University of Colorado,
// Carnegie Mellon University, Princeton University.
//
// This file is part of The Emergent Toolkit
//
//   This library is free software; you can redistribute it and/or
//   modify it under the terms of the GNU Lesser General Public
//   License as published by the Free Software Foundation; either
//   version 2.1 of the License, or (at your option) any later version.
//   
//   This library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   Lesser General Public License for more details.


#ifndef ta_group_h
#define ta_group_h 1

#include "ta_base.h"

#ifndef __MAKETA__
#include <sstream>
#endif

class 	TA_API taGroup_impl;
typedef taGroup_impl* 	TAGPtr;

typedef taList<taGroup_impl> TALOG; // list of groups (LOG)

class 	TA_API taSubGroup : public TALOG {
  // #INSTANCE ##NO_TOKENS ##NO_UPDATE_AFTER has the sub-groups for a group
INHERITED(TALOG)
public:
  override  void DataChanged(int dcr, void* op1 = NULL, void* op2 = NULL); // forward LIST events as GROUP events to owner

  bool	Transfer(taBase* item);

  TA_BASEFUNS_NOCOPY(taSubGroup);
private:
  void	Initialize()	{ };
  void	Destroy()	{ };
};

typedef int taGroupItr;

class	TA_API taLeafItr {		// contains the indicies for iterating over leafs
public:
  TAGPtr	cgp;		// pointer to current group
  int		g;		// index of current group
  int		i;		// index of current leaf element
};

// forward iteration *cannot* involve adding or deleting
#define FOR_ITR_EL(T, el, grp, itr) \
for(el = (T*) grp FirstEl(itr); el; el = (T*) grp NextEl(itr))

// reverse iteration is allowed to involve deleting of current or later element
#define FOR_ITR_EL_REV(T, el, grp, itr) \
for(el = (T*) grp LastEl(itr); el; el = (T*) grp PrevEl(itr))

#define FOR_ITR_GP(T, el, grp, itr) \
for(el = (T*) grp FirstGp(itr); el; el = (T*) grp NextGp(itr))


class TA_API taGroup_impl : public taList_impl {
  // #INSTANCE #NO_UPDATE_AFTER #STEM_BASE implementation of a group
INHERITED(taList_impl)
public:
  static bool	def_nw_item; // #IGNORE default
  virtual TAGPtr GetSuperGp_();			// #IGNORE Parent super-group, or NULL
  virtual void	 UpdateLeafCount_(int no); 	// #IGNORE updates the leaves count

public:
  int 		leaves;		// #READ_ONLY #NO_SAVE #CAT_taList total number of leaves
  taSubGroup	gp; 		// #NO_SHOW #NO_FIND #NO_SAVE #CAT_taList sub-groups within this one
  TAGPtr	super_gp;	// #READ_ONLY #NO_SHOW #NO_SAVE #NO_SET_POINTER #CAT_taList super-group above this
  TAGPtr	root_gp; 	// #READ_ONLY #NO_SHOW #NO_SAVE #NO_SET_POINTER #CAT_taList the root group, 'this' for root group itself; never NULL

  bool		IsEmpty() const	{ return (leaves == 0) ? true : false; }
  bool		IsRoot() const	{ return (root_gp == this); } // 'true' if this is the root
  override void	DataChanged(int dcr, void* op1 = NULL, void* op2 = NULL);

  void* 	FindMembeR(const String& nm, MemberDef*& ret_md) const;

  // IO routines
  ostream& 	OutputR(ostream& strm, int indent = 0) const;

  override String GetValStr(void* par = NULL, MemberDef* md = NULL,
			    TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
			    bool force_inline = false) const;
  override bool  SetValStr(const String& val, void* par = NULL, MemberDef* md = NULL,
			   TypeDef::StrContext sc = TypeDef::SC_DEFAULT,
			   bool force_inline = false);

  override taObjDiffRec* GetObjDiffVal(taObjDiff_List& odl, int nest_lev,
				       MemberDef* memb_def=NULL, const void* par=NULL,
				       TypeDef* par_typ=NULL, taObjDiffRec* par_od=NULL) const;

  override void Dump_Save_GetPluginDeps(); // note: in ta_dump.cpp
  override int	Dump_SaveR(ostream& strm, taBase* par=NULL, int indent=0);
  override int	Dump_Save_PathR(ostream& strm, taBase* par=NULL, int indent=0);
  override int	Dump_Save_PathR_impl(ostream& strm, taBase* par=NULL, int indent=0);

  override void	Search_impl(const String& srch, taBase_PtrList& items,
			    taBase_PtrList* owners = NULL, 
			    bool contains = true, bool case_sensitive = false,
			    bool obj_name = true, bool obj_type = true,
			    bool obj_desc = true, bool obj_val = true,
			    bool mbr_name = true, bool type_desc = false);
  override void	CompareSameTypeR(Member_List& mds, TypeSpace& base_types,
				 voidptr_PArray& trg_bases, voidptr_PArray& src_bases,
				 taBase* cp_base,
				 int show_forbidden = taMisc::USE_SHOW_GUI_DEF,
				 int show_allowed = taMisc::SHOW_CHECK_MASK,
				 bool no_ptrs = true);
  override int	UpdatePointers_NewPar(taBase* old_par, taBase* new_par);
  override int	UpdatePointers_NewParType(TypeDef* par_typ, taBase* new_par);
  override int	UpdatePointers_NewObj(taBase* old_ptr, taBase* new_ptr);
  override int 	UpdatePointersToMyKids_impl(taBase* scope_obj, taBase* new_ptr);
  override int	SelectForEditSearch(const String& memb_contains, SelectEdit*& editor); // 

  ////////////////////////////////////////////////
  // 	functions that return the type		//
  ////////////////////////////////////////////////
#ifndef __MAKETA__
  TAGPtr	Gp_(int i) const	{ return gp.SafeEl(i); } // #IGNORE
  TAGPtr 	FastGp_(int i) const	{ return gp.FastEl(i); } // #IGNORE
  virtual taBase* Leaf_(int idx) const;	// #IGNORE DFS through all subroups for leaf i
  TAGPtr 	FastLeafGp_(int gp_idx) const // #IGNORE the flat leaf group, note: 0 is "this"
    { if (gp_idx == 0) return const_cast<TAGPtr>(this); if (!leaf_gp) InitLeafGp();
      return (TAGPtr)leaf_gp->el[gp_idx];}
  TAGPtr 	SafeLeafGp_(int gp_idx) const; // #IGNORE the flat leaf group, note: 0 is "this"
  
  // iterator-like functions
  TAGPtr 	FirstGp_(int& g) const	// #IGNORE first sub-gp
  { g = 0; if(leaf_gp == NULL) InitLeafGp(); return leaf_gp->SafeEl(0); }
  TAGPtr 	LastGp_(int& g) const	// #IGNORE last sub-gp (for rev iter)
  {if(leaf_gp == NULL) InitLeafGp(); g = leaf_gp->size - 1; return leaf_gp->Peek(); }
  TAGPtr 	NextGp_(int& g)	const	// #IGNORE next sub-gp
  { return leaf_gp->SafeEl(++g); }
  int	 	LeafGpCount()	const	// #IGNORE count of leaf groups **including self**; optimized for no subgroups
    { if (gp.size == 0) return 1; if (leaf_gp == NULL) InitLeafGp(); return leaf_gp->size; }

  taBase*	 	FirstEl_(taLeafItr& lf) const	// #IGNORE first leaf iter init
  { taBase* rval=NULL; lf.i = 0; lf.cgp = FirstGp_(lf.g);
    if(lf.cgp != NULL) rval=(taBase*)lf.cgp->el[0]; return rval; }
  inline taBase*	FirstEl(taLeafItr& lf) const {return FirstEl_(lf);} // #IGNORE
  taBase*	 	NextEl_(taLeafItr& lf)	const	// #IGNORE next leaf
  { if (++lf.i >= lf.cgp->size) {
    lf.i = 0; if (!(lf.cgp = leaf_gp->SafeEl(++lf.g))) return NULL; }
    return (taBase*)lf.cgp->el[lf.i];}
  inline taBase*	NextEl(taLeafItr& lf) const {return NextEl_(lf);} // #IGNORE

  taBase*	 	LastEl_(taLeafItr& lf) const	// #IGNORE last leaf iter init
  { if (!(lf.cgp = LastGp_(lf.g))) return NULL;
    lf.i = lf.cgp->size - 1; return (taBase*)lf.cgp->el[lf.i];  }
  inline taBase*	LastEl(taLeafItr& lf) const {return LastEl_(lf);} // #IGNORE
  taBase*	 	PrevEl_(taLeafItr& lf)	const	// #IGNORE prev leaf -- delete item safe
  { if (--lf.i < 0) {
      if (leaf_gp == NULL) InitLeafGp(); // in case we did a delete of an item
      if (!(lf.cgp = leaf_gp->SafeEl(--lf.g))) return NULL; 
      lf.i = lf.cgp->size - 1;}
    return (taBase*)lf.cgp->el[lf.i];}
  inline taBase*	PrevEl(taLeafItr& lf) const {return PrevEl_(lf);} // #IGNORE
#endif
  virtual TAGPtr  NewGp_(int no, TypeDef* typ=NULL, const String& name_ = "");
    // #IGNORE create sub groups
  virtual taBase* NewEl_(int no, TypeDef* typ=NULL);	// #IGNORE create items

  TAGPtr  		NewGp_gui(int n_gps=1, TypeDef* typ=NULL,
    const String& name="");
  // #BUTTON #MENU #MENU_ON_Object #MENU_CONTEXT #TYPE_this #NULL_OK_typ #NULL_TEXT_SameType #LABEL_NewGroup #NO_SAVE_ARG_VAL #CAT_Modify Create and add n_gps new sub group(s) of given type (typ=NULL: same type as this group)

  virtual taBase* FindLeafName_(const String& it) const; 	// #IGNORE
  virtual taBase* FindLeafNameContains_(const String& it) const;	// #IGNORE
  virtual taBase* FindLeafType_(TypeDef* it) const;	// #IGNORE
  virtual taBase* FindLeafNameType_(const String& it) const;	// #IGNORE

  virtual int 	FindLeafNameIdx(const String& item_nm) const;
  // #CAT_Access Find element anywhere in full group and subgroups with given name (item_nm)
  virtual int 	FindLeafNameContainsIdx(const String& item_nm) const;
  // #MENU #MENU_ON_Edit #USE_RVAL #ARGC_1 #LABEL_Find #CAT_Access Find anywhere in full group and subgroups first element whose name contains given name (item_nm)
  virtual int 	FindLeafTypeIdx(TypeDef* item_tp) const;
  // #CAT_Access find anywhere in full group and subgroups given type leaf element (NULL = not here)
  virtual int	FindLeafNameTypeIdx(const String& item_nm) const;
  // #CAT_Access Find anywhere in full group and subgroups element with given object name or type name (item_nm)

  virtual TAGPtr FindMakeGpName(const String& gp_nm, TypeDef* typ=NULL,
    bool& nw_item=def_nw_item);
  // #IGNORE find subgroup of given name; if it doesn't exist, then make it (using type if specified, else default type for subgroup)

  ////////////////////////////////////////////////
  // functions that don't depend on the type	//
  ////////////////////////////////////////////////

  virtual void	InitLeafGp() const;
  // #CAT_Access Initialize the leaf group iter list, always ok to call
  virtual void	InitLeafGp_impl(TALOG* lg) const; // #IGNORE impl of init leaf gp
  virtual void	AddOnly_(void* it); 		// #IGNORE update leaf count
//  virtual bool	Remove(const char* item_nm)	{ return taList_impl::Remove(item_nm); }
//  virtual bool	Remove(taBase* item)		{ return taList_impl::Remove(item); }

  virtual bool 	RemoveLeafEl(taBase* item);
  // #CAT_Modify remove given leaf element
  virtual bool	RemoveLeafName(const char* item_nm);
  // #CAT_Modify remove given named leaf element
  virtual bool  RemoveLeafIdx(int idx);
  // #CAT_Modify Remove leaf element at leaf index
  virtual void 	RemoveAll();
  // #CAT_Modify Remove all elements of the group

  virtual bool	RemoveGpIdx(int idx) 			{ return gp.RemoveIdx(idx); }
  // #CAT_Modify remove group at given index
  virtual bool	RemoveGpEl(TAGPtr group)		{ return gp.RemoveEl(group); }
  // #MENU #FROM_GROUP_gp #MENU_ON_Edit #CAT_Modify remove given group
  virtual TALOG* EditSubGps() 				{ return &gp; }
  // #MENU #USE_RVAL #CAT_Access edit the list of sub-groups (e.g., so you can move around subgroups)

  virtual void	EnforceLeaves(int sz);
  // #CAT_Modify ensure that sz leaves exits by adding new ones to top group and removing old ones from end
  void	EnforceSameStru(const taGroup_impl& cp);
  // #CAT_Modify enforce this group to have same structure as cp

  int	ReplaceType(TypeDef* old_type, TypeDef* new_type);

  virtual int	FindLeafEl(taBase* item) const;  // find given leaf element (-1 = not here)
  // #CAT_Access find given leaf element -1 = not here.

  override bool		ChildCanDuplicate(const taBase* chld,
    bool quiet = true) const;
  override taBase*	ChildDuplicate(const taBase* chld);
  
  void	Duplicate(const taGroup_impl& cp);
  void	DupeUniqNameOld(const taGroup_impl& cp);
  void	DupeUniqNameNew(const taGroup_impl& cp);

  void	Borrow(const taGroup_impl& cp);
  void	BorrowUnique(const taGroup_impl& cp);
  void	BorrowUniqNameOld(const taGroup_impl& cp);
  void	BorrowUniqNameNew(const taGroup_impl& cp);

  void	Copy_Common(const taGroup_impl& cp);
  void	Copy_Duplicate(const taGroup_impl& cp);
  void	Copy_Borrow(const taGroup_impl& cp);

  virtual void 	List(ostream& strm=cout) const; // Display list of elements in the group

  void 	InitLinks();		// inherit the el_typ from parent group..
  void 	CutLinks();
  TA_BASEFUNS(taGroup_impl);
private:
  void 	Initialize();
  void  Destroy();
  void 	Copy_(const taGroup_impl& cp);
protected:
  mutable TALOG*	leaf_gp; 	// #READ_ONLY #NO_SAVE cached 'flat' list of leaf-containing-gps for iter
  override void 	CanCopy_impl(const taBase* cp_fm, bool quiet, 
    bool& ok, bool virt) const;
  override void 	CheckChildConfig_impl(bool quiet, bool& rval);
  override void		ItemRemoved_(); // update the leaf counts (supercursively)
  override taBase* 	New_impl(int n_objs, TypeDef* typ, const String& name_);

  virtual TAGPtr LeafGp_(int leaf_idx) const; // #IGNORE the leaf group containing leaf item -- **NONSTANDARD FUNCTION** put here to try to flush out any use
#ifdef TA_GUI
protected: // clip functions
  override void	ChildQueryEditActions_impl(const MemberDef* md,
    const taBase* child, const taiMimeSource* ms,
    int& allowed, int& forbidden);
  virtual void	ChildQueryEditActionsG_impl(const MemberDef* md,
    taGroup_impl* subgrp, const taiMimeSource* ms,
    int& allowed, int& forbidden);
  override int	ChildEditAction_impl(const MemberDef* md, taBase* child,
    taiMimeSource* ms, int ea);
    // if child or ms is a group, dispatch to new G version
  virtual int	ChildEditActionGS_impl(const MemberDef* md, taGroup_impl* subgrp, int ea);
  virtual int	ChildEditActionGD_impl_inproc(const MemberDef* md, 
    taGroup_impl* subgrp, taiMimeSource* ms, int ea);
  virtual int	ChildEditActionGD_impl_ext(const MemberDef* md,
    taGroup_impl* subgrp, taiMimeSource* ms, int ea);
#endif
};

template<class T> class taGroup : public taGroup_impl {
  // #INSTANCE #NO_UPDATE_AFTER
  INHERITED(taGroup_impl)
public:
  ////////////////////////////////////////////////
  // 	functions that return the type		//
  ////////////////////////////////////////////////

  // operators
  T*		SafeEl(int idx) const		{ return (T*)SafeEl_(idx); }
  // #CAT_Access get element at index
  T*		FastEl(int i) const		{ return (T*)el[i]; }
  // #CAT_Access fast element (no checking)
  T* 		operator[](int i) const		{ return (T*)el[i]; }

  T*		DefaultEl() const		{ return (T*)DefaultEl_(); }
  // #CAT_Access returns the element specified as the default for this group

  // note that the following is just to get this on the menu (it doesn't actually edit)
  T*		Edit_El(T* item) const		{ return SafeEl(FindEl((taBase*)item)); }
  // #MENU #MENU_ON_Edit #USE_RVAL #ARG_ON_OBJ #CAT_Access Edit given group item

  taGroup<T>*	SafeGp(int idx) const		{ return (taGroup<T>*)gp.SafeEl(idx); }
  // #CAT_Access get group at index
  taGroup<T>*	FastGp(int i) const		{ return (taGroup<T>*)gp.FastEl(i); }
  // #CAT_Access the sub group at index
  taGroup<T>* 	FastLeafGp(int gp_idx) const	{ return (taGroup<T>*)FastLeafGp_(gp_idx); } 
  // #CAT_Access the leaf sub group at index, note: 0 is always "this"
  taGroup<T>* 	SafeLeafGp(int gp_idx) const	{ return (taGroup<T>*)SafeLeafGp_(gp_idx); } 
  // #CAT_Access the leaf sub group at index, note: 0 is always "this"

  T*		Leaf(int idx) const		{ return (T*)Leaf_(idx); }
  // #CAT_Access get leaf element at index
  taGroup<T>* 	RootGp() const 			{ return (taGroup<T>*)root_gp;  }
  // #CAT_Access the root group ('this' for the root group)

  // iterator-like functions
  inline T*	FirstEl(taLeafItr& lf) const	{ return (T*)FirstEl_(lf); }
  // #CAT_Access returns first leaf element and inits indexes
  inline T*	NextEl(taLeafItr& lf) const 	{ return (T*)NextEl_(lf); }
  // #CAT_Access returns next leaf element and incs indexes
  inline T*	LastEl(taLeafItr& lf) const	{ return (T*)LastEl_(lf); }
  // #CAT_Access returns first leaf element and inits indexes
  inline T*	PrevEl(taLeafItr& lf) const 	{ return (T*)PrevEl_(lf); }
  // #CAT_Access returns next leaf element and incs indexes

  taGroup<T>*	FirstGp(int& g)	const		{ return (taGroup<T>*)FirstGp_(g); }
  // #CAT_Access returns first leaf group and inits index
  taGroup<T>*	NextGp(int& g) const		{ return (taGroup<T>*)NextGp_(g); }
  // #CAT_Access returns next leaf group and incs index

  virtual T* 	NewEl(int n_els=1, TypeDef* typ=NULL) { return (T*)NewEl_(n_els, typ);}
  // #CAT_Modify Create and add n_els new element(s) of given type to the group (NULL = default type, el_typ)
  virtual taGroup<T>* NewGp(int n_gps=1, TypeDef* typ=NULL,
    const String& name="") { return (taGroup<T>*)NewGp_(n_gps, typ, name);}
  // #CAT_Modify Create and add n_gps new sub group(s) of given type (NULL = same type as this group)

  T*		FindName(const String& item_nm)  const
  { return (T*)FindName_(item_nm); }
  // #CAT_Access Find element in top-level list with given name (nm) (NULL = not here)
  virtual T*	FindNameContains(const String& item_nm) const
  { return (T*)FindNameContains_(item_nm); }
  // #CAT_Access Find (first) element in top-level list whose name contains given string (NULL = not here)
  virtual T* 	FindType(TypeDef* item_tp) const
  { return (T*)FindType_(item_tp); }
  // #CAT_Access find in top-level list given type element (NULL = not here)
  T*		FindNameType(const String& item_nm) const
  { return (T*)FindNameType_(item_nm); }
  // #CAT_Access Find element in top-level list with given object name or type name (item_nm)

  T*		Pop()				{ return (T*)Pop_(); }
  // #CAT_Modify pop the last element off the stack
  virtual T*	Peek()				{ return (T*)Peek_(); }
  // #CAT_Access peek at the last element on the stack

  virtual T*	AddUniqNameOld(T* item)		{ return (T*)AddUniqNameOld_((void*)item); }
  // #CAT_Modify add so that name is unique, old used if dupl, returns one used
  virtual T*	LinkUniqNameOld(T* item)	{ return (T*)LinkUniqNameOld_((void*)item); }
  // #CAT_Modify link so that name is unique, old used if dupl, returns one used

//   virtual bool	MoveBefore(T* trg, T* item) { return MoveBefore_((void*)trg, (void*)item); }
//   // #CAT_Modify move item so that it appears just before the target item trg in the list
//   virtual bool	MoveAfter(T* trg, T* item) { return MoveAfter_((void*)trg, (void*)item); }
//   // #CAT_Modify move item so that it appears just after the target item trg in the list

  T* 		FindLeafName(const String& item_nm) const
  { return (T*)FindLeafName_(item_nm); }
  // #CAT_Access Find element anywhere in full group and subgroups with given name (item_nm)
  T* 		FindLeafNameContains(const String& item_nm) const
  { return (T*)FindLeafNameContains_(item_nm); }
  // #MENU #MENU_ON_Edit #USE_RVAL #ARGC_1 #LABEL_Find #CAT_Access Find anywhere in full group and subgroups first element whose name contains given name (item_nm)
  T* 		FindLeafType(TypeDef* item_tp) const
  { return (T*)FindLeafType_(item_tp);}
  // #CAT_Access find anywhere in full group and subgroups given type leaf element (NULL = not here)
  T*		FindLeafNameType(const String& item_nm) const
  { return (T*)FindLeafNameType_(item_nm); }
  // #CAT_Access Find anywhere in full group and subgroups element with given object name or type name (item_nm)

  TA_TMPLT_BASEFUNS(taGroup,T);
protected:
  taGroup<T>* 	LeafGp(int leaf_idx) const		{ return (taGroup<T>*)LeafGp_(leaf_idx); }
  // the group containing given leaf; NOTE: **don't confuse this with the Safe/FastLeafGp funcs*** -- moved here to try to flush out any use, since it is so confusing and nonstandard and likely to be mixed up with the XxxLeafGp funcs 
private:
  TMPLT_NOCOPY(taGroup,T)
  void Initialize() 	{ SetBaseType(T::StatTypeDef(1));}
  void	Destroy () {}
};


// do not use this macro, since you typically will want ##NO_TOKENS, #NO_UPDATE_AFTER
// which cannot be put inside the macro!
//
// #define taGroup_of(T)
// class T ## _Group : public taGroup<T> {
// public:
//   void Initialize()	{ };
//   void Destroy()	{ };
//   TA_BASEFUNS(T ## _Group);
// }

// use the following as a template instead..

// define default base group to not keep tokens
class TA_API taBase_Group : public taGroup<taBase> {
  // #NO_TOKENS #NO_UPDATE_AFTER ##EXPAND_DEF_0 group of objects
INHERITED(taGroup<taBase>)
public:
  void	Initialize() 		{ SetBaseType(&TA_taBase); }
  void 	Destroy()		{ };
  TA_BASEFUNS_NOCOPY(taBase_Group);
};

#define BaseGroup_of(T)							      \
class T ## _Group : public taBase_Group {				      \
public:									      \
  void	Initialize() 		{ SetBaseType(T::StatTypeDef(0)); }		      \
  void 	Destroy()		{ };					      \
  TA_BASEFUNS(T ## _Group);					      \
}

class TA_API UserDataItem_List: public taGroup<UserDataItemBase> {
  // #CHILDREN_INLINE
INHERITED(taGroup<UserDataItemBase>)
public:
  bool			hasVisibleItems() const; // #IGNORE lets gui avoid putting up panel unless any user-visible items are present
  
  UserDataItem*		NewItem(const String& name, const Variant& value,
    const String& desc); // #CAT_UserData #BUTTON #NO_SAVE_ARG_VAL Make a new simple user data entry with given name and value (desc optional)

  TA_BASEFUNS_NOCOPY(UserDataItem_List)
protected:

private:
  void Initialize() {SetBaseType(&TA_UserDataItemBase);}
  void Destroy() {}
};

TA_SMART_PTRS(UserDataItem_List) // UserDataItem_ListPtr


///////////////////////////////////////////////////////
//	float_CircBuffer

class TA_API float_CircBuffer : public float_Array {
  // ##NO_TOKENS Circular buffer for holding state information -- efficient way to store a fixed window of state information without actually shifting memory around -- use CircAdd to initially populate and CircShiftLeft to make room for new items
INHERITED(float_Array)
public:
  int		st_idx;		// #READ_ONLY index in underlying array where the list starts (i.e., the position of the logical 0 index) -- updated by functions and should not be set manually
  int		length;		// #READ_ONLY logical length of the list -- is controlled by adding and shifting, and should NOT be set manually

  /////////////////////////////////////////////////////////
  //	Special Access Routines

  int	CircIdx(int cidx) const
  { int rval = cidx+st_idx; if(rval >= size) rval -= size; return rval; }
  // #CAT_CircAccess gets physical index from logical circular index

  bool 	CircIdxInRange(int cidx) const { return InRange(CircIdx(cidx)); }
  // #CAT_CircAccess check if logical circular index is in range
  
  const float&	CircSafeEl(int cidx) const { return SafeEl(CircIdx(cidx)); }
  // #CAT_CircAccess returns element at given logical circular index, or err value which is 0.0

  const float&	CircPeek() const {return SafeEl(CircIdx(length-1));}
  // #CAT_CircAccess returns element at end of circular buffer

  /////////////////////////////////////////////////////////
  //	Special Modify Routines

  void		CircShiftLeft(int nshift)
  { st_idx = CircIdx(nshift); length -= nshift; }
  // #CAT_CircModify shift the buffer to the left -- shift the first elements off the start of the list, making room at the end for more elements (decreasing length)

  void		CircAddExpand(const float& item) {
    if((st_idx == 0) && (length >= size)) {
      inherited::Add(item); length++; 	// must be building up the list, so add it
    }
    else {
      Set(CircIdx(length++), item);	// expand the buffer length and set to the element at the end
    }
  }
  // #CAT_CircModify add a new item to the circular buffer, expanding the length of the list by 1 under all circumstances

  void		CircAddLimit(const float& item, int max_length) {
    if(length >= max_length) {
      CircShiftLeft(1 + length - max_length); // make room
      Set(CircIdx(length++), item);	// set to the element at the end
    }
    else {
      CircAddExpand(item);
    }
  }
  // #CAT_CircModify add a new item to the circular buffer, shifting it left if length is at or above max_length to ensure a fixed overall length list (otherwise expanding list up to max_length)

  override void	Reset();

  void 	Copy_(const float_CircBuffer& cp);
  TA_BASEFUNS(float_CircBuffer);
private:
  void 	Initialize();
  void	Destroy()		{ };
};

#endif // ta_group_h
