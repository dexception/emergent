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

#ifndef taiArgTypeOfListBasePtr_h
#define taiArgTypeOfListBasePtr_h 1

// parent includes:
#include <taiArgTypeOfTokenPtr>

// member includes:

// declare all other types mentioned but not required to include:


taTypeDef_Of(taiArgTypeOfListBasePtr);

class TA_API taiArgTypeOfListBasePtr : public taiArgTypeOfTokenPtr {
  // for taBase pointers in groups, sets the typedef to be the right one..
  TAI_ARGTYPE_SUBCLASS(taiArgTypeOfListBasePtr, taiArgTypeOfTokenPtr);
public:
  int           BidForArgType(int aidx, TypeDef* argt, MethodDef* md, TypeDef* td) override;
  cssEl*        GetElFromArg(const char* arg_nm, void* base) override;
private:
  void          Initialize() {}
  void          Destroy() {}
};

#endif // taiArgTypeOfListBasePtr_h
