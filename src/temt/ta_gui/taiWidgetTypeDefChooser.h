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

#ifndef taiWidgetTypeDefChooser_h
#define taiWidgetTypeDefChooser_h 1

// parent includes:
#include <taiWidgetItemChooser>

// member includes:

// declare all other types mentioned but not required to include:


class TA_API taiWidgetTypeDefChooser : public taiWidgetItemChooser {
  // for TypeDefs
  INHERITED(taiWidgetItemChooser)
public:
  inline TypeDef*       td() const {return (TypeDef*)m_sel;}
  int                   columnCount(int view) const override;
  const String          headerText(int index, int view) const override;
  const String          viewText(int index) const override;
  bool                  hasNoItems() override;
  bool                  hasOnlyOneItem() override;

  using inherited::GetImage;
  void                  GetImage(TypeDef* cur_sel, TypeDef* targ_typ)
    {taiWidgetItemChooser::GetImage((void*)cur_sel, targ_typ);}
  TypeDef*              GetValue() {return td();}

  void                  BuildChooser(iDialogItemChooser* ic, int view = 0) override;
  void         btnHelp_clicked() override;

  taiWidgetTypeDefChooser(TypeDef* typ_, IWidgetHost* host,
                   taiWidget* par, QWidget* gui_parent_, int flags_ = 0,
                   const String& flt_start_txt = "");
protected:
  enum TypeCat {
    TC_NoAdd, // for ptrs, non classes, etc.
    TC_NoAddCheckChildren, // typically for templates
    TC_Add // "normal" class types
  };
  const String          itemTag() const override {return "Type: ";}
  const String          labelNameNonNull() const override;

  TypeCat               AddType_Class(TypeDef* typ); // true if should be shown to user
  void         BuildCategories_impl() override;
  void                  BuildCategoriesR_impl(TypeDef* top_typ);
  int                   BuildChooser_0(iDialogItemChooser* ic, TypeDef* top_typ,
    QTreeWidgetItem* top_item); // we use this recursively
  int                   CountChildren(TypeDef* td);
};

#endif // taiWidgetTypeDefChooser_h
