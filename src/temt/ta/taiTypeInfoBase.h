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

#ifndef taiTypeInfoBase_h
#define taiTypeInfoBase_h 1

// parent includes:
#include <taiData>

// member includes:

// declare all other types mentioned but not required to include:

class TA_API taiTypeInfoBase : public taiData {
  // common base for MemberDefs, MethodDefs, TypeDefs, and Enums of a typedef in the object with a MDTYPE_xxx option
  INHERITED(taiData)
public:
  MemberDef*    memb_md; // MemberDef of the member that will get the target pointer
  TypeDef*      targ_typ;       // target type from which to get list of items -- may be same as typ, but could differ
  void*         menubase;       // the address of the object
  taiActions*   ta_actions;

  override QWidget* GetRep();


  virtual void          GetMenu() = 0;
  virtual void          GetImage(const void* base, bool get_menu = true, void* cur_sel = NULL);

  taiTypeInfoBase(taiActions::RepType rt, int ft, MemberDef* memb_md_,
    TypeDef* typ_, IDataHost* host, taiData* par,
    QWidget* gui_parent_, int flags_ = 0); // if targ_type is null, it will be determined
  ~taiTypeInfoBase();
protected:
  virtual void          GetTarget(); // determines the target type for the lookup menu
};

#endif // taiTypeInfoBase_h
