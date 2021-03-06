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

#include "AddNewDataRow.h"
#include <Program>
#include <NameVar_PArray>
#include <taMisc>

TA_BASEFUNS_CTORS_DEFN(AddNewDataRow);


void AddNewDataRow::Initialize() {
}

String AddNewDataRow::GetDisplayName() const {
  String rval = "Add New Row: ";
    
  if(data_var)
    rval += " table = " + data_var->name + " ";
  else
    rval += " table = ? ";
    
  return rval;
}

void AddNewDataRow::GenCssBody_impl(Program* prog) {
  if(!data_var) {
    prog->AddLine(this, "// data_var not set!", ProgLine::MAIN_LINE);
    return;
  }
  prog->AddLine(this, data_var->name + ".AddBlankRow();", ProgLine::MAIN_LINE);
  prog->AddVerboseLine(this);
}

bool AddNewDataRow::CanCvtFmCode(const String& code, ProgEl* scope_el) const {
  if (CvtFmCodeCheckNames(code))
    return true;
  
  String dc = code;  dc.downcase();
  dc.gsub(" ", "");
  if(dc.startsWith("addnew") || dc.startsWith("add_new") ||
     dc.startsWith("newrow") || dc.startsWith("new_row"))
    return true;
  return false;
}

bool AddNewDataRow::CvtFmCode(const String& code) {
  String dc = code;  dc.downcase();    
  String remainder = code.after(":");
  if(remainder.empty()) return true;
    
  NameVar_PArray nv_pairs;
  taMisc::ToNameValuePairs(remainder, nv_pairs);
    
  for (int i=0; i<nv_pairs.size; i++) {
    String name = nv_pairs.FastEl(i).name;
    name.downcase();
    String value = nv_pairs.FastEl(i).value.toString();
        
    if (name.startsWith("tab")) {
      data_var = FindVarNameInScope(value, false); // don't make
    }
  }
    
  SigEmitUpdated();
  return true;
}

