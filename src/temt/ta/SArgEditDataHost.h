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

#ifndef SArgEditDataHost_h
#define SArgEditDataHost_h 1

// parent includes:
#include <gpiArrayEditDataHost>

// member includes:

// declare all other types mentioned but not required to include:

class TA_API SArgEditDataHost : public gpiArrayEditDataHost {
  // ##NO_INSTANCE
INHERITED(gpiArrayEditDataHost)
public:
  bool          ShowMember(MemberDef* md) const;

  SArgEditDataHost(void* base, TypeDef* tp, bool read_only_ = false,
        bool modal_ = false, QObject* parent = 0);
  SArgEditDataHost()    { };
protected:
  override void         Constr_AryData();
};

#endif // SArgEditDataHost_h
