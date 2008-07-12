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

// ta_qtviewer.h -- gui classes for main window viewing

#ifndef TA_QTVIEWER_H
#define TA_QTVIEWER_H

#include "ta_viewer.h"
#include "ta_qtdata.h"
#include "ta_datatable.h"


#ifndef __MAKETA__
# include <QAction>
# include <QDockWidget>
# include <QFrame>
# include <QMainWindow>
# include <QObject>
# include <qpalette.h>
# include <QIcon>
# include <QList>
# include <qtabbar.h>
# include <QTreeWidget>
# include <QTreeWidgetItem>
# include <qtoolbar.h>

# include "icliptoolwidget.h"
# include "itreewidget.h"
#endif


// (new) forwards this file:
class DataLink_QObj;
class taiDataLink;
class tabDataLink;
class tabListDataLink;
class tabGroupDataLink; 
class ISelectable;
class ISelectable_PtrList;
class ISelectableHost; 
class iBrowseViewer;
class iBaseClipWidgetAction;
class iTreeView;
class iTreeViewItem;
class taiListDataNode;
class taiTreeDataNode;
class tabTreeDataNode;
class tabParTreeDataNode;
class tabDefChildTreeDataNode;
class tabListTreeDataNode;
class tabGroupTreeDataNode;
class iDataPanel;
class iDataPanelFrame;
class iViewPanelSet;
class iListDataPanel;
class iSearchDialog;


// externals
class taGroup_impl;
class taProject;


class TA_API DataLink_QObj: public QObject {
Q_OBJECT
public:
  taiDataLink*	dl;
  DataLink_QObj(taiDataLink* dl_) {dl = dl_;} //
};


class TA_API taiDataLink: public taDataLink { // interface for viewing system
INHERITED(taDataLink)
public:
  static String		AnonymousItemName(const String& type_name, int index); // [index]:Typename
  
  virtual int		checkConfigFlags() const { return 0;}
    // taBase::THIS_INVALID|CHILD_INVALID
  virtual taiDataLink*	ownLink() const {return NULL;} // owner link (NOT gui par)

  virtual void		FillContextMenu(taiActions* menu); // only override to prepend to menu
  virtual void		FillContextMenu_EditItems(taiActions* menu, int allowed) {}
  virtual bool		GetIcon(int bmf, int& flags_supported, QIcon& ic) {return false;}
  virtual taiDataLink*	GetListChild(int itm_idx) {return NULL;} // returns NULL when no more
  virtual taiDataLink*	GetListChild(void* el) {return NULL;} // get link when item is known (esp for change notifies)
  virtual String	GetPath() const {return _nilString;} // esp taBase path 
  virtual String	GetTypeDecoKey() const {return _nilString;}
  virtual String	GetStateDecoKey() const {return _nilString;}
  virtual const QVariant GetColData(const KeyString& key, int role) const 
    {return QVariant();} // for getting things like status text, tooltip text, etc.
  virtual void		Search(iSearchDialog* dlg) {} // called to do a search
  virtual bool		ShowMember(MemberDef* md, TypeItem::ShowContext show_context) const
    {return false;} // asks this type if we should show the md member

  iDataPanel* 		CreateDataPanel(); 
  taiTreeDataNode* 	CreateTreeDataNode(MemberDef* md, taiTreeDataNode* parent,
    taiTreeDataNode* after, const String& node_name, int dn_flags = 0);
    // create the proper tree node, with a tree node as a parent
  taiTreeDataNode* 	CreateTreeDataNode(MemberDef* md, iTreeView* parent,
    taiTreeDataNode* after, const String& node_name, int dn_flags = 0);
    // create the proper tree node, with a iTreeView as a parent

  taiDataLink(void* data_, taDataLink* &link_ref_);
  DL_FUNS(taiDataLink) //

protected:
  DataLink_QObj*	qobj; // #IGNORE delegate object, when we need to connect or signal

  virtual void		Assert_QObj(); // makes sure the qobj is created
  virtual void		FillContextMenu_impl(taiActions* menu) {} // this is usually the one to override
  virtual iDataPanel* 	CreateDataPanel_impl(); // default uses taiView
  taiTreeDataNode* 	CreateTreeDataNode(MemberDef* md,
    taiTreeDataNode* nodePar, iTreeView* tvPar, taiTreeDataNode* after,
    const String& node_name, int dn_flags); // combined version, only 1 xxPar is set
  virtual taiTreeDataNode* CreateTreeDataNode_impl(MemberDef* md,
    taiTreeDataNode* nodePar, iTreeView* tvPar, taiTreeDataNode* after,
    const String& node_name, int dn_flags) = 0; // NOTE: only 1 of the parents is non-null -- use that version of the taiTreeNode constructor

  virtual ~taiDataLink(); // we only ever implicitly destroy, when 0 clients

public: // DO NOT CALL
  virtual void		QueryEditActions_impl(taiMimeSource* ms, int& allowed, int& forbidden) {}
    // get edit items allowed for this one item
  virtual int		EditAction_impl(taiMimeSource* ms, int ea) {return 0;}
  virtual void		ChildQueryEditActions_impl(const MemberDef* par_md, taiDataLink* child,
    taiMimeSource* ms, int& allowed, int& forbidden) {}
  virtual int		ChildEditAction_impl(const MemberDef* par_md, taiDataLink* child,
    taiMimeSource* ms, int ea) {return 0;} //
};


//////////////////////////
//   tabDataLink	//
//////////////////////////

class TA_API tabDataLink: public taiDataLink { // DataLink for taBase objects
INHERITED(taiDataLink)
public:
  taBase*		data() {return (taBase*)m_data;}
  taBase*		data() const {return (taBase*)m_data;}
  override bool		isBase() const {return true;} 
  override int		checkConfigFlags() const; // we call CheckConfig
  override bool		isEnabled() const;
  override taiDataLink*	ownLink() const; 

  
  override bool		GetIcon(int bmf, int& flags_supported, QIcon& ic);
    // delegates to taBase::GetDataNodeBitmap
  override bool		HasChildItems();
  override TypeDef*	GetDataTypeDef() const;
  override String	GetPath() const {return data()->GetPath();} 
  override String	GetTypeDecoKey() const;
  override String	GetStateDecoKey() const;
  override String	GetName() const;
  override String	GetDisplayName() const;
  override void		Search(iSearchDialog* dlg);
  override bool		ShowMember(MemberDef* md, TypeItem::ShowContext show_context) const; // asks this type if we should show the md member
  override String	GetColText(const KeyString& key, int itm_idx = -1) const; // #IGNORE
  override const QVariant GetColData(const KeyString& key, int role) const;

  DL_FUNS(tabDataLink); //

protected:
  static void		SearchStat(taBase* tab, iSearchDialog* sd, int level = 0); // for searching
  
  tabDataLink(taBase* data_, taDataLink* &link_ref_); 
  override taiTreeDataNode* CreateTreeDataNode_impl(MemberDef* md, taiTreeDataNode* nodePar,
    iTreeView* tvPar, taiTreeDataNode* after, const String& node_name, int dn_flags);
  override void		QueryEditActions_impl(taiMimeSource* ms, int& allowed, int& forbidden);
  override int		EditAction_impl(taiMimeSource* ms, int ea);
  override void		ChildQueryEditActions_impl(const MemberDef* par_md, taiDataLink* child,
     taiMimeSource* ms,  int& allowed, int& forbidden);
  override int		ChildEditAction_impl(const MemberDef* par_md, taiDataLink* child,
    taiMimeSource* ms, int ea);
  override void		FillContextMenu_impl(taiActions* menu);
};


//////////////////////////
//   tabODataLink	//
//////////////////////////

class TA_API tabODataLink: public tabDataLink { // DataLink for taOBase objects
INHERITED(tabDataLink)
public:
  taOBase*		data() {return (taOBase*)m_data;}
  taOBase*		data() const {return (taOBase*)m_data;}
    
  override MemberDef*	GetDataMemberDef() const;

  tabODataLink(taOBase* data_);
  DL_FUNS(tabODataLink); //
  
public: // for taLists, and default children (where defined) in taOBase
  virtual taList_impl*	list() {return ((taOBase*)m_data)->children_();}
  virtual taList_impl*	list() const {return ((taOBase*)m_data)->children_();}
  virtual taiDataLink* 	listLink() {taList_impl* lst = list();
    return (lst) ? (taiDataLink*)lst->GetDataLink() : NULL;} 
    
  override taiTreeDataNode* CreateTreeDataNode_impl(MemberDef* md,
    taiTreeDataNode* nodePar, iTreeView* tvPar, taiTreeDataNode* after,
    const String& node_name, int dn_flags);
  override taiDataLink*	GetListChild(int itm_idx);
  override taiDataLink*	GetListChild(void* el);
   override int		NumListCols() const; // number of columns in a list view for this item type
  override const KeyString GetListColKey(int col) const; // #IGNORE
  override String	GetColHeading(const KeyString& key) const; // #IGNORE
  override String	ChildGetColText(taDataLink* child, const KeyString& key, 
    int itm_idx = -1) const;	// #IGNORE
//   override bool		HasChildItems() {return true;} // at very least, has the 'items' subnode

};

//////////////////////////
//   tabListDataLink	//
//////////////////////////

class TA_API tabListDataLink: public tabODataLink {
  // DataLink for taList objects -- note that it also manages the ListView nodes
INHERITED(tabODataLink)
public:
  taList_impl*		list() {return (taList_impl*)m_data;}
  taList_impl*		list() const {return (taList_impl*)m_data;}
  override taiDataLink* listLink() {return this;}
  
  tabListDataLink(taList_impl* data_);
  DL_FUNS(tabListDataLink) //
protected:
  override taiTreeDataNode* CreateTreeDataNode_impl(MemberDef* md,
    taiTreeDataNode* nodePar, iTreeView* tvPar, taiTreeDataNode* after,
    const String& node_name, int dn_flags);
};


//////////////////////////
//   tabGroupDataLink	//
//////////////////////////

class TA_API tabGroupDataLink: public tabListDataLink {
  // DataLink for taGroup objects -- adds additional 'subgroups' node under the 'items' node, for any subgroups
INHERITED(tabListDataLink)
public:
  taGroup_impl*		data() {return (taGroup_impl*)m_data;}
  const taGroup_impl*	data() const {return (taGroup_impl*)m_data;}
  
  tabGroupDataLink(taGroup_impl* data_);
  DL_FUNS(tabGroupDataLink)
protected:
  override taiTreeDataNode* CreateTreeDataNode_impl(MemberDef* md, taiTreeDataNode* nodePar,
    iTreeView* tvPar, taiTreeDataNode* after, const String& node_name, int dn_flags);
};


class TA_API IDataViewWidget { // interface that all DataViewer::widget() class must implement
friend class DataViewer;
public:
  virtual bool		isDirty() const {return false;}
  virtual QWidget*	widget() = 0; // return the widget
  DataViewer*		viewer() {return m_viewer;} // often lexically overridden to strongly type
  virtual iMainWindowViewer* viewerWindow() const; // the main window in which this widget lives
  
  void			Constr() {Constr_impl();} // called virtually, after new, override impl
  void			Close(); // deletes us, and disconects us from viewer -- YOU MUST NOT MAKE ANY CALLS TO OBJ AFTER THIS
  
//  inline operator QWidget()	{return &(widget());} // enables convenient implicit conversion
  void			Refresh(); // manual refresh (brackets with Busy)
  virtual void		ResolveChanges(CancelOp& cancel_op) {ResolveChanges_impl(cancel_op);} 
  // called from viewer prior to close; should normally autosave unsaved changes
  virtual void		SaveData() {}
      
  IDataViewWidget(DataViewer* viewer);
  virtual ~IDataViewWidget(); // informs mummy of our destruction
  
protected:
  DataViewer*		m_viewer; // our mummy
  mutable iMainWindowViewer* m_window; // can be set by creator, or dyn looked up/cached
  virtual void		closeEvent_Handler(QCloseEvent* e,
    CancelOp def_cancel_op = CO_PROCEED);
    // default says "proceed", delegates decision to viewer; call with CO_NOT_CANCELLABLE for unconditional
  virtual void		Constr_impl() {} // override for virtual construction (called after new)
  virtual void		Constr_post() {} // called virtually, in DV::Constr_post -- entire win struct is now available
  virtual void		OnClosing_impl(CancelOp& cancel_op); // invoked in dtor (uncancellable); you should also invoke in the closeEvent (maybe cancellable)
  virtual void		Refresh_impl() {}
  virtual void		ResolveChanges_impl(CancelOp& cancel_op) {}
};


//////////////////////////
//   ISelectable	//
//////////////////////////

/*
  this interface is for selectable items in the view, such as tree nodes in the browser, or
 Inventor objects in T3Viewer

 There are two major modes of clipboard handling: single selection, and multi-selection.
 In the single selection case, the ISelectable item itself controls everything; in this case,
   the sel_items list parameter can be null, or can be a list containing just the ISelectable
   item itself.
 In the multi-select case, the first selected item is the root (controlling) item. It must be
   passed a list of all the items (including itself, in position 0).

 NOTE: you must call viewer_win()->ItemRemoving(item) in the implementation when an object is
 deleted or being removed from the viewing hierarchy.
 
   "eff_data" is the concept of what the object refers to -- for things like Layers,
     it is the object itself; but for graphs, it is NOT the table in the view, it
     is the view itself -- so the various links etc. are modalized according to
     this major distinction -- this is operationalized in T3 where a view guy can
     override his effLink
*/

class TA_API ISelectable: public virtual IDataLinkProxy { //
INHERITED(IDataLinkProxy)
friend class ISelectableHost;
public: // Interface Properties and Methods
  enum GuiContext { // primarily for the T3 guys that have dual identities (don't reorder these)
    GC_DEFAULT		= 0, // this code only used when requesting the default link
    GC_SINGLE_DATA	= 1, // for trees, etc., only one identity, and link() is data
    GC_DUAL_DEF_DATA	= 2, // for T3, guys like Layer, where def identity is the data
    GC_DUAL_DEF_VIEW	= 3,  // for T3, guys like GraphView, where def ident is the graphview obj itself
  };
  
  virtual MemberDef*	md() const {return NULL;} // memberdef in data parent, if any, of the selected item
  virtual taiDataLink*	own_link(GuiContext sh_typ = GC_DEFAULT) const; // owner item's link -- this is the *eff_data* parent (not the gui parent) 
  virtual ISelectable*	par() const = 0; // gui parent, if any
  virtual taiDataLink*	par_link() const; // parent item's link -- this is the *gui* parent (not the data parent/owner) 
  virtual taiDataLink*	viewLink() const {return NULL;} // only defined for T3 guys
  taiDataLink*		effLink(GuiContext sh_typ = GC_DEFAULT) const; // for trees and things like layers, we just just use the data, for things like graphs (where view==data) we use the view itself (override shType)
  virtual taiDataLink*	clipParLink(GuiContext sh_typ) const;
    // for tree stuff, we use the gui parent for clip ops; for t3 we use the eff_data owner
  virtual MemberDef* 	par_md() const;// eff_data parent item's (if any) md
  virtual ISelectableHost* host() const = 0; //
  virtual GuiContext	shType() const {return GC_SINGLE_DATA;} // mediates menu handling, and default source for clip ops
  taBase*		taData(GuiContext sh_typ = GC_DEFAULT) const; // if the eff_data is taBase, this returns it
//obs  virtual String	view_name() const = 0; // for members, the member name; for list items, the name if any, otherwise a created name using the index
  QWidget*		widget() const; // gets from host
  QObject*		clipHandlerObj() const; // shortcut for host()->clipHanderObj(); 

  virtual TypeDef* 	GetEffDataTypeDef(GuiContext sh_typ = GC_DEFAULT) const; // gets it from effLink
  virtual int		EditAction_(ISelectable_PtrList& sel_items, int ea,
    GuiContext sh_typ = GC_DEFAULT);
   // do the indicated edit action (called from browser or list view); normally implement the _impl
  virtual void 		FillContextMenu(ISelectable_PtrList& sel_items,
    taiActions* menu, GuiContext sh_typ = GC_DEFAULT);
   // for multi or single (normally implement the _impl)
//  virtual void 		FillContextMenu(taiActions* menu);
   // for single (normally implement the _impl)
  virtual taiClipData*	GetClipData(const ISelectable_PtrList& sel_items,
    int src_edit_action, bool for_drag, GuiContext sh_typ = GC_DEFAULT) const; // works for single or multi; normally not overridden
  virtual taiClipData*	GetClipDataSingle(int src_edit_action,
    bool for_drag, GuiContext sh_typ = GC_DEFAULT) const = 0;
  virtual taiClipData*	GetClipDataMulti(const ISelectable_PtrList& sel_items, 
    int src_edit_action, bool for_drag, GuiContext sh_typ = GC_DEFAULT) const {return NULL;}// only needed if multi is handled
  virtual int		QueryEditActions_(taiMimeSource* ms,
    GuiContext sh_typ = GC_DEFAULT) const; // typically called on single item for canAcceptDrop
  int			QueryEditActions_(const ISelectable_PtrList& sel_items,
    GuiContext sh_typ = GC_DEFAULT) const;
    // called to get edit items available on clipboard for the sel_items
  virtual int		RefUnref(bool ref) {return 1;} // ref'ed/unrefed in select lists etc.; optional, and can be used for lifetime mgt; returns count after operation

  ~ISelectable();
protected:
  void 			DropHandler(const QMimeData* mime, const QPoint& pos, 
    int key_mods, int where);
    //  handles all aspects of a drag drop operation -- ALWAYS uses default context
    // (iTreeWidgetItem::WhereIndicator where)
  virtual int		EditActionD_impl_(taiMimeSource* ms,
    int ea, GuiContext sh_typ) = 0;
    // do Dst op for single selected item; generally doesn't need extending
  virtual int		EditActionS_impl_(int ea, GuiContext sh_typ) = 0;
    // do Src op for single or one of multi selected items; CUT and COPY usually just a 1 return code; we actually implement the actual clipboard transfer
  virtual void		FillContextMenu_EditItems_impl(taiActions* menu,
    int allowed, GuiContext sh_typ); // might be extended
  virtual void		FillContextMenu_impl(taiActions* menu,
    GuiContext sh_typ) {} // link handles most, called in FCM
  virtual void		GetContextCaptions(String& view_cap, String& obj_cap);
  virtual void		QueryEditActionsD_impl_(taiMimeSource* ms,
   int& allowed, int& forbidden, GuiContext sh_typ) const = 0;
    // get Dst ops allowed for a single item,
  virtual void		QueryEditActionsS_impl_(int& allowed,
   int& forbidden, GuiContext sh_typ) const = 0;
    // get Src ops allowed for a single item, possibly of many selected items
};

class TA_API IObjectSelectable: public ISelectable { // specialized for taBase object items
INHERITED(ISelectable)
public: // Interface Properties and Methods
  override taiClipData*	GetClipDataSingle(int src_edit_action, bool for_drag,
    GuiContext sh_typ = GC_DEFAULT) const;
  override taiClipData*	GetClipDataMulti(const ISelectable_PtrList& sel_items, 
    int src_edit_action, bool for_drag, GuiContext sh_typ = GC_DEFAULT) const;
protected:
  override int		EditActionD_impl_(taiMimeSource* ms, int ea,
    GuiContext sh_typ);
  override int		EditActionS_impl_(int ea, GuiContext sh_typ);
  override void		QueryEditActionsD_impl_(taiMimeSource* ms, int& allowed,
    int& forbidden, GuiContext sh_typ) const;
  override void		QueryEditActionsS_impl_(int& allowed, int& forbidden,
    GuiContext sh_typ) const;
};


//////////////////////////
//   ISelectable_PtrList//
//////////////////////////

// Note: for inheritance hierarchies, only the first parent is searched, as that is
// always the primary class in cases where there is multiple inheritance

class TA_API ISelectable_PtrList: public taPtrList<ISelectable> { // for selection lists
INHERITED(taPtrList<ISelectable>)
friend class ISelectableHost;
public:
  TypeDef*	Type1(ISelectable::GuiContext gc_typ = ISelectable::GC_DEFAULT); 
    // data type of 1st item
  TypeDef*	CommonSubtype1N(ISelectable::GuiContext gc_typ = ISelectable::GC_DEFAULT); 
    // greatest common data subtype of items 1-N
  TypeDef*	CommonSubtype2N(ISelectable::GuiContext gc_typ = ISelectable::GC_DEFAULT); 
    // greatest common data subtype of items 2-N
  
  ISelectable_PtrList() {Initialize();}
  ISelectable_PtrList(const ISelectable_PtrList& cp);
  ~ISelectable_PtrList();
protected:
  static taPtrList_impl* insts; // global lists of all insts, to enable removing deleting items
private:
  void	Initialize();
};


//////////////////////////
//   DynMethodDesc	//
//////////////////////////

class TA_API DynMethodDesc { // #IGNORE
friend class DynMethod_PtrList;
public:
  int			idx; // list index, used as param in the action callback routine
  int			dmd_type;
  MethodDef*		md;
protected:
  DynMethodDesc() {}
  ~DynMethodDesc() {}
};

//////////////////////////
//   DynMethod_PtrList	//
//////////////////////////

class TA_API DynMethod_PtrList: public taPtrList<DynMethodDesc> { // #IGNORE
#ifndef __MAKETA__
typedef taPtrList<DynMethodDesc> inherited;
#endif
public:
  enum DMDType {
    Type_1N,	// <methname>() method; applied to all selected object
    Type_1_2N,	// <methname>(Type2N* param) method; called on object 1 for objects 2:N
    Type_2N_1,	// <methname>(Type1* param) method; called on objects 2:N for object 1
    Type_MimeN_N // <methname>(Type1* param) method; called on objects 1:N for mime source object(s) of common subtype Type1
  };

  DynMethodDesc*	AddNew(int dmd_type, MethodDef* md); // creates new DMD and adds, returning ref
  void			Fill(ISelectable_PtrList& sel_items,
    ISelectable::GuiContext gc_typ); // clear, then fill based on sel_items
  void			FillForDrop(const taiMimeSource& ms, 
    ISelectable* drop_item); // clear, then fill based on ms and sel_items (used for Drop operations)
  ~DynMethod_PtrList();
protected:
  override void 	El_Done_(void* it) {delete (DynMethodDesc*)it;}
  override void		El_SetIndex_(void* it, int idx) {((DynMethodDesc*)it)->idx = idx; };
  // sets the element's self-index
};

class SelectableHostHelper;

class TA_API ISelectableHost { // interface on the controlling widget hosting ISelectable items
friend class SelectableHostHelper;
friend class ISelectable;
public:
  enum NotifyOp { // notify ops for the NotifySignal -- note, passed as int in the sig/slot
    OP_GOT_FOCUS,
    OP_SELECTION_CHANGED,
    OP_DESTROYING
  };
  
  static const char* edit_enabled_slot; // for the SetClipboardHandler call, takes int& param
  static const char* edit_action_slot; // for the SetClipboardHandler call, takes int param
  static const char* actions_enabled_slot; // currently NULL
  static const char* update_ui_signal; // currently NULL
  static const char* edit_menu_action_slot;
  
  ISelectable*		curItem() {return selItems().SafeEl(0);} // convenience
  virtual void		setCurItem(ISelectable* item, bool forceUpdate = false);
//  virtual taiAction_List& dynActions() {return dyn_actions;} 
    // Action list, of current dynamic actions available
//  virtual DynMethod_PtrList& dynMethods() {return dyn_methods;}
   // -- list of current dynamic methods availableselectionChanged
  QObject*		clipHandlerObj() const; 
    // provided so client can connect to us as a ClipHandler (EditEnabled, EditAction only)
  virtual bool 		hasMultiSelect() const = 0; // true if supports multi select
  SelectableHostHelper* helperObj() const {return helper;} // for attaching slots
  iMainWindowViewer*	mainWindow() const; // returns main window we are embedded in
  virtual bool		selectionChanging() {return (m_sel_chg_cnt != 0);}
    // you can use this to escape from sundry gui notifies to yourself (to avoid recursion)
  virtual ISelectable_PtrList&	selItems() {return sel_items;} // currently selected items
  virtual QWidget*	widget() = 0; // provides a gui parent for things like context menus
  
  virtual void		FillContextMenu(taiActions* menu); 
    // s/b called by desc class, to put dynaction items onto menu
  virtual void		AddDynActions(taiActions* menu, int dyn_list,
    ISelectable::GuiContext gc_typ = ISelectable::GC_DEFAULT);
   // add the dynamic guys to the given menu (note: FillContextMenu does this too)
  
  virtual void 		SelectionChanging(bool begin, bool forced = true); // if used, must be called in pairs, with true then false
  virtual void 		ClearSelectedItems(bool forced = true);
  virtual void 		AddSelectedItem(ISelectable* item,  bool forced = true);
  virtual bool 		RemoveSelectedItem(ISelectable* item,  bool forced = true); 
    // 'true' if item was actually removed from (i.e. was in) list

  virtual void 		EditActionsEnabled(int&); // return enabled flags
  virtual void 		EditAction(int, 
    ISelectable::GuiContext gc_typ = ISelectable::GC_DEFAULT); // perform the action
  virtual void 		DropEditAction(int ea); // perform the action (from drop handler)
  
  void			Emit_GotFocusSignal() {Emit_NotifySignal(OP_GOT_FOCUS);} 
    // only signal external guys should call (when we or a parent get focus)

  void 			Connect_SelectableHostNotifySignal(QObject* sink_obj,
    const char* sink_slot, bool discnct = false);  // connects (or disconnects) a sink (ex iFrame) to the notify signal raised when sel changes (or gets focus, etc.)
  void 			Connect_SelectableHostItemRemovingSlot(QObject* src_obj, 
    const char* src_signal, bool discnct = false); // connects (or disconnects) an optional ItemRemoving notification
  void 			UpdateMethodsActionsForDrop();
    // uses ctxt_ms and ctxt_item
  void			ctxtMenu_destroyed();

  ISelectableHost();
  virtual ~ISelectableHost();
  
protected:
  static void 		ItemDeleting(ISelectable* item); // make sure it is off all sel lists
  
  int			m_sel_chg_cnt; 
   // counter to track batch selection changes; -ve means we are in Update (prog calls ignored) 
  ISelectable_PtrList	sel_items;
  DynMethod_PtrList	dyn_methods[2]; // available dynamic methods; 0=single/view 1=obj
  taiAction_List	dyn_actions[2]; // actions corresponding to methods (always 1:1)
  ISelectable::GuiContext dyn_context[2]; 
  int			dyn_idx; // the current dyn index (of all [])
  taiMimeSource*	ctxt_ms; // during a drop and context holds the ms used for dyn and edit actions 
  ISelectable*		ctxt_item; // during drop, holds the item dropped on; for context, holds the item
  
  virtual void		FillContextMenu_pre(ISelectable_PtrList& sel_items, 
    taiActions* menu) {} // hook
  void 			FillContextMenu_int(ISelectable_PtrList& sel_items,
    taiActions* menu, int dyn_list, ISelectable::GuiContext sh_typ);
  virtual void		FillContextMenu_post(ISelectable_PtrList& sel_items, 
    taiActions* menu) {} // hook 
  
  virtual void 		EditAction_Delete(ISelectable::GuiContext gc_typ); // actually does the Edit/Delete
  virtual void		UpdateSelectedItems_impl() = 0; 
    // called when force=true for changes, force gui to be selItems
  void			Emit_NotifySignal(NotifyOp op);
  virtual void		DoDynAction(int idx); // do the action in the list
  virtual void 		SelectionChanged(bool forced);
    // invoked when selection of current clipboard/focus handler changes
  virtual void		UpdateMethodsActions(int dyn_list,
    ISelectable::GuiContext gc_typ = ISelectable::GC_DEFAULT); 
    // updates the dyn methods/actions

private:
  SelectableHostHelper* helper;
};

class TA_API SelectableHostHelper: public QObject { 
 // #IGNORE this is a helper QObject that handles signals/slots
  Q_OBJECT
INHERITED(QObject)
friend class ISelectableHost;
friend class ISelectable;
public:

public slots:
  void		EditActionsEnabled(int& ea) {host->EditActionsEnabled(ea);} 
    // callback for when we are ClipHandler
  void		EditAction(int ea) {host->EditAction(ea);} //  callback for when we are ClipHandler
  void		ctxtMenu_destroyed() {host->ctxtMenu_destroyed();}
   // attached to ctxt menu so we can clean up the mimesource 
  void		EditAction(taiAction*); //  callback for context menu 
    
#ifndef __MAKETA__
signals:
  void		NotifySignal(ISelectableHost* src, int op);
#endif

protected:
  ISelectableHost* 	host;
  
  override void 	customEvent(QEvent* ev); // dispatch
  void			Emit_NotifySignal(ISelectableHost::NotifyOp op);
  
  SelectableHostHelper(ISelectableHost* host_) {host = host_;}

protected slots:
  void		DynAction(int i) {host->DoDynAction(i);}
  void		DropEditAction(int ea) {host->DropEditAction(ea);}  // for drops
  void		ItemRemoving(ISelectable* item) {host->RemoveSelectedItem(item, false);}
private:
  SelectableHostHelper& operator=(const SelectableHostHelper&); // not allowed
};


class TA_API iFrameViewer: public QWidget, public IDataViewWidget {
// ##NO_INSTANCE ##NO_TOKENS ##NO_CSS ##NO_MEMBERS base class for things in the center splitter of main
  Q_OBJECT
INHERITED(QWidget)
friend class FrameViewer;
friend class MainWindowViewer;
friend class iMainWindowViewer;
public:
  
  virtual int		stretchFactor() const {return 2;} // helps set sensible alloc of space in split
  inline FrameViewer*	viewer() {return (FrameViewer*)m_viewer;} // usually lex overridden in subclass
  MainWindowViewer* 	mainWindowViewer();
  
//nn??  virtual void		UpdateTabNames(); // called by a datalink when a tab name might have changed
  virtual void 		GetWinState() {GetWinState_impl();} // when saving view state
  virtual void		SetWinState() {SetWinState_impl();} // when showing, from view state
  
  iFrameViewer(FrameViewer* viewer_, QWidget* parent = NULL);
  ~iFrameViewer();
  
public slots:
  void      	SelectableHostNotifySlot_Internal(ISelectableHost* src, int op);
    // connects guys nested below us; lets us trap, and forward
  void      	SelectableHostNotifySlot_External(ISelectableHost* src, int op);
   // from external guys (forwarded from main window)

#ifndef __MAKETA__
signals:
  void      	SelectableHostNotifySignal(ISelectableHost* src, int op);
    // forwarder, from all internal guys
#endif

public: // IDataViewerWidget i/f
  override QWidget*	widget() {return this;}
protected:
//  override void		Constr_impl(); // called virtually, after new
  virtual void 		GetWinState_impl() {} // when saving view state
  virtual void		SetWinState_impl() {} // when showing, from view state

protected:
  short int		shn_changing; // for marking forwarding, so we don't reflect back

  void 			hideEvent(QHideEvent* e); // override
  void 			showEvent(QShowEvent* e); // override
  virtual void		Showing(bool showing); // #IGNORE called by the show/hide handlers
  virtual void		SelectionChanged_impl(ISelectableHost* src_host) {}
    // we call this when we receive a valid incoming change, or maybe new focus
  
private:
  void			Init();
};


class TA_API iBrowseViewer : public iFrameViewer { // base of viewer window used for object and class browsing
Q_OBJECT
INHERITED(iFrameViewer)
public:

  iTreeView*		lvwDataTree; 

  inline BrowseViewer*	browser() {return (BrowseViewer*)m_viewer;}
  void*			root() {return (browser()) ? browser()->root() : NULL;}
  TypeDef*		root_typ() {return (browser()) ? browser()->root_typ : &TA_void;}
  MemberDef*		root_md() {return (browser()) ? browser()->root_md : NULL;} //
  taiDataLink*		rootLink() {return (browser()) ? browser()->rootLink() : NULL;} 
  override int		stretchFactor() const {return 1;} //  1/2 default

  void			Reset();
  virtual void 		ApplyRoot(); // #IGNORE actually applies the new root value
  
  iBrowseViewer(BrowseViewer* browser_, QWidget* parent = 0);
  ~iBrowseViewer();

public slots:
  virtual void		mnuBrowseNodeDrop(int param) {mnuBrowseNodeDrop_param = param;} 
    // called from within the node->dropped event

protected slots:
  virtual void		lvwDataTree_FillContextMenuHookPost(
    ISelectable_PtrList& sel_items, taiActions* menu);

protected: // IDataViewWidget i/f
  override void		Refresh_impl(); 

protected:
  int			mnuBrowseNodeDrop_param;
    // param from the mnuBrowseDrop slot -- called by a node, only valid for its call
private:
  void			Init();
};


class TA_API iTabViewer : public iFrameViewer { // viewer window used for tabbed window
    Q_OBJECT
INHERITED(iFrameViewer)
friend class iTabView;
public:
  QSplitter*		spl_main; // toplevel guy
  
  virtual taiDataLink*	sel_link() const {return (cur_item) ? cur_item->link() : NULL;} // datalink of selected item that is controlling the current data panel view, ex. datalink of the selected tree node in a browser; return NULL if unknown, mult-select is in force, etc. -- controls things like clip handling
  virtual MemberDef*	sel_md() const {return (cur_item) ? cur_item->md() : NULL;}; // as for sel_link
  override int		stretchFactor() const {return 4;} // 3/2 default
  iTabView*		tabView() {return m_curTabView;} // currently active
  iTabView_PtrList*	tabViews() {return m_tabViews;} // currently active

  virtual void		AddPanel(iDataPanel* panel); // adds a new pane, and sets active in current tab
  void			AddPanelNewTab(iDataPanel* panel, bool lock = false); // adds a new tab, sets panel active in it, locks if requested
  virtual iTabView*	AddTabView(QWidget* parCtrl, iTabView* splitBuddy = NULL); // adds a new tab view, optionally as a split
  void 			ShowLink(taiDataLink* link, bool not_in_cur = false);
  void			ShowPanel(iDataPanel* panel); // shows the panel, according to showing rules
  
  virtual void		TabView_Destroying(iTabView* tv); // called when a tabview deletes
  virtual void		TabView_Selected(iTabView* tv); // called when a tabview gets focus
  override void		UpdateTabNames(); // called by a datalink when a tab name might have changed
  iTabViewer(PanelViewer* viewer_, QWidget* parent = NULL); //
  ~iTabViewer();

public slots:
  virtual void		AddTab(); // causes tab bar to open a new tab, on current panel
  virtual void		CloseTab(); // causes current tab to close (unless only 1 tab)
  void			Closing(CancelOp& cancel_op); // override
  
  virtual void 		viewCloseCurrentView();
  virtual void 		viewSplitVertical();
  virtual void 		viewSplitHorizontal();

protected: // IDataViewWidget i/f
  override void		Refresh_impl(); 

protected:
  iTabView_PtrList*	m_tabViews; // all created tab views
  iTabView*		m_curTabView; // tab view (split) that currently has the focus
  ISelectable*		cur_item; // the last item that was curItem -- NOTE: somewhat dangerous to cache, but according to spec, src_host should issue a new notify if this deletes
  void			Constr_Menu_impl(); // override
  override void		Constr_post(); // called virtually, in DV::Constr_post 
  override void		ResolveChanges_impl(CancelOp& cancel_op);
  override void 	SelectionChanged_impl(ISelectableHost* src_host); // called when sel changes
  void 			viewSplit(int o);
  override void 	GetWinState_impl();
  override void		SetWinState_impl();

private:
  void			Init();
};


class TA_API iDockViewer: public QDockWidget, public IDataViewWidget {
// ##NO_INSTANCE ##NO_TOKENS ##NO_CSS ##NO_MEMBERS gui portion of the DockViewer
  Q_OBJECT
INHERITED(QDockWidget)
public:
  inline DockViewer*	viewer() {return (DockViewer*)m_viewer;}
  
  iDockViewer(DockViewer* viewer_, QWidget* parent = NULL);
  ~iDockViewer();
  
public: // IDataViewerWidget i/f
  override QWidget*	widget() {return this;}
protected:
//  override void		Constr_impl();
  
protected:
  override void 	closeEvent(QCloseEvent* ev);
  bool			event(QEvent* ev);
  void 			hideEvent(QHideEvent* e); // override
  void 			showEvent(QShowEvent* e); // override
  virtual void		Showing(bool showing); // #IGNORE called by the show/hide handlers

private:
  void			Init();
};


class TA_API iToolBoxDockViewer: public iDockViewer {
  Q_OBJECT
INHERITED(iDockViewer)
public:
  QToolBox*		tbx;
  
  inline ToolBoxDockViewer*	viewer() {return (ToolBoxDockViewer*)m_viewer;}
  
  int			AssertSection(const String& sec_name); // insures the tab exists; returns idx
  void			AddClipToolWidget(int sec, iClipWidgetAction* cwa);
  void			AddSeparator(int sec); // adds a separator (if one is not at the end already)
  
  iToolBoxDockViewer(ToolBoxDockViewer* viewer_, QWidget* parent = NULL);
  ~iToolBoxDockViewer(); //
  
public: // IDataViewerWidget i/f
//  override QWidget*	widget() {return this;}
protected:
//  override void		Constr_impl();
  override void		Constr_post();
  
protected:
  QToolBar*		sectionWidget(int sec);
  
private:
  void			Init();
};


class TA_API iToolBar: public QToolBar, public IDataViewWidget {
  // ##NO_INSTANCE ##NO_TOKENS ##NO_CSS ##NO_MEMBERS
INHERITED(QToolBar)
friend class ToolBar;
friend class iMainWindowViewer;
  Q_OBJECT
public:
  ToolBar*		viewer() {return (ToolBar*)m_viewer;}
  
  iToolBar(ToolBar* viewer, QWidget* parent = NULL);
  ~iToolBar();
  
public: // IDataViewerWidget i/f
  override QWidget*	widget() {return this;}
protected:
//  override void		Constr_impl();

protected:
  void 			hideEvent(QHideEvent* e); // override
  void 			showEvent(QShowEvent* e); // override
  virtual void		Showing(bool showing); // #IGNORE called by the show/hide handlers
private:
  void			Init();
};

class TA_API iToolBar_List: public taPtrList<iToolBar> {
 // each mainwin maintains its existent toolbars in this list
public:
protected:
  override String El_GetName_(void* it) const;
};


class TA_API iApplicationToolBar: public iToolBar {
  // widget for the Application toolbar -- created by ToolBar if name=Application
INHERITED(iToolBar)
public:
  iApplicationToolBar(ToolBar* viewer, QWidget* parent = NULL)
  :iToolBar(viewer, parent){}
 protected:
  override void		Constr_post(); 
};

class TA_API iBaseClipWidgetAction: public iClipWidgetAction {
  // for making drag/copy guys from a taBase instance (ex. see programs_qtso)
INHERITED(iClipWidgetAction)
  Q_OBJECT
public:
  taBase*		base() const {return m_inst;}
  
  
  // tooltip defaults to key_desc of the instance		
  iBaseClipWidgetAction(taBase* inst = NULL, QObject* parent = NULL);
  iBaseClipWidgetAction(const QIcon & icon, taBase* inst = NULL, QObject* parent = NULL);
  iBaseClipWidgetAction(const String& tooltip, const QIcon & icon,
    taBase* inst = NULL, QObject* parent = NULL);
  iBaseClipWidgetAction(const String& text,
    taBase* inst = NULL, QObject* parent = NULL);
  iBaseClipWidgetAction(const String& tooltip, const String& text,
    taBase* inst = NULL, QObject* parent = NULL);
   
  
protected:
  taSmartRef		m_inst;
  
  override QMimeData* 	mimeData() const; // delegates to the inst
  override QStringList  mimeTypes() const; // the ta custom type
private:
  void			Init(taBase* inst, String tooltip = _nilString);
};


/////////////////////////
//   iMainWindowViewer	//
//////////////////////////

class TA_API iMainWindowViewer: public QMainWindow, public IDataViewWidget {
// ##NO_INSTANCE ##NO_TOKENS ##NO_CSS ##NO_MEMBERS gui portion of the DataViewer
  Q_OBJECT
INHERITED(QMainWindow)
friend class taDataLink;
friend class MainWindowViewer;
public: 
//nn  iToolBar_List		toolbars; // list of all created toolbars
  taiAction_List	actions; // our own list of all created actions

  taiMenuBar*		menu;		// menu bar -- note: we use the window's built-in QMenu
  QSplitter*		body;		// #IGNORE body of the window

  taiMenu* 		fileMenu;
  taiMenu* 		fileOpenRecentMenu;
  taiMenu* 		fileExportMenu; // submenu -- empty and disabled in base
  taiMenu* 		editMenu;
  taiMenu* 		viewMenu;
  taiMenu* 		show_menu;
  taiMenu* 		frameMenu; // enumeration of all Frame guys
  taiMenu* 		toolBarMenu; // enumeration of all ToolBar guys
  taiMenu* 		dockMenu; // enumeration of all Dock guys
  taiMenu* 		toolsMenu;
  taiMenu* 		windowMenu; // on-demand
  taiMenu* 		helpMenu;
  taiAction* 		fileNewAction;
  taiAction* 		fileOpenAction;
  taiAction* 		fileSaveAction;
  taiAction* 		fileSaveAsAction;
  taiAction* 		fileSaveAllAction;
  taiAction* 		fileCloseAction;
  taiAction* 		fileOptionsAction;
  taiAction* 		filePrintAction;
  taiAction* 		fileCloseWindowAction; //note: special, because it is always at bottom of File menu for non-root
  taiAction* 		fileQuitAction; //note: only on app window, except on all in Mac
  taiAction* 		editUndoAction;
  taiAction* 		editRedoAction;
  taiAction* 		editCutAction;
  taiAction* 		editCopyAction;
  taiAction* 		editDupeAction;
  taiAction* 		editDeleteAction;
  taiAction* 		editPasteAction;
  taiAction* 		editPasteIntoAction;
  taiAction* 		editPasteAssignAction;
  taiAction* 		editPasteAppendAction;
  taiAction* 		editLinkAction;
  taiAction* 		editLinkIntoAction;
  taiAction* 		editUnlinkAction;
  taiAction*		editFindAction;
  taiAction*		editFindNextAction;
  
  taiAction* 		viewRefreshAction;
  taiAction* 		viewSplitVerticalAction;
  taiAction* 		viewSplitHorizontalAction;
  taiAction* 		viewCloseCurrentViewAction;
  taiAction* 		viewSaveViewAction;

  taiAction*	        toolsClassBrowseAction;
  
  taiAction* 		helpHelpAction;
  taiAction* 		helpAboutAction;

  QObject* 		clipHandler() {return last_clip_handler;} // obj (if any) controlling clipboard handling
  
  taProject*		myProject() const; // project of which this viewer is a part
  taProject*		curProject() const; // only if we are a projviewer
  inline bool		isRoot() const {return m_is_root;} // if this is app root, closing quits
  inline bool		isProjBrowser() const {return m_is_proj_browser;} // if a project browser, persistent
  inline bool		isProjViewer() const {return m_is_proj_viewer;} // if a project viewer, persistent
  inline bool		isProjShower() const 
    {return m_is_proj_viewer || m_is_proj_browser;} // if either
  
  int 			uniqueId() const {return m_unique_id;} // for urls 
  inline MainWindowViewer* viewer() const {return (MainWindowViewer*)m_viewer;} 

  virtual taiAction*	AddAction(taiAction* act); // add the action to the list, returning the instance (for convenience)
  void			AddPanel(iDataPanel* panel, bool new_tab = true); 
    // insures we have a iTabViewer; adds panel; if requested, adds a new tab, sets panel active in it
  virtual void		AddToolBar(iToolBar* tb); // add the toolbar, showing it if it is mapped
  virtual void		AddFrameViewer(iFrameViewer* fv, int at_index = -1); // -1=end
#ifndef __MAKETA__
  virtual void		AddDockViewer(iDockViewer* dv,
    Qt::DockWidgetArea in_area = Qt::BottomDockWidgetArea); 
  QPointer<iSearchDialog> search_dialog;

  void 			Find(taiDataLink* root, const String& find_str="");
  // common find called by main menu, and context menu finds
#endif
  iTreeViewItem* 	AssertBrowserItem(taiDataLink* link);
  iTreeViewItem* 	BrowserExpandAllItem(taiDataLink* link);
  iTreeViewItem* 	BrowserCollapseAllItem(taiDataLink* link);
  bool		 	AssertPanel(taiDataLink* link, bool new_tab = false,
				    bool new_tab_lock = true); // used for things like wizards and edits; note: ntl ignored if !nt
  void			EditItem(taiDataLink* link, bool not_in_cur = false); // edit this guy in a new panel, making a tab viewer if necessary
  int			GetEditActions(); // after a change in selection, update the available edit actions (cut, copy, etc.)
  iTabViewer* 		GetTabViewer(bool force = false); // get the tab viewer, or make one if force


  void 			setFrameGeometry(const iRect& r);
  void			setFrameGeometry(int left, int top, int width, int height); //bogus: see Qt docs on geometry under X

  iMainWindowViewer(MainWindowViewer* viewer_, QWidget* parent = NULL);
    // uses: WFlags flags = (WType_TopLevel | WStyle_SysMenu | WStyle_MinMax | WDestructiveClose)
  ~iMainWindowViewer();

public slots:
  virtual void 	fileNew(); // New Project (in new viewer)
  virtual void 	fileOpen(); // Open Project (in new viewer)
  virtual void 	fileOpenRecent_aboutToShow();
  virtual void	fileOpenFile(const Variant& file); // for recent menu
  virtual void 	fileSave();  // Save Project (only enabled if viewer)
  virtual void 	fileSaveAs();  // SaveAs Project (only enabled if viewer)
  virtual void 	fileSaveAll();  // Save All Projects (always enabled)
  virtual void 	fileClose(); // Close Project (only enabled if viewer)
  virtual void 	fileOptions(); // edits taMisc
  virtual void 	filePrint(){}
  virtual void 	fileCloseWindow();// (non-root only)
  virtual void 	fileQuit(); // (root) or all on Mac (needed for App menu)
/*  virtual void editUndo();
  virtual void editRedo();*/
  virtual void 	editFind(); 
  virtual void 	editFindNext(); 
  virtual void	viewRefresh() {Refresh();} // manually rebuild/refresh the current view
  void		viewSaveView(); // save view state
  
  virtual void	showMenu_aboutToShow();
  virtual void	ShowChange(taiAction* sender);	// when show/hide menu changes
  
  virtual void	toolsClassBrowser();

  virtual void	windowMenu_aboutToShow();
  void 		windowActivate(int win); // activate the indicated win
/*  virtual void helpIndex();*/
  virtual void  helpHelp();
  virtual void 	helpAbout();

  virtual void	mnuEditAction(taiAction* mel);
//nn  void		actionsMenu_aboutToShow(); // populates dynamic items
  
  void		SetClipboardHandler(QObject* handler_obj,
    const char* edit_enabled_slot = NULL,
    const char* edit_action_slot = NULL,
    const char* actions_enabled_slot = NULL,
    const char* update_ui_signal = NULL); // see "Clipboard Handling" in .cpp
    
  void		SelectableHostNotifySlot(ISelectableHost* src_host, int op); 
    // see "Selection Handling" in .cpp
    
  virtual void	UpdateUi(); 
    // Clipboard server: called by cliphandler after major events, to refresh menus, toolbars, etc.

#ifndef __MAKETA__
  void		globalUrlHandler(const QUrl& url);
    // root browser handles the pdp:// urls that point to internal objects -- routed

signals:
  void 		EditAction(int ea); 
    // Clipboard server: param is one of the taiClipData editAction values
  void		GetEditActionsEnabled(int& ea); 
    // Clipboard server: param is one of the taiClipData EditAction values
  void		SetActionsEnabled(); 
    // Clipboard server: enable/disable actions
    
  void		SelectableHostNotifySignal(ISelectableHost* src_host, int op); 
    // see "Selection Handling" in .cpp
#endif

public: // IDataViewWidget i/f
  override bool		isDirty() const;
  override QWidget*	widget() {return this;}
  override void		SaveData();
protected:
  override void		Constr_impl();
  override void 	ResolveChanges_impl(CancelOp& cancel_op); // only for project browsers
  override void		Refresh_impl(); 

protected slots:
  void			ch_destroyed(); // cliphandler destroyed (just in case it doesn't deregister)

  virtual void 		this_FrameSelect(taiAction* me); // user has selected or unselected one of the frames
  virtual void 		this_ToolBarSelect(taiAction* me); // user has selected or unselected one of the toolbars
  virtual void 		this_DockSelect(taiAction* me); // user has selected or unselected one of the docks

protected:
  static int		s_next_unique_id;
  
  bool			m_is_root; // true if this is a root window (has Quit menu)
  bool			m_is_proj_browser; // true if this is a project browser (false for simple browsers)
  bool			m_is_proj_viewer; // true if this is a project viewer
  int			m_unique_id;
  override void 	closeEvent(QCloseEvent* ev);
  bool			event(QEvent* ev);
  override void 	resizeEvent(QResizeEvent* ev);
  override void 	moveEvent(QMoveEvent* ev);
  virtual void 		emit_EditAction(int param); // #IGNORE param is one of the taiClipData editAction values; desc can trap this and implement virtually, if desired
  override void 	windowActivationChange(bool oldActive); // we manage active_wins in order of activation

  virtual void		Constr_MainMenu_impl(); // #IGNORE constructs the main menu items, and loads static images
  virtual void		Constr_Menu_impl(); // #IGNORE constructs the menu and actions; MUST construct all static actions
  virtual void		SelectableHostNotifying_impl(ISelectableHost* src_host, int op);
    // called when we should handle this for sure

#ifndef _MAKETA__
private:
  QObject* last_clip_handler; //we need to remember this, because you can't anonymously disconnect signals from your own slots
  ISelectableHost* last_sel_server; // last guy to get focus
  void			Init();
#endif
};


//////////////////////////
// 	iTabBar 	//
//////////////////////////

class TA_API iTabBar: public QTabBar { //  encapsulates the TabBar for iTabView
  Q_OBJECT
INHERITED(QTabBar)
public:
  enum TabIcon {
    TI_NONE		= -1,
    TI_UNPINNED,
    TI_PINNED,
    TI_LOCKED
  };
  
  static QIcon*		tab_icon[TI_LOCKED + 1]; // 0=unpinned, 1=pinned
  
  static void		InitClass(); // auto executed
  
  iDataPanel*		panel(int tab_idx); // #IGNORE gets the current panel, if any
  iTabView*		tabView() {return (iTabView*)parent();} // #IGNORE

#ifndef __MAKETA__
  using			QTabBar::addTab; // bring also into scope
#endif
  int			addTab(iDataPanel* panel); //#IGNORE puts at end if locked else inserts at end of unlocked
  void 			setTabIcon(int idx, TabIcon ti);
  void			SetPanel(int idx, iDataPanel* value, bool force = false); //#IGNORE set or remove (NULL) a panel
  
  iTabBar(iTabView* parent_ = NULL);
  ~iTabBar();
protected:
//  QColor	defBackgroundColor; // for when tabpanel doesn't provide one
//  QColor	defBaseColor; // for when tabpanel doesn't provide one
  QPalette	defPalette;
  override void contextMenuEvent(QContextMenuEvent * e);
  override void mousePressEvent(QMouseEvent* e);
//  override void paint(QPainter* p, QTab* t, bool selected ) const;
};


//////////////////////////
//   iDataPanel_PtrList	//
//////////////////////////

class TA_API iDataPanel_PtrList: public taPtrList<iDataPanel> { // ##NO_INSTANCE ##NO_TOKENS ##NO_CSS ##NO_MEMBERS
public:
  iTabView*	m_tabView; // optional, we manage the refs in panels if set
  iDataPanel_PtrList() {m_tabView = NULL;}
  ~iDataPanel_PtrList() {}
protected:
  override void*	El_Own_(void* it); // set ref to our tabview (on Add, not Link)	
  override void		El_disOwn_(void* it_); // remove ref if it is our tabview	
};


// NOTE: pan_idx is of panelCount and panel()
//       tab_idx is of tabCount and tabPanel()
class TA_API iTabView: public QWidget {
// ##NO_INSTANCE ##NO_TOKENS ##NO_CSS ##NO_MEMBERS  control for managing tabbed data pages
  Q_OBJECT
friend class iTabBar;
friend class iTabView_PtrList;
friend class iDataPanel;
public:
  QVBoxLayout*		layDetail;
    iTabBar*		tbPanels; //note: we always maintain at least one tab
    QStackedWidget*	wsPanels; //
  
  bool			autoCommit() const {return true;} // if we should autocommit dirty panels when browsing (NOTE: currently always true, could be made a user option)
  iDataPanel*		curPanel() const; // currently active panel
  int			panelCount() const; // total number of panels (not just in tabs)
  iDataPanel*		panel(int pan_idx = 0); // implementation-independent way to access panels
  int			tabCount() const; // number of tabs
  iDataPanel*		tabPanel(int tab_idx); // panel from indicated tab (can be NULL)
  taiDataLink*		par_link() const {return (m_viewer_win) ? m_viewer_win->sel_link() : NULL;}
  MemberDef*		par_md() const {return (m_viewer_win) ? m_viewer_win->sel_md() : NULL;}
  iTabViewer* 		tabViewerWin() {return m_viewer_win;}
  iMainWindowViewer* 	viewerWindow() {return (m_viewer_win) ? m_viewer_win->viewerWindow() : NULL;}

  void			Activated(bool val); // called by parent to indicate if we are active tabview or not
  bool 			ActivatePanel(taiDataLink* dl); // if a panel exists for the link, make it active and return true
  bool			AddPanel(iDataPanel* panel); // adds a panel if not already, true if newly added
  void			AddPanelNewTab(iDataPanel* panel, bool lock = false); // adds a panel in a new tab
  void 			Closing(CancelOp& cancel_op);
  void 			DataPanelDestroying(iDataPanel* panel);
  void			FillTabBarContextMenu(QMenu* contextMenu, int tab_idx = -1); 
  iDataPanel*		GetDataPanel(taiDataLink* link); // get panel for indicated link, or make new one; par_link is not necessarily data item owner (ex. link lists, references, etc.)
  void 			RemoveDataPanel(iDataPanel* panel);
  void			Refresh(); // manually refresh; just delegates to all
  void			ResolveChanges(CancelOp& cancel_op);
  void			OnWindowBind(iTabViewer* itv); // called at constr_post time
  void 			ShowLink(taiDataLink* link, bool not_in_cur = false);
  void			ShowPanel(iDataPanel* panel, bool not_in_cur = false); // top level guy, checks if exists, adds or sets current; if not_in_cur then won't replace current tab
  bool			SetCurrentTab(int tab_idx); 
  // focus indicated tab, but usually not if current is lockInPlace -- returns success
  bool			SetCurrentTabName(const String& tab_nm); 
  // focus indicated tab, but usually not if current is lockInPlace -- returns success
  void			ShowTab(iDataPanel* panel, bool show, bool focus = false); // show/hide tab, esp for ctrl panel in visible frame
  int			TabIndexOfPanel(iDataPanel* panel) const; // or -1 if not showing in a tab
  int			TabIndexByName(const String& nm) const;
  void 			UpdateTabName(iDataPanel* pan); // called only by individual panel when its name may have changed
  void			GetWinState();
  void			SetWinState();
  
  iTabView(QWidget* parent = NULL);
  iTabView(iTabViewer* data_viewer_, QWidget* parent = NULL);
  ~iTabView();

public slots:
  void 			AddTab(int tab = -1);
  void 			CloseTab(int tab = -1);
  virtual void		panelSelected(int idx);
  void 			UpdateTabNames(); // called by a datalink when a tab name might have changed; panels also hook to this

protected:
  iTabViewer* 	m_viewer_win;

private:
  iDataPanel_PtrList	panels; // no external hanky-panky with this puppie
  void			Init();
};

//////////////////////////
//   iTabView_PtrList	//
//////////////////////////

class TA_API iTabView_PtrList: public taPtrList<iTabView> {
public:
  void			DataPanelDestroying(iDataPanel* panel); // dispatch to all
  iTabView_PtrList() {}
  ~iTabView_PtrList();
};


//////////////////////////
//   iDataPanel 	//
//////////////////////////

class TA_API iDataPanel: public QFrame, public IDataLinkClient {
  // ##NO_INSTANCE ##NO_TOKENS ##NO_CSS ##NO_MEMBERS interface for panels (note: don't inherit directly)
  Q_OBJECT
INHERITED(QFrame)
friend class taDataLink;
friend class iPanelTab;
friend class iDataPanel_PtrList;
friend class iDataPanelSetBase;
//friend class iDataPanelSet;
public:
#ifndef __MAKETA__
  enum CustomEventType { // note: just copied from taiDataHost, not all used
    CET_SHOW_PANEL	= QEvent::User + 1,  // to get panel to show 
    CET_SHOW_PANEL_FOCUS,  // to get panel to show/focus 
  };
#endif
  virtual QWidget*	centralWidget() const; // contents
  virtual void		setCentralWidget(QWidget* widg); // sets the contents
  void			setButtonsWidget(QWidget* widg); // is put at the bottom, not in a scroll
  virtual void		ClearDataPanelSet() {} // used for clearing by the set in its dtor
  virtual bool		dirty() {return HasChanged();}
    // true if panel should not be replaced, but a new panel should be opened for the new item
  virtual bool		lockInPlace() const {return false;}
    // true if panel should not be replaced 
  virtual String	panel_type() const {return _nilString;}
   //  this string is on the subpanel button for a panel (n/a to panelsets)
  virtual taiDataLink*	par_link() const = 0; // *current* visual parent link of this data panel; this could change dynamically, if a datapanel is shared across all referring instances, ex. link lists, references, etc. -- return NULL if unknown, not set, or not applicable -- controls things like clip enabling etc.
  virtual MemberDef*	par_md() const = 0; // as for par_link
  inline bool		pinned() const {return m_pinned;}
  inline bool		rendered() const {return m_rendered;}
  void			setPinned(bool value);
//  DataViewer*		viewer() {return (m_dps) ? m_dps->viewer() : m_tabView->viewer();}
  iTabBar::TabIcon	tabIcon() const;
  inline iTabView*	tabView() const {return m_tabView;} // tab view in which we are shown
  virtual void		setTabView(iTabView* value) {m_tabView = value;} // just set the value, no side effects
  virtual iTabViewer* 	tabViewerWin() const = 0;
  iMainWindowViewer* 	viewerWindow() {return (m_tabView) ? m_tabView->viewerWindow() : NULL;}
  virtual bool		isViewPanelFrame() const {return false;} // we group the vpf's to the right, all others to the left


  virtual void		AddedToPanelSet() {} // called when fully added to DataPanelSet
  virtual void		Closing(CancelOp& cancel_op) {} // called to notify panel is(forced==true)/wants(forced=false) to close -- set cancel 'true' (if not forced) to prevent
  virtual void		ClosePanel() = 0; // anyone can call this to get the panel to close (ex. edit panel contents are deleted externally)
  //NOTE: due to various versioning/compatibility reasons, the following 2 routines get replaced sometimes in subclasses, to implement more elaborate rules before dispatching their worker bee impls
  virtual void		InitPanel() {InitPanel_impl();} // called when creating ViewPanels
  virtual void		UpdatePanel(); // called when reshowing a panel, to insure latest data
  virtual const iColor	GetTabColor(bool selected, bool& ok) const 
    {ok = false; return iColor();} // special color for tab; NULL means use default
  virtual void		FrameShowing(bool showing, bool focus = false); // called esp by t3 frames when show/hide; lets us show hide the tabs

  virtual bool		HasChanged() {return HasChanged_impl();} // 'true' if user has unsaved changes -- used to autosave when browsing away etc.
  virtual bool		HasChanged_impl() {return false;} // this impl applies only to a single panel, not to sets
  virtual void		OnWindowBind(iTabViewer* itv);
    // called in post, when all windows are built
  virtual void		ResolveChanges(CancelOp& cancel_op);
  virtual void		Render(); // actually create content; override _impl; used to defer creation of button panels
  virtual String 	TabText() const; // text for the panel tab -- usually just the view_name of the curItem
  virtual void 		GetWinState(); // when saving view state
  virtual void		SetWinState(); // when showing, from view state

  iDataPanel(taiDataLink* dl_); //note: created with no parent -- later added to stack
  ~iDataPanel();

public slots:
    void		Pin() {setPinned(true);}
    void		Unpin() {setPinned(false);}

public: // IDataLinkClient interface
  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_iDataPanel;}
  override bool		ignoreDataChanged() const {return (!isVisible());}
  override void		DataDataChanged(taDataLink*, int dcr, void* op1, void* op2)
    {DataChanged_impl(dcr, op1, op2);} // called when the data item has changed, esp. ex lists and groups
  override void		DataLinkDestroying(taDataLink* dl) {} // called by DataLink when it is destroying --

protected:
  bool			m_pinned;
  bool			m_rendered; // set once rendered
  bool			show_req; // set when we send the SHOW_REQ event
  QVBoxLayout* 		layOuter; 
  QScrollArea*		scr; // central scrollview
  override void 	customEvent(QEvent* ev_);
  override void		hideEvent(QHideEvent* ev); // auto-apply
  override void		showEvent(QShowEvent* ev);
  virtual void		DataChanged_impl(int dcr, void* op1, void* op2); // tab name may have changed
  virtual void		OnWindowBind_impl(iTabViewer* itv) {}
  virtual void		Render_impl() {} // only called once, when content needs to be created
  virtual void		ResolveChanges_impl(CancelOp& cancel_op) {}
  
  virtual void		InitPanel_impl() {} 
  virtual void 		UpdatePanel_impl();
  virtual void 		GetWinState_impl() {} // when saving view state
  virtual void		SetWinState_impl() {} // when showing, from view state
protected slots:
  virtual void		FrameShowing_Async(bool focus); // we forward async from FS (only when true) as a useful hack to make sure all constr etc is done before doing it
  
private:
  iTabView*		m_tabView; // force access through accessors only
};


//////////////////////////
//   iDataPanelFrame 	//
//////////////////////////

class TA_API iDataPanelFrame: public iDataPanel {
  // interface for panel frames
  Q_OBJECT
INHERITED(iDataPanel)
friend class iDataPanelSet;
public:
  QWidget*		minibarCtrls() const {return m_minibar_ctrls;}
    //non-null if any have been added
  inline bool		hasMinibarCtrls() const {return (m_minibar_ctrls);}
  override taiDataLink*	par_link() const; // taken from dps if any, else from tabview
  override MemberDef*	par_md() const;
  override iTabViewer*	tabViewerWin() const;


  override void		ClearDataPanelSet() {m_dps = NULL;}
  override void		ClosePanel();
  override void		UpdatePanel(); // we add a test for HasChanged and invoke the inherited
  override String 	TabText() const; // text for the panel tab -- usually just the text of the sel_node

  void			AddMinibarWidget(QWidget* ctrl);
    // adds the ctrl (typically a tool button) to the minibar (area to right of PanelSet selector buttons); note: right-justified, and fills inward; ctrl should be parentless; can force the DPF to be put into a set, if wouldn't have been otherwise
  
  iDataPanelFrame(taiDataLink* dl_);
  ~iDataPanelFrame();

public: // IDataLinkClient interface
  override void*	This() {return (void*)this;}
  override void		DataLinkDestroying(taDataLink* dl); // called by DataLink when it is destroying --
  override TypeDef*	GetTypeDef() const {return &TA_iDataPanelFrame;}

protected:
  iDataPanelSet*	m_dps; // set if we are in a datapanelset
  QWidget*		m_minibar_ctrls;
  QHBoxLayout* 		lay_minibar_ctrls;
};

//////////////////////////
//   iViewPanelFrame 	//
//////////////////////////

class TA_API iViewPanelFrame: public iDataPanel, public virtual IDataHost {
  // frame for gui interface to a view element -- usually posted by the view, and locked
  // provides optional IDataHost and Apply/Revert services, so you can use taiData ctrls
  Q_OBJECT
INHERITED(iDataPanel)
friend class iViewPanelSet;
public:
  enum ViewPanelFlags { // #BITS misc flags
    VP_0		= 0, // #IGNORE
    VP_USE_BTNS		= 0x0001, // use the Apply/Revert buttons on bottom
  };
  
#ifndef __MAKETA__
  enum CustomEventType { // note: just copied from taiDataHost, not all used
    CET_RESHOW		= QEvent::User + 1,  // uses ReShowEvent
    CET_GET_IMAGE,
    CET_APPLY
  };
#endif

//QHBoxLayout*		layButtons;
  HiLightButton*	  btnApply; //note: only if created
  HiLightButton*	  btnRevert;
  
  int			vp_flags;
  bool			read_only; // set true if we are
  
  taDataView*		dv() {return m_dv;} // can be statically replaced with subclass
  override bool		lockInPlace() const {return true;}
    // true if panel should not be replaced, ex. if dirty, or viewpanel
  override taiDataLink*	par_link() const {return NULL;} // n/a
  override MemberDef*	par_md() const {return NULL;}
  override iTabViewer* tabViewerWin() const;
  override bool		isViewPanelFrame() const {return true;}


  override void		ClearDataPanelSet() {m_dps = NULL;}
  void			MakeButtons(QBoxLayout* par_lay = NULL, QWidget* par_widg = NULL);
    // make the Apply/Revert btns, par=this if NULL, if no lay, use par_widg
  override void		InitPanel(); // we do a more elaborate check for m_dv and !updating
  override void		UpdatePanel(); // ditto
  override void		ClosePanel();
  override String 	TabText() const; // text for the panel tab -- usually just the text of the sel_node

  iViewPanelFrame(taDataView* dv_);
    // NOTE: dv will be nulled out if it destroys
  ~iViewPanelFrame();

public slots:
  void			Apply();
  void			Revert();

public: // IDataLinkClient interface
  override void*	This() {return (void*)this;} //
  override void		DataLinkDestroying(taDataLink* dl); //note: dl is on the view, not underlying data
  override TypeDef*	GetTypeDef() const {return &TA_iViewPanelFrame;}

public: // IDataHost i/f -- some delegate up to mommy
  const iColor	 	colorOfCurRow() const; // #IGNORE probably not used, we just return our own bg color
  bool  		HasChanged() {return m_modified;}	
  bool			isConstructed() {return true;}
  bool			isModal() {return false;} // never for us
  bool			isReadOnly() {return read_only;}
  taMisc::ShowMembs 	show() const {return taMisc::show_gui;}
    // used by polydata
  iMainWindowViewer* 	window() const {return (tabView()) ? tabView()->viewerWindow() : NULL;}
  void*			Root() const {return (void*)m_dv;} // (typical, could replace)
  taBase*		Base() const {return m_dv;} // (typical, could replace)
  TypeDef*		GetRootTypeDef() const {return (m_dv) ? m_dv->GetTypeDef() : NULL;} // (could replace)
  void			GetValue(); // does setup, override the impl
  void			GetImage() {UpdatePanel();}
public slots:
  void			Changed(); // called by embedded item to indicate contents have changed
  void			Apply_Async();

protected:
  iViewPanelSet*	m_dps; // set if we are in a viewpanelset
  taDataView*		m_dv;
  int			updating; // #IGNORE >0 used to suppress update-related widget signals
  bool			m_modified;
  bool			apply_req;
  bool			warn_clobber; // set if we get a notify and are already modified
  
  override void 	customEvent(QEvent* ev_);
  
  virtual void		GetValue_impl() {}
  void			InternalSetModified(bool value); // does all the gui config
  override void 	ResolveChanges_impl(CancelOp& cancel_op); //generic behavior
  void 			UpdateButtons();
};

class TA_API iDataPanelSetBase: public iDataPanel { //  common subclass for iDataPanelSet and iViewPanelSet
  Q_OBJECT
INHERITED(iDataPanel)
public:
  QWidget* 		widg;
  QVBoxLayout*		layDetail;
  QStackedWidget*	  wsSubPanels; // subpanels -- note, built by descendents
  
  int			cur_panel_id; // -1 if none
  iDataPanel_PtrList	panels;
  
  override taiDataLink*	par_link() const {return (m_tabView) ? m_tabView->par_link() : NULL;}
  override MemberDef*	par_md() const {return (m_tabView) ? m_tabView->par_md() : NULL;}
  override iTabViewer* tabViewerWin() const {return (m_tabView) ? m_tabView->tabViewerWin() : NULL;}

  iDataPanel*		curPanel() const {return panels.SafeEl(cur_panel_id);} // NULL if none
  override void		setTabView(iTabView* tv); // set for kids too
  
  override void		Closing(CancelOp& cancel_op);
  override void		ClosePanel();
  iDataPanel*		GetDataPanelOfType(TypeDef* typ, int& start_idx);
    // get the first data panel of the specified type, starting at panel index; NULL if none
  override const iColor GetTabColor(bool selected, bool& ok) const;
  override bool		HasChanged();
  override void 	ResolveChanges(CancelOp& cancel_op); // do the children first, then our impl
  override void		UpdatePanel(); // iterate over all kiddies
  override void 	GetWinState(); // when saving view state
  override void		SetWinState(); // when showing, from view state

  iDataPanelSetBase(taiDataLink* dl_);
  ~iDataPanelSetBase();

public slots:
  void			setCurrentPanelId(int id);

public: // IDataLinkClient interface
  override void*	This() {return (void*)this;}
  override void		DataLinkDestroying(taDataLink* dl);
  override TypeDef*	GetTypeDef() const {return &TA_iDataPanelSetBase;}
  
protected:
  virtual void		setCurrentPanelId_impl(int id) {}
  void			removeChild(QObject* obj);
  override void		OnWindowBind_impl(iTabViewer* itv);
};

class TA_API iDataPanelSet: public iDataPanelSetBase { //  contains 0 or more sub-data-panels, and a small control bar for selecting panels
  Q_OBJECT
INHERITED(iDataPanelSetBase)
public:
//QVBoxLayout*		layDetail;
  QFrame*		  frmButtons;
  QHBoxLayout*		  layButtons;
  QButtonGroup*		    buttons; // one QPushButton for each (note: not a widget)
  QStackedLayout*	    layMinibar; // if any panels use the minibar, created

  void			setPanelAvailable(iDataPanel* pn); // dynamically show/hide a btn/pn

  void			SetMenu(QWidget* menu); // sets the menu (s/b a menubar; or toolbar on mac)
  void			AddSubPanel(iDataPanelFrame* pn);
  void			AllSubPanelsAdded(); // call after all subpanels added, to finalize layout
  void			AddSubPanelDynamic(iDataPanelFrame* pn); // call this after fully built to dynamically add a new frame
  void			SetMethodBox(QWidget* meths); // sets a box that contains methods, on bottom


  iDataPanelSet(taiDataLink* dl_);
  ~iDataPanelSet();

public: // IDataLinkClient interface
  override void*	This() {return (void*)this;}
//  override void		DataLinkDestroying(taDataLink* dl) {} // nothing for us; subpanels handle
  override TypeDef*	GetTypeDef() const {return &TA_iDataPanelSet;}
protected:
  override void		DataChanged_impl(int dcr, void* op1, void* op2); // dyn subpanel detection

protected:
  override void		setCurrentPanelId_impl(int id);
  void 			AddMinibar();
  void 			AddMinibarCtrls();
  void			AddMinibarCtrl(iDataPanelFrame* pn);
};


class TA_API iViewPanelSet: public iDataPanelSetBase { //  contains 0 or more sub-view-panels, and btm hor tab for selecting panels
  Q_OBJECT
INHERITED(iDataPanelSetBase)
public:
  QTabBar*		  tbSubPanels; 

  override bool		lockInPlace() const {return true;}
  
  void			AddSubPanel(iViewPanelFrame* pn);
  
  override void		UpdatePanel(); // update tab names too
  void			PanelDestroying(iViewPanelFrame* pn); // so we remove tab

  iViewPanelSet(taiDataLink* dl_);
  ~iViewPanelSet();

public: // IDataLinkClient interface
  override void*	This() {return (void*)this;}
//  override void		DataLinkDestroying(taDataLink* dl) {} // nothing for us; subpanels handle
  override TypeDef*	GetTypeDef() const {return &TA_iViewPanelSet;}

protected:
  override void		setCurrentPanelId_impl(int id);
};


//////////////////////////
//   iListDataPanel 	//
//////////////////////////

class TA_API iListDataPanel: public iDataPanelFrame {
  Q_OBJECT
#ifndef __MAKETA__
typedef iDataPanelFrame inherited;
#endif
public:
  iTreeView*		list; //actually an iLDPListView

  override String	panel_type() const; // this string is on the subpanel button for this panel
  void			ClearList(); // for when data changes -- we just rebuild the list
  void			FillList();
  void			RenumberList();
  
  iListDataPanel(taiDataLink* dl_, const String& custom_name = _nilString);
  ~iListDataPanel();

public: // IDataLinkClient interface
  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_iListDataPanel;}
  
public slots:
  void			list_itemDoubleClicked(QTreeWidgetItem* item, int col);
    // for editing
    
protected:
  String		m_custom_name; // used instead of "List View", typically for defchild lists
  void 			ConfigHeader();
  override void		DataChanged_impl(int dcr, void* op1, void* op2); //
  override void		UpdatePanel_impl();  
  override void		OnWindowBind_impl(iTabViewer* itv);
};

class TA_API iTextDataPanel: public iDataPanelFrame {
  // a panel frame for displaying text; used, ex. by Scripts and Programs
  Q_OBJECT
INHERITED(iDataPanelFrame)
public:
  QTextEdit*		txtText; // the text of the script
  
  virtual bool		readOnly();
  virtual void		setReadOnly(bool value);
  virtual void		setText(const String& value);
  

  override String	panel_type() const;

  override int 		EditAction(int ea);
  override int		GetEditActions(); // after a change in selection, update the available edit actions (cut, copy, etc.)

  iTextDataPanel(taiDataLink* dl_);
  ~iTextDataPanel();

public: // IDataLinkClient interface
  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_iTextDataPanel;}
protected:
  override void		DataChanged_impl(int dcr, void* op1, void* op2); //
//  override int 		EditAction_impl(taiMimeSource* ms, int ea, ISelectable* single_sel_node = NULL);

protected slots:
  void 			textText_copyAvailable (bool yes);

};

class TA_API iDocDataPanel: public iDataPanelFrame {
  // a panel frame for displaying docs
  Q_OBJECT
INHERITED(iDataPanelFrame)
public:
  QTextBrowser*		br; // the doc
  
  virtual void		setDoc(taDoc* doc); // only called if changes after creation
  
  taDoc*		doc() {return (m_link) ? (taDoc*)(link()->data()) : NULL;}
  override String	panel_type() const {return "Doc";}

//  override int 		EditAction(int ea);
//  override int		GetEditActions(); // after a change in selection, update the available edit actions (cut, copy, etc.)

  iDocDataPanel(); // NOTE: use the setDoc api to (indirectly) set the DataLink
  ~iDocDataPanel();

public: // IDataLinkClient interface
  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_iDocDataPanel;}
  override void		DataLinkDestroying(taDataLink* dl);
  override bool		ignoreDataChanged() const;
protected:
  taDoc*		m_doc; // ref managed through link; we just put ptr here to detect change
  override void		DataChanged_impl(int dcr, void* op1, void* op2); //

#ifndef __MAKETA__
protected slots:
  void			doc_setSourceRequest(iTextBrowser* src,
					     const QUrl& url, bool& cancel);
//  void 			br_copyAvailable (bool yes);
#endif

};

#ifndef __MAKETA__
typedef QMap<QString, QVariant> QMap_qstr_qvar; // the QMap type that QVariant supports
// maps are always refcounted in Qt, similar to QString, so you pass them by value
#endif

class TA_API iTreeView: public iTreeWidget, public ISelectableHost {
  //  ##NO_INSTANCE ##NO_TOKENS ##NO_CSS ##NO_MEMBERS base class for all views of iTreeViewItems
INHERITED(iTreeWidget)
  Q_OBJECT
friend class iTreeViewItem;
public:
#ifndef __MAKETA__
  enum Roles { // extra roles, for additional data, etc.
    ObjDataRole = Qt::UserRole + 1, // for additional data
    ColKeyRole,	// store a string in header to indicate the col key to use for data
    ColDataRole, // store a QString::QVariant map of colkey/Role numbers to add addtl data to nodes; ex, "desc":Qt::ToolTipRole for tooltip text for the col "desc"
    ColFormatRole, // stores values from the ColFormatFlags enum
    MaxColCharsRole // store an int of max col width (in chars), we elide text to that length
  };
#endif
  enum ColFormatFlags { // used to tweak formatting/display of cols
    CF_ELIDE_TO_FIRST_LINE	= 0x0001 // elide to first line, so row doesn't multi
  };
  
  enum TreeViewFlags { // #BITS
    TV_NONE			= 0, // #NO_BIT
    TV_AUTO_EXPAND		= 0x0001, // invokes DefaultExpand automatically on open
    TV_AUTO_EXPANDED		= 0x0002, // #IGNORE flag marks when done first time
    TV_EXPAND_DISABLED 	= 0x0004, // expands nested disabled items (usually they are not expanded unless you expand from the item itself)
    TV_NO_AUTO_RESIZE	= 0x0008 // when we autoexpand etc. doesn't resize cols
  };
  enum ContextMenuPosition {
    CM_START,		// called before filling of menu -- use to add items to start
    CM_END		// called after filling menu -- use to add items to end
  };
  
  static const String  	opt_treefilt; // "TREEFILT_"

#ifndef __MAKETA__  
  static void 		FillTypedList(const QList<QTreeWidgetItem*>& items,
    ISelectable_PtrList& list); // helper, for filling our own typed list
    
  QMap_qstr_qvar	colDataKeys(int col) const;
    // the map of role/key pairs, or an empty map if none
#endif
  
  bool			useCustomExpand() const;
  bool			doubleClickExpandsAll() const; // use at own risk...
  const KeyString	colKey(int col) const; // the key we set for data lookup
  void			setColKey(int col, const KeyString& key); 
    // sets in ColKeyRole -- you can do it yourself if you want	
  int 			colFormat(int col);
  void			setColFormat(int col, int format_flags); 
  bool			decorateEnabled() const {return m_decorate_enabled;}
  void			setDecorateEnabled(bool value); //note: must be done at create time
  int			defaultExpandLevels() const {return m_def_exp_levels;}
    // how many levels the DefaultExpand expands
  void			setDefaultExpandLevels(int value) 
    {m_def_exp_levels = (int)value;}
  iTreeViewItem*	item(int i) const; // item at i, NULL if out of range
  inline int		itemCount() const {return topLevelItemCount();}
  void			setHeaderText(int col, const String& value); // convenience
  int 			maxColChars(int col); // value if set, -1 otherwise
  void			setMaxColChars(int col, int value); // sets max number of chars for that text (when retrieved from the link); elided if greater
  taMisc::ShowMembs	show() const {return m_show;} 
  virtual void		setShow(taMisc::ShowMembs value); 
  const String		showContext() const {return m_show_context;} 
  virtual void		setShowContext(const String& value)
    {m_show_context = value;}
   // ONLY SET DURING CREATE -- THIS IS NOT DYNAMIC

  inline TreeViewFlags	tvFlags() const {return (TreeViewFlags)tv_flags;}
  void			setTvFlags(int value);
  
  void			AddFilter(const String& value);
    // add a TREEFILT_xxx expression to exclude members and/or types; note: not dynamic, must be added before items created
  iTreeViewItem* 	AssertItem(taiDataLink* link, bool super = true);
    // insures that the item for the link exists; returns NULL if it doesn't exist/couldn't be assertedtaMisc::
  bool			HasFilter(TypeItem* ti) const;
    // true if the typeitem has a TREEFILT_xxx filter that was added to our list
    
  void			AddColDataKey(int col, const KeyString& key, int role);
    // sets in ColDataRole, ex for a tooltip text or font for the col
  bool			RemoveColDataKey(int col, const KeyString& key, int role);
    // removes ColDataRole; true if it was there
  void			ClearColDataKeys(int col);
    // clears all the ColDataKeys in the col (provided for completeness)
      
  virtual void		Refresh() {Refresh_impl();} // manually refresh
  virtual bool		ShowNode(iTreeViewItem* item) const;
    // whether the node is visible in this show context
#ifndef __MAKETA__
  void			scrollTo(QTreeWidgetItem* item, ScrollHint hint = EnsureVisible);
    // convenience static override, to work directly with items
#endif  
  
  iTreeView(QWidget* parent = 0, int tv_flags = 0);
  ~iTreeView();
   
#ifndef __MAKETA__
signals:
  void			CustomExpandFilter(iTreeViewItem* item, int level, bool& expand);
    // invoked when we want our mummy to do custom filtering, expand=true by default
  void			FillContextMenuHookPre(ISelectable_PtrList& sel_items,
     taiActions* menu);
    // hook to allow client to add items to start of context menu before it shows
  void			FillContextMenuHookPost(ISelectable_PtrList& sel_items,
     taiActions* menu);
    // hook to allow client to add items to end of context menu before it shows
  void			ItemSelected(iTreeViewItem* item); 
    // NULL if none -- NOTE: the preferred way is to use ISelectableHost::Notify signal
#endif
  
public slots:
  void			mnuFindFromHere(taiAction* mel); // called from context 'Find from here'; cast obj to iTreeViewItem*
  void			ExpandDefault(); 
    // expand to the default level specified for this tree, or invokes CustomExpand if set 
  void			ExpandAll(int max_levels = 6); 
    // expand all nodes, ml=-1 for "infinite" levels (there better not be any loops!!!)
  void			CollapseAll(); // collapse all nodes
  void			ExpandAllUnder(iTreeViewItem* item, int max_levels = 6); 
    // expand all nodes under item, ml=-1 for "infinite" levels (there better not be any loops!!!)
  void			CollapseAllUnder(iTreeViewItem* item); // collapse all nodes under item

public: // ISelectableHost i/f
  override bool 	hasMultiSelect() const;
  override QWidget*	widget() {return this;} 
protected:
  override void		FillContextMenu_pre(ISelectable_PtrList& sel_items,
    taiActions* menu);
  override void		FillContextMenu_post(ISelectable_PtrList& sel_items,
    taiActions* menu); 
  
  override void		UpdateSelectedItems_impl(); 
  
protected:
  enum ExpandFlags {
    EF_CUSTOM_FILTER		= 0x01,
    EF_DEFAULT			= 0x02, // we are in the DefaultExpand context
    EF_EXPAND_DISABLED		= 0x04 // either Expand on that guy, or set in flags
  };
  int			tv_flags;
  String_PArray* 	m_filters; // only created if any added
  short			m_def_exp_levels; // level of default expand, typically 2
  taMisc::ShowMembs 	m_show;
  bool			m_decorate_enabled;
  String		m_show_context;
  int			in_mouse_press; // ugly hack
  
  void 			focusInEvent(QFocusEvent* ev); // override
  QFont&		italicFont() const; // so we don't create a new guy each node
  void 			mousePressEvent(QMouseEvent* ev); //for exp/coll all
  void 			mouseDoubleClickEvent(QMouseEvent* ev); //for exp/coll all
  void			showEvent(QShowEvent* ev); // override, for expand all
  void 			ExpandAll_impl(int max_levels,
    bool use_custom_filt = false); // inner code
  void 			ExpandItem_impl(iTreeViewItem* item,
     int level, int max_levels, int exp_flags = 0); // inner code; level=-1 when not known
  void			GetSelectedItems(ISelectable_PtrList& lst); // list of the selected datanodes
#ifndef __MAKETA__
  override QMimeData* 	mimeData(const QList<QTreeWidgetItem*> items) const; 
    // we replace this and provide the ta custom mime data (not the treewidget data)
  override QStringList 	mimeTypes () const; // for dnd to work, we just permit almost anything via "text/plain", then decide on the drop whether to accept  
#endif
  virtual void		ItemDestroyingCb(iTreeViewItem* item); 
  virtual void		Refresh_impl();
  virtual void		Show_impl();
  
protected slots:
  void 			this_contextMenuRequested(QTreeWidgetItem* item,
    const QPoint & pos, int col ); //note: should probably rejig to use a virtual method
  void			this_currentItemChanged(QTreeWidgetItem* curr, QTreeWidgetItem* prev);
  void 			this_itemSelectionChanged();
  void			ExpandAllUnderInt(void* item); 
  void			CollapseAllUnderInt(void* item); 
private:
  mutable QFont*	italic_font;
};


class TA_API iTreeViewItem: public iTreeWidgetItem, 
  public virtual IDataLinkClient, public virtual IObjectSelectable {
  //  ##NO_TOKENS ##NO_CSS ##NO_MEMBERS base class for Tree and List nodes
INHERITED(iTreeWidgetItem)
friend class iTreeView;
public:
  enum DataNodeFlags {
    DNF_IS_FOLDER 	= 0x001, // true for list/group folder nodes (note: does *not* indicate whether item can contain other things or not)
    DNF_LAZY_CHILDREN 	= 0x002, // start w/ dummy child; create real children when expanded
    DNF_UPDATE_NAME	= 0x004, // typically for list items, update the visual name (tree, tab, etc.) after item edited
    DNF_CAN_BROWSE	= 0x008, // can be a new browser root
    DNF_CAN_DRAG	= 0x010, // 16 can allow drags
    DNF_NO_CAN_DROP	= 0x020, // 32 cannot accept drops
    DNF_IS_MEMBER 	= 0x040, // 64 true for members (and root), not for list/group items -- helps node configure edit ops
    DNF_IS_LIST_NODE 	= 0x080, // true for nodes in a list view (in panel, not on tree)
    DNF_IS_LINK 	= 0x100, // true for nodes that are links, not owned by parent
    DNF_NO_UPDATE_NAME	= 0x200 // for root items that don't have a md, we autoset UPDATE_NAME unless this flag is set
  };

  int			dn_flags; // any of DataNodeFlags
  String		given_name; // name given to the obj during init or setName -- used as default for col0

  override bool 	canAcceptDrop(const QMimeData* mime) const;
  void*			linkData() const;
  virtual void		setName(const String& value); // is the first col, except for lists, which is 2nd -- note: only applicable for updates, not in constructor
  iTreeViewItem*	parent() const; // strongly typed version of base
  iTreeView*		treeView() const;

  virtual const String	GetColText(int col, const String& def = _nilString) const;
  
  override void 	CreateChildren(); 
  void			DataChanged(int dcr, void* op1, void* op2)
    {DataChanged_impl(dcr, op1, op2);} // primarily to support Refresh
  virtual void		DecorateDataNode(); // sets icon and other visual attributes, based on state of node
  bool			ShowNode(int show) const 
    {return ShowNode_impl(show, _nilString);}
    // whether to show the node, given the context
  bool			ShowNode(int show, const String& context) const
    {return ShowNode_impl(show, context);}

  iTreeViewItem(taiDataLink* link_, MemberDef* md_, iTreeViewItem* parent_,
    iTreeViewItem* after, const String& tree_name, int dn_flags_ = 0);
  iTreeViewItem(taiDataLink* link_, MemberDef* md_, iTreeView* parent_,
    iTreeViewItem* after, const String& tree_name, int dn_flags_ = 0);
  ~iTreeViewItem();
  
public: // qt3 compatability functions, for convenience
  bool			dragEnabled() const {return flags() & Qt::ItemIsDragEnabled;}
  void			setDragEnabled(bool value) {Qt::ItemFlags f = flags(); if (value) 
    setFlags(f | Qt::ItemIsDragEnabled); else setFlags(f & ~Qt::ItemIsDragEnabled);}
    
  bool			dropEnabled() const {return flags() & Qt::ItemIsDropEnabled;}
  void			setDropEnabled(bool value) {Qt::ItemFlags f = flags(); if (value) 
    setFlags(f | Qt::ItemIsDropEnabled); else setFlags(f & ~Qt::ItemIsDropEnabled);}
    
  void			moveChild(int fm_idx, int to_idx); //note: to_idx is based on before
  void			swapChildren(int n1_idx, int n2_idx);

public: // ITypedObject interface
  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_iTreeViewItem;}

public: // IDataLinkClient interface
  override void		DataDataChanged(taDataLink*, int dcr, void* op1, void* op2)
    {DataChanged(dcr, op1, op2);} // called when the data item has changed, esp. ex lists and groups
  override void		DataLinkDestroying(taDataLink* dl); // called by DataLink when it is destroying --

public: // ISelectable interface
  override taiDataLink*	link() const {return IDataLinkClient::link();}
  override MemberDef*	md() const {return m_md;}
  override ISelectable*	par() const;
//obs  override String	view_name() const; // for members, the member name; for list items, the name if
  override ISelectableHost* host() const;
//  override taiClipData*	GetClipData(int src_edit_action, bool for_drag);
//  override int		GetEditActions(taiMimeSource* ms) const; // simpler version uses Query
protected:
//  override int		EditAction_impl(taiMimeSource* ms, int ea);
//  override void		FillContextMenu_EditItems_impl(taiActions* menu, int allowed);
  override void		FillContextMenu_impl(taiActions* menu,
    GuiContext sh_typ); // this is the one to extend in inherited classes
  override void		QueryEditActionsS_impl_(int& allowed, int& forbidden,
    GuiContext sh_typ) const;  // OR's in allowed; OR's in forbidden
    
#ifndef __MAKETA__ 
protected:
  MemberDef*		m_md; // for members, the MemberDef (otherwise NULL)
  override void		dropped(const QMimeData* mime, const QPoint& pos, 
    int key_mods, WhereIndicator where);
  virtual void		DataChanged_impl(int dcr, void* op1, void* op2); // called for each node when the data item has changed, esp. ex lists and groups
  override void 	itemExpanded(bool value);
  virtual bool		ShowNode_impl(int show, const String& context) const;
private:
  void			init(const String& tree_name, taiDataLink* link_, 
    MemberDef* md_, int dn_flags_); // #IGNORE
#endif
};


class TA_API taiListDataNode: public iTreeViewItem {
INHERITED(iTreeViewItem)
public:
  int			num; // item number, starting from 0
  iListDataPanel*	panel; // logical parent node of the list items

  QString		text(int col) const; // override
  override void		setName(const String& value); // 2nd col for us

  bool 			operator<(const QTreeWidgetItem& item) const; // override

  override void		DecorateDataNode();
  taiListDataNode(int num_, iListDataPanel* panel_, taiDataLink* link_,
    iTreeView* parent_, taiListDataNode* after, int dn_flags_ = 0);
    //note: list flag automatically or'ed in
  ~taiListDataNode(); //
  
public: // IDataLinkClient interface
//  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_taiListDataNode;}

public: // ISelectable interface
  override taiDataLink* par_link() const; // we get from the panel, which gets from the viewer window
//obs  override MemberDef* 	par_md() const; // as for par_link
};


class TA_API taiTreeDataNode: public iTreeViewItem {
INHERITED(iTreeViewItem)
public:
  taiTreeDataNode*	parent() const {return (taiTreeDataNode*) QTreeWidgetItem::parent();} //note: NULL for root item

  taiTreeDataNode*	FindChildForData(void* data, int& idx = no_idx); // find the Child Node (if any) that has data as the data of its link; NULL/-1 if not found
  virtual void		UpdateChildNames() {} // #IGNORE update child names of this node

  taiTreeDataNode(taiDataLink* link_, MemberDef* md_, taiTreeDataNode* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  taiTreeDataNode(taiDataLink* link_, MemberDef* md_, iTreeView* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  ~taiTreeDataNode();
public: // ITypedObject interface
//  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_taiTreeDataNode;}
public: // ISelectable interface
//obs  override taiDataLink* par_link() const; 
//obs  override MemberDef* 	par_md() const; //provided at create time
//  override ISelectableHost* host() const;
protected:
  taiTreeDataNode*	last_member_node; // #IGNORE last member node created, so we know where to start list/group items
  taiTreeDataNode*	last_child_node; // #IGNORE last child node created, so we can pass to createnode
  override void 	CreateChildren_impl();
private:
  static int		no_idx; // dummy parameter
  void			init(taiDataLink* link_, int dn_flags_); // #IGNORE
};


class TA_API tabTreeDataNode: public taiTreeDataNode {
INHERITED(taiTreeDataNode)
public:
  taBase* 		data() {return ((tabDataLink*)m_link)->data();}
  tabDataLink* 		link() const {return (tabDataLink*)m_link;}

  tabTreeDataNode(tabDataLink* link_, MemberDef* md_, taiTreeDataNode* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  tabTreeDataNode(tabDataLink* link_, MemberDef* md_, iTreeView* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  ~tabTreeDataNode();
public: // IDataLinkClient interface
//  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_tabTreeDataNode;}
  
private:
  void			init(tabDataLink* link_, int dn_flags_); // #IGNORE
};


class TA_API tabParTreeDataNode: public tabTreeDataNode {
INHERITED(tabTreeDataNode)
public:
  taList_impl* 		list() {return ((tabODataLink*)m_link)->list();}
//nuke  tabParDataLink* 	link() const {return (tabParDataLink*)m_link;}

  void			AssertLastListItem(); // #IGNORE updates last_list_items_node -- called by Group node before dynamic inserts/updates etc.
  override void		UpdateChildNames(); // #IGNORE update child names of the indicated node

  tabParTreeDataNode(tabODataLink* link_, MemberDef* md_, taiTreeDataNode* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  tabParTreeDataNode(tabODataLink* link_, MemberDef* md_, iTreeView* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  ~tabParTreeDataNode();
public: // IDataLinkClient interface
//  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_tabParTreeDataNode;}
protected:
  taiTreeDataNode*	last_list_items_node; // #IGNORE last list member node created, so we know where to start group items
  override void		DataChanged_impl(int dcr, void* op1, void* op2);
  override void 	CreateChildren_impl(); 
  taiTreeDataNode*	CreateListItem(taiTreeDataNode* par_node,
    taiTreeDataNode* after, taBase* el);
  void			UpdateListNames(); // #IGNORE updates names after inserts/deletes etc.
private:
  void			init(tabODataLink* link_, int dn_flags_); // #IGNORE
};


class TA_API tabDefChildRef: protected IDataLinkClient { // ##NO_INSTANCE ##NO_TOKENS "safe" reference for taList objects -- does not ref count, but is a dlc so it tracks changes etc.
public:
  inline taList_impl*	ptr() const {return m_ptr;}
  void			set(taList_impl* src) {if (src == m_ptr) return;
    if (m_ptr) m_ptr->RemoveDataClient(this); m_ptr = NULL;
    if (src && src->AddDataClient(this)) m_ptr = src;} 
  
  void			Init(tabDefChildTreeDataNode* own_, taList_impl* ptr_)
    {m_own = own_; set(ptr_);}
  tabDefChildRef() {m_own = NULL; m_ptr = NULL;}
  ~tabDefChildRef() {set(NULL);}
  
protected:
  tabDefChildTreeDataNode*	m_own; 
  mutable taList_impl*	m_ptr;
  
public: // ITypedObject interface
  override void*	This() {return (void*)this;} //
  override TypeDef*	GetTypeDef() const {return &TA_tabDefChildRef;}

public: // IDataLinkClient interface
  override TypeDef*	GetDataTypeDef() const 
    {return (m_ptr) ? m_ptr->GetTypeDef() : &TA_taList_impl;}
  override void		DataDataChanged(taDataLink*, int dcr, void* op1, void* op2);
  override void		DataLinkDestroying(taDataLink* dl);
};


class TA_API tabDefChildTreeDataNode: public tabParTreeDataNode {
INHERITED(tabParTreeDataNode)
friend class tabDefChildRef;
public:
  tabDefChildTreeDataNode(tabODataLink* link_, MemberDef* md_,
    taiTreeDataNode* parent_, taiTreeDataNode* after,
    const String& tree_name, int dn_flags_ = 0);
  tabDefChildTreeDataNode(tabODataLink* link_, MemberDef* md_,
    iTreeView* parent_, taiTreeDataNode* after,
    const String& tree_name, int dn_flags_ = 0);
  ~tabDefChildTreeDataNode();
public: // IDataLinkClient interface
//  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_tabDefChildTreeDataNode;}
protected:
  tabDefChildRef	m_def_child; // for list notifies
  virtual void		DefChild_DataChanged(int dcr, void* op1, void* op2);
private:
  void			init(tabODataLink* link_, int dn_flags_); // #IGNORE
};


class TA_API tabListTreeDataNode: public tabParTreeDataNode {
INHERITED(tabParTreeDataNode)
public:
  tabListTreeDataNode(tabListDataLink* link_, MemberDef* md_, taiTreeDataNode* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  tabListTreeDataNode(tabListDataLink* link_, MemberDef* md_, iTreeView* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  ~tabListTreeDataNode();
public: // IDataLinkClient interface
//  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_tabListTreeDataNode;}
private:
  void			init(tabListDataLink* link_, int dn_flags_); // #IGNORE
};


class TA_API tabGroupTreeDataNode: public tabListTreeDataNode {
INHERITED(tabListTreeDataNode)
public:
  taGroup_impl* 	data() {return ((tabGroupDataLink*)m_link)->data();}
  tabGroupDataLink* 	link() const {return (tabGroupDataLink*)m_link;}

  taiTreeDataNode*	CreateSubGroup(taiTreeDataNode* after, void* el); 
    // for dynamic changes to tree
  override void		UpdateChildNames(); // #IGNORE

  tabGroupTreeDataNode(tabGroupDataLink* link_, MemberDef* md_, taiTreeDataNode* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  tabGroupTreeDataNode(tabGroupDataLink* link_, MemberDef* md_, iTreeView* parent_,
    taiTreeDataNode* after, const String& tree_name, int dn_flags_ = 0);
  ~tabGroupTreeDataNode();
public: // IDataLinkClient interface
//  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_tabGroupTreeDataNode;}
protected:
  override void 	CreateChildren_impl(); 
  override void		DataChanged_impl(int dcr, void* op1, void* op2); // handle DCR_GROUP_xxx ops
  void			UpdateGroupNames(); // #IGNORE updates names after inserts/deletes etc.
private:
  void			init(tabGroupDataLink* link_, int dn_flags_); // #IGNORE
};

class TA_API iSearchDialog: public QDialog, public IDataLinkClient {
//   search a project (or more)
INHERITED(QDialog)
  Q_OBJECT
public:
#ifndef __MAKETA__
  enum Roles { // extra roles, for additional data, etc.
    ObjUrlRole = Qt::UserRole + 1, // Url stored in this
//    ObjCatRole  // for object category string, whether shown or not
  };
#endif

  enum SearchOptions { // #BITS
    SO_OBJ_NAME		= 0x0001, // #LABEL_object_name the name given to the object by the user
    SO_OBJ_TYPE		= 0x0002, // #LABEL_object_type the type name
    SO_OBJ_DESC		= 0x0004, // #LABEL_object_desc object description (where applicable), including full display name
    SO_MEMB_NAME	= 0x0008, // #LABEL_member_name the name of the member of an object
    SO_MEMB_VAL		= 0x0010, // #LABEL_member_value searches in the values of members, especially strings
    SO_ALL_MEMBS	= 0x0020, // #LABEL_all_members searches in all members, otherwise excludes the members that are not typicaly shown according to current settings
    SO_TYPE_DESC	= 0x0040, // #LABEL_type_desc searches in the type description information, including the description of the type of the object, and the description of the member (all of this information is hard-coded into the program)
#ifndef __MAKETA__
    SO_DEF_OPTIONS	= SO_OBJ_NAME | SO_OBJ_TYPE | SO_OBJ_DESC | SO_MEMB_NAME | SO_MEMB_VAL
#endif
  };
  
#ifndef __MAKETA__ // needed to allow us to get SearchOptions
  static const int	col_row = 0; // note: cols must be created in this order!
  static const int	col_level = 1;
  static const int	col_headline = 2;
  static const int	col_href = 3;
  static const int	col_desc = 4;
  static const int	col_hits = 5;
  static const int	col_relev = 6;
  static const int	num_cols = col_relev + 1;
  
//static const int	num_vis_cols = 3;
//static const char*	vis_col_names[] = {"nest", "item", "hits"};
  
  
  static iSearchDialog* New(int ft = 0, iMainWindowViewer* par_window_ = NULL);

  QVBoxLayout*		layOuter;
//QHBoxLayout*		  layOptions;
  taiBitBox*		  bbOptions;
//QHBoxLayout*		  laySearch;
  iLineEdit*		    search;
  QAbstractButton*	    btnGo;
  QAbstractButton*	    btnStop;
  iTextBrowser* 	  results; 	// list of result items
  QStatusBar*		  status_bar;
  
  int			options() const {return m_options;}
  void			setRoot(taiDataLink* root); // set or reset the root and window used for the search; sets caption and clears
  void			setSearchStr(const String& srch_str);
  // set the search string to given value (erases any that might be there already -- if non-empty, starts the search going too!
  String		searchStr() const;
  // gets the current value of the search string 

  bool			stop() const; // allow event loop, then check stop
  bool			setFirstSort(int col); // set first sort column, pushes others down; true if order changed
  inline const String_PArray& targets() const {return m_targets;} // ref for max speed; only use in search
  inline const String_PArray& kickers() const {return m_kickers;} // ref for max speed; only use in search
  
  void			FindNext(); // really only applicable if already have results
  
  void			Reset(); // clears items and window, esp for when link detaches
  void			Search(); // search, based on search line
// following are all rendering apis
  void			Start(); // resets everything, and starts new results page
  void			StartSection(const String& sec_name);
  void			EndSection(); // end the current section
  void			AddItem(const String& headline,
    const String& href, const String& desc, const String& hits,
    int level = 0, int relev = 0); 
  void			End(); // end all and display results
  
public: // IDataLinkClient interface
  override void*	This() {return (void*)this;}
  override TypeDef*	GetTypeDef() const {return &TA_iSearchDialog;}
  override bool		ignoreDataChanged() const {return true;}
  override void		DataDataChanged(taDataLink*, int dcr, void* op1, void* op2) {}
  override void		DataLinkDestroying(taDataLink* dl);


protected:
  static const int	num_sorts = 3;
  
  int			m_options; // any of the option values
  String		m_caption; // base portion of caption
  String		src;
  int			m_changing;
  bool			m_stop;
  String_PArray		m_targets;
  String_PArray		m_kickers;
  DataTable		m_items;
  int			m_row; // row num, for baking into results table
  int			m_sorts[num_sorts];
  
  virtual void		Constr(); 
   // does constr, called in static, so can extend
  void			RootSet(taiDataLink* root); // called when root changes
  void			ParseSearchString();
  void			Render();
  void 			RenderItem(int level, const String& headline,
    const String& href, const String& desc, const String& hits);

  iSearchDialog(iMainWindowViewer* par_window_);
  
protected slots:
  void			go_clicked();
  void			stop_clicked();
  void			results_setSourceRequest(iTextBrowser* src,
    const QUrl& url, bool& cancel);
  
private:
  void 		init(); // called by constructors
#endif // !__MAKETA__
};


#endif // TA_QTVIEWER_H
