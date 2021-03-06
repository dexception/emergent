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

#include "iPanelOfDocView.h"
#include <taDoc>
#include <iLineEdit>
#include <iNetworkAccessManager>
#include <iHelpBrowser>
#include <iTextBrowser>
#include <iWebView>
#include <iMainWindowViewer>
#include <taProject>

#include <SigLinkSignal>
#include <taMisc>
#include <taiMisc>

#include <QVBoxLayout>
#include <QToolBar>
#include <QProgressBar>
#include <iWebView>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QApplication>

#ifdef USE_QT_WEBENGINE

#include <QWebEnginePage>
#include <QWebEngineProfile>

#else // USE_QT_WEBENGINE

#include <QWebFrame>

#endif // USE_QT_WEBENGINE


#if (QT_VERSION >= 0x050000)
#include <QUrlQuery>
#endif

iPanelOfDocView::iPanelOfDocView()
:inherited(NULL) // usual case: we dynamically set the link, via setDoc
{
  is_loading = false;

  wb_widg = new QWidget();
  wb_box = new QVBoxLayout(wb_widg);
  wb_box->setMargin(0); wb_box->setSpacing(2);

  int font_spec = taiMisc::fonMedium;

  url_bar = new QToolBar(wb_widg);
  wb_box->addWidget(url_bar);

  bak_but = url_bar->addAction("<");
//   bak_but->setArrowType(Qt::LeftArrow);
  bak_but->setToolTip(taiMisc::ToolTipPreProcess("Go backward one step in browsing history"));
  fwd_but = url_bar->addAction(">");
//   fwd_but->setArrowType(Qt::RightArrow);
  fwd_but->setToolTip(taiMisc::ToolTipPreProcess("Go forward one step in browsing history"));

//   url_bar->addSpacing(taiM->hsep_c);

  go_but = url_bar->addAction("Go");
  go_but->setToolTip(taiMisc::ToolTipPreProcess("Go to currently specified URL (can also just press enter in URL field)"));

//   url_bar->addSpacing(taiM->hsep_c);

  wiki_label = taiM->NewLabel("wiki:", wb_widg, font_spec);
  wiki_label->setToolTip(taiMisc::ToolTipPreProcess("name of a wiki, as specified in global preferences, where this object should be stored -- this is used to lookup the wiki name -- if blank then url must be a full URL path"));
  url_bar->addWidget(wiki_label);
//   url_bar->addSpacing(taiM->hsep_c);
  wiki_edit = new iLineEdit(wb_widg);
  wiki_edit->setCharWidth(12);  // make this guy shorter
  url_bar->addWidget(wiki_edit);
//   url_bar->addSpacing(taiM->hsep_c);

  url_label = taiM->NewLabel("URL:", wb_widg, font_spec);
  url_label->setToolTip(taiMisc::ToolTipPreProcess("a URL location for this document -- if blank or 'local' then text field is used as document text -- otherwise if wiki name is set, then this is relative to that wiki, as wiki_url/index.php/Projects/url, otherwise it is a full URL path to a valid location"));
  url_bar->addWidget(url_label);
//   url_bar->addSpacing(taiM->hsep_c);
  url_edit = new iLineEdit(wb_widg);
  url_bar->addWidget(url_edit);
//   url_bar->addSpacing(taiM->hsep_c);

  prog_bar = new QProgressBar(wb_widg);
  prog_bar->setRange(0, 100);
  prog_bar->setMaximumWidth(30);
  url_bar->addWidget(prog_bar);

  seturl_but = url_bar->addAction("Set");
  seturl_but->setToolTip(taiMisc::ToolTipPreProcess("Set current web page to be the URL for this document -- each document is associated with a single URL"));

  // find within item
  url_bar->addSeparator();
  find_lbl = taiM->NewLabel("find:", wb_widg, font_spec);
//   find_lbl = new QLabel("| find:");
  find_lbl->setToolTip(taiMisc::ToolTipPreProcess("Find text string within currently viewed page"));
  url_bar->addWidget(find_lbl);
  find_text = new iLineEdit();
  find_text->setCharWidth(16);
  url_bar->addWidget(find_text);
  find_clear = url_bar->addAction("x");
  find_clear->setToolTip(taiMisc::ToolTipPreProcess("Clear find text and reset any prior highlighting"));
  find_prev = url_bar->addAction("<");
  find_prev->setToolTip(taiMisc::ToolTipPreProcess("Find previous occurrence of find: text within current page"));
  find_next = url_bar->addAction(">");
  find_next->setToolTip(taiMisc::ToolTipPreProcess("Find next occurrence of find: text within current page"));

  webview = new iWebView(wb_widg);
  wb_box->addWidget(webview);

  setCentralWidget(wb_widg);

  webview->installEventFilter(this); // translate keys..

  connect(go_but, SIGNAL(triggered()), this, SLOT(doc_goPressed()) );
  connect(bak_but, SIGNAL(triggered()), this, SLOT(doc_bakPressed()) );
  connect(fwd_but, SIGNAL(triggered()), this, SLOT(doc_fwdPressed()) );
  connect(url_edit, SIGNAL(returnPressed()), this, SLOT(doc_goPressed()) );
  connect(wiki_edit, SIGNAL(returnPressed()), this, SLOT(doc_goPressed()) );
  connect(seturl_but, SIGNAL(triggered()), this, SLOT(doc_seturlPressed()) );

  connect(find_clear, SIGNAL(triggered()), this, SLOT(find_clear_clicked()) );
  connect(find_next, SIGNAL(triggered()), this, SLOT(find_next_clicked()) );
  connect(find_prev, SIGNAL(triggered()), this, SLOT(find_prev_clicked()) );
  connect(find_text, SIGNAL(returnPressed()), this, SLOT(find_next_clicked()) );

  connect(webview, SIGNAL(loadProgress(int)), prog_bar, SLOT(setValue(int)) );
  connect(webview, SIGNAL(loadStarted()), this, SLOT(doc_loadStarted()) );
  connect(webview, SIGNAL(loadFinished(bool)), this, SLOT(doc_loadFinished(bool)) );

#ifdef USE_QT_WEBENGINE
//  webview->page()->setNetworkAccessManager(taiMisc::net_access_mgr);
//  webview->page()->setForwardUnsupportedContent(true);
//  webview->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  connect(webview->page(), SIGNAL(linkClicked(const QUrl&)),
          this, SLOT(doc_linkClicked(const QUrl&)) );
  connect(webview, SIGNAL(sigCreateWindow(QWebEnginePage::WebWindowType,
                                          iWebView*&)), this, SLOT(doc_createWindow(QWebEnginePage::WebWindowType,
                                                                                    iWebView*&)) );
  // QWebEngineProfile* profile = webview->page()->prof
#else // USE_QT_WEBENGINE
  webview->page()->setNetworkAccessManager(taiMisc::net_access_mgr);
  webview->page()->setForwardUnsupportedContent(true);

  webview->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  connect(webview->page(), SIGNAL(linkClicked(const QUrl&)),
          this, SLOT(doc_linkClicked(const QUrl&)) );
  connect(webview, SIGNAL(sigCreateWindow(QWebPage::WebWindowType,
    iWebView*&)), this, SLOT(doc_createWindow(QWebPage::WebWindowType,
    iWebView*&)) );
#endif // USE_QT_WEBENGINE


//TODO  connect(txtText, SIGNAL(copyAvailable(bool)),
//      this, SLOT(textText_copyAvailable(bool)) );
  m_doc = NULL; // changed via setDoc -- if diff, we change our dl
}

iPanelOfDocView::~iPanelOfDocView() {
  if(taiMisc::net_access_mgr)
    taiMisc::net_access_mgr->setMainWindow(NULL); // not us anymore -- someone else will set..
  m_doc = NULL;
}

QWidget* iPanelOfDocView::firstTabFocusWidget() {
  return url_edit;
}

#ifdef USE_QT_WEBENGINE

#else // USE_QT_WEBENGINE

void iPanelOfDocView::doc_createWindow(QWebPage::WebWindowType type, iWebView*& window) {
  // fork to browser
  if (type == QWebPage::WebBrowserWindow) {
    iHelpBrowser* hbrow = iHelpBrowser::instance();
    window = hbrow->AddWebView(_nilString);
  }
}

#endif // USE_QT_WEBENGINE

void iPanelOfDocView::doc_linkClicked(const QUrl& url) {
  String path = url.toString();
  bool ta_path = false;
  QUrl new_url(url);
#if (QT_VERSION >= 0x050000)
  QUrlQuery urq(url);
#else
#endif
  if(path.startsWith("ta:") || path.startsWith(".")) {
    ta_path = true;
  }
  String qry;
#if (QT_VERSION >= 0x050000)
  if(!ta_path && urq.hasQueryItem("title")) { // wiki versions of our action urls get translated into queries with title=...
    qry = urq.queryItemValue("title");
  }
#else
  if(!ta_path && url.hasQueryItem("title")) { // wiki versions of our action urls get translated into queries with title=...
    qry = url.queryItemValue("title");
  }
#endif
  if(qry.nonempty()) {
    if(qry.startsWith("ta:") || qry.startsWith(".")) {
      if(!qry.startsWith("ta:"))
        qry = "ta:"+qry;        // rectify
      new_url.setUrl(qry);      // start from there.
      ta_path = true;
    }
  }
  if(!ta_path) {
    // a standard path:
    webview->load(url);
    // todo: we could make a note of this somewhere, but key idea is that our URL
    // is fixed!!!
    return;
  }

  // handle it internally
  if (!new_url.hasFragment()) {
    if (viewerWindow())
      new_url.setFragment("#winid_" + QString::number(viewerWindow()->uniqueId()));
  }
  // goes to: iMainWindowViewer::taUrlHandler  in ta_qtviewer.cpp
  QDesktopServices::openUrl(new_url);
}

void iPanelOfDocView::doc_loadStarted() {
  is_loading = true;
  go_but->setText("X");
  prog_bar->reset();
}

void iPanelOfDocView::doc_loadFinished(bool ok) {
  is_loading = false;
  go_but->setText("Go");
  if(!ok) return;
  taDoc* doc_ = this->doc();
  if(!doc_) return;
  if(!webview) return;
#ifdef USE_QT_WEBENGINE
#else // USE_QT_WEBENGINE
  QWebFrame* mnfrm = webview->page()->mainFrame();
  if(!mnfrm) return;
  doc_->html_text = mnfrm->toHtml(); // harvest it!
#endif // USE_QT_WEBENGINE
}

void iPanelOfDocView::doc_goPressed() {
  taDoc* doc_ = this->doc();
  if(!doc_) return;
  if(!url_edit) return;
  if(is_loading) {
    // now means stop!
    if(webview) webview->stop();
  }
  else {
    doc_->wiki = wiki_edit->text();
    doc_->url = url_edit->text();
    doc_->UpdateAfterEdit();    // this will drive all the updating, including toggle from local etc
    UpdatePanel();              // also update us..
  }
}

void iPanelOfDocView::doc_bakPressed() {
  // todo: could trap things here too
  webview->back();
}

void iPanelOfDocView::doc_fwdPressed() {
  // todo: could trap things here too
  webview->forward();
}

void iPanelOfDocView::doc_seturlPressed() {
  taDoc* doc_ = this->doc();
  if(!doc_) return;

  String url = webview->url().toString();
  doc_->SetURL(url);
}

void iPanelOfDocView::find_clear_clicked() {
  find_text->clear();
#ifdef USE_QT_WEBENGINE

#else // USE_QT_WEBENGINE
  
#if (QT_VERSION >= 0x040600)
  webview->page()->findText("", QWebPage::HighlightAllOccurrences);
#else
  webview->page()->findText("");
#endif

#endif // USE_QT_WEBENGINE
  
}

void iPanelOfDocView::find_next_clicked() {
  QString cur_find = find_text->text();
//   if(cur_find != last_find) {
    // first one highlights all then goes to first one

#ifdef USE_QT_WEBENGINE

#else // USE_QT_WEBENGINE
  
#if (QT_VERSION >= 0x040600)
    webview->page()->findText(cur_find, QWebPage::HighlightAllOccurrences);
#endif
    webview->page()->findText(cur_find, QWebPage::FindWrapsAroundDocument);
//     last_find = cur_find;
//   }
//   else {
//     // subsequent ones go through one by one
//     curWebView()->page()->findText(cur_find, QWebPage::FindWrapsAroundDocument);
//   }

#endif // USE_QT_WEBENGINE
    
}

void iPanelOfDocView::find_prev_clicked() {
#ifdef USE_QT_WEBENGINE

#else // USE_QT_WEBENGINE
   webview->page()->findText(find_text->text(), QWebPage::FindWrapsAroundDocument | QWebPage::FindBackward);

#endif // USE_QT_WEBENGINE
 }

bool iPanelOfDocView::ignoreSigEmit() const {
  return false;
  //  return !isVisible(); -- this doesn't seem to be giving accurate results!!!
}

void iPanelOfDocView::SigLinkDestroying(taSigLink* dl) {
  setDoc(NULL);
}

void iPanelOfDocView::UpdatePanel_impl() {
  inherited::UpdatePanel_impl();
  taDoc* doc_ = this->doc();
  if(!doc_) return;

  if(taiMisc::net_access_mgr)
    taiMisc::net_access_mgr->setMainWindow(viewerWindow());

  wiki_edit->setText(doc_->wiki);
  url_edit->setText(doc_->url);

#if (QT_VERSION >= 0x050000)
  float trg_font_sz = 14.0f;    // fonts got upsized..
#else
  float trg_font_sz = 12.0f;
#endif
  if(doc_->url.empty() || doc_->url == "local")
    trg_font_sz = 14.0f;

#ifdef USE_QT_WEBENGINE

#else // USE_QT_WEBENGINE
  
  webview->setTextSizeMultiplier(taMisc::doc_text_scale * doc_->text_size *
                                 ((float)taMisc::font_size / trg_font_sz));
#endif // USE_QT_WEBENGINE

  String nw_url = doc_->GetURL();
  bool net_doc = nw_url.contains(":/") && !nw_url.startsWith("file:");
  // indicates some kind of network-based transport protocol..

  if(doc_->web_doc && (!net_doc || taMisc::InternetConnected())) {
    String cur_url = webview->url().toString();
    String nw_url = doc_->GetURL();
    if(cur_url != nw_url)
      webview->load(QUrl(nw_url));
        url_edit->setEnabled(true);
    fwd_but->setEnabled(true);
    bak_but->setEnabled(true);
    //     go_but->setEnabled(true);
    seturl_but->setEnabled(true);
  }
  else {
    // Use the project filename as a base URL so it's possible to load
    // external stylesheets, images, javascript, etc. with a relative path.
    QUrl qurl;
    if (const iMainWindowViewer *mainwin = viewerWindow()) {
      if (const taProject *proj = mainwin->myProject()) {
        qurl = QUrl::fromLocalFile(proj->GetFileName());
      }
    }
    webview->setHtml(doc_->html_text, qurl);
    fwd_but->setEnabled(false);
    bak_but->setEnabled(false);
    seturl_but->setEnabled(false);
  }
}

void iPanelOfDocView::SigEmit_impl(int sls, void* op1_, void* op2_) {
  inherited::SigEmit_impl(sls, op1_, op2_);
  if (sls <= SLS_ITEM_UPDATED_ND) {
    this->m_update_req = true; // so we update next time we show
    UpdatePanel();
  }
}

void iPanelOfDocView::setDoc(taDoc* doc) {
  if (m_doc == doc) return;
  // if the doc is NULL, or different (regardless of NULL->val or val1->val2)
  // we will necessarily have to change links, so we always revoke link
  if (m_link) {
    m_link->RemoveSigClient(this);
  }
  
  m_doc = doc;
  if (doc) {
    taSigLink* dl = doc->GetSigLink();
    if (!dl) return; // shouldn't happen!
    dl->AddSigClient(this);
    UpdatePanel();
  } else {
    webview->setHtml("(no doc set)");
  }
}
  
bool iPanelOfDocView::eventFilter(QObject* obj, QEvent* event) {
  if (event->type() != QEvent::KeyPress) {
    return inherited::eventFilter(obj, event);
  }
  QKeyEvent* key_event = static_cast<QKeyEvent *>(event);
  if (!taiMisc::KeyEventCtrlPressed(key_event)) {
    return false;
  }
  QCoreApplication* app = QCoreApplication::instance();
  switch (key_event->key()) {
  case Qt::Key_P:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier));
    return true;                // we absorb this event
  case Qt::Key_N:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier));
    return true;                // we absorb this event
  case Qt::Key_F:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier));
    return true;                // we absorb this event
  case Qt::Key_B:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier));
    return true;                // we absorb this event
  case Qt::Key_U:
  case Qt::Key_Up:              // translate ctrl+up to page up
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_PageUp, Qt::NoModifier));
    return true;                // we absorb this event
  case Qt::Key_Down:            // translate ctrl+down to page down
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier));
    return true;
#ifdef TA_OS_MAC
  case Qt::Key_V:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier));
    return true;              // we absorb this event
#endif
  case Qt::Key_D:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier));
    return true;              // we absorb this event
  case Qt::Key_H:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier));
    return true;              // we absorb this event
  case Qt::Key_K:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_Clear, Qt::NoModifier));
    return true;              // we absorb this event
  case Qt::Key_Y:             // this doesn't seem to work to generate paste event
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier));
    return true;              // we absorb this event
  case Qt::Key_W:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier));
    return true;              // we absorb this event
  case Qt::Key_Slash:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier));
    return true;              // we absorb this event
  case Qt::Key_Minus:
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier));
    return true;              // we absorb this event
  }
  if(key_event->modifiers() & Qt::AltModifier && (key_event->key() == Qt::Key_W
#if defined(TA_OS_MAC) && (QT_VERSION >= 0x050000)
                                                  || key_event->key() == 0x2211   // weird mac key
#endif
                                                  )) { // copy
    app->postEvent(obj, new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier));
    return true;                // we absorb this event
  }
  return inherited::eventFilter(obj, event);
}

 /*void iPanelOfDocView::br_copyAvailable (bool) {
   viewerWindow()->UpdateUi();
   }*/

