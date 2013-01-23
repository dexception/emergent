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

#ifndef taiTokenPtrType_h
#define taiTokenPtrType_h 1

// parent includes:
#include <taiType>

// member includes:

// declare all other types mentioned but not required to include:
class taBase; //

TypeDef_Of(taiTokenPtrType);

class TA_API taiTokenPtrType : public taiType {
  TAI_TYPEBASE_SUBCLASS(taiTokenPtrType, taiType);
public:
  enum Mode {
    MD_BASE,            // taBase pointer
    MD_SMART_PTR,       // taSmartPtr -- acts almost identical to taBase*
    MD_SMART_REF        // taSmartRef
  };
  override bool handlesReadOnly() const { return true; } // uses a RO tokenptr button
  taBase*       GetTokenPtr(const void* base) const; // depends on mode
  TypeDef*      GetMinType(const void* base);
  int           BidForType(TypeDef* td);
protected:
  taiData*      GetDataRep_impl(IDataHost* host_, taiData* par,
    QWidget* gui_parent_, int flags_, MemberDef* mbr);
  void          GetImage_impl(taiData* dat, const void* base);
  void          GetValue_impl(taiData* dat, void* base);

  Mode          mode; // set during first GetDataRep (is garbage until then)
};

#endif // taiTokenPtrType_h
