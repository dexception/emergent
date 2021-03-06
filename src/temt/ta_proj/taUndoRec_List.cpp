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

#include "taUndoRec_List.h"

TA_BASEFUNS_CTORS_DEFN(taUndoRec_List);

void taUndoRec_List::Initialize() {
  SetBaseType(&TA_taUndoRec);
  st_idx = 0;
  length = 0;
}

void taUndoRec_List::Copy_(const taUndoRec_List& cp) {
  st_idx = cp.st_idx;
  length = cp.length;
}

void taUndoRec_List::Reset() {
  inherited::Reset();
  st_idx = 0;
  length = 0;
}
