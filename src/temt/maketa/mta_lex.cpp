// Copyright, 1995-2005, Regents of the University of Colorado,
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


// the lexical analyzer

#include "maketa.h"
#include "ta/ta_platform.h"
#include <ctype.h>

/* note: we handle the following line end types:
  Windows     cr lf
  MacOS	      cr
  Unix	      lf

  NOTE: crlf is treated as a single character -- last_char will be the char
    before the cr, not cr
*/
int MTA::Getc() {
  char c = fh.get();
  //  strm_pos++;
  strm_pos = strm_pos + streampos(1);
  // now, detect for CRLF and skip to the lf
  if(verbose > 4) // && (state != MTA::Find_Item))
    cerr << "C!!: => " << line << " :\t" << c << "\n";
  if (c == '\r') {
    if (Peekc() == '\n')
      c = fh.get();
      strm_pos = strm_pos + streampos(1);
  }
  if ((c == '\n') || (c == '\r')) {
    MTA::LastLn[col] = '\0';	// always terminate
    if(verbose > 3) // && (state != MTA::Find_Item))
      cerr << "I!!: => " << line << " :\t" << MTA::LastLn << "\n";
    line++;
    st_line_pos = strm_pos;
    col = 0;
  } else {
    if(col >= 8191)
      cout << "W!!: Warning, line length exceeded in line: " << line << "\n";
    else {
      MTA::LastLn[col++] = c;
      MTA::LastLn[col] = '\0';	// always terminate
    }
  }
  return c;
}

void MTA::unGetc(int c) {
  col--;
  //  strm_pos--;
  strm_pos = strm_pos - streampos(1);
  fh.putback(c);
}

int MTA::skipwhite() {
  int c;
  while (((c=Getc()) == ' ') || (c == '\t') || (c == '\n') || (c == '\r'));
  return c;
}

int MTA::skipwhite_peek() {
  int c;
  while (((c=Peekc()) == ' ') || (c == '\t') || (c == '\n') || (c == '\r')) Getc();
  return c;
}

int MTA::skipwhite_nocr() {
  int c;
  while (((c=Getc()) == ' ') || (c == '\t'));
  return c;
}

int MTA::skipline() {
  int c;
  char lastc = '\0';
  while ((c=Getc()) != EOF) {
     if ( ((c == '\n') || ((c == '\r'))) && lastc != '\\') break;
     lastc = c;
  }
  return c;
}

int MTA::skiptocommarb() {
  int c;
  while (((c=Peekc()) != EOF) && !((c == ',') || (c == '}'))) Getc();
  return c;
}


int MTA::readword(int c) {
  LexBuf = "";

  LexBuf += (char)c;
  while ((c=Peekc()) != EOF && (isalnum(c) || (c == '_'))) { LexBuf += (char)c; Getc(); } 
  if(c == EOF)
    return EOF;
  if ((c == '\n') || (c == '\r'))
    Getc();
  return c;
}  

int MTA::readfilename(int c) {
  // if c is a quote, then read to end quote, else assume it is unquoted fname
  // and read until next ws 
  // note that quoted fnames can contain embedded spaces
  LexBuf = "";
  if (c == '\"') { // since c was the opening ", we don't add it to lexbuf
    // note: ws inside will be Very Bad, but we terminate regardless
    while (((c=Getc()) != EOF) && (c != '\t') && (c != '\n') && (c != '\r')) {
      if (c == '\"') { // found terminator, so skip it and break out
        c = Getc();
        break;
      }
      LexBuf += (char)c;
    } 
  } else {
    LexBuf += (char)c; // gotta include the first guy!
    while (((c=Getc()) != EOF) && (c != ' ') && (c != '\t') && (c != '\n') && (c != '\r')) {
      LexBuf += (char)c;
    } 
  }
  if(c == EOF)
    return EOF;
  return c;
}  

int MTA::follow(int expect, int ifyes, int ifno) {
  int c = Peekc();
  
  if(c == expect) {
    Getc();
    return ifyes;
  }
  return ifno;
}

// static int follow2(int expect1, int if1, int expect2, int if2, int ifno) {
//   int c = Getc();
//   
//   if(c == expect1) return if1;
//   if(c == expect2) return if2;
//   unGetc(c);
//   return ifno;
// }
// 
// static int follow3(int expect1, int if1, int expect2, int if2, 
// 		   int expect3, int if3, int ifno) {
//   int c = Getc();
//   
//   if(c == expect1) return if1;
//   if(c == expect2) return if2;
//   if(c == expect3) return if3;
//   unGetc(c);
//   return ifno;
// }

int yylex()
{
  return mta->lex();
}

int MTA::lex() {
  int c, nxt, prv;
  TypeDef *itm;
  int bdepth = 0;

  do {
    st_line = line;		// save for burping, if necc.
    st_col = col;
    st_pos = strm_pos;

    c = skipwhite();

    if(c == EOF)
      return YYRet_Exit;
    
    if(c == '/') {
      c = Peekc();
      if((c == '/') || (c == '*')) { 	// comment
	Getc();
	ComBuf = "";
	prv = c;
	ComBuf += (char)' ';
	if(prv == '/') {
	  c = skipwhite_nocr();
	  if((c != EOF) && ((c != '\n') && (c != '\r')))
	    ComBuf += (char)c;
	  while((c != EOF) && ((c != '\n') && (c != '\r'))) {
	    if((c == ' ') || (c == '\t')) {
	      c = skipwhite_nocr();
	    }
	    else
	      c = Getc();
	    if((c != EOF) && ((c != '\n') && (c != '\r')))
	      ComBuf += (char)c;
	  }
	}
	else {
	  c = skipwhite();
	  if(c != EOF)
	    ComBuf += (char)c;
	  while(c != EOF) {
	    if((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r')) {
	      c = skipwhite();
	    }
	    else
	      c = Getc();
	    if(c == '*') {
	      if(Peekc() == '/') {
		c = Getc();
		break;
	      }
	    }
	    if(c != EOF) {
	      if ((c == '\n') || (c == '\r') || (c == '\t'))
		ComBuf += ' ';
	      else
		ComBuf += (char)c;
	    }
	  }
	}

	if(state == Find_Item) {
	  String tmp = ComBuf;
	  if(tmp.contains("#REG_FUN")) { // function follows comment
	    state = Parse_fundef;
	    return REGFUN;
	  }
	}
	if((state == Find_Item) || (state == Parse_infun)
	   || (state == Skip_File))
	  continue;
	yylval.chr = ComBuf;
	return COMMENT;
      }
      else {
	c = '/';
      }
    }

    if(c == '#') {
      c = skipwhite();
      c = readword(c);
      if((LexBuf == "pragma") || (LexBuf == "ident")) {
	c = skipline();
	continue;
      }
      // note: following for MS VC++
      // VC++ outputs "#line xxx ..." where xxx is lineno
      // gcc simply outputs "# xxx ..." where xxx is lineno
      if ((LexBuf == "line") ) {
        c = skipwhite();
	c = readword(c);
      }

      line = (int)atol(LexBuf); // the line number
      if ((c != '\n') && (c != '\r'))		// readword did not Getc() this char
	c = skipwhite_nocr();
      if ((c == '\n') || (c == '\r'))		// ignore #<lineno><RETURN> directives
	continue;
      else if(c != '\"') {	// "
	cerr << "E!!: Directive Not Recognized: " << LexBuf << (char)c << "\n";
	continue;
      }
      c = readfilename(c);
      // ignore gcc stuff like "<built-in>"
      if (LexBuf.startsWith('<')) {
        if ((c != '\n') && (c != '\r'))
          skipline();
        continue;
      }
      cur_fname = taPlatform::lexCanonical(LexBuf);
      if ((c != '\n') && (c != '\r'))
	skipline();		// might be training stuff to skip here
      if (state == Skip_File) // if previously skipping, default is to find
	state = Find_Item;

      String cur_fname_only = taPlatform::getFileName(cur_fname);

      // don't include the base_TA.h file for this one
      if((cur_fname.contains(ta_type_h)) ||
	 (cur_fname.contains(ta_inst_h)))
      {
	if(verbose > 1)
	  cout << "\nSkipping: " << cur_fname << " because TA_xxx.h\n";
	state = Skip_File;
	continue;
      }
      // don't include any other files that will be included later...
      int i;
      for(i=0; i<headv.size; i++) {
	if((fname != headv.FastEl(i)) &&
	   (cur_fname_only == head_fn_only.FastEl(i))) {
	  if(verbose > 1)
	    cout << "\nSkipping: " << cur_fname << " because of: "
		 << head_fn_only.FastEl(i) << "\n";
	  state = Skip_File;
	  break;
	}
      }
      if((state != Skip_File) && (included.FindEl(cur_fname) >= 0)) {
	if(verbose > 1)
	  cout << "\nSkipping: " << cur_fname << " because prev included\n";
	state = Skip_File;
      }
//TEMP
if (cur_fname_only == "ta_qtclipdata.h") {
  int i = 0;
  ++i;
}
// /TEMP
      String fname_only = taPlatform::getFileName(fname);

      if(cur_fname_only != fname_only) {
	spc = &(spc_other); // add to other space when not in cur space
	if(state != Skip_File) {
	  tmp_include.AddUnique(cur_fname); // note: already lexcanonicalized
	}
      }
      else {
	spc = &(spc_target); // add to target space
      }

      if(verbose > 1) {
	cout << "file: " << cur_fname << "\n";
      }
      continue;			// now actually parse something new..
    }

    if(state == Skip_File) {
      continue;
    }

    if(state == Find_Item) {
      if(readword(c) == EOF)
	return YYRet_Exit;
      if((itm = spc_keywords.FindName(LexBuf)) != NULL) {
	yylval.typ = itm;
	switch (itm->idx) {
	case TYPEDEF:
	  state = Parse_typedef;
	  return itm->idx;
	case ENUM:
	  state = Parse_enum;
	  return itm->idx;
	case CLASS:
	  state = Parse_class;
	  return itm->idx;
	case STRUCT:
	  if(class_only)
	    break;
	  state = Parse_class;
	  return itm->idx;
	case UNION:
	  if(class_only)
	    break;
	  state = Parse_class;
	  return itm->idx;
	case TEMPLATE:
	  state = Parse_class;
	  return itm->idx;
	case TA_TYPEDEF:
	  {
	    String ta_decl;
	    if(last_word == "extern") {
	      readword(skipwhite());
	      ta_decl = LexBuf;
	      if(ta_decl.contains("TA_")) {
		ta_decl = ta_decl.after("TA_");
		itm = new TypeDef(ta_decl, true);
		itm->pre_parsed = true;
		yylval.typ = spc_pre_parse.AddUniqNameOld(itm);
		pre_parse_inits.AddUnique(cur_fname);
		return TA_TYPEDEF;
	      }
	    }
	  }
	}
      }
      last_word = LexBuf;
      continue;
    }

    // eliminate any FUNCTION defns inline
    if(state == Parse_infun) {
      if(c == '{')
	bdepth++;
      else if(c == '}') {
	bdepth--;
	if(bdepth == 0) {
	  state = Parse_inclass;
	  c = skipwhite_peek();	// only peek at nonwhite stuff
	  if(c == ';')		// and get trailing semi
	    Getc();
	  return FUNCTION;
	}
      }
      else if(readword(c) == EOF) {
        yyerror("premature end of file");
	return YYRet_Exit;
      }
      continue;
    }

    if((state == Parse_inclass) && (c == '{')) {
      bdepth = 1;
      state = Parse_infun;
      continue;
    }
    
    // get rid of things after equals signs inside of classes (always return EQUALS)
    if(c == '=') {
      if((state != Parse_enum)) {
	bdepth = 0;
	EqualsBuf = "";
	do {
	  c = Peekc();
	  if(((bdepth == 0) && ((c == ',') || (c == ')') || (c == '>'))) || (c == ';')) {
	    yylval.chr = EqualsBuf;
	    return EQUALS;
	  }
	  Getc();
	  if ((c != '"') && (c != '\n') && (c != '\r'))
	    EqualsBuf += (char)c;
	  if(c == ')')	bdepth--;
	  if(c == '(')	bdepth++;
	} while (c != EOF);
	return YYRet_Exit;
      }
      yylval.chr = "";
      return EQUALS;
    }

    if((state == Parse_inclass) && (c == '[')) {
      bdepth = 0;
      do {
	c = Getc();
	if(((bdepth == 0) && (c == ']')) || (c == ';'))
	  return ARRAY;
	if(c == ')')	bdepth--;
        if(c == '(')	bdepth++;
	if(c == ']')	bdepth--;
        if(c == '[')	bdepth++;
      } while (c != EOF);
      return YYRet_Exit;
    }

    if((state == Parse_inclass) && (thisname) && 
	(constcoln) && (c == '(')) {
      bdepth = 0;
      do {
	c = Getc();
	if(((bdepth == 0) && (c == ')')) || (c == ';'))
	  return FUNCALL;
	if(c == ')')	bdepth--;
        if(c == '(')	bdepth++;
      } while (c != EOF);
      return YYRet_Exit;
    }

    if(c == '.') {
      nxt = Peekc();
      if(!(isdigit(nxt)))
	return '.';		// pointsat
    }

    if((c == '.') || isdigit(c) || (c == '-')) {	// number
      int iv, gotreal = 0;
      LexBuf = (char)c;
      if(c == '.') gotreal = 1;
      
      while(((c=Peekc()) != EOF) &&
	    ((c == '.') || isxdigit(c) || (c == 'x') || (c == 'e') ||
	     (c == 'X') || (c == 'E')))	{
	LexBuf += (char)c; if(c == '.') gotreal = 1;
	Getc();
      } 

      iv = (int)strtol(LexBuf, NULL, 0);	// only care about ints
      yylval.rval = iv;
      return NUMBER;
    }
    
    if(isalpha(c) || (c == '_')) {
      LexBuf = (char)c;

      while((c=Peekc()) != EOF && (isalnum(c) || (c == '_'))) {
	LexBuf += (char)c;
	Getc();
      } 

      int lx_tok;
      yylval.typ = FindName(LexBuf, lx_tok);
      
      if(yylval.typ == NULL) {
	yylval.chr = LexBuf;
	return NAME;
      }
      if(lx_tok == OPERATOR) {
	while((c=Peekc()) != '(') Getc(); // skip to the parens
      }
      if(lx_tok == ENUM) {
	state = Parse_enum;
      }
      return lx_tok;
    }

    if(c == ':') return follow(':', SCOPER, ':');
    return c;

  } while(1);

  return 0;
}
