// Copyright, 1995-2013, Regents of the University of Colorado,
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

#ifndef taOBase_h
#define taOBase_h 1

// parent includes:
#include <taBase>

// member includes:

// declare all other types mentioned but not required to include:
class UserDataItem_List; // 
class taiMimeSource; //

TypeDef_Of(taOBase);

class TA_API taOBase : public taBase {
  // #NO_TOKENS #NO_UPDATE_AFTER owned base class of taBase
INHERITED(taBase)

// Data Members:
public:
  taBase*               owner;  // #NO_SHOW #READ_ONLY #NO_SAVE #NO_SET_POINTER #NO_FIND #CAT_taBase pointer to owner
  mutable UserDataItem_List* user_data_; // #OWN_POINTER #NO_SHOW_EDIT #HIDDEN_TREE #NO_SAVE_EMPTY #CAT_taBase storage for user data (created if needed)

protected:
  taDataLink*           m_data_link; //

// Methods:
public:
  taDataLink**          addr_data_link() {return &m_data_link;} // #IGNORE
  override taDataLink*  data_link() {return m_data_link;}       // #IGNORE
  taBase*               GetOwner() const        { return owner; }
  USING(inherited::GetOwner)
  taBase*               SetOwner(taBase* ta)    { owner = ta; return ta; }
  UserDataItem_List*    GetUserDataList(bool force = false) const;
  void                  CutLinks();
  TA_BASEFUNS(taOBase); //

protected:
  override void         CanCopy_impl(const taBase* cp_fm, bool quiet, bool& ok, bool virt) const {
    if (virt)
      inherited::CanCopy_impl(cp_fm, quiet, ok, virt);
  }

#ifdef TA_GUI
protected: // all related to taList or DEF_CHILD children_
  override void ChildQueryEditActions_impl(const MemberDef* md, const taBase* child,
    const taiMimeSource* ms, int& allowed, int& forbidden);
     // gives the src ops allowed on child (ex CUT)
  virtual void  ChildQueryEditActionsL_impl(const MemberDef* md, const taBase* lst_itm,
    const taiMimeSource* ms, int& allowed, int& forbidden);
    // returns the operations allowed for list items (ex Paste)

  virtual int   ChildEditAction_impl(const MemberDef* md, taBase* child, taiMimeSource* ms, int ea);
  virtual int   ChildEditActionLS_impl(const MemberDef* md, taBase* lst_itm, int ea);
  virtual int   ChildEditActionLD_impl_inproc(const MemberDef* md, taBase* lst_itm, taiMimeSource* ms, int ea);
  virtual int   ChildEditActionLD_impl_ext(const MemberDef* md, taBase* lst_itm, taiMimeSource* ms, int ea);
#endif

private:
  void  Copy_(const taOBase& cp);
  void  Initialize()    { owner = NULL; user_data_ = NULL; m_data_link = NULL; }
  void  Destroy();
};

#endif // taOBase_h
