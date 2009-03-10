// Copyright, 1995-2007, Regents of the University of Colorado,
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


// mta_constr.cpp

#include "mta_constr.h"

#include "ta_platform.h"
#include "ta_type.h"

//////////////////////////////////
// 	Link Resolution		//
//////////////////////////////////
// (called before generating types)

// types that are _not_ part of the specified include files but are refered to
// by types that _are_ need to be added to the destination type space (dst)
// which is what will be used to generate type info
// these link-generated types are added as "internal" types

void TypeSpace_Generate_AddUndef(TypeSpace* ths, TypeSpace* trg, TypeSpace* dst) {
  int i;
  for(i=0; i< ths->size; i++) {
    TypeDef* td = ths->FastEl(i);
    if((td->owner == NULL) || (td->owner == trg) || (td->pre_parsed)
       || (td->owner->owner != NULL))
      continue;

    TypeDef* utd = dst->AddUniqNameOld(td);
    utd->internal = true;		// make internal
    if(utd == td) {		// add to dst
      if((!td->DerivesFormal(TA_class) && !td->DerivesFormal(TA_enum))
	 || (td->ptr > 0))	// add this stuff when unique
	TypeSpace_Generate_AddUndef(&(td->parents), trg, dst); // non-class parents are added..
    }
  }
}

void MemberSpace_Generate_AddUndef(MemberSpace* ths, TypeSpace* trg, TypeSpace* dst) {
  int i;
  for(i=0; i< ths->size; i++) {
    MemberDef* md = ths->FastEl(i);
    if((md->type->owner == NULL) || (md->type->owner == trg)
       || (md->type->pre_parsed) || (md->type->owner->owner != NULL))
      continue;

    TypeDef* utd = dst->AddUniqNameOld(md->type);
    utd->internal = true;		// make internal
    if(utd == md->type) {	// add to dst
      if((!utd->DerivesFormal(TA_class) && !utd->DerivesFormal(TA_enum))
	 || (utd->ptr > 0))
	TypeSpace_Generate_AddUndef(&(utd->parents), trg, dst); // non-class parents are added..
    }
  }
}

void MethodSpace_Generate_AddUndef(MethodSpace* ths, TypeSpace* trg, TypeSpace* dst) {
  int i;
  for(i=0; i< ths->size; i++) {
    MethodDef* md = ths->FastEl(i);
    // get the arguments
    TypeSpace_Generate_AddUndef(&(md->arg_types), trg, dst);

    // and the return type
    if((md->type->owner == NULL) || (md->type->owner == trg)
       || (md->type->pre_parsed) || (md->type->owner->owner != NULL))
      continue;

    TypeDef* utd = dst->AddUniqNameOld(md->type);
    utd->internal = true;		// make internal
    if(utd == md->type) {	// add to dst
      if((!utd->DerivesFormal(TA_class) && !utd->DerivesFormal(TA_enum))
	 || (utd->ptr > 0))
	TypeSpace_Generate_AddUndef(&(utd->parents), trg, dst); // non-class parents are added..
    }
  }
}

void TypeDef_Generate_LinkRefs(TypeDef* ths, TypeSpace* trg, TypeSpace* dst) {
  if(ths->internal || ths->pre_parsed)
     return;
  TypeSpace_Generate_AddUndef(&(ths->parents), trg, dst); // make sure parents are there
  if(ths->InheritsFormal(TA_class)) {
    MemberSpace_Generate_AddUndef(&(ths->members), trg, dst);
    MethodSpace_Generate_AddUndef(&(ths->methods), trg, dst);
  }
}

void TypeSpace_Generate_LinkRefs(TypeSpace* ths,TypeSpace* dst) {
  int i;
  for(i=0; i< ths->size; i++)
    TypeDef_Generate_LinkRefs(ths->FastEl(i), ths, dst);
  dst->AddUnique(&TA_taRegFun);	// always keep this one around
}


//////////////////////////////////////////////////
//   	List Sorting: Parents Before Children	//
//////////////////////////////////////////////////
// (called before generating types)

bool TypeSpace_Sort_Order(TypeSpace* ths) {
  bool move_occurred = false;
  int i;
  for(i=0; i< ths->size; i++) {
    TypeDef* td = ths->FastEl(i);
    int j;
    for(j=0; j<td->parents.size; j++) {
      TypeDef* par_td = td->parents.FastEl(j);
      if((td->idx < par_td->idx) && (td->owner == ths) && (par_td->owner == ths)) {
	if(mta->verbose > 0)
	  cerr << "M!!: Switching order of: " << td->name << " fm: " << td->idx << " to: "
	       << par_td->idx+1 << "\n";
	// child comes before parent..
	ths->MoveIdx(td->idx, par_td->idx+1); // move after parent
	move_occurred = true;
      }
    }
  }
  return move_occurred;
}


//////////////////////////////////
// 	Declarations		//
//////////////////////////////////
// (_TA_type.h _TA_inst.h files)


void MTA::TypeSpace_Declare_Types(TypeSpace* ths, ostream& strm, const String_PArray&) {
  strm << "#ifndef " << ths->name << "_TA_types_h\n";
  strm << "#define " << ths->name << "_TA_types_h 1\n";
  strm << "\n// File Automatically Generated by MakeTA\n"
    << "// DO NOT EDIT\n\n\n";

#ifdef TA_OS_WIN
  if (win_dll)
    strm << "#include \"" << basename << "_def.h\"\n\n";
#endif

  strm << "class TypeDef;\n";
#ifdef TA_OS_WIN
  if (win_dll)
  strm << "extern " << win_dll_str << " void" << " ta_Init_" << ths->name << "();\n\n";
  else
#endif
  strm << "extern void" << " ta_Init_" << ths->name << "();\n\n";

  int i;
  for(i=0; i < ths->size; i++) {
    TypeDef_Declare_Types(ths->FastEl(i), strm);
  }

  strm << "\n\n#endif // " << ths->name << "_TA_types_h\n";
}

void MTA::TypeDef_Declare_Types(TypeDef* ths, ostream& strm) {
  if(ths->pre_parsed)   return;
#ifdef TA_OS_WIN
  if (win_dll)
    strm << "extern " << win_dll_str << " TypeDef TA_" << ths->name << ";\n";
  else
#endif
    strm << "extern TypeDef TA_" << ths->name << ";\n";
}

void MTA::TypeSpace_Declare_Instances(TypeSpace* ths, ostream& strm, const String_PArray& hv) {
  strm << "#ifndef " << ths->name << "_TA_insts_h\n";
  strm << "#define " << ths->name << "_TA_insts_h 1\n";
  strm << "\n// File Automatically Generated by MakeTA\n"
    << "// DO NOT EDIT\n\n\n";

  TypeSpace_Includes(ths, strm, hv, true);   // not necc. on declares

  int i;
  for(i=0; i < ths->size; i++) {
    TypeDef_Declare_Instances(ths->FastEl(i), strm);
  }

  strm << "\n\n#endif // " << ths->name << "_TA_insts_h\n";
}

void MTA::TypeDef_Declare_Instances(TypeDef* ths, ostream& strm) {
  if(ths->internal || ths->pre_parsed)   return;

  if((mta->gen_instances || ths->HasOption("INSTANCE"))
     && !ths->HasOption("NO_INSTANCE"))
#ifdef TA_OS_WIN
    if (win_dll)
      strm << "extern " << win_dll_str << " " << ths->Get_C_Name() << "*\tTAI_" << ths->name << ";\n";
    else
#endif
      strm << "extern " << ths->Get_C_Name() << "*\tTAI_" << ths->name << ";\n";
}


//////////////////////////////////
// 	      Includes 		//
//////////////////////////////////

void TypeSpace_Includes(TypeSpace* ths, ostream& strm, const String_PArray& hv,
			bool instances)
{
  strm << "#include \"ta_type.h\"\n";
  if(!instances) {
    strm << "#include \"ta_type_constr.h\"\n";
    if(mta->gen_css) {
      strm << "#include \"css_basic_types.h\"\n";
      strm << "#include \"css_c_ptr_types.h\"\n";
      strm << "#include \"css_ta.h\"\n";
    }
  }

  int i;
  for(i=0; i<hv.size; i++) {
    String nstr = hv.FastEl(i);
    if(nstr.contains('/')) //note: assumed that all source uses / for path sep
      nstr = nstr.after('/',-1); // just the file name
    else if(nstr.contains('\\')) //note: for Windows
      nstr = nstr.after('\\',-1); // just the file name
    if(nstr.contains("streambuf") || nstr.contains("iostream") ||
       nstr.contains("fstream") || nstr.contains("sstream")) continue;
    strm << "#include \"" << nstr << "\"\n";
  }
  strm << "#include \"" << ths->name << "_TA_type.h\"\n";
  if(!instances)
    strm << "#include \"" << ths->name << "_TA_inst.h\"\n";
  strm << "\n\n";
}


//////////////////////////////////
// 	  _TA.cc File		//
//////////////////////////////////


void MTA::TypeSpace_Generate(TypeSpace* ths, ostream& strm, const String_PArray& hv,
			const String_PArray& ppfiles)
{
  strm << "// File Automatically Generated by MakeTA\n"
       << "// DO NOT EDIT\n\n\n";

  TypeSpace_Includes(ths, strm, hv);
  TypeSpace_Generate_Types(ths, strm);
  TypeSpace_Generate_Instances(ths, strm);
  TypeSpace_Generate_Data(ths, strm);
  TypeSpace_Generate_Init(ths, strm, ppfiles);
}


//////////////////////////////////
// 	  References		//
//////////////////////////////////

String TypeDef_Gen_Ref(TypeDef* ths) {
  if(ths->owner == NULL) {
    cerr << "W!!: Warning: referring to unowned type: " << ths->name << "\n";
    return "TA_void";
  }
  if(ths->owner->owner == NULL)	// on some kind of global list
    return "TA_" + ths->name;
  String rval = TypeDef_Gen_Ref_To(ths->owner->owner) + ths->owner->name;
  rval += ".FindName(\"" + ths->name + "\")";
  return rval;
}

String TypeDef_Gen_Ref_To(TypeDef* ths) {
  String rval = TypeDef_Gen_Ref(ths);
  if((ths->owner == NULL) || (ths->owner->owner == NULL)) {
    return rval + ".";
  }
  return rval + "->";
}

String TypeDef_Gen_Ref_Of(TypeDef* ths) {
  String rval = TypeDef_Gen_Ref(ths);
  if((ths->owner == NULL) || (ths->owner->owner == NULL)) {
    return String("&") + rval;
  }
  return rval;
}


//////////////////////////////////
// 	TypeDef Constructors	//
//////////////////////////////////
// (part 1 of _TA.cc file)

// no need to save all that instance stuff (make sure not to get the NO_)

void MTA::TypeDef_FixOpts(String_PArray& op) {
  op.RemoveEl("INSTANCE");
  op.RemoveEl("NO_TOKENS");
}

void MTA::TypeDef_Generate_Types(TypeDef* ths, ostream& strm) {
  if(ths->pre_parsed)    return;

#ifdef TA_OS_WIN
    if (win_dll)
      strm << win_dll_str << " TypeDef TA_" << ths->name;
    else
#endif
      strm << "TypeDef TA_" << ths->name;

 if (ths->internal) {
    strm <<  "(\"" << ths->name << "\", 1, " << ths->ptr;
    if(ths->ref)	strm << ", 1";
    else		strm << ", 0";
    if(ths->formal)	strm << ", 1";
    else		strm << ", 0";
    strm << ", 1";		// this true makes it global object
    // if it is a built-in actual type, we get its size, otherwise just output 0
    if ((ths->size == 1) & !ths->formal)	
      strm << ", sizeof(" << ths->Get_C_Name() << ")";
    else		
      strm << ", " << 0;
    strm << ");\n";
  } else {
    // non-internals, includes, ex. taString and all parsed classes
    String_PArray act_opts = ths->opts;
    String_PArray act_inh_opts = ths->inh_opts;

    TypeDef_FixOpts(act_opts);
    TypeDef_FixOpts(act_inh_opts);

    String str_opts = taMisc::StrArrayToChar(act_opts);
    String str_inh_opts = taMisc::StrArrayToChar(act_inh_opts);
    String str_lists = taMisc::StrArrayToChar(ths->lists);

    strm << "(\"" << ths->name << "\", \"" << ths->desc << "\", ";
    strm << "\n\t\"" << str_inh_opts << "\", \"" << str_opts << "\", \""
	 << str_lists << "\", ";
    if(ths->InheritsFormal(TA_enum))
      strm << "sizeof(int), ";
    else
      strm << "sizeof(" << ths->Get_C_Name() << "), ";

    strm << "(void**)";
    if((mta->gen_instances || ths->HasOption("INSTANCE"))
       && !ths->HasOption("NO_INSTANCE"))
      strm << "&TAI_" << ths->name;
    else
      strm << "0";

    if(ths->tokens.keep)	strm << ", 1";
    else			strm << ", 0";
    strm << ", " << ths->ptr;
    if(ths->ref)	strm << ", 1";
    else		strm << ", 0";
    strm << ",1);\n";		// last true makes it global object
  }
}

void MTA::TypeSpace_Generate_Types(TypeSpace* ths, ostream& strm) {
  strm << "// Types\n\n";
  int i;
  for(i=0; i<ths->size; i++)
    TypeDef_Generate_Types(ths->FastEl(i), strm);
}


//////////////////////////////////
//   Type Instances & stubs	//
//////////////////////////////////
// (part 2 of _TA.cc file)


void MTA::TypeDef_Generate_Instances(TypeDef* ths, ostream& strm) {

  // this is just for reg_fun
  if(ths->InheritsFrom(TA_taRegFun) && this->gen_css
     && !ths->HasOption("NO_CSS"))
    MethodSpace_Generate_Stubs(&(ths->methods), ths, strm);

  if(ths->internal || ths->pre_parsed) return;

  if((this->gen_instances || ths->HasOption("INSTANCE"))
     && !ths->HasOption("NO_INSTANCE"))
#ifdef TA_OS_WIN
    if (win_dll)
      strm << win_dll_str << " " << ths->Get_C_Name() << "*\t\t TAI_" << ths->name << "=NULL;\n";
    else
#endif
    strm << ths->Get_C_Name() << "*\t\t TAI_" << ths->name << "=NULL;\n";

  if(!ths->InheritsFormal(TA_class)) return;

  if(this->gen_css && !ths->HasOption("NO_CSS"))
    MethodSpace_Generate_Stubs(&(ths->methods), ths, strm);
    
  // generate property stubs
  MemberSpace_Generate_PropStubs(&(ths->members), ths, strm);
  MethodSpace_Generate_PropStubs(&(ths->methods), ths, strm);
}

void MTA::TypeSpace_Generate_Instances(TypeSpace* ths, ostream& strm) {
  strm << "\n// Instances\n\n";
  int i;
  for(i=0; i<ths->size; i++)
    TypeDef_Generate_Instances(ths->FastEl(i), strm);
}


//////////////////////////////////
//  	      CSS Stubs		//
//////////////////////////////////
// (part 2 of _TA.cc file)

String VariantToTargetConversion(TypeDef* param_td) {
  String conv;
  if (param_td) { // better exist!
    if (param_td->InheritsFrom(TA_taBase) && (param_td->ptr == 1)) {
      conv = "dynamic_cast<" + param_td->GetNonPtrType()->Get_C_Name() +
        "*>(val.toBase())";
    } else { // anything else -- if illegal, will show up as compile error in TA.cpp
//      conv = "(" + param_td->GetNonConstType()->GetNonRefType()->Get_C_Name() + ")VarCvt(val)";
      conv = "VarCvt(val)";
    }
  }
  return conv;
}

void MemberSpace_Generate_PropStubs(MemberSpace* ths, TypeDef* ownr, ostream& strm) {
  for (int i=0; i<ths->size; i++) {
    MemberDef* md = ths->FastEl(i);
    bool is_new = false;
    // getter stub
    String prop_name = md->OptionAfter("GET_");
    if (prop_name.nonempty()) {
      ownr->properties.AssertProperty(prop_name, is_new, true, md);
      strm << "  Variant ta_" << ownr->name << "_" << prop_name 
        << "_get(const void* inst){return Variant(((const "
        << ownr->GetNonPtrType()->Get_C_Name() 
        << "*)inst)->" << md->name << ");}\n";
    }
    // setter stub
    prop_name = md->OptionAfter("SET_");
    if (prop_name.nonempty()) {
      // make conversion for the setter param
      ownr->properties.AssertProperty(prop_name, is_new, false, md);
      TypeDef* param_td = md->type;
      if (param_td) { // better exist!
        String conv = VariantToTargetConversion(param_td);
        strm << "  void ta_" << ownr->name << "_" << prop_name 
          << "_set(void* inst, const Variant& val) {(("
          << ownr->GetNonPtrType()->Get_C_Name() << "*)inst)->" << md->name 
          << " = " << conv << ";}\n";
      }
    }
  }
}

void MethodSpace_Generate_PropStubs(MethodSpace* ths, TypeDef* ownr, ostream& strm) {
  for (int i=0; i<ths->size; i++) {
    MethodDef* md = ths->FastEl(i);
    bool is_new = false;
    // getter stub
    String prop_name = md->OptionAfter("GET_");
    if (prop_name.nonempty()) {
      ownr->properties.AssertProperty(prop_name, is_new, true, md);
      strm << "  Variant ta_" << ownr->name << "_" << prop_name 
        << "_get(const void* inst){return Variant(((const "
        << ownr->GetNonPtrType()->Get_C_Name()
        << "*)inst)->" << md->name << "());}\n";
    }
   // setter stub
    prop_name = md->OptionAfter("SET_");
    if (prop_name.nonempty()) {
      // make conversion for the setter param
      TypeDef* param_td = md->arg_types.SafeEl(0);
      if (param_td) { // better exist!
        ownr->properties.AssertProperty(prop_name, is_new, false, md);
        String conv = VariantToTargetConversion(param_td);
        strm << "  void ta_" << ownr->name << "_" << prop_name 
          << "_set(void* inst, const Variant& val) {(("
          << ownr->GetNonPtrType()->Get_C_Name() << "*)inst)->" << md->name 
          << "(" << conv << ");}\n";
      } else {
      cerr << "**ERROR " << md->name << "SET method must have at least one arg!\n";
      }
    }
  }
}


void MethodSpace_Generate_Stubs(MethodSpace* ths, TypeDef* ownr, ostream& strm) {
  int i;
  for(i=0; i<ths->size; i++) {
    MethodDef* md = ths->FastEl(i);
    if(!MethodSpace_Filter_Method(ths, md))
      continue;

    MethodDef_GenStubName(ownr, md, strm);

    /* argd should always be less than argc, but scanner might screw up
       (in fact it does in certain cases..) (if so, then just reset!) */
    if(md->fun_argd > md->fun_argc) md->fun_argd = -1;

    if(md->fun_argd >= 0) {
      int j;
      for(j=md->fun_argd; j<= md->fun_argc; j++) {
	MethodDef_GenFunCall(ownr, md, strm, j);
      }
    }
    else {
      MethodDef_InitTempArgVars(md, strm, md->fun_argc); // declare and init temp arg vars
      MethodDef_GenFunCall(ownr, md, strm, md->fun_argc);
      MethodDef_AssgnTempArgVars(ownr, md, strm, md->fun_argc);
    }

    strm << "    return rval;}\n";
  }
}

void MethodDef_InitTempArgVars(MethodDef* md, ostream& strm, int act_argc) {
  bool got_one = false;
  int j;
  for(j=0;j < act_argc; j++) {
    TypeDef* argt = md->arg_types[j];
    if(!(argt->ref && !argt->InheritsFrom(TA_const)))
      continue;
    TypeDef* nrt = argt->GetNonRefType();
    if(nrt == NULL) {
      taMisc::Error("Null NonRefType in GenTempArgVars()", argt->name);
      continue;
    }

    if((nrt->ptr == 0) &&
	 !(nrt->DerivesFrom(TA_taString) || !nrt->InheritsFormal(TA_class))) {
      // a non-ptr class reference: create a ref variable that doesn't get
      // assigned later (just to get around compiler warnings)
      got_one = true;
      strm << "    " << argt->Get_C_Name() << " refarg_" << j << "=";
      MethodDef_GenArgCast(md, nrt, j, strm);
      strm << ";";
    }
    else {
      got_one = true;
      strm << "    " << nrt->Get_C_Name() << " refarg_" << j << "=";
      MethodDef_GenArgCast(md, nrt, j, strm);
      strm << ";";
    }
  }
  if(got_one)
    strm << "\n";
}

void MethodDef_AssgnTempArgVars(TypeDef* ownr, MethodDef* md, ostream& strm, int act_argc) {
  bool got_one = false;
  int j;
  for(j=0;j < act_argc; j++) {
    TypeDef* argt = md->arg_types[j];
    if(!(argt->ref && !argt->InheritsFrom(TA_const)))
      continue;

    TypeDef* nrt = argt->GetNonRefType();
    if(nrt == NULL) {
      taMisc::Error("Null NonRefType in GenTempArgVars()", argt->name);
      continue;
    }
    bool not_mod = true;
    if(nrt->ptr == 0) {	// harder to do the non-ptr refs
      if(nrt->DerivesFrom(TA_taString) || !nrt->InheritsFormal(TA_class)) {
	strm << "    *arg[" << j+1 << "]=" << MethodDef_GetCSSType(nrt);
	strm << "refarg_" << j << ";";
	not_mod = false;	got_one = true;
      }
    }
    else if(nrt->DerivesFrom(TA_taBase)) {
      // need to use taBase* to preserve reffing sanity!
      strm << "    *arg[" << j+1 << "]=(taBase" << nrt->GetPtrString() << ")";
      strm << "refarg_" << j << ";";
      not_mod = false;		got_one = true;
    }
    else {			// all ptrs just done through (void*)..
      strm << "    *arg[" << j+1 << "]=(void" << nrt->GetPtrString() << ")";
      strm << "refarg_" << j << ";";
      not_mod = false;		got_one = true;
    }
    //TODO: seems to be a conceptual bug, since we should be able to pass
    //  generic pointer to &ref to access the arg by value; therefore, no
    // copying back should be necessary, in fact, that is probably not desirable
    if(not_mod && (mta->verbose > 0)) {
      String stargno = String(j);
      String stmbnm = ownr->Get_C_Name() + "::" + md->name + "()";
      taMisc::Error("non-const reference argument value not modified:",
		     md->arg_names[j],"(arg",stargno,") in",stmbnm);
    }
  }
  if(got_one)
    strm << "\n";
}

String MethodDef_GetCSSType(TypeDef* td) {
//NOTE: changed 4/13/06 BA
/*was  if(td->DerivesFrom(TA_int) || td->DerivesFrom(TA_short) ||
     td->DerivesFrom(TA_long) || td->DerivesFrom(TA_char) ||
     td->DerivesFormal(TA_enum) || td->DerivesFrom(TA_signed) ||
     td->DerivesFrom(TA_unsigned) || td->DerivesFrom(TA_bool))
    return String("(Int)"); */

  if (td->DerivesFrom(TA_int) || td->DerivesFrom(TA_short) ||
      td->DerivesFrom(TA_unsigned_short) ||
     td->DerivesFrom(TA_unsigned_int) || td->DerivesFormal(TA_unsigned_long) ||
     td->DerivesFrom(TA_long) || td->DerivesFormal(TA_enum) ||
     td->DerivesFrom(TA_unsigned_char) || td->DerivesFormal(TA_signed_char))
    return String("(Int)");
  
  if(td->DerivesFrom(TA_char))
    return String("(Char)");
  
  if(td->DerivesFrom(TA_bool))
    return String("(bool)");
  
  if (td->DerivesFrom(TA_int64_t) || td->DerivesFrom(TA_uint64_t) ||
    td->DerivesFrom(TA_Variant))
    return String("(Variant)");
  
  if(td->DerivesFrom(TA_double) || td->DerivesFrom(TA_float))
    return String("(Real)");

  if(td->DerivesFrom(TA_taString))
    return String("(String)");

  return String("");
}


void MethodDef_GenArgCast(MethodDef* md, TypeDef* argt, int j, ostream& strm) {
  if(argt->ref) {
    if(!argt->InheritsFrom(TA_const)) { // non-const reference arg!
      strm << "refarg_" << j;
    }
    else {
      TypeDef* nrt = argt->GetNonRefType();
      if(nrt == NULL) {
	taMisc::Error("Null NonRefType in GenArgCast()", argt->name); return;
      }
      TypeDef* nct = nrt->GetNonConstType();
      if(nct == NULL) {
	taMisc::Error("Null NonConstType in GenArgCast()", nrt->name); return;
      }
      MethodDef_GenArgCast(md, nct, j, strm); // use the base type for args
      return;
    }
  }
  else if(argt->DerivesFrom(TA_TypeDef)) { // we convert these now
    strm << "(TypeDef";
    strm << argt->GetPtrString() << ")";
    strm << "*arg[" << j+1 << "]";
  }
  else if(argt->DerivesFrom(TA_MemberDef)) { // we convert these now
    strm << "(MemberDef";
    strm << argt->GetPtrString() << ")";
    strm << "*arg[" << j+1 << "]";
  }
  else if(argt->DerivesFrom(TA_MethodDef)) { // we convert these now
    strm << "(MethodDef";
    strm << argt->GetPtrString() << ")";
    strm << "*arg[" << j+1 << "]";
  }
  else if(argt->DerivesFrom(TA_taString)) {
    if(argt->ptr == 0)
      strm << "arg[" << j+1 << "]->GetStr()";
    else
      strm << "(String" << argt->GetPtrString() << ")*arg[" << j+1 << "]";
  }
  else if(argt->DerivesFrom(TA_Variant)) {
    if(argt->ptr == 0)
      strm << "arg[" << j+1 << "]->GetVar()";
    else
      strm << "(Variant" << argt->GetPtrString() << ")*arg[" << j+1 << "]";
  }
  else if(argt->DerivesFromName("ios")) { // cssEl's can cast these directly
    if(argt->ptr == 0)
      strm << "*(" << argt->Get_C_Name() << "*)" << "*arg[" << j+1 << "]";
    else
      strm << "(" << argt->Get_C_Name() << ")" << "*arg[" << j+1 << "]";
  }
  else if(argt->InheritsFormal(TA_class)) {
    strm << "*(" << argt->Get_C_Name() << "*)arg[" << j+1 << "]"
	 << "->GetVoidPtrOfType(&TA_" << argt->name << ")";
  }
  else if((argt->ptr == 1) && (argt->DerivesFrom(TA_char))) {
    strm << "(" << argt->Get_C_Name() << ")" << "*arg[" << j+1 << "]";
  }
  else if(argt->InheritsFormal(TA_enum)) {
    strm << "(" << argt->Get_C_Name() << ")"; // always cast the args
    strm << "(int)*arg[" << j+1 << "]";		    // use int conversion
  }
  else if((argt->ptr == 1) && argt->DerivesFormal(TA_class)) {
    strm << "(" << argt->Get_C_Name() << ")arg[" << j+1 << "]"
	 << "->GetVoidPtrOfType(&TA_" << argt->name << ")";
  }
  else if(argt->ptr > 0) {
    strm << "(" << argt->Get_C_Name() << ")(void";
    strm << argt->GetPtrString() << ")";
    strm << "*arg[" << j+1 << "]";
  }
  else {
    strm << "(" << argt->Get_C_Name() << ")"; // always cast the args
    strm << "*arg[" << j+1 << "]";
  }
}


void MethodDef_GenArgs(MethodDef* md, ostream& strm, int act_argc) {
  int j;

  for(j=0;j < act_argc; j++) {
    TypeDef* argt = md->arg_types[j];

    MethodDef_GenArgCast(md, argt, j, strm);

    if(j+1 < act_argc)
      strm << ", ";
  }
}

void MethodDef_GenStubName(TypeDef* ownr, MethodDef* md, ostream& strm) {
  if(ownr->InheritsFrom(TA_taRegFun))
    strm << "  static cssEl* cssElCFun_" << md->name;
  else
    strm << "  static cssEl* cssElCFun_" << ownr->name << "_" << md->name;
  strm << "_stub(";
  if(ownr->InheritsFrom(TA_taRegFun))
    strm << "void*,";
  else
    strm << "void* ths,";
  if(md->fun_argc == 0)
    strm << "int, cssEl**) {\n";
  else if (md->fun_argd >= 0)
    strm << "int na, cssEl** arg) {\n";
  else
    strm << "int, cssEl** arg) {\n";
  strm << "    cssEl* rval=&cssMisc::Void;\n";
}

void MethodDef_GenStubCall(TypeDef* ownr, MethodDef* md, ostream& strm) {
  if(ownr->InheritsFrom(TA_taRegFun))
    strm << md->name << "(";
  else
    strm << "((" << ownr->Get_C_Name() << "*)ths)->" << md->name << "(";
}

void MethodDef_GenFunCall(TypeDef* ownr, MethodDef* md, ostream& strm, int act_argc) {
  String cmd;

  if(md->fun_argd >= 0) {
    strm << "    if(na == " << act_argc << ") {\n  ";
    MethodDef_InitTempArgVars(md, strm, act_argc); // declare and init temp arg vars
  }
  strm << "    ";

  bool has_rval = ((md->type->ptr > 0) || (md->type != &TA_void));
  if (md->type->ptr == 0) {
    has_rval = true;
    if (md->type == &TA_void) // explicitly not, so ok,
      has_rval = false;
    else if (md->type->DerivesFrom(TA_int) || md->type->DerivesFrom(TA_unsigned_int) ||
       md->type->DerivesFrom(TA_signed_char) || md->type->DerivesFrom(TA_unsigned_char) ||
       md->type->DerivesFrom(TA_short) || md->type->DerivesFrom(TA_unsigned_short) ||
       md->type->DerivesFrom(TA_long) || md->type->DerivesFrom(TA_unsigned_long))
      cmd = "cssInt((int)";
    else if (md->type->DerivesFrom(TA_bool))
      cmd = "cssBool(";
    else if(md->type->DerivesFrom(TA_float) || md->type->DerivesFrom(TA_double))
      cmd = "cssReal((double)";
    else if (md->type->DerivesFrom(TA_taString))
      cmd = "cssString(";
    else if(md->type->DerivesFrom(TA_Variant) ||
      md->type->DerivesFrom(TA_uint64_t))
      cmd = "cssVariant(";
    else if(md->type->DerivesFrom(TA_int64_t))
      cmd = "cssInt64(";
    else if (md->type->DerivesFrom(TA_char))
      cmd = "cssChar(";
    else if (md->type->InheritsFormal(TA_enum))
      // NOTE: we can't make strongly typed enums here, but an int will
      // convert properly to any strong enum, and should work fine
      cmd = "cssInt((int)";
    else {
      // we don't know how to handle this rval -- not good!!!!
      has_rval = false;
      if (mta->verbose > 0) {
      cerr << "W!!: Warning: in file: " << mta->cur_fname << " method: " << md->name << 
        " don't know how to handle return type: " << md->type->name << 
        " so it will be ignored!\n";
     }
    }
    if (has_rval)
      strm << "rval=new " << cmd;
    MethodDef_GenStubCall(ownr, md, strm);
    MethodDef_GenArgs(md, strm, act_argc);
    if (has_rval) strm << ")";
    strm << ");";
  } else { // ptr > 0, therefore, ptr of some kind
    bool include_td = false;
    // TODO: wrapping unsigned types with a signed wrapper as we do will
    // give wrong behavior when the value is > MAX_xxx (i.e. looks -ve to int type
    // We should have distinct signed/unsigned types
    if(md->type->DerivesFrom(TA_int) || md->type->DerivesFrom(TA_unsigned_int))
      cmd = "cssCPtr_int(";
    else if(md->type->DerivesFrom(TA_short) || md->type->DerivesFrom(TA_unsigned_short))
      cmd = "cssCPtr_short(";
    else if(md->type->DerivesFrom(TA_long) || md->type->DerivesFrom(TA_unsigned_long))
      cmd = "cssCPtr_long(";
    //NOTE: slightly wrong conceptually to include the signed/unsigned, since these
    // are really math types, not a char type, but the ptr groks the size, so that is
    // more important, and the cssChar type can grok math, so only output might be affected
    else if(md->type->DerivesFrom(TA_char) || md->type->DerivesFrom(TA_signed_char) ||
      md->type->DerivesFrom(TA_unsigned_char))
      cmd = "cssCPtr_char(";
    else if(md->type->DerivesFormal(TA_enum))
      cmd = "cssCPtr_int(";
    else if(md->type->DerivesFrom(TA_double))
      cmd = "cssCPtr_double(";
    else if(md->type->DerivesFrom(TA_float))
      cmd = "cssCPtr_float(";
    else if(md->type->DerivesFrom(TA_bool))
      cmd = "cssCPtr_bool("; 
    //TODO: wrapping uint64 in int64 will give incorrect results for large vals!
    else if(md->type->DerivesFrom(TA_int64_t) || md->type->DerivesFrom(TA_uint64_t))
      cmd = "cssCPtr_long_long(";
    else if(md->type->DerivesFrom(TA_taString))
      cmd = "cssCPtr_String(";
    else if(md->type->DerivesFrom(TA_Variant))
      cmd = "cssCPtr_Variant(";
    else if(md->type->DerivesFrom(TA_taBase)) {
      cmd = "cssTA_Base(";
      include_td = true;
    }
    else if(md->type->DerivesFrom(TA_TypeDef)) {
      cmd = "cssTypeDef(";
      include_td = true;
    }
    else if(md->type->DerivesFrom(TA_MethodDef)) {
      cmd = "cssMethodDef(";
      include_td = true;
    }
    else if(md->type->DerivesFrom(TA_MemberDef)) {
      cmd = "cssMemberDef(";
      include_td = true;
    }
    else {
      cmd = "cssTA(";
      include_td = true;
    }
    // everything is cast into a void*! 
    strm << "rval=new " << cmd << "(void*)";
    MethodDef_GenStubCall(ownr, md, strm);
    MethodDef_GenArgs(md, strm, act_argc);
    strm << ")";
    strm << ", " << (int)(md->type->ptr);
    if(include_td) {
      TypeDef* nptd = md->type->GetNonPtrType();
      strm << ", " << TypeDef_Gen_Ref_Of(nptd);
    }
    strm << ");";
  } 

  if(md->fun_argd >= 0) {
    MethodDef_AssgnTempArgVars(ownr, md, strm, act_argc);
    strm << "}\n";
  } else
    strm << "\n";
}


//////////////////////////////////
//   	      Type Data		//
//////////////////////////////////
// (part 3 of _TA.cc file)

void TypeSpace_Generate_Data(TypeSpace* ths, ostream& strm) {
  strm << "\n// Type Data\n\n";
  int i;
  for(i=0; i<ths->size; i++)
    TypeDef_Generate_Data(ths->FastEl(i), strm);
}

void TypeDef_Generate_Data(TypeDef* ths, ostream& strm) {
  if(!(ths->internal || ths->pre_parsed)) {
    if(ths->InheritsFormal(TA_enum)) {
      TypeDef_Generate_EnumData(ths, strm);
    }
    if(ths->InheritsFormal(TA_class) && !(ths->HasOption("NO_MEMBERS"))) {
      TypeDef_Generate_EnumData(ths, strm);
      TypeDef_Generate_MemberData(ths, strm);
      TypeDef_Generate_MethodData(ths, strm);
      TypeDef_Generate_PropertyData(ths, strm);
    }
  }
  if(ths->InheritsFrom(TA_taRegFun)
     && (mta->gen_css) && !ths->HasOption("NO_CSS")) {
    TypeDef_Generate_MethodData(ths, strm);
  }
}

// generates the two type fields (either a ptr to the type or a string descr)

String TypeDef_Generate_TypeFields(TypeDef* ths, TypeDef* ownr_ownr) {
  if(ths->owner == NULL) {
    cerr << "W!!: Warning: referring to unowned type: " << ths->name << "\n";
    return "&TA_void,NULL";
  }
  if(ths->owner->owner == NULL)	// on some kind of global list
    return "&TA_" + ths->name + ",NULL";

  if(ths->owner->owner == ownr_ownr) // on this type's list
    return "NULL,\"::" + ths->name + "\"";

  return "NULL,\"" + ths->owner->owner->name + "::" + ths->name + "\"";
}


//////////////////////////////////
//   	     Enum Data		//
//////////////////////////////////

void TypeDef_Generate_EnumData(TypeDef* ths, ostream& strm) {
  if(ths->InheritsFormal(TA_class)) {
    int i;
    for(i=0; i<ths->sub_types.size; i++) {
      TypeDef* enm = ths->sub_types.FastEl(i);
      if(!(enm->InheritsFormal(TA_enum)) || (enm->owner != &(ths->sub_types))
	 || (enm->enum_vals.size == 0))
	continue;

      strm << "static EnumDef_data TA_" << ths->name << "_" << enm->name
	   << "[]={\n";
      EnumSpace_Generate_Data(&(enm->enum_vals), strm);
    }
  }
  else if(ths->InheritsFormal(TA_enum)) {
    strm << "static EnumDef_data TA_" << ths->name << "_EnumDef[]={\n";
    EnumSpace_Generate_Data(&(ths->enum_vals), strm);
  }
}

void EnumSpace_Generate_Data(EnumSpace* ths, ostream& strm) {
  int j;
  for(j=0; j<ths->size; j++) {
    EnumDef* enm = ths->FastEl(j);
    if (enm->HasOption("IGNORE")) continue;
    String str_opts = taMisc::StrArrayToChar(enm->opts);
    strm << "  {\"" << enm->name << "\",\"" << enm->desc << "\",\""
	 << str_opts << "\"," << enm->enum_no << "},\n";
  }
  strm << "  NULL};\n";
}


void TypeDef_Init_EnumData(TypeDef* ths, ostream& strm) {
  if(ths->InheritsFormal(TA_class)) {
    int i;
    for(i=0; i<ths->sub_types.size; i++) {
      TypeDef* enm = ths->sub_types.FastEl(i);
      if(!(enm->InheritsFormal(TA_enum)) || (enm->owner != &(ths->sub_types))
	 || (enm->enum_vals.size == 0))
	continue;

      String str_opts = taMisc::StrArrayToChar(enm->opts);
      String str_inh_opts = taMisc::StrArrayToChar(enm->inh_opts);
      String str_lists = taMisc::StrArrayToChar(enm->lists);

      strm << "    tac_AddEnum(TA_" << ths->name << ", \"" << enm->name << "\", \""
	   << enm->desc  << "\", \"" << str_opts << "\", \"" << str_inh_opts
	   << "\", \"" << str_lists << "\", ";
      strm << "TA_" << ths->name << "_" << enm->name << ");\n";
    }
  }
  else if(ths->InheritsFormal(TA_enum)) {
    strm << "    tac_ThisEnum(TA_" << ths->name << ", ";
    strm << "TA_" << ths->name << "_EnumDef);\n";
  }
}


//////////////////////////////////
// 	   Member Data		//
//////////////////////////////////

void TypeDef_Generate_MemberData(TypeDef* ths, ostream& strm) {
  int cnt = 0;
  int i;
  for(i=0; i<ths->members.size; i++) {
    if(MemberSpace_Filter_Member(&(ths->members), ths->members.FastEl(i))) {
      cnt++;
      break;
    }
  }
  if(cnt <= 0) return;

  MemberSpace_Generate_Data(&(ths->members), ths, strm);
}

bool MemberSpace_Filter_Member(MemberSpace* ths, MemberDef* md) {
  if((md->owner == ths) && !md->HasOption("IGNORE"))
    return true;
  return false;
}

void MemberSpace_Generate_Data(MemberSpace* ths, TypeDef* ownr, ostream& strm) {
  String mbr_off_nm;

  int n_non_statics = 0;
  int i;
  for(i=0; i<ths->size; i++) {
    MemberDef* md = ths->FastEl(i);
    if(!MemberSpace_Filter_Member(ths, md))
      continue;
    if(!md->is_static)
      n_non_statics++;
  }

  if (n_non_statics > 0) {
    mbr_off_nm = String("TA_") + ownr->name + "_MbrOff";
    strm << "static int " << ownr->Get_C_Name() << "::* " << mbr_off_nm << ";\n";
  }

  strm << "static MemberDef_data TA_" << ownr->name << "_MemberDef[]={\n";

  for(i=0; i<ths->size; i++) {
    MemberDef* md = ths->FastEl(i);
    if(!MemberSpace_Filter_Member(ths, md))
      continue;

    String str_opts = taMisc::StrArrayToChar(md->opts);
    String str_lists = taMisc::StrArrayToChar(md->lists);

    String tpfld = TypeDef_Generate_TypeFields(md->type, ownr);
    strm << "  {" << tpfld << ",\"" << md->name << "\",\"" << md->desc << "\",\""
	 << str_opts << "\",\"" << str_lists << "\",\n";

    if (md->is_static) {
      strm << "    (ta_memb_ptr)NULL,1,";
      strm << "(void*)(&" << ownr->Get_C_Name() << "::" << md->name << ")";
    } else {
      strm << "    *((ta_memb_ptr*)&(" << mbr_off_nm
	    << "=(int " << ownr->Get_C_Name() << "::*)(&"
	    << ownr->Get_C_Name() << "::" << md->name << ")))";
      strm << ",0,NULL";
    }
    if(md->fun_ptr)	strm << ",1";
    else		strm << ",0";
    strm << "},\n";
  }
  strm << "  NULL};\n";
}

void TypeDef_Init_MemberData(TypeDef* ths, ostream& strm) {
  int cnt = 0;
  int i;
  for(i=0; i<ths->members.size; i++) {
    if(MemberSpace_Filter_Member(&(ths->members), ths->members.FastEl(i))) {
      cnt++;
      break;
    }
  }
  if(cnt == 0) return;

  strm << "    tac_AddMembers(TA_" << ths->name << ","
       << "TA_" << ths->name << "_MemberDef);\n";
}



//////////////////////////////////
// 	   Method Data		//
//////////////////////////////////

void TypeDef_Generate_MethodData(TypeDef* ths, ostream& strm) {
  int cnt = 0;
  int i;
  for(i=0; i<ths->methods.size; i++) {
    if(MethodSpace_Filter_Method(&(ths->methods), ths->methods.FastEl(i))) {
      cnt++;
      break;
    }
  }
  if(cnt <= 0) return;

  MethodSpace_Generate_ArgData(&(ths->methods), ths, strm);
  MethodSpace_Generate_Data(&(ths->methods), ths, strm);
}

bool MethodSpace_Filter_Method(MethodSpace* ths, MethodDef* md) {
  // always ignore these
  if((md->name == "Copy_") || md->HasOption("IGNORE")
     || ths->owner->IgnoreMeth(md->name))
    return false;

  if((md->owner != ths) && !ths->owner->HasOption("MULT_INHERIT"))
    return false;		// don't reproduce owners functions (except multi inh)

  // is_static is a problem in mult_inherit cases for compiler
  if(ths->owner->HasOption("MULT_INHERIT")) {
    if(!md->is_static)
      return true;
    return false;
  }

  return true;
}

void MethodSpace_Generate_ArgData(MethodSpace* ths, TypeDef* ownr, ostream& strm) {
  int i;
  for(i=0; i<ths->size; i++) {
    MethodDef* md = ths->FastEl(i);
    if(!MethodSpace_Filter_Method(ths, md))
      continue;

    if(md->fun_argc > 0)
      MethodDef_Generate_ArgData(md, ownr, strm);
  }
}

void MethodDef_Generate_ArgData(MethodDef* ths, TypeDef* ownr, ostream& strm) {
  strm << "static MethodArgs_data TA_" << ownr->name << "_" << ths->name
       << "_MethArgs[]={\n";

  int i;
  for(i=0; i<ths->arg_types.size; i++) {
    String tpfld = TypeDef_Generate_TypeFields(ths->arg_types[i], ownr);
    strm << "  {" << tpfld << ",\"" << ths->arg_names[i] << "\",\""
	 << ths->arg_defs[i] << "\"},\n";
  }
  strm << "  NULL};\n";
}


void MethodSpace_Generate_Data(MethodSpace* ths, TypeDef* ownr, ostream& strm) {
  strm << "static MethodDef_data TA_" << ownr->name << "_MethodDef[]={\n";

  int i;
  for(i=0; i<ths->size; i++) {
    MethodDef* md = ths->FastEl(i);
    if(!MethodSpace_Filter_Method(ths, md))
      continue;

    String str_opts = taMisc::StrArrayToChar(md->opts);
    String str_lists = taMisc::StrArrayToChar(md->lists);

    String tpfld = TypeDef_Generate_TypeFields(md->type, ownr);
    strm << "  {" << tpfld << ",\"" << md->name << "\",\"" << md->desc << "\",\""
	 << str_opts << "\",\"" << str_lists << "\",\n    ";

    strm << md->fun_overld << "," << md->fun_argc
	 << "," << md->fun_argd;

    if (md->is_virtual) strm << ",1"; else strm << ",0";
    if(md->is_static) {				// only static gets addr
      strm << ",1,(ta_void_fun)(";
      if(ownr->InheritsFrom(TA_taRegFun))
	strm <<  md->name << ")";
      else
	strm << ownr->Get_C_Name() << "::" << md->name << ")";
    }
    else
      strm << ",0,NULL";

    if((mta->gen_css && !ownr->HasOption("NO_CSS"))) {
      strm << ",cssElCFun_";
      if(ownr->InheritsFrom(TA_taRegFun))
	strm << md->name << "_stub";
      else
	strm << ownr->name << "_" << md->name << "_stub";
    }
    else {
      strm << ",NULL";
    }

    if(md->fun_argc > 0)
      strm << ",TA_" << ownr->name << "_" << md->name << "_MethArgs";
    else
      strm << ",NULL";

    strm << "},\n";
  }
  strm << "  NULL};\n";
}


void TypeDef_Init_MethodData(TypeDef* ths, ostream& strm) {
  int cnt = 0;
  int i;
  for(i=0; i<ths->methods.size; i++) {
    if(MethodSpace_Filter_Method(&(ths->methods), ths->methods.FastEl(i))) {
      cnt++;
      break;
    }
  }
  if(cnt <= 0) return;

  strm << "    tac_AddMethods(TA_" << ths->name << ","
       << "TA_" << ths->name << "_MethodDef);\n";
}


//////////////////////////////////
// 	  Property Data		//
//////////////////////////////////

/*NOTE: in maketa we only ever use the .properties to hold PropertyDef objs
*/
void TypeDef_Generate_PropertyData(TypeDef* ths, ostream& strm) {
  int cnt = 0;
  int i;
  for(i=0; i<ths->properties.size; i++) {
    if(PropertySpace_Filter_Property(&(ths->properties), 
      dynamic_cast<PropertyDef*>(ths->properties.FastEl(i)))) {
      cnt++;
      break;
    }
  }
  if(cnt <= 0) return;

  PropertySpace_Generate_Data(&(ths->properties), ths, strm);
}

bool PropertySpace_Filter_Property(PropertySpace* ths, PropertyDef* md) {
  if (!md) return false;
  if((md->owner == ths) && !md->HasOption("IGNORE"))
    return true;
  return false;
}

void PropertySpace_Generate_Data(PropertySpace* ths, TypeDef* ownr, ostream& strm) {
  String mbr_off_nm;

  int n_non_statics = 0;
  int i;
  for(i=0; i<ths->size; i++) {
    PropertyDef* md = dynamic_cast<PropertyDef*>(ths->FastEl(i));
    if(!PropertySpace_Filter_Property(ths, md))
      continue;
    if(!md->is_static)
      n_non_statics++;
  }

  strm << "static PropertyDef_data TA_" << ownr->name << "_PropertyDef[]={\n";

  for(i=0; i<ths->size; i++) {
    PropertyDef* md = dynamic_cast<PropertyDef*>(ths->FastEl(i));
    if(!PropertySpace_Filter_Property(ths, md))
      continue;

    String str_opts = taMisc::StrArrayToChar(md->opts);
    String str_lists = taMisc::StrArrayToChar(md->lists);

    String tpfld = TypeDef_Generate_TypeFields(md->type, ownr);
    strm << "  {" << tpfld << ",\"" << md->name << "\",\"" << md->desc << "\",\""
	 << str_opts << "\",\"" << str_lists << "\",\n";

    if (md->is_static) {
      strm << "1,";
    } else {
      strm << "0,";
    }
    String prop_stub;
    if (md->get_mth || md->get_mbr) {
      prop_stub = "ta_" + ownr->name + "_" + md->name + "_get";
      strm << prop_stub << ",";
    } else {
      strm << "NULL,";
    }
    if (md->set_mth || md->set_mbr) {
      prop_stub = "ta_" + ownr->name + "_" + md->name + "_set";
      strm << prop_stub;
    } else {
      strm << "NULL";
    }
    strm << "},\n";
  }
  strm << "  NULL};\n";
}

void TypeDef_Init_PropertyData(TypeDef* ths, ostream& strm) {
  int cnt = 0;
  int i;
  for(i=0; i<ths->properties.size; i++) {
    if(PropertySpace_Filter_Property(&(ths->properties), 
      dynamic_cast<PropertyDef*>(ths->properties.FastEl(i)))) {
      cnt++;
      break;
    }
  }
  if(cnt == 0) return;

  strm << "    tac_AddProperties(TA_" << ths->name << ","
       << "TA_" << ths->name << "_PropertyDef);\n";
}


//////////////////////////////////
// 	  Init Function		//
//////////////////////////////////
// (part 4 of _TA.cc file)


void MTA::TypeSpace_Generate_Init(TypeSpace* ths, ostream& strm, const String_PArray& ppfiles) {
  strm << "\n// Init Function\n";
  strm << "\n\nstatic bool ta_Init_" << ths->name << "_done = false;\n\n";

#ifdef TA_OS_WIN
  if (win_dll)
    strm << win_dll_str << " void ta_Init_" << ths->name << "() {\n";
  else
#endif
    strm << "void ta_Init_" << ths->name << "() {\n";
  strm << "  TypeDef* sbt;\n\n";

  strm << "  if(ta_Init_" << ths->name << "_done) return;\n";
  strm << "  ta_Init_" << ths->name << "_done = true;\n\n";

  // call the pre-processed files..
  int i;
  for(i=0; i<ppfiles.size; i++) {
    String tmp = taPlatform::getFileName(ppfiles.FastEl(i)).before("_TA_type.h");
    strm << "  ta_Init_" << tmp << "();\n";
  }

  strm << "\n  taMisc::in_init = true;\n\n";

  for(i=0; i<ths->size; i++) {
    TypeDef_Generate_Init(ths->FastEl(i), strm);
  }

  strm << "\n\n  taMisc::in_init = false;\n";

  if(ths->name == "ta")		// add to the ta init function..
    strm << "  taMisc::Initialize();\n"; // so the ta_Globals stuff will be there
  strm << "} \n";
}

#define PAR_ARG_COUNT	6

void TypeDef_Generate_AddParents(TypeDef* ths, char* typ_ref, ostream& strm) {
  TypeDef_Generate_AddOtherParents(ths, typ_ref, strm);
  
// (u)intptr_t requires test at runtime:
  if (ths == &TA_intptr_t) {
    strm << "    if (sizeof(intptr_t) == sizeof(int)) " << typ_ref 
      << "AddParents(&TA_int);\n    else "  << typ_ref << "AddParents(&TA_int64_t);\n";
    return;
  } else if (ths == &TA_uintptr_t) {
    strm << "    if (sizeof(uintptr_t) == sizeof(unsigned int)) " << typ_ref 
      << "AddParents(&TA_unsigned_int);\n    else "  << typ_ref << "AddParents(&TA_uint64_t);\n";
    return;
  } else
// long types just get parented and thus aliased to the correct size
  if (ths == &TA_long) {
    strm << "    if (sizeof(long) == sizeof(int)) " << typ_ref 
      << "AddParents(&TA_int);\n    else "  << typ_ref << "AddParents(&TA_int64_t);\n";
    return;
  } else if (ths == &TA_unsigned_long) {
    strm << "    if (sizeof(unsigned long) == sizeof(unsigned int)) " << typ_ref 
      << "AddParents(&TA_unsigned_int);\n    else "  << typ_ref << "AddParents(&TA_uint64_t);\n";
    return;
  }
  if(ths->parents.size == 0)
    return;

  int cnt=0;
  int i;
  // see if there are any parents, and also check for too many for us to handle!
  for (i=0; i < ths->parents.size; i++) {
    TypeDef* ptd = ths->parents.FastEl(i);
/*NOTE: following is very gruesome...
  in short, we don't want to add parent TypeDef's that don't actually exist,
  BUT, we can have them from earlier files, in which they will be in
  the spc_pre_parse list -- so we need to generate if in there
  (earlier version 3.x handled this wrong -- the use cases are fairly
   infrequent, but critical, typical example is where a base class is defined
   in an early module, and things like subclasses or pointers to that class
   only defined in a later module)
*/
    if ((ptd->owner != ths->owner) && !ptd->pre_parsed &&
	(mta->spc_builtin.FindName(ptd->name) == NULL) &&
	(mta->spc_pre_parse.FindName(ptd->name) == NULL)) {
      if (mta->verbose > 0) {
	cerr << "M!!: maketa: ("
	     << (ptd->owner != ths->owner) << ", "
	     << !ptd->pre_parsed << ", "
	     << (mta->spc_builtin.FindName(ptd->name) == NULL) 
	     << ") skipping parent '" << ptd->name << "' for: " << ths->name << endl;
	if (ptd->owner) cerr << " ptd space: " <<  ptd->owner->name << endl;
	if (ths->owner) cerr << " ths space: " <<  ths->owner->name << endl;
      }
      continue; // add parents only if on same list. (except if pre-parsed or builtin)
    } 
    cnt++;
  }
  if (cnt == 0)
    return;
  if (cnt > PAR_ARG_COUNT) {
    taMisc::Error("AddParents(): parents.size > ", String(PAR_ARG_COUNT), 
      ", increase number of args to AddParents()", "type name:", ths->name);
    return;
  }

  strm << "    " << typ_ref;
  if(ths->InheritsFormal(TA_class) && !ths->internal)	strm << "AddClassPar(";
  else					strm << "AddParents(";
  String ths_cnm = ths->Get_C_Name();
  cnt = 0;
  for (i=0; i < ths->parents.size; ++i) {
    TypeDef* ptd = ths->parents.FastEl(i);
    int idx;
    if ((ptd->owner != ths->owner) && !ptd->pre_parsed &&
	(mta->spc_builtin.FindName(ptd->name) == NULL) &&
	(mta->spc_pre_parse.FindName(ptd->name) == NULL)) {
      continue; // add parents only if on same list. (except if pre-parsed)
    } 
    if (cnt > 0)
      strm << ", ";
    if((ptd->owner != NULL) && (ptd->owner->owner != NULL))
      strm << "TA_" << ptd->owner->owner->name << ".sub_types.FindName(\"" << ptd->name << "\")";
    else
      strm << "&TA_" << ptd->name;
    if(ths->InheritsFormal(TA_class) && !ths->internal) {
      if((ths->parents.size>1) && !ths->InheritsFormal(TA_template) && !ptd->InheritsFormal(TA_template)) {
	if((mta->gen_instances || (ths->HasOption("INSTANCE")))
	   && !(ths->HasOption("NO_INSTANCE")))
	{
	  strm << ",(int)((unsigned long)((" << ptd->Get_C_Name() << "*)"
	       << "TAI_" << ths->name << ")-(unsigned long)TAI_" << ths->name
	       << ")";
	}
	else {
	  strm << ",0";
	  if((ths->parents.size > 1) && (i==0) && !ths->InheritsFromName("ios") &&
	    !ths->HasOption("NO_MEMBERS"))
	    taMisc::Error("warning: type:",ths->name,"has mult inherit but no instance",
			   "-parent offset cannot be computed!");
	}
      }
      else
	strm << ",0";
    }
    ++cnt;
  }
  strm << ");\n";
}

// this assumes never more than PAR_ARG_COUNT of either

void TypeDef_Generate_AddOtherParents(TypeDef* ths, char* typ_ref, ostream& strm) {
  int i;
  if(ths->par_formal.size > 0) {
    strm << "    " << typ_ref << "AddParFormal(";
    for(i=0; i < ths->par_formal.size; i++) {
      TypeDef* ptd = ths->par_formal.FastEl(i);
      strm << "&TA_" << ptd->name;
      if(i < ths->par_formal.size-1)
	strm << ", ";
    }
    strm << ");\n";
  }

  if(ths->par_cache.size > 0) {
    strm << "    " << typ_ref << "AddParCache(";
    for(i=0; i < ths->par_cache.size; i++) {
      TypeDef* ptd = ths->par_cache.FastEl(i);
      strm << "&TA_" << ptd->name;
      if(i < ths->par_cache.size-1)
	strm << ", ";
    }
    strm << ");\n";
  }
}

// this one is for subtypes
void TypeDef_Generate_AddAllParents(TypeDef* ths, char* typ_ref, ostream& strm) {
  TypeDef_Generate_AddOtherParents(ths, typ_ref, strm);
  if(ths->parents.size == 0)
    return;

  strm << "    " << typ_ref << "AddParents(";
  int i;
  for(i=0; i < MIN(ths->parents.size, PAR_ARG_COUNT); i++) {
    TypeDef* ptd = ths->parents.FastEl(i);
    if((ptd->owner != NULL) && (ptd->owner->owner != NULL))
      strm << "TA_" << ptd->owner->owner->name << ".sub_types.FindName(\"" << ptd->name << "\")";
    else
      strm << "&TA_" << ptd->name;
    if(i < ths->parents.size-1)
      strm << ", ";
  }
  strm << ");\n";

  if(ths->parents.size > PAR_ARG_COUNT) {
    taMisc::Error("AddParents(): parents.size > 6, increase number of args to AddParents()",
		   "type name:", ths->name);
  }
}

void SubTypeSpace_Generate_Init(TypeSpace* ths, TypeDef* ownr, ostream& strm) {
  int i;
  for(i=0; i<ths->size; i++) {
    TypeDef* sbt = ths->FastEl(i);
    if((sbt->owner != ths) || (sbt->pre_parsed) || (sbt->InheritsFormal(TA_enum)))
      continue;

    if(sbt->internal) {
      strm << "    sbt = new TypeDef(\"" << sbt->name << "\", 1, " << sbt->ptr;
      if(sbt->ref)	strm << ", 1";
      else		strm << ", 0";
      strm << ");\n";
    }
    else {
      String str_opts = taMisc::StrArrayToChar(sbt->opts);
      String str_inh_opts = taMisc::StrArrayToChar(sbt->inh_opts);
      String str_lists = taMisc::StrArrayToChar(sbt->lists);

      strm << "    sbt = new TypeDef(\"" << sbt->name << "\", \"" << sbt->desc << "\", ";
      strm << "\n\t\"" << str_inh_opts << "\", \"" << str_opts << "\", \"";
      strm << str_lists << "\", ";
      strm << "sizeof(int), (void**)0);\n";
      // todo: not putting out ptrs or ref stuff
    }

    String sbt_ref = "sbt->";
    TypeDef_Generate_AddAllParents(sbt, sbt_ref, strm);

    strm << "    TA_" << ownr->name << ".sub_types.Add(sbt);\n";
  }
}

void TypeDef_Generate_Init(TypeDef* ths, ostream& strm) {
  if((ths->pre_parsed) && !(ths->InheritsFrom(TA_taRegFun)))
    return;

  int reg_fun_level = 0;
  if(ths->InheritsFrom(TA_taRegFun)) {
    if(mta->basename == "ta")
      reg_fun_level = 1;	// add reg_fun and its methods
    else
      reg_fun_level = 2;	// just add reg_fun methods (not the typedef)
  }

  if(reg_fun_level < 2) {
    strm << "  taMisc::types.Add(&TA_" << ths->name << ");\n";

    if(!ths->internal) {
      if((mta->gen_instances || (ths->HasOption("INSTANCE")))
	 && !(ths->HasOption("NO_INSTANCE")))
	strm << "    TAI_" << ths->name << " = new "<< ths->Get_C_Name() << ";\n";
    }

    String ths_ref = "TA_" + ths->name + ".";
    TypeDef_Generate_AddParents(ths, ths_ref, strm);

    if(!ths->internal) {
      if(ths->InheritsFormal(TA_enum)) {
	TypeDef_Init_EnumData(ths, strm);
      }
      if(ths->InheritsFormal(TA_class) && !(ths->HasOption("NO_MEMBERS"))) {
	TypeDef_Init_EnumData(ths, strm);
	SubTypeSpace_Generate_Init(&(ths->sub_types), ths, strm);
	TypeDef_Init_MemberData(ths, strm);
	TypeDef_Init_MethodData(ths, strm);
	TypeDef_Init_PropertyData(ths, strm);
      }
    }
  }
  if((reg_fun_level > 0) && (mta->gen_css) && !ths->HasOption("NO_CSS")) {
    TypeDef_Init_MethodData(ths, strm);
  }
}
