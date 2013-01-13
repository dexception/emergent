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

#include "taiDataHost.h"
#include <iLabel>
#include <taiData>
#include <iFormLayout>
#include <iEditGrid>
#include <iScrollArea>
#include <iStripeWidget>
#include <iSplitter>

#include <taiMisc>

#include <QHBoxLayout>

#define LAYBODY_MARGIN  1
#define LAYBODY_SPACING 0

iLabel* taiDataHost::MakeInitEditLabel(const String& name, QWidget* par,
  int ctrl_size, const String& desc, taiData* buddy,
  QObject* ctx_obj, const char* ctx_slot, int row)
{
  iLabel* label = new iLabel(row, name, par);
  label->setFont(taiM->nameFont(ctrl_size));
  label->setFixedHeight(taiM->label_height(ctrl_size));
  if (buddy) label->setUserData((ta_intptr_t)buddy);
  if (ctx_obj) QObject::connect(
    label, SIGNAL(contextMenuInvoked(iLabel*, QContextMenuEvent*)),
      ctx_obj, ctx_slot );
// if it is an iLabel connecting a taiData, then connect the highlighting for non-default values
  QWidget* buddy_widg = NULL;
  if (buddy) {
    buddy->setLabel(label);
    buddy_widg = buddy->GetRep();
    QObject::connect(buddy, SIGNAL(settingHighlight(bool)),
        label, SLOT(setHighlight(bool)) );
  }


  if (!desc.empty()) {
    label->setToolTip(desc);
    label->setStatusTip(desc);
    if (buddy_widg != NULL) {
      buddy_widg->setToolTip(desc);
      buddy_widg->setStatusTip(desc);
    }
  }
  return label;
}


taiDataHost::taiDataHost(TypeDef* typ_, bool read_only_, bool modal_, QObject* parent)
:inherited(typ_, read_only_, modal_, parent)
{
  InitGuiFields(false);

  cur_row = 0;
  dat_cnt = 0;
  first_tab_foc = NULL;
}

taiDataHost::~taiDataHost() {
}

// note: called non-virtually in our ctor, and virtually in WidgetDeleting
void taiDataHost::InitGuiFields(bool virt) {
  if (virt)  inherited::InitGuiFields(virt);
  splBody = NULL;
  scrBody = NULL;
  layBody = NULL;
  first_tab_foc = NULL;
}

int taiDataHost::AddSectionLabel(int row, QWidget* wid, const String& desc) {
  if (row < 0)
    row = layBody->rowCount();
  QFont f(taiM->nameFont(ctrl_size));
  f.setBold(true);
  wid->setFont(f);
  wid->setFixedHeight(row_height);
  SET_PALETTE_BACKGROUND_COLOR(wid, colorOfRow(row));
  if (!desc.empty()) {
    wid->setToolTip(desc);
  }
  QHBoxLayout* layH = new QHBoxLayout();


  layH->addSpacing(2);
  // we add group-box-like frame lines to separate sections
  QFrame* ln = new QFrame(body);
  ln->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  ln->setMinimumWidth(16);
  layH->addWidget(ln, 0, (Qt::AlignLeft | Qt::AlignVCenter));
  ln->show();
  layH->addSpacing(4); // leave a bit more room before ctrl
  layH->addWidget(wid, 0, (Qt::AlignLeft | Qt::AlignVCenter));
  layH->addSpacing(2); // don't need as much room to look balanced
  ln = new QFrame(body);
  ln->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  ln->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  layH->addWidget(ln, 1, (Qt::AlignVCenter));
  ln->show();
  layH->addSpacing(2);
  // add the item to span both cols
#if ((QT_VERSION >= 0x040400) && defined(TA_USE_QFORMLAYOUT))
  layBody->addRow(layH);
#else
  layBody->setRowMinimumHeight(row, row_height + (2 * LAYBODY_MARGIN)); //note: margins not automatically baked in to max height
  layBody->addLayout(layH, row, 0, 1, 2, (Qt::AlignLeft | Qt::AlignVCenter));
#endif
  wid->show(); // needed for rebuilds, to make the widget show
  return row;
}

int taiDataHost::AddNameData(int row, const String& name, const String& desc,
   QWidget* data, taiData* buddy, MemberDef* md, bool fill_hor)
{
  if (row < 0)
    row = layBody->rowCount();
//LABEL
  iLabel* label = MakeInitEditLabel(name, body, ctrl_size, desc, buddy,
    this, SLOT(label_contextMenuInvoked(iLabel*, QContextMenuEvent*)), row);

//DATA
  // note1: margins not automatically baked in to max height
  // note2: if guy goes invisible, we'll set its row height to 0 in GetImage
  QHBoxLayout* lay_dat = new QHBoxLayout();
  lay_dat->setMargin(0);
  lay_dat->addWidget(data, 0, Qt::AlignVCenter/*, (Qt::AlignLeft | Qt::AlignVCenter)*/);
  if (!fill_hor) lay_dat->addStretch();

// add label/body and show
#if ((QT_VERSION >= 0x040400) && defined(TA_USE_QFORMLAYOUT))
  label->setMinimumHeight(row_height);
  label->setMaximumHeight(row_height);
  lay_dat->addStrut(row_height); // make it full height, so controls center
  layBody->addRow(label, lay_dat);
#else


  QHBoxLayout* lay_lbl = new QHBoxLayout();
  lay_lbl->setMargin(0);
  lay_lbl->addWidget(label, 0, (Qt::AlignLeft | Qt::AlignVCenter));
  lay_lbl->addSpacing(2);
  layBody->setRowMinimumHeight(row, row_height + (2 * LAYBODY_MARGIN));
  layBody->addLayout(lay_lbl, row, 0, (Qt::AlignLeft | Qt::AlignVCenter));
  layBody->addLayout(lay_dat, row, 1);
#endif

  label->show(); // needed for rebuilds, to make the widget show
  data->show(); // needed for rebuilds, to make the widget show

  return row;
}

int taiDataHost::AddData(int row, QWidget* data, bool fill_hor)
{
  if (row < 0)
    row = layBody->rowCount();


//DATA
  // note1: margins not automatically baked in to max height
  // note2: if guy goes invisible, we'll set its row height to 0 in GetImage
  QHBoxLayout* hbl = new QHBoxLayout();
  hbl->setMargin(0);
  hbl->addWidget(data, 0, Qt::AlignVCenter);
  if (!fill_hor) hbl->addStretch();

// add label/body and show
#if ((QT_VERSION >= 0x040400) && defined(TA_USE_QFORMLAYOUT))
  layBody->addRow(hbl);
#else
  layBody->setRowMinimumHeight(row, row_height + (2 * LAYBODY_MARGIN));
  layBody->addLayout(hbl, row, 0, 1, 2); // col 0, span 1 row, span 2 cols
#endif

//   if(!first_tab_foc) {
//     if(data->focusPolicy() & Qt::TabFocus) {
//       first_tab_foc = data;
//     }
//   }

  data->show(); // needed for rebuilds, to make the widget show

  return row;
}
void taiDataHost::AddMultiRowName(iEditGrid* multi_body, int row, const String& name, const String& desc) {
  SetMultiSize(row + 1, 0); //0 gets set to multi_col
  QLabel* label = new QLabel(name, (QWidget*)NULL);
  label->setFont(taiM->nameFont(ctrl_size));
  label->setFixedHeight(taiM->label_height(ctrl_size));
  SET_PALETTE_BACKGROUND_COLOR(label, colorOfRow(row));
  if (!desc.empty()) {
    label->setToolTip(desc);
  }
  multi_body->setRowNameWidget(row, label);
  label->show(); //required to show when rebuilding
}

void taiDataHost::AddMultiColName(iEditGrid* multi_body, int col, const String& name, const String& desc) {
  SetMultiSize(0, col + 1); // 0 gets set to multi_rows
  QLabel* label = new QLabel(name, (QWidget*)NULL);
  label->setFont(taiM->nameFont(ctrl_size));
  label->setFixedHeight(taiM->label_height(ctrl_size));
  if (!desc.empty()) {
    label->setToolTip(desc);
  }
  multi_body->setColNameWidget(col, label);
  label->show(); //required to show when rebuilding
}

void taiDataHost::AddMultiData(iEditGrid* multi_body, int row, int col, QWidget* data) {
//  SetMultiSize(row - 1, col - 1);
  SetMultiSize(row + 1, col + 1);
  QHBoxLayout* hbl = new QHBoxLayout();
  hbl->setMargin(0);
  hbl->addWidget(data, 0,  (Qt::AlignLeft | Qt::AlignVCenter));
  hbl->addStretch();
  multi_body->setDataLayout(row, col, hbl);
  data->show(); //required to show when rebuilding
}

void taiDataHost::Constr_Box() {
  //note: see also gpiMultiEditDialog::Constr_Box, if changes made to this implementation
  //note: see ClearBody for guards against deleting the structural widgets when clearing
  QWidget* scr_par = (splBody == NULL) ? widget() : splBody;
  scrBody = new iScrollArea(scr_par);
  SET_PALETTE_BACKGROUND_COLOR(scrBody->viewport(), bg_color_dark);
  scrBody->setWidgetResizable(true);
  body = new iStripeWidget();
  body_vlay = new QVBoxLayout(body);
  body_vlay->setMargin(0);

  scrBody->setWidget(body);
  SET_PALETTE_BACKGROUND_COLOR(body, bg_color);
  ((iStripeWidget*)body)->setHiLightColor(bg_color_dark);
  ((iStripeWidget*)body)->setStripeHeight(row_height + (2 * LAYBODY_MARGIN));
  //TODO: if adding spacing, need to include LAYBODY_SPACING;
  if (splBody == NULL) {
    vblDialog->addWidget(scrBody, 1); // gets all the space
  }
  //note: the layout is added in Constr_Body, because it gets deleted when we change the 'show'
}

void taiDataHost::Constr_Body_impl() {
  first_tab_foc = NULL;         // reset
#if ((QT_VERSION >= 0x040400) && defined(TA_USE_QFORMLAYOUT))
  layBody = new iFormLayout();
  layBody->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
  layBody->setLabelAlignment(Qt::AlignLeft);
  layBody->setRowWrapPolicy(iFormLayout::DontWrapRows);
  layBody->setHorizontalSpacing(2 * LAYBODY_MARGIN);
  layBody->setVerticalSpacing(2 * LAYBODY_MARGIN);
  layBody->setContentsMargins(LAYBODY_MARGIN, 0, LAYBODY_MARGIN, 0);
  layBody->setFieldGrowthPolicy(iFormLayout::AllNonFixedFieldsGrow); // TBD
#else
  layBody = new QGridLayout();
#if QT_VERSION >= 0x040300
  layBody->setHorizontalSpacing(LAYBODY_SPACING);
  layBody->setVerticalSpacing(0);
  layBody->setContentsMargins(LAYBODY_MARGIN, 0, LAYBODY_MARGIN, 0);
#else
  layBody->setSpacing(LAYBODY_SPACING);
  layBody->setMargin(LAYBODY_MARGIN);
#endif
  layBody->setColumnStretch(1,1);
#endif // 4.4 vs. <4.4
  body_vlay->addLayout(layBody);
  body_vlay->addStretch(1);
}

void taiDataHost::ClearBody_impl() {
  if(body) {
    delete body->layout();      // nuke our vboxlayout guy
    taiMisc::DeleteWidgetsLater(body);
    body_vlay = new QVBoxLayout(body);
    body_vlay->setMargin(0);
  }
}

void taiDataHost::Constr_Final() {
  inherited::Constr_Final();
  // we put all the stretch factor setting here, so it is easy to make code changes if necessary
  if (splBody) vblDialog->setStretchFactor(splBody, 1);
  else         vblDialog->setStretchFactor(scrBody, 1);
}

