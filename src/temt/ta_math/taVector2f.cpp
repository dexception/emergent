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

#include "taVector2f.h"

#include <taMatrix>
#include <taVector2i>
#include <MatrixIndex>

TA_BASEFUNS_CTORS_LITE_DEFN(taVector2f);

taVector2f::taVector2f(const taVector2i& cp) {
  Register(); Initialize(); x = (float)cp.x; y = (float)cp.y;
}

taVector2f& taVector2f::operator=(const taVector2i& cp) {
  x = (float)cp.x; y = (float)cp.y;
  return *this;
}

void taVector2f::ToMatrix(taMatrix& mat) const {
  mat.SetGeom(1,2); mat.SetFmVar(x,0);  mat.SetFmVar(y,1);
}

void taVector2f::FromMatrix(taMatrix& mat) {
  x = mat.SafeElAsVar(0).toFloat();  y = mat.SafeElAsVar(1).toFloat();
}
