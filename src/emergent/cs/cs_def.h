// Copyright, 1995-2007, Regents of the University of Colorado,
// Carnegie Mellon University, Princeton University.
//
// This file is part of Emergent
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


#ifndef CS_DEF_H
#define CS_DEF_H

#include "ta_stdef.h"
 
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the TA_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#if ((defined(CS_DLL)) && (defined(TA_OS_WIN) && defined(_MSC_VER)) && (!defined(__MAKETA__)))
  #ifdef CS_EXPORTS
  #define CS_API __declspec(dllexport)
  #else
  #define CS_API __declspec(dllimport)
  #endif
#else 
#define CS_API
#endif


#endif // CS_DEF_H
