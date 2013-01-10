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

#include "taiCompData.h"

taiCompData::taiCompData(TypeDef* typ_, IDataHost* host_, taiData* parent_, QWidget* gui_parent_, int flags_)
  : taiData(typ_, host_, parent_, gui_parent_, flags_)
{
  lay = NULL; // usually created in InitLayout;
  last_spc = -1;
  lay_type = LT_HBox; // default
  mwidgets = new QObjectList();
  m_child_base = NULL;
}

taiCompData::~taiCompData() {
  mwidgets->clear();
  delete mwidgets;
  taiData* dat;
  while (data_el.size > 0) {
    dat = data_el.Pop();
    dat->mparent = NULL; // don't call setParent, otherwise, calls our ChildRemove
  }
}

void taiCompData::ChildAdd(taiData* child) {
  data_el.Add(child);
}

void taiCompData::ChildRemove(taiData* child) {
  //NOTE: this will get called while being removed/deleted from an own list!
  //2004-08-31 workaround, change taListptr to remove items from list before deleting, thus item not found
  data_el.RemoveEl(child);
}

void taiCompData::InitLayout() { //virtual/overridable
  switch (lay_type) {
  case LT_HBox:
    lay = new QHBoxLayout(GetRep());
    break;
  case LT_Flow:
    lay = new iFlowLayout(GetRep(), 0, -1, (Qt::AlignLeft));
    break;
  case LT_Stacked:
    lay = new QStackedLayout(GetRep());
    break;
  //no default -- must handle all cases
  }
  lay->setMargin(0); // supposedly deprecated...
  last_spc = taiM->hsep_c; // give it a bit of room
}

void taiCompData::AddChildMember(MemberDef* md) {
  const int ctrl_size = taiM->ctrl_size;

  // establish container
  QWidget* wid;
  switch (lay_type) {
  case LT_HBox:
    wid = GetRep(); // directly into the guy
    break;
  case LT_Flow:
  case LT_Stacked:
    wid = MakeLayoutWidget(GetRep());
    break;
  }
  // get gui representation of data
  int child_flags = (mflags & flg_INHERIT_MASK);
  taiData* mb_dat = md->im->GetDataRep(host, this, wid, NULL, child_flags); //adds to list
  //nn, done by im mb_dat->SetMemberDef(md);

  // get caption
  String name;
  String desc;
  taiDataHost::GetName(md, name, desc);
  iLabel* lbl = taiDataHost::MakeInitEditLabel(name, wid, ctrl_size, desc, mb_dat);
  lbl->setUserData((ta_intptr_t)mb_dat); // primarily for context menu, esp for SelectEdit

  QWidget* ctrl = mb_dat->GetRep();
  connect(mb_dat, SIGNAL(DataChangedNotify(taiData*)),
          this, SLOT(ChildDataChanged(taiData*)) );

  // check for a compatible taiDataHost, and if so, connect context menu
  if (host) {

    taiDataHost* tadh = dynamic_cast<taiDataHost*>((QObject*)host->This());
    if (tadh) {
      connect(lbl, SIGNAL(contextMenuInvoked(iLabel*, QContextMenuEvent*)),
        tadh, SLOT(label_contextMenuInvoked(iLabel*, QContextMenuEvent*)));
    }
  }

  switch (lay_type) {
  case LT_HBox:
    AddChildWidget(lbl, 1); // taiM->hsep_c);
    AddChildWidget(ctrl, taiM->hsep_c);
    break;
  case LT_Flow:
  case LT_Stacked:
    QHBoxLayout* hbl = new QHBoxLayout(wid);
    hbl->setMargin(0);
    hbl->setSpacing(taiM->hsep_c);
    hbl->addWidget(lbl);
    hbl->addWidget(ctrl);
    AddChildWidget(wid, -1); // no explicit seps
    break;
  }

  if (!desc.empty()) {
    lbl->setToolTip(desc);
    ctrl->setToolTip(desc);
  }
}

void taiCompData::EndLayout() { //virtual/overridable
  switch (lay_type) {
  case LT_HBox:
    layHBox()->addStretch();
    break;
  case LT_Flow:
    break;
  case LT_Stacked:
    break;
  }
}

void taiCompData::AddChildWidget(QWidget* child_widget, int space_after,
  int stretch)
{
  if (space_after == -1) space_after = taiM->hsep_c;
  mwidgets->append(child_widget);
  AddChildWidget_impl(child_widget, last_spc, stretch);
  last_spc = space_after;
}

void taiCompData::AddChildWidget_impl(QWidget* child_widget, int spacing,
  int stretch)
{
  switch (lay_type) {
  case LT_HBox:
    if (spacing != -1)
      //lay->addItem(new QSpacerItem(last_spc, 0, QSizePolicy::Fixed));
      layHBox()->addSpacing(last_spc);
    layHBox()->addWidget(child_widget, stretch, (Qt::AlignLeft | Qt::AlignVCenter));
    child_widget->show();
    break;
  case LT_Flow:
    layFlow()->addWidget(child_widget);
    child_widget->show();
    break;
  case LT_Stacked:
    layStacked()->addWidget(child_widget);
    layStacked()->setAlignment(child_widget, (Qt::AlignLeft | Qt::AlignVCenter));
    break;
  }
}

QWidget* taiCompData::widgets(int index) {
  return (QWidget*)mwidgets->at(index);
}

int taiCompData::widgetCount() {
  return mwidgets->count();
}

void taiCompData::ChildDataChanged(taiData* sender) {
  emit ChildDataChangedNotify(sender);
}
