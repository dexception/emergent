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

#include "VEBodyView.h"
#include <VEBody>
#include <T3VEBody>
#include <T3ExaminerViewer>
#include <VEWorldView>
#include <T3VEWorld>

#include <taMisc>

#include <QFileInfo>

#ifdef TA_QT3D

#include <T3Sphere>
#include <T3Cube>
#include <T3Cylinder>
#include <T3Capsule>
#include <T3SceneLoader>

#else // TA_QT3D

#include <T3TransformBoxDragger>
#include <SoCapsule>

#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTexture2Transform.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/draggers/SoTransformBoxDragger.h>

#endif // TA_QT3D

TA_BASEFUNS_CTORS_DEFN(VEBodyView);


bool VEBodyView::isVisible() const {
  return (taMisc::use_gui );
}

void VEBodyView::Initialize(){
  data_base = &TA_VEBody;
}

void VEBodyView::Destroy() {
  CutLinks();
}

void VEBodyView::SetBody(VEBody* ob) {
  if (Body() == ob) return;
  SetData(ob);
  if (ob) {
    if (!name.contains(ob->name)) {
      SetName(ob->name);
    }
  }
}

void VEBodyView::Render_pre() {
  bool show_drag = false;       // default is off!
  T3ExaminerViewer* vw = GetViewer();
  if(vw)
    show_drag = vw->interactionModeOn();
  VEWorldView* wv = parent();
  if(!wv->drag_objs) show_drag = false;
  VEBody* ob = Body();
  
#ifdef TA_QT3D
  T3VEBody* obv = new T3VEBody(NULL, this, show_drag, wv->drag_size);
  setNode(obv);

  if(ob) {
    if(ob->HasBodyFlag(VEBody::FM_FILE) && !ob->obj_fname.empty()) {
      T3SceneLoader* sp = new T3SceneLoader(obv);
      obv->obj = sp;
      sp->setSource((QString)ob->obj_fname.chars()); // currently not working!
      goto finish;
      // QFileInfo qfi(ob->obj_fname);
      // if(qfi.isFile() && qfi.isReadable() && in.openFile(ob->obj_fname)) {
      // }
      // String msg;
      // msg << "object file: " << ob->obj_fname << " not found, reverting to shape";
      // taMisc::ConsoleOutput(msg, true, false); // straight msg
      // NOTE: do NOT use Info or Error here: ProcessEvents at this point is BAD
//       ob->ClearBodyFlag(VEBody::FM_FILE);
    }

    switch(ob->shape) {
    case VEBody::SPHERE: {
      T3Sphere* sp = new T3Sphere(obv);
      obv->obj = sp;
      sp->setRadius(ob->radius);
      break;
    }
    case VEBody::CAPSULE: {
      T3Capsule* sp = new T3Capsule(obv);
      obv->obj = sp;
      sp->setGeom(T3Capsule::LONG_Z, ob->radius, ob->length);
      break;
    }
    case VEBody::CYLINDER: {
      T3Cylinder* sp = new T3Cylinder(obv);
      obv->obj = sp;
      sp->setGeom(T3Cylinder::LONG_Z, ob->radius, ob->length);
      break;
    }
    case VEBody::BOX: {
      T3Cube* sp = new T3Cube(obv);
      obv->obj = sp;
      sp->setSize(ob->box);
      break;
    }
    case VEBody::NO_SHAPE: {
      obv->obj = NULL;
      break;
    }
    }
  }
#else // TA_QT3D
  T3VEBody* obv = new T3VEBody(this, show_drag, wv->drag_size);
  setNode(obv);
  SoSeparator* ssep = obv->shapeSeparator();

  if(ob) {
    if(ob->HasBodyFlag(VEBody::FM_FILE) && !ob->obj_fname.empty()) {
      SoInput in;
      QFileInfo qfi(ob->obj_fname);
      if(qfi.isFile() && qfi.isReadable() && in.openFile(ob->obj_fname)) {
        SoSeparator* root = SoDB::readAll(&in);
        if (root) {
          ssep->addChild(root);
          SoTransform* tx = obv->txfm_shape();
          ob->obj_xform.CopyTo(tx);
          goto finish;
        }
      }
      String msg;
      msg << "object file: " << ob->obj_fname << " not found, reverting to shape";
      taMisc::ConsoleOutput(msg, true, false); // straight msg
      // NOTE: do NOT use Info or Error here: ProcessEvents at this point is BAD
//       ob->ClearBodyFlag(VEBody::FM_FILE);
    }

    if((bool)ob->texture && wv) {
      SoSwitch* tsw = ((T3VEWorld*)wv->node_so())->getTextureSwitch();
      SoSwitch* txfsw = ((T3VEWorld*)wv->node_so())->getTextureXformSwitch();
      VETexture* vtex = ob->texture.ptr();
      int idx = vtex->GetIndex();
      if(idx >= 0 && tsw->getNumChildren() > idx) {
        SoTexture2* tex = (SoTexture2*)tsw->getChild(idx);
        ssep->addChild(tex);
        // taMisc::Info("tex idx:", (String)(((int)ssep->getNumChildren())-1));
        if(vtex->NeedsTransform()) {
          SoTexture2Transform* ttx = (SoTexture2Transform*)txfsw->getChild(idx);
          ssep->addChild(ttx);
        }
      }
    }

    switch(ob->shape) {
    case VEBody::SPHERE: {
      SoSphere* sp = new SoSphere;
      sp->radius = ob->radius;
      ssep->addChild(sp);
      break;
    }
    case VEBody::CAPSULE: {
      SoCapsule* sp = new SoCapsule;
      sp->radius = ob->radius;
      sp->height = ob->length;
      SoTransform* tx = new SoTransform;
      tx->rotation.setValue(SbVec3f(1.0f, 0.0f, 0.0f), 1.5708f); // orient to Z
      ssep->addChild(tx);
      ssep->addChild(sp);
      break;
    }
    case VEBody::CYLINDER: {
      SoCylinder* sp = new SoCylinder;
      sp->radius = ob->radius;
      sp->height = ob->length;
      SoTransform* tx = new SoTransform;
      tx->rotation.setValue(SbVec3f(1.0f, 0.0f, 0.0f), 1.5708f); // orient to Z
      ssep->addChild(tx);
      ssep->addChild(sp);
      break;
    }
    case VEBody::BOX: {
      SoCube* sp = new SoCube;
      sp->width = ob->box.x;
      sp->depth = ob->box.z;
      sp->height = ob->box.y;
      ssep->addChild(sp);
      break;
    }
    case VEBody::NO_SHAPE: {
      break;
    }
    }
  }
#endif // TA_QT3D
  
 finish:

  SetDraggerPos();

  inherited::Render_pre();
}

void VEBodyView::SetDraggerPos() {
  T3VEBody* obv = (T3VEBody*)node_so();
  if(!obv) return;
  VEBody* ob = Body();
  if(!ob) return;

#ifdef TA_QT3D

#else // TA_QT3D
  
  // set dragger position
  T3TransformBoxDragger* drag = obv->getDragger();
  if(!drag) return;

  switch(ob->shape) {
  case VEBody::SPHERE: {
    drag->xf_->translation.setValue(-ob->radius, -ob->radius, ob->radius);
    break;
  }
  case VEBody::CAPSULE:
  case VEBody::CYLINDER: {
    if(ob->long_axis == VEBody::LONG_X)
      drag->xf_->translation.setValue(-ob->radius, ob->radius, -ob->length*.5f);
    else
      drag->xf_->translation.setValue(-ob->radius, -ob->radius, ob->length*.5f);
    break;
  }
  case VEBody::BOX: {
    drag->xf_->translation.setValue(-ob->box.x*.5f, -ob->box.y*.5f, ob->box.z*.5f);
    break;
  }
  case VEBody::NO_SHAPE: {
    break;
  }
  }
#endif // TA_QT3D
}

void VEBodyView::Render_impl() {
  inherited::Render_impl();

  T3VEBody* obv = (T3VEBody*)this->node_so(); // cache
  if(!obv) return;
  VEBody* ob = Body();
  if(!ob) return;

  bool show_drag = false;       // default is off!
  T3ExaminerViewer* vw = GetViewer();
  if(vw)
    show_drag = vw->interactionModeOn();
  VEWorldView* wv = parent();
  if(!wv) return;
  if(!wv->drag_objs) show_drag = false;

#ifdef TA_QT3D

  obv->Translate(ob->cur_pos.x, ob->cur_pos.y, ob->cur_pos.z);
  obv->RotateRad(ob->cur_rot.x, ob->cur_rot.y, ob->cur_rot.z, ob->cur_rot.rot);
   
  float off_size = 1.0e-12f;    // tiny size if it is turned off..

  // todo: once switched over to Qt3D, remove whole texture apparatus..
  // just need local params
  
  if(ob->IsCurShape()) {// only if we are currently the right shape, incl fm file flag
    if(ob->HasBodyFlag(VEBody::FM_FILE)) {
      T3SceneLoader* sp = dynamic_cast<T3SceneLoader*>(obv->obj);
      if(sp) {
        sp->setSource((QString)ob->obj_fname.chars());
        if(ob->HasBodyFlag(VEBody::OFF)) {
          sp->Scale(off_size);
        }
      }
    }
    else {
      switch(ob->shape) {
      case VEBody::SPHERE: {
        T3Sphere* sp = dynamic_cast<T3Sphere*>(obv->obj);
        if(sp) {
          if(ob->texture) {
            sp->setTexture(QString(ob->texture->fname.chars()));
          }
          else if(ob->set_color) {
            sp->setColor(ob->color.color(),
                         ob->phong_color.ambient, ob->phong_color.specular,
                         ob->phong_color.shininess);
          }
          if(ob->HasBodyFlag(VEBody::OFF))
            sp->setRadius(off_size);
          else
            sp->setRadius(ob->radius);
        }
        break;
      }
      case VEBody::CAPSULE: {
        T3Capsule* sp = dynamic_cast<T3Capsule*>(obv->obj);
        if(sp) {
          if(ob->texture) {
            sp->setTexture(QString(ob->texture->fname.chars()));
          }
          else if(ob->set_color) {
            sp->setColor(ob->color.color(),
                         ob->phong_color.ambient, ob->phong_color.specular,
                         ob->phong_color.shininess);
          }
          if(ob->HasBodyFlag(VEBody::OFF)) {
            sp->setGeom(T3Capsule::LONG_Z, off_size, off_size);
          }
          else {
            sp->setGeom(T3Capsule::LONG_Z, ob->radius, ob->length);
          }
        }
        break;
      }
      case VEBody::CYLINDER: {
        T3Cylinder* sp = dynamic_cast<T3Cylinder*>(obv->obj);
        if(sp) {
          if(ob->texture) {
            sp->setTexture(QString(ob->texture->fname.chars()));
          }
          else if(ob->set_color) {
            sp->setColor(ob->color.color(),
                         ob->phong_color.ambient, ob->phong_color.specular,
                         ob->phong_color.shininess);
          }
          if(ob->HasBodyFlag(VEBody::OFF)) {
            sp->setGeom(T3Cylinder::LONG_Z, off_size, off_size);
          }
          else {
            sp->setGeom(T3Cylinder::LONG_Z, ob->radius, ob->length);
          }
        }
        break;
      }
      case VEBody::BOX: {
        T3Cube* sp = dynamic_cast<T3Cube*>(obv->obj);
        if(sp) {
          if(ob->texture) {
            sp->setTexture(QString(ob->texture->fname.chars()));
          }
          else if(ob->set_color) {
            sp->setColor(ob->color.color(),
                         ob->phong_color.ambient, ob->phong_color.specular,
                         ob->phong_color.shininess);
          }
          if(ob->HasBodyFlag(VEBody::OFF)) {
            sp->setSize(off_size, off_size, off_size);
          }
          else {
            sp->setSize(ob->box);
          }
        }
        break;
      }
      case VEBody::NO_SHAPE: {
        break;
      }
      }
    }
  }
#else // TA_QT3D
  
  SoTransform* tx = obv->transform();
  tx->translation.setValue(ob->cur_pos.x, ob->cur_pos.y, ob->cur_pos.z);
  tx->rotation.setValue(ob->cur_quat.x, ob->cur_quat.y, ob->cur_quat.z, ob->cur_quat.s);

  if(ob->set_color) {
    taColor amb;
    taColor spec;
    ob->phong_color.GetAmbient(amb, ob->color);
    ob->phong_color.GetSpecular(spec);

    SoMaterial* mat = obv->material();
    mat->diffuseColor.setValue(ob->color.r, ob->color.g, ob->color.b);
    mat->transparency.setValue(1.0f - ob->color.a);

    mat->ambientColor.setValue(amb.r, amb.g, amb.b);
    mat->specularColor.setValue(spec.r, spec.g, spec.b);
    mat->shininess.setValue(ob->phong_color.shininess);
  }
  else {
    SoMaterial* mat = obv->material();
    mat->transparency.setValue(0.0f);
  }

  SoSeparator* ssep = obv->shapeSeparator();

  if((bool)ob->texture && wv) {
    int tex_idx = show_drag ? 4 : 3; // NOTE: this may need to be updated if structure changes -- should probably have a better solution to this..
    SoSwitch* tsw = ((T3VEWorld*)wv->node_so())->getTextureSwitch();
    SoSwitch* txfsw = ((T3VEWorld*)wv->node_so())->getTextureXformSwitch();
    VETexture* vtex = ob->texture.ptr();
    int idx = vtex->GetIndex();
    if(idx >= 0 && tsw->getNumChildren() > idx && ssep->getNumChildren() > tex_idx) {
      SoTexture2* tex = (SoTexture2*)tsw->getChild(idx);
      SoTexture2* curtex = (SoTexture2*)ssep->getChild(tex_idx);
      if(tex != curtex && curtex->getClassTypeId() == tex->getClassTypeId()) {
        ssep->replaceChild(tex_idx, tex);
        if(vtex->NeedsTransform() && ssep->getNumChildren() > tex_idx + 1) {
          // note: transform only works if previously also had a transform.. Init will fix
          SoTexture2Transform* curttx = (SoTexture2Transform*)ssep->getChild(tex_idx + 1);
          SoTexture2Transform* ttx = (SoTexture2Transform*)txfsw->getChild(idx);
          if(ttx != curttx && curttx->getClassTypeId() == ttx->getClassTypeId()) {
            ssep->replaceChild(tex_idx + 1, ttx);
          }
        }
      }
    }
  }

  float off_size = 1.0e-12f;    // tiny size if it is turned off..

  if(ob->IsCurShape()) {// only if we are currently the right shape, incl fm file flag
    if(ob->HasBodyFlag(VEBody::FM_FILE)) {
      SoTransform* tx = node_so()->txfm_shape();
      if(ob->HasBodyFlag(VEBody::OFF)) {
        taTransform off_tx = ob->obj_xform;
        off_tx.scale.SetXYZ(off_size, off_size, off_size);
        off_tx.CopyTo(tx);
      }
      else {
        ob->obj_xform.CopyTo(tx);
      }
    }
    else {
      switch(ob->shape) {
      case VEBody::SPHERE: {
        SoSphere* sp = (SoSphere*)ssep->getChild(ssep->getNumChildren()-1); // last thing
        if(ob->HasBodyFlag(VEBody::OFF))
          sp->radius = off_size;
        else
          sp->radius = ob->radius;
        break;
      }
      case VEBody::CAPSULE: {
        SoCapsule* sp = (SoCapsule*)ssep->getChild(ssep->getNumChildren()-1); // last thing
        if(ob->HasBodyFlag(VEBody::OFF)) {
          sp->radius = off_size;
          sp->height = off_size;
        }
        else {
          sp->radius = ob->radius;
          sp->height = ob->length;
        }
        break;
      }
      case VEBody::CYLINDER: {
        SoCylinder* sp = (SoCylinder*)ssep->getChild(ssep->getNumChildren()-1); // last thing
        if(ob->HasBodyFlag(VEBody::OFF)) {
          sp->radius = off_size;
          sp->height = off_size;
        }
        else {
          sp->radius = ob->radius;
          sp->height = ob->length;
        }
        break;
      }
      case VEBody::BOX: {
        SoCube* sp = (SoCube*)ssep->getChild(ssep->getNumChildren()-1); // last thing
        if(ob->HasBodyFlag(VEBody::OFF)) {
          sp->width = off_size;
          sp->depth = off_size;
          sp->height = off_size;
        }
        else {
          sp->width = ob->box.x;
          sp->depth = ob->box.z;
          sp->height = ob->box.y;
        }
        break;
      }
      case VEBody::NO_SHAPE: {
        break;
      }
      }
    }
  }
#endif // TA_QT3D
}


#ifndef TA_QT3D
// callback for transformer dragger
void T3VEBody_DragFinishCB(void* userData, SoDragger* dragr) {
  SoTransformBoxDragger* dragger = (SoTransformBoxDragger*)dragr;
  T3VEBody* obso = (T3VEBody*)userData;
  VEBodyView* obv = static_cast<VEBodyView*>(obso->dataView());
  VEBody* ob = obv->Body();
  VEWorldView* wv = obv->parent();

  SbRotation cur_rot;
  cur_rot.setValue(ob->cur_quat.x, ob->cur_quat.y, ob->cur_quat.z, ob->cur_quat.s);

  SbVec3f trans = dragger->translation.getValue();
  cur_rot.multVec(trans, trans); // rotate the translation by current rotation
  ob->Translate(trans[0], trans[1], trans[2], false); // cur -- user can call cur to init

  const SbVec3f& scale = dragger->scaleFactor.getValue();
  taVector3f sc(scale[0], scale[1], scale[2]);
  if(sc < .1f) sc = .1f;        // prevent scale from going to small too fast!!
  ob->radius *= sc.x;
  ob->length *= sc.x;
  ob->box *= sc;
  ob->obj_xform.scale *= sc;

  SbVec3f axis;
  float angle;
  dragger->rotation.getValue(axis, angle);
  if(axis[0] != 0.0f || axis[1] != 0.0f || axis[2] != 1.0f || angle != 0.0f) {
    ob->RotateAxis(axis[0], axis[1], axis[2], angle, false); // cur -- user can call cur to init
  }

//   float h = 0.04f; // nominal amount of height, so we don't vanish
  obso->txfm_shape()->scaleFactor.setValue(1.0f, 1.0f, 1.0f);
  obso->txfm_shape()->rotation.setValue(SbVec3f(0.0f, 0.0f, 1.0f), 0.0f);
  obso->txfm_shape()->translation.setValue(0.0f, 0.0f, 0.0f);
  dragger->translation.setValue(0.0f, 0.0f, 0.0f);
  dragger->rotation.setValue(SbVec3f(0.0f, 0.0f, 1.0f), 0.0f);
  dragger->scaleFactor.setValue(1.0f, 1.0f, 1.0f);

  wv->UpdateDisplay();
}

#endif // TA_QT3D
