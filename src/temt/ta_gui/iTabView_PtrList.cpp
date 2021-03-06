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

#include "iTabView_PtrList.h"

iTabView_PtrList::~iTabView_PtrList()
{
  for (int i = 0; i < size; ++i) {
    iTabView* tv = FastEl(i);
    tv->m_viewer_win = NULL; // prevents callback during destruction
  }
}

void iTabView_PtrList::DataPanelDestroying(iPanelBase* panel) {
  for (int i = 0; i < size; ++i) {
    iTabView* tv = FastEl(i);
    tv->DataPanelDestroying(panel);
  }
}

