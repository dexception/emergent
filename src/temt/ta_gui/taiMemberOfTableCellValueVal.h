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

#ifndef taiMemberOfTableCellValueVal_h
#define taiMemberOfTableCellValueVal_h 1

// parent includes:
#include <taiMember>

// member includes:

// declare all other types mentioned but not required to include:

taTypeDef_Of(taiMemberOfTableCellValueVal);

class TA_API taiMemberOfTableCellValueVal : public taiMember {
  // the value member in a DataTableCell
  TAI_MEMBER_SUBCLASS(taiMemberOfTableCellValueVal, taiMember);
public:

  int                   BidForMember(MemberDef* md, TypeDef* td) override;
  taiWidget*            GetWidgetRep_impl(IWidgetHost* host_, taiWidget* par,
                          QWidget* gui_parent_, int flags_, MemberDef* mbr_) override;
protected:
  void                  GetImage_impl(taiWidget* dat, const void* base) override;
  void                  GetMbrValue_impl(taiWidget* dat, void* base) override;

private:
  void Initialize()  { };
  void Destroy()     { };
};

#endif // taiMemberOfTableCellValueVal_h
