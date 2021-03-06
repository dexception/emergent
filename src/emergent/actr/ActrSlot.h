// Copyright, 1995-2013, Regents of the University of Colorado,
// Carnegie Mellon University, Princeton University.
//
// This file is part of Emergent
//
//   Emergent is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   Emergent is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.

#ifndef ActrSlot_h
#define ActrSlot_h 1

// parent includes:
#include <taOBase>
#include "network_def.h"

// member includes:
#include <ActrChunk>
#include <Relation>

// declare all other types mentioned but not required to include:
class ActrSlotType; // 
class ActrProduction; //

eTypeDef_Of(ActrSlot);

class E_API ActrSlot : public taOBase {
  // ##INSTANCE ##EDIT_INLINE ##CAT_ActR ##SCOPE_ActrModel One slot in an ActR memory chunk
INHERITED(taOBase)
public:
  enum SlotFlags { // #BITS ActR slot flags
    SF_NONE             = 0, // #NO_BIT
    COND                = 0x0001, // this slot belongs in a condition of a production
    ACT                 = 0x0002, // this slot belongs in an action of a production
  };

  enum SlotValType {            // slot value type -- must be same as in ActrSlotType
    LITERAL,                    // value is a literal value (string)
    CHUNK,                      // value is a pointer to another chunk
  };

  String        name;           // #READ_ONLY #SHOW name of this slot (automatically set by chunk type)
  SlotFlags     flags;          // #READ_ONLY flags controlling behavior of this slot
  SlotValType   val_type;       // #CONDSHOW_OFF_flags:COND what type of value fills this slot
  ActrChunkRef  val_chunk;      // #CONDSHOW_ON_val_type:CHUNK the value as a pointer to another chunk
  String        val;            // #CONDSHOW_ON_val_type:LITERAL the value as a literal value -- empty or "nil" = not set -- for production conditions use =var for variable, and =var- (trailing -) for != var
  Relation::Relations   rel;    // #CONDSHOW_ON_flags:COND,ACT for production conditionals, specifies relationship to use in comparison to potential matching chunk slot values

  inline void           SetSlotFlag(SlotFlags flg)   { flags = (SlotFlags)(flags | flg); }
  // #CAT_Flags set flag state on
  inline void           ClearSlotFlag(SlotFlags flg) { flags = (SlotFlags)(flags & ~flg); }
  // #CAT_Flags clear flag state (set off)
  inline bool           HasSlotFlag(SlotFlags flg) const { return (flags & flg); }
  // #CAT_Flags check if flag is set
  inline void           SetSlotFlagState(SlotFlags flg, bool on)
  { if(on) SetSlotFlag(flg); else ClearSlotFlag(flg); }
  // #CAT_Flags set flag state according to on bool (if true, set flag, if false, clear it)
  inline void           ToggleSlotFlag(SlotFlags flg)
  { SetSlotFlagState(flg, !HasSlotFlag(flg)); }
  // #CAT_Flags toggle flag

  virtual bool          IsEmpty() const;
  // #CAT_ActR is this item empty or not?
  virtual bool          IsNil() const;
  // #CAT_ActR does this have an explicit 'nil' value set (different than empty)
  inline  bool          IsCond() const  { return HasSlotFlag(COND); }
  // #CAT_ActR is this a production condition slot?
  inline  bool          IsAct() const   { return HasSlotFlag(ACT); }
  // #CAT_ActR is this a production action slot?
  inline  bool          IsCondAct() const { return IsCond() || IsAct(); }
  // #CAT_ActR is this a production condition or action slot?
  inline  bool          CondIsVar() const { return val.startsWith('='); }
  // #CAT_ActR production condition value is a variable name
  inline  bool          CondIsNeg() const { return val.endsWith('-'); }
  // #CAT_ActR production condition value is a negation (-)
  virtual String        GetVarName() const;
  // #CAT_ActR get variable name from val 

  virtual bool          MatchesProd(ActrProduction& prod, ActrSlot* os,
                                    bool exact, bool why_not = false);
  // #CAT_ActR for production condition matching: does this match against other slot?  'this' is the LHS of production or current value of a variable -- exact = require exact value match (e.g., for matching var values), else treat as first-pass match where nil and vars match anything 
  virtual bool          MatchesMem(ActrSlot* os, bool exact, bool why_not = false);
  // #CAT_ActR for memory matching: does this match against other slot?  exact = require exact value match (e.g., for matching var values), else nil matches anything 

  virtual void          CopyValFrom(const ActrSlot& cp);
  // #CAT_ActR copy slot value from other slot -- slot val_types must match
  virtual void          CopyValFromChunk(ActrChunk* ck);
  // #CAT_ActR copy slot value as pointer to given chunk



  virtual bool          UpdateFromType(const ActrSlotType& typ);
  // initialize this slot from given slot type -- this is called by parent chunk based on chunk type -- returns true if updated

  virtual bool          SetVal(const String& str,
                               Relation::Relations rl = Relation::EQUAL);
  // #IGNORE for parsing -- set slot value and potentially relation

  bool HasName() const override { return true; }
  bool SetName(const String& nm) override { name = nm; return true; }
  String GetName() const  override { return name; }
  String& Print(String& strm, int indent = 0) const override;
  String GetDisplayName() const override;
  String GetTypeDecoKey() const override { return "ProgType"; }

  void  InitLinks() override;
  void  CutLinks() override;
  TA_BASEFUNS(ActrSlot);
protected:
  void UpdateAfterEdit_impl() override; 

private:
  SIMPLE_COPY(ActrSlot);
  void Initialize();
  void Destroy()     { CutLinks(); }
};

#endif // ActrSlot_h
