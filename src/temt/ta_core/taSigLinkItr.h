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

#ifndef taSigLinkItr_h
#define taSigLinkItr_h 1

// parent includes:
#include "ta_def.h"

// member includes:

// declare all other types mentioned but not required to include:
class taSigLink;

taTypeDef_Of(taSigLinkItr);

class TA_API taSigLinkItr {
  // ##INSTANCE ##NO_TOKENS ##NO_CSS ##NO_MEMBERS iterator for siglink clients -- use as a value type
public:
  void*         NextEl(taSigLink* dl, const TypeDef* typ);
  void          Reset() {i = 0;}
  taSigLinkItr() {Reset();}
protected:
  int           i;
};

// iterates through a siglink, returning only object refs to objects of indicated type (or descendant)
#define FOR_DLC_EL_OF_TYPE(T, el, dl, itr) \
for(itr.Reset(), el = (T*) itr.NextEl(dl, &TA_ ## T); el; el = (T*) itr.NextEl(dl, &TA_ ## T))

#endif // taSigLinkItr_h
