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

#include "iMatrixTableModel.h"
#include <DataCol>
#include <taMatrix>
#include <String_Matrix>
#include <MatrixIndex>
#include <CellRange>
#include <taProject>
#include <ColorScale>

#include <SigLinkSignal>
#include <taMisc>
#include <taiMisc>

#include <QColor>
#include <QBrush>
#include <QMimeData>


iMatrixTableModel::iMatrixTableModel(taMatrix* mat_) 
:inherited(NULL)
{
  col_idx = -1;
  m_mat = mat_;
  m_mat_col = NULL;
  m_pat_4d = false;
  m_dim_names = NULL;
  m_view_layout = taMisc::TOP_ZERO;
}

iMatrixTableModel::~iMatrixTableModel() {
  // note: following shouldn't really execute since mat manages our lifetime
  if (m_mat) {
    m_mat->RemoveSigClient(this);
    m_mat = NULL;
  }
  m_dim_names = NULL;
  m_mat_col = NULL;
}

void iMatrixTableModel::setDataCol(DataCol* dc) {
  m_mat_col = dc;
}

int iMatrixTableModel::columnCount(const QModelIndex& parent) const {
  if (!m_mat) return 0;
  return m_mat->geom.colCount(pat4D());
}

QVariant iMatrixTableModel::data(const QModelIndex& index, int role) const {
  if (!m_mat) return QVariant();
  switch (role) {
  case Qt::DisplayRole: 
  case Qt::EditRole:
    return static_cast<const char *>(m_mat->SafeElAsStr_Flat(matIndex(index)));
//Qt::DecorationRole
//Qt::ToolTipRole
//Qt::StatusTipRole
//Qt::WhatsThisRole
//Qt::SizeHintRole -- QSize
//Qt::FontRole--  QFont: font for the text
  case Qt::TextAlignmentRole:
    return m_mat->defAlignment();
  case Qt::BackgroundRole: {
    //-- QColor
 /* note: only used when !(option.showDecorationSelected && (option.state
    & QStyle::State_Selected)) */
    if (!(flags(index) & Qt::ItemIsEditable))
      return QBrush(Qt::lightGray);
    ColorScale* cs = m_mat->GetColorScale();
    if(cs) {
      float val = m_mat->SafeElAsFloat_Flat(matIndex(index));
      cs->UpdateMinMax(val); // lame attempt to keep in range -- probably ok tho
      float sc_val;
      iColor ic = cs->GetColor(val, sc_val);
      return (QBrush)(QColor)ic;
    }
    break;
  }
/*Qt::TextColorRole
  QColor: color of text
Qt::CheckStateRole*/
  default: break;
  }
  return QVariant();
}

void iMatrixTableModel::SigLinkDestroying(taSigLink* dl) {
  m_mat = NULL;
}

void iMatrixTableModel::SigLinkRecv(taSigLink* dl, int sls,
  void* op1, void* op2)
{
  if (notifying) return;
  if ((sls <= SLS_ITEM_UPDATED_ND) || // data itself updated
    (sls == SLS_STRUCT_UPDATE_END) ||  // for col insert/deletes
    (sls == SLS_DATA_UPDATE_END)) // for row insert/deletes
  { 
    emit_layoutChanged();
  }
}


void iMatrixTableModel::emit_dataChanged(const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
  if (!m_mat) return;
  emit dataChanged(topLeft, bottomRight);
  if (col_idx >= 0)
    emit matSigEmit(col_idx);
}

void iMatrixTableModel::emit_dataChanged(int row_fr, int col_fr, int row_to, int col_to) {
  if (!m_mat) return;
  // lookup actual end values when we are called with sentinels
  if (row_to < 0) row_to = rowCount() - 1;
  if (col_to < 0) col_to = columnCount() - 1;  
  
  emit_dataChanged(createIndex(row_fr, col_fr), createIndex(row_to, col_to));
}

void iMatrixTableModel::emit_layoutChanged() {
  if (!m_mat) return;
  emit layoutChanged();
}

Qt::ItemFlags iMatrixTableModel::flags(const QModelIndex& index) const {
  if (!m_mat) return 0;
  //TODO: maybe need to qualify!, plus drag-drop handling, etc.
  Qt::ItemFlags rval = 0;
  
  if (ValidateIndex(index)) {
    rval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  }
  // editability is a property of the whole mat
  if (!m_mat->isGuiReadOnly()) {
    rval |= Qt::ItemIsEditable;
  }
  return rval; 
}
/*
  index = i0 + ((i1*d0) + i2)*d1 etc.
to find i1,i2... from index:
1. divide by d0 gives rowframe -- remainder is i1
2. divide by d1 gives 2d-frame
*/
QVariant iMatrixTableModel::headerData(int section, 
  Qt::Orientation orientation, int role) const
{
  if (!m_mat) return QVariant();
  if (role != Qt::DisplayRole)
    return QVariant();
  // get an effective pat4D guaranteed true only if applicable
  bool pat_4d = (pat4D() && (m_mat->dims() >= 4));
  if (orientation == Qt::Horizontal) {
    if (pat_4d) {
      // need to break the flat col down 
      div_t qr = div(section, m_mat->dim(0));
      // d2:d0
      if(m_dim_names) {
	QString d2nm = m_dim_names->SafeEl_Flat(2);
	QString d0nm = m_dim_names->SafeEl_Flat(0);
	if(d2nm.isEmpty()) d2nm = "g";
	if(d0nm.isEmpty()) d0nm = "x";
	return QString("%1%2:%3%4").arg(d2nm).arg(qr.quot).arg(d0nm).arg(qr.rem);
      }
      else {
	return QString("g%1:x%2").arg(qr.quot).arg(qr.rem);
      }
    }
    else {
      if(m_dim_names) {
	QString dnm = m_dim_names->SafeEl_Flat(0);
	return dnm + QString::number(section);
      }
      else {
	return QString::number(section);
      }
    }
  }
  else {// in form: d1[[:d2]:d3]
    if (m_mat->dims() < 2) {
      return ""; // no dim is applicable, only in cols
    };
    // use same formula as matIndex()
    int row_flat_idx =  m_mat->geom.IndexFmDims2D(0, section, pat_4d, matView());
    MatrixIndex coords;
    m_mat->geom.DimsFmIndex(row_flat_idx, coords);
    if (m_mat->dims() == 2) {
      if(m_dim_names) {
	QString dnm = m_dim_names->SafeEl_Flat(1);
	return dnm + QString::number(coords[1]);
      }
      else {
	return QString::number(coords[1]);
      }
    }
    else {
      //const int cc = m_mat->colCount(pat_4d);
      //int row_flat_idx = matIndex(index); //eff_row * cc;
      QString rval;
      int j;
      if (pat_4d) {
        // d3:d1...
	if(m_dim_names) {
	  QString d3nm = m_dim_names->SafeEl_Flat(3);
	  QString d1nm = m_dim_names->SafeEl_Flat(1);
	  if(d3nm.isEmpty()) d3nm = "g";
	  if(d1nm.isEmpty()) d1nm = "x";
	  rval = QString("%1%2:%3%4").arg(d3nm).arg(coords[3]).arg(d1nm).arg(coords[1]);
	}
	else {
	  rval = QString("g%1:y%2").arg(coords[3]).arg(coords[1]);
	}
        j = 4;
      }
      else {
	if(m_dim_names) {
	  QString dnm = m_dim_names->SafeEl_Flat(1);
	  rval = dnm + QString("%1").arg(coords[1]);
	}
	else {
	  rval = QString("%1").arg(coords[1]);
	}
        j = 2;
      }
      for (int i = j; i < coords.dims(); ++i) {
	if(m_dim_names) {
	  QString dnm = m_dim_names->SafeEl_Flat(i);
	  rval = dnm + QString("%1:").arg(coords[i]) + rval;
	}
	else {
	  rval = QString("%1:").arg(coords[i]) + rval;
	}
      }
      return rval;
    }
  }
}

int iMatrixTableModel::matIndex(const QModelIndex& idx) const {
  //note: we dimensionally reduce all dims >1 to 1
  return (m_mat) ? m_mat->geom.IndexFmDims2D(idx.column(), idx.row(), pat4D(), matView())
    : 0;
}

QMimeData* iMatrixTableModel::mimeData (const QModelIndexList& indexes) const {
  if (!m_mat) return NULL;
  CellRange cr(indexes);
  String str = mat()->FlatRangeToTSV(cr);
  QMimeData* rval = new QMimeData;
  rval->setText(str);
  return rval;
}

QStringList iMatrixTableModel::mimeTypes () const {
  QStringList types;
  types << "text/plain";
  return types;
}

int iMatrixTableModel::rowCount(const QModelIndex& parent) const {
  return (m_mat) ? m_mat->rowCount(pat4D()) : 0;
  //note: for visual stuff, there is always at least one row
}

bool iMatrixTableModel::setData(const QModelIndex& index, const QVariant & value, int role) {
  if (!m_mat) return false;
  if (index.isValid() && role == Qt::EditRole) {
    if(m_mat_col) {
      taProject* proj = (taProject*)m_mat_col->GetOwner(&TA_taProject);
      // save undo state!
      if(proj) {
	proj->undo_mgr.SaveUndo(m_mat_col, "DataCol Matrix Cell Edit", m_mat_col);
      }
    }
    m_mat->SetFmStr_Flat(value.toString(), matIndex(index));
    ++notifying;
    emit_dataChanged(index, index);
    m_mat->SigEmit(SLS_ITEM_UPDATED); // propagates up slice chain, but typically not used, i.e in particular, doesn't affect DataCol
    --notifying;
    return true;
  }
  return false;
}

void iMatrixTableModel::setDimNames(String_Matrix* dnms) {
  m_dim_names = dnms;
}

void iMatrixTableModel::setPat4D(bool val, bool notify) {
  if (m_pat_4d == val) return;
  m_pat_4d = val;
  if (notify) 
    emit_layoutChanged();
}

bool iMatrixTableModel::ValidateIndex(const QModelIndex& index) const {
  // TODO: maybe need to check bounds???
  return (m_mat);
}

int iMatrixTableModel::matView() const {
  return taMisc::matrix_view;
}
