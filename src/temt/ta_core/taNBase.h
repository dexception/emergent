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

#ifndef taNBase_h
#define taNBase_h 1

// parent includes:
#include <taOBase>

// member includes:

// declare all other types mentioned but not required to include:


taTypeDef_Of(taNBase);

class TA_API taNBase : public taOBase { // #NO_TOKENS Named, owned base class of taBase
INHERITED(taOBase)
public:
  String      name; // #CONDEDIT_OFF_base_flags:NAME_READONLY #CAT_taBase name of the object

  bool            HasName() const override { return true; }
  bool            SetName(const String& nm) override;
  String          GetName() const  override { return name; }
  void            SetDefaultName() override;
  void            MakeNameUnique() override;
  bool            SetCopyName(const taBase& cp) override;

  bool            BrowserEditEnable() override;
  String          BrowserEditString() override { return GetDisplayName(); }
  bool            BrowserEditSet(const String& new_val_str, int move_after = 0) override;
  
  virtual bool    AddFromTemplate(taBase* obj, bool& is_acceptable) { return false; }
  // handles drops from toolbar - when adding an object to a program, network, etc - e.g. dropping generic data table onto a program - set is_acceptable for objects "not handled" but which are acceptable

  TA_BASEFUNS(taNBase);
protected:
  void         UpdateAfterEdit_impl() override;

private:
  void  Copy_(const taNBase& cp);

  void  Initialize()    { }
  void  Destroy()       { }
};

#endif // taNBase_h
