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

#ifndef MiscCall_h
#define MiscCall_h 1

// parent includes:
#include <StaticMethodCall>

// member includes:

// declare all other types mentioned but not required to include:
class ProgEl; // 


taTypeDef_Of(MiscCall);

class TA_API MiscCall : public StaticMethodCall { 
  // call a taMisc function
INHERITED(StaticMethodCall)
public:
  String	GetToolbarName() const override { return "misc()"; }

  bool		CanCvtFmCode(const String& code, ProgEl* scope_el) const override;

  TA_BASEFUNS_NOCOPY(MiscCall);
private:
  void	Initialize();
  void	Destroy()	{ };
}; 

#endif // MiscCall_h
