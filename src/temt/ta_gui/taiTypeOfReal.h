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

#ifndef taiTypeOfReal_h
#define taiTypeOfReal_h 1

// parent includes:
#include <taiType>

// member includes:

// declare all other types mentioned but not required to include:


taTypeDef_Of(taiTypeOfReal);

class TA_API taiTypeOfReal : public taiType { // handles real types, provides validators
  TAI_TYPEBASE_SUBCLASS(taiTypeOfReal, taiType);
public:
  int           BidForType(TypeDef* td) override;
protected:
  taiWidget*      GetWidgetRep_impl(IWidgetHost* host_, taiWidget* par,
    QWidget* gui_parent_, int flags_, MemberDef* mbr) override;
  void          GetValue_impl(taiWidget* dat, void* base) override;
};

#endif // taiTypeOfReal_h
