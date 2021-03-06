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

#ifndef taiMemberOfSubTokenPtr_h
#define taiMemberOfSubTokenPtr_h 1

// parent includes:
#include <taiMember>

// member includes:

// declare all other types mentioned but not required to include:


taTypeDef_Of(taiMemberOfSubTokenPtr);

class TA_API taiMemberOfSubTokenPtr : public taiMember {
  // a token ptr that points to sub-objects of current object
  TAI_MEMBER_SUBCLASS(taiMemberOfSubTokenPtr, taiMember);
public:
  int           BidForMember(MemberDef* md, TypeDef* td) override;
  taiWidget*      GetWidgetRep_impl(IWidgetHost* host_, taiWidget* par,
    QWidget* gui_parent_, int flags_, MemberDef* mbr) override;
protected:
  void GetImage_impl(taiWidget* dat, const void* base) override;
  void GetMbrValue_impl(taiWidget* dat, void* base) override;
private:
  void          Initialize() {}
  void          Destroy() {}
};

#endif // taiMemberOfSubTokenPtr_h
