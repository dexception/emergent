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

#ifndef SArgEdit_h
#define SArgEdit_h 1

// parent includes:
#include <gpiArrayEdit>

// member includes:

// declare all other types mentioned but not required to include:


TypeDef_Of(SArgEdit);

class TA_API SArgEdit : public gpiArrayEdit {
  TAI_TYPEBASE_SUBCLASS(SArgEdit, gpiArrayEdit);
public:
  int           BidForEdit(TypeDef* td);
protected:
  override taiEditDataHost* CreateDataHost(void* base, bool readonly); // called when we need a new instance
};

#endif // SArgEdit_h
