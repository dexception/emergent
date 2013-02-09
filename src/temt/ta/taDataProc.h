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

#ifndef taDataProc_h
#define taDataProc_h 1

// parent includes:
#include <taNBase>

// member includes:
#include <DataCol>

// declare all other types mentioned but not required to include:
class DataTable; // 
class DataOpList; // 
class DataSortSpec; // 
class DataGroupSpec; // 
class DataSelectSpec; // 
class DataJoinSpec; // 


taTypeDef_Of(taDataProc);

class TA_API taDataProc : public taNBase {
  // #STEM_BASE ##CAT_Data collection of commonly-used datatable processing operations (database-style)
INHERITED(taNBase)
public:
  
  static bool	GetDest(DataTable*& dest, const DataTable* src, const String& suffix,
			bool& in_place_req);
  // #IGNORE helper function: if dest is NULL, a new one is created in proj.data.AnalysisData, with name from source + suffix -- if dest == src, then in_place_req is returned as true, and dest is created as a 'new' object (on the heap) and ref-counted once -- should then be copied back to src at the end of the process, and then deleted

  ///////////////////////////////////////////////////////////////////
  // manipulating lists of columns

  static bool	GetCommonCols(DataTable* dest, DataTable* src, DataOpList* dest_cols, DataOpList* src_cols);
  // #CAT_ColumnLists find common columns between dest and src by name, cell_size, and type if matrix 
  static bool	GetColIntersection(DataOpList* trg_cols, DataOpList* ref_cols);
  // #CAT_ColumnLists get the intersection (common columns) between two lists of columns: trg_cols and ref_cols (based only on name) -- i.e., remove any columns in trg_cols that are not in the ref_cols list

  ///////////////////////////////////////////////////////////////////
  // basic copying and concatenating

  static bool	CopyData(DataTable* dest, DataTable* src);
  // #CAT_Copy #MENU_BUTTON #MENU_ON_Copy #NULL_OK_0 #NULL_TEXT_0_NewDataTable just copy the data from source to destination, completely removing any existing data in the destination, but not changing anything else about the dest (e.g., its name) (if dest is NULL, a new one is created in proj.data.AnalysisData)
  static bool	CopyCommonColsRow_impl(DataTable* dest, DataTable* src, DataOpList* dest_cols, DataOpList* src_cols, int dest_row, int src_row);
  // #CAT_Copy copy one row of data from src to dest for the lists of common columns generated by GetCommonCols
  static bool	CopyCommonColsRow(DataTable* dest, DataTable* src, int dest_row, int src_row);
  // #CAT_Copy copy one row of data from src to dest for the cols that are in common between the two tables (by name)
  static bool	CopyCommonColData(DataTable* dest, DataTable* src);
  // #CAT_Copy #MENU_BUTTON copy data from src to dest for all columns that are common between the two (adds to end of dest rows)
  static bool	AppendRows(DataTable* dest, DataTable* src);
  // #CAT_Copy #MENU_BUTTON append rows of src to the end of dest (structure must be the same -- more efficient than CopyCommonColData when this is true)
  static bool	ReplicateRows(DataTable* dest, DataTable* src, int n_repl);
  // #CAT_Copy #MENU_BUTTON #NULL_OK_0 #NULL_TEXT_0_NewDataTable replicate each row of src n_repl times in dest -- dest is completely overwritten (if dest is NULL, a new one is created in proj.data.AnalysisData) -- dest can also be same as src
  static bool	ConcatRows(DataTable* dest, DataTable* src_a, DataTable* src_b, DataTable* src_c=NULL,
			   DataTable* src_d=NULL, DataTable* src_e=NULL, DataTable* src_f=NULL);
  // #CAT_Copy #MENU_BUTTON #NULL_OK_0 #NULL_TEXT_0_NewDataTable concatenate rows of data from all the source data tables into the destination, which is completely overwritten with the new data.  (if dest is NULL, a new one is created in proj.data.AnalysisData).  just a sequence of calls to CopyCommonColData
  static bool	AllDataToOne2DCell(DataTable* dest, DataTable* src, ValType val_type = VT_FLOAT, 
				   const String& col_nm_contains="", const String& dest_col_nm = "One2dCell");
  // #CAT_Copy #MENU_BUTTON #NULL_OK_0 #NULL_TEXT_0_NewDataTable convert all data of given val_type from source (src) data table to a single 2-dimensional Matrix cell in dest -- can be useful as a predecessor to various data analysis operations etc -- any combination of matrix or scalar cols is ok -- if col_nm_contains is provided, column names must contain this string to be included -- resulting geometry depends on configuration -- if multiple columns are involved, then all column data stretched out linearly is the x (inner) dimension and y is rows; if one matrix column is selected, then x is the first dimension and y is all the other dimensions multiplied out, including the rows
  static bool Slice2D(DataTable* dest, DataTable* src, int src_row = 0, String src_col_nm = "", int dest_row = -1,
		      String dest_col_nm = "", int d0_start = 0, int d0_end = -1, int d1_start = 0, int d1_end = -1);
  // #CAT_Copy #MENU_BUTTON #NULL_OK_0 #NULL_TEXT_0_NewDataTable See http://en.wikipedia.org/wiki/Array_slicing. Copies a 2d slice out of the first 2 dimensions of the src_row of the src matrix column into the dest_row of the dest matrix column. If src_row = 0 and src_col_nm = "" it will slice out of the matrix in the first src row and col (default). If dest = NULL a new table will be created in AnalysisData named src.name + "_" + SliceData and if it exists already a new row will be added to it and written to. If dest_col_nm = "" the name SliceData will be used (default) and if dest_row = -1 a new row will be created and written to (default). If d0_end or d1_end = -1 they will be assigned to the size of that matrix dimension (default). By default the entire first 2 dimensions are are sliced out of src into dest.

  ///////////////////////////////////////////////////////////////////
  // reordering functions

  static bool	Sort(DataTable* dest, DataTable* src, DataSortSpec* spec);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Order #MENU_BUTTON #MENU_ON_Order sort data from src into dest according to sorting specifications in spec; if src == dest, then it is sorted in-place, otherwise, dest is completely overwritten, and if dest is NULL, a new one is created in proj.data.AnalysisData
  static bool 	SortInPlace(DataTable* dt, DataSortSpec* spec);
  // #CAT_Order #MENU_BUTTON #MENU_ON_Order sort given data table in place (modifies data table) according to sorting specifications in spec

  static int 	Sort_Compare(DataTable* dt_a, int row_a, DataTable* dt_b, int row_b,
			     DataSortSpec* spec);
  // #IGNORE helper function for sorting: compare values -1 = a is < b; 1 = a > b; 0 = a == b
  static bool 	Sort_impl(DataTable* dt, DataSortSpec* spec);
  // #IGNORE actually perform sort on data table using specs

  static bool	Permute(DataTable* dest, DataTable* src);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Order #MENU_BUTTON permute (randomly reorder) the rows of the data table -- note that it is typically much more efficient to just use a permuted index to access the data rather than physically permuting the items -- if src == dest, then a temp dest is used and results are copied back to src (i.e., in-place operation)

  static bool	Group(DataTable* dest, DataTable* src, DataGroupSpec* spec);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Order #MENU_BUTTON group data from src into dest according to grouping specifications in spec (if dest is NULL, a new one is created in proj.data.AnalysisData) -- if src == dest, then a temp dest is used and results are copied back to src (i.e., in-place operation)

  static bool	Group_nogp(DataTable* dest, DataTable* src, DataGroupSpec* spec);
  // #IGNORE helper function to do grouping when there are no GROUP items
  static bool	Group_gp(DataTable* dest, DataTable* src, DataGroupSpec* spec,
			 DataSortSpec* sort_spec);
  // #IGNORE helper function to do grouping when there are GROUP items, as spec'd in sort_spec

  static bool	TransposeColsToRows(DataTable* dest, DataTable* src,
	    const Variant& data_col_st, int n_cols=-1, const Variant& col_names_col=-1);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Order #MENU_BUTTON transpose column(s) of data from the source table into row(s) of data in the destination data table -- data_col_st indicates the starting column (specify either name or index), n_cols = number of columns after that (-1 = all columns), col_names_col specifies the column in the source table that contains names for the resulting columns in the destination table (-1 or empty string = no specified names -- call them row_0, etc)

  static bool	TransposeRowsToCols(DataTable* dest, DataTable* src, int st_row=0, int n_rows=-1,
				    DataCol::ValType val_type = DataCol::VT_FLOAT);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Order #MENU_BUTTON transpose row(s) of data from the source table into column(s) of the destination table -- can specify a range of rows -- n_rows=-1 means all rows -- the first column of the dest table will be the names of the columns in the source table, followed by columns of data, in order, one for each row specified.  The type of data colunns to create is specified by the val_type field -- should be sufficient to handle all fo the column data

  ///////////////////////////////////////////////////////////////////
  // row-wise functions: selecting/splitting

  static bool	SelectRows(DataTable* dest, DataTable* src, DataSelectSpec* spec);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Select #MENU_BUTTON #MENU_ON_Select select rows of data from src into dest according to selection specifications in spec (all columns are copied) (if dest is NULL, a new one is created in proj.data.AnalysisData) -- if src == dest, then a temp dest is used and results are copied back to src (i.e., in-place operation)

  static bool	SplitRows(DataTable* dest_a, DataTable* dest_b, DataTable* src,
			  DataSelectSpec* spec);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Select #MENU_BUTTON splits the source datatable rows into two sets, those that match the selection specifications go into dest_a, else dest_b (if dest are NULL, new ones are created in proj.data.AnalysisData)

  static bool	SplitRowsN(DataTable* src, DataTable* dest_1, int n1, DataTable* dest_2, int n2=-1,
			 DataTable* dest_3=NULL, int n3=0, DataTable* dest_4=NULL, int n4=0,
			 DataTable* dest_5=NULL, int n5=0, DataTable* dest_6=NULL, int n6=0);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Select #MENU_BUTTON splits the source datatable rows into distinct non-overlapping sets, with specific number of elements (sequentially) in each (-1 = the remainder, can appear *only once* anywhere) (new dest datatables are created if NULL)
  static bool	SplitRowsNPermuted(DataTable* src, DataTable* dest_1, int n1, DataTable* dest_2, int n2=-1,
				   DataTable* dest_3=NULL, int n3=0, DataTable* dest_4=NULL, int n4=0,
				   DataTable* dest_5=NULL, int n5=0, DataTable* dest_6=NULL, int n6=0);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Select #MENU_BUTTON splits the source datatable rows into distinct non-overlapping sets, with specific number of elements (order permuted efficiently via an index list) in each (-1 = the remainder, can appear *only once* anywhere) (new dest datatables are created if NULL).  this is good for creating random training/testing subsets

  ///////////////////////////////////////////////////////////////////
  // column-wise functions: selecting, joining

  static bool	SelectCols(DataTable* dest, DataTable* src, DataOpList* spec);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Columns #MENU_BUTTON #MENU_ON_Columns select columns of data from src into dest according to list of columnns in spec (all rows are copied) -- if src == dest, then a temp dest is used and results are copied back to src (i.e., in-place operation)

  static bool	Join(DataTable* dest, DataTable* src_a, DataTable* src_b, DataJoinSpec* spec);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Columns #MENU_BUTTON joins two datatables (src_a and src_b) into dest datatable.  tables are internally sorted first according to the join column.  all matching row values from both tables are included in the result.  for the left join, all rows of src_a are included even if src_b does not match, and vice-versa for the right join.  inner only includes the matches.  all columns are included (without repeating the common column)

  static bool	ConcatCols(DataTable* dest, DataTable* src_a, DataTable* src_b);
  // #NULL_OK_0 #NULL_TEXT_0_NewDataTable #CAT_Columns #MENU_BUTTON concatenate two datatables into one datatable by adding both sets of columns together -- if dest == src_a then the results go into the src_a table directly, and no additional table is created -- if the number of rows is unequal, then result has the maximum of the two sources, with blank padding for the shorter of the two.

  override String 	GetTypeDecoKey() const { return "DataTable"; }
  TA_BASEFUNS(taDataProc);
private:
  NOCOPY(taDataProc)
  void Initialize() { };
  void Destroy() { };
};

#endif // taDataProc_h
