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

#include "taiWidgetMethodMenu.h"

taiWidgetMethodMenu::taiWidgetMethodMenu(void* bs, MethodDef* md, TypeDef* typ_, IWidgetHost* host_, taiWidget* par,
                         QWidget* gui_parent_, int flags_)
  : taiWidgetMethod(bs, md, typ_, host_, par, gui_parent_, flags_)
{
  is_menu_item = true;
}

