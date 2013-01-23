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

#ifndef T3DataViewFrame_List_h
#define T3DataViewFrame_List_h 1

// parent includes:
#include <DataViewer_List>

// member includes:
#include <T3DataViewFrame>


// declare all other types mentioned but not required to include:

TypeDef_Of(T3DataViewFrame_List);

class TA_API T3DataViewFrame_List: public DataViewer_List { // #NO_TOKENS
INHERITED(DataViewer_List)
public:
  TA_DATAVIEWLISTFUNS(T3DataViewFrame_List, DataViewer_List, T3DataViewFrame)
private:
  NOCOPY(T3DataViewFrame_List)
  void  Initialize() { SetBaseType(&TA_T3DataViewFrame);}
  void  Destroy() {}
};


#endif // T3DataViewFrame_List_h
