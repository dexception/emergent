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

#include "iDataTableView.h"
#include <DataTable>
#include <DataTableModel>
#include <taiTabularDataMimeFactory>
#include <taiClipData>
#include <CellRange>
#include <taiMimeSource>
#include <taProject>
#include <taiDataLink>
#include <taiMenu>

#include <taMisc>


#include <QHeaderView>



iDataTableView::iDataTableView(QWidget* parent)
:inherited(parent)
{
  setSelectionMode(QAbstractItemView::ContiguousSelection);
  gui_edit_op = false;

  // this is important for faster viewing:
#if (QT_VERSION >= 0x050000)
  verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);
#else
  verticalHeader()->setResizeMode(QHeaderView::Interactive);
#endif
}

void iDataTableView::currentChanged(const QModelIndex& current, const QModelIndex& previous) {
  inherited::currentChanged(current, previous);
  emit sig_currentChanged(current);
}

void iDataTableView::dataChanged(const QModelIndex& topLeft,
				 const QModelIndex & bottomRight
#if (QT_VERSION >= 0x050000)
				 , const QVector<int> &roles
#endif
				 )
{
#if (QT_VERSION >= 0x050000)
  inherited::dataChanged(topLeft, bottomRight, roles);
#else
  inherited::dataChanged(topLeft, bottomRight);
#endif
  emit sig_dataChanged(topLeft, bottomRight);
}

DataTable* iDataTableView::dataTable() const {
  DataTableModel* mod = qobject_cast<DataTableModel*>(model());
  if (mod) return mod->dataTable();
  else return NULL;
}

void iDataTableView::EditAction(int ea) {
  DataTable* tab = this->dataTable(); // may not exist
  if (!tab || !selectionModel()) return;
  gui_edit_op = true;
  taiTabularDataMimeFactory* fact = taiTabularDataMimeFactory::instance();
  CellRange sel(selectionModel()->selectedIndexes());
  if (ea & taiClipData::EA_SRC_OPS) {
    fact->Table_EditActionS(tab, sel, ea);
  } else {// dest op
    taiMimeSource* ms = taiMimeSource::NewFromClipboard();
    fact->Table_EditActionD(tab, sel, ms, ea);
    delete ms;
  }
  gui_edit_op = false;
}

void iDataTableView::GetEditActionsEnabled(int& ea) {
  int allowed = 0;
  int forbidden = 0;
  DataTable* tab = this->dataTable(); // may not exist
  if (tab && selectionModel()) {
    taiTabularDataMimeFactory* fact = taiTabularDataMimeFactory::instance();
    CellRange sel(selectionModel()->selectedIndexes());
    taiMimeSource* ms = taiMimeSource::NewFromClipboard();
    fact->Table_QueryEditActions(tab, sel, ms, allowed, forbidden);
    delete ms;
  }
  ea = allowed & ~forbidden;
}

void iDataTableView::RowColOp_impl(int op_code, const CellRange& sel) {
  DataTable* tab = this->dataTable(); // may not exist
  if (!tab) return;
  taProject* proj = (taProject*)tab->GetOwner(&TA_taProject);

  gui_edit_op = true;
  if (op_code & OP_ROW) {
    // must have >=1 row selected to make sense
    if ((op_code & (OP_APPEND | OP_INSERT | OP_DUPLICATE | OP_DELETE))) {
      if (sel.height() < 1) goto bail;
      if (op_code & OP_APPEND) {
        if(proj) proj->undo_mgr.SaveUndo(tab, "AddRows", tab);
        tab->AddRows(sel.height());
      }
      else if (op_code & OP_INSERT) {
        if(proj) proj->undo_mgr.SaveUndo(tab, "Insertows", tab);
        tab->InsertRows(sel.row_fr, sel.height());
      }
      else if (op_code & OP_DUPLICATE) {
        if(proj) proj->undo_mgr.SaveUndo(tab, "DuplicateRows", tab);
        tab->DuplicateRows(sel.row_fr, sel.height());
      }
      else if (op_code & OP_DELETE) {
        if(taMisc::delete_prompts || !tab->HasDataFlag(DataTable::SAVE_ROWS)) {
          if (taMisc::Choice("Are you sure you want to delete the selected rows?", "Yes", "Cancel") != 0) goto bail;
        }
        if(proj) proj->undo_mgr.SaveUndo(tab, "RemoveRows", tab);
        tab->RemoveRows(sel.row_fr, sel.height());
      }
    }
  }
  else if (op_code & OP_COL) {
    // must have >=1 col selected to make sense
    if ((op_code & (OP_APPEND | OP_INSERT | OP_DELETE))) {
      if (sel.width() < 1) goto bail;
/*note: not supporting col ops here
      if (op_code & OP_APPEND) {
      } else
      if (op_code & OP_INSERT) {
      } else */
      if (op_code & OP_DELETE) {
        if(taMisc::delete_prompts || !tab->HasDataFlag(DataTable::SAVE_ROWS)) {
          if (taMisc::Choice("Are you sure you want to delete the selected columns?", "Yes", "Cancel") != 0) goto bail;
        }
        tab->StructUpdate(true);
        if(proj) proj->undo_mgr.SaveUndo(tab, "RemoveCols", tab);
        for (int col = sel.col_to; col >= sel.col_fr; --col) {
          tab->RemoveCol(col);
        }
        tab->StructUpdate(false);
      }
    }
  }
 bail:
  gui_edit_op = false;
}

void iDataTableView::FillContextMenu_impl(ContextArea ca,
  taiMenu* menu, const CellRange& sel)
{
  inherited::FillContextMenu_impl(ca, menu, sel);
  DataTable* tab = this->dataTable(); // may not exist
  if (!tab) return;
  // only do col items if one selected only
  if ((ca == CA_COL_HDR) && (sel.width() == 1)) {
    DataCol* col = tab->GetColData(sel.col_fr, true);
    if (col) {
      taiDataLink* link = (taiDataLink*)col->GetDataLink();
      if (link) link->FillContextMenu(menu);
    }
  }
}
