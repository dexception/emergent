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

#include "taiWidgetArrayEditButton.h"
#include <taArray_base>


taiWidgetArrayEditButton::taiWidgetArrayEditButton
(void* base, TypeDef* tp, IWidgetHost* host_, taiWidget* par, QWidget* gui_parent_, int flags_)
: taiWidgetEditButton(base, NULL, tp, host_, par, gui_parent_, flags_)
{
}

void taiWidgetArrayEditButton::SetLabel() {
  taArray_base* gp = (taArray_base*)cur_base;
  if(gp == NULL) {
    taiWidgetEditButton::SetLabel();
    return;
  }
  String nm = " Size: " + String(gp->size);
  TypeDef* td = gp->GetTypeDef()->GetTemplInstType();
  if ((td != NULL) && (td->templ_pars.size > 0))
    nm += String(" (") + td->templ_pars.FastEl(0)->name + ")";
  setRepLabel(nm);
}
