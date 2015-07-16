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

#ifndef taiMemberOfEnumTypePtr_h
#define taiMemberOfEnumTypePtr_h 1

// parent includes:
#include <taiMemberOfTypePtr>

// member includes:

// declare all other types mentioned but not required to include:


taTypeDef_Of(taiMemberOfEnumTypePtr);

class TA_API taiMemberOfEnumTypePtr : public taiMemberOfTypePtr {
  // typedef ptrs that have member-comment directives, Enums
  TAI_MEMBER_SUBCLASS(taiMemberOfEnumTypePtr, taiMemberOfTypePtr);
public:
  int           BidForMember(MemberDef* md, TypeDef* td);
protected:
  taiWidget*     GetWidgetRep_impl(IWidgetHost* host_, taiWidget* par,
    QWidget* gui_parent_, int flags_, MemberDef* mbr);
private:
  void          Initialize() {}
  void          Destroy() {}
};

#endif // taiMemberOfEnumTypePtr_h