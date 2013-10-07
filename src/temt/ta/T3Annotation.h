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

#ifndef T3Annotation_h
#define T3Annotation_h 1

// parent includes:
#include <taNBase>

// member includes:
#include <taSmartRefT>
#include <taVector3f>
#include <taAxisAngle>
#include <taColor>

// declare all other types mentioned but not required to include:

taTypeDef_Of(T3Annotation);

class TA_API T3Annotation : public taNBase {
  // #CAT_Gui annotation object for a 3d view display -- works with any such display
INHERITED(taNBase)
public:
  enum AnnoteType {             // what type of annotation to create
    LINE,                       // plain line
    ARROW,                      // single ended arrow -- arrow is at the end point of the line
    DOUBLEARROW,                // double-ended arrow
    RECTANGLE,                  // rectange
    ELLIPSE,                    // ellipse 
    TEXT,                       // text label
    OBJECT,                     // 3d object loaded from an open inventor format 3d object file
  };

  AnnoteType    type;           // type of annotation to create
  AnnoteType    rend_type;      // #READ_ONLY #NO_SAVE #HIDDEN type when rendered
  taVector3f    pos;            // 3d position of object (can be moved through a dragger in gui too) -- this is the starting point for lines & arrows, and the lower left-hand corner for rectangles and the bounding box of ellipses
  taVector3f    size;           // #CONDSHOW_OFF_type:TEXT,OBJECT size of the line, rectangle, or ellipse, in 3d dimensions (defines the bounding box for the ellipse)
  float         line_width;     // #CONDSHOW_OFF_type:TEXT,OBJECT line thickness
  float         arrow_size;     // #CONDSHOW_ON_type:ARROW,DOUBLEARROW size of the arrow head, in normalized coordinates
  String        text;           // #CONDSHOW_ON_type:TEXT text to displayobject
  float         font_size;      // #CONDSHOW_ON_type:TEXT font size to display text in, in normalized units (the entire space is typically 1x1x1, so this should usually be a smaller fraction like .05)
  String        obj_fname;      // #CONDSHOW_ON_type:OBJECT #FILE_DIALOG_LOAD #EXT_iv,wrl #FILETYPE_OpenInventor file name of Open Inventor file that contains the 3d geometry of the object
  taColor       color;          // color (a=alpha used for transparency)
  taAxisAngle   rot;            // 3d rotation of body, specifying an axis and a rot along that axis in radians: 180deg = 3.1415, 90deg = 1.5708, 45deg = .7854)
  taVector3f    scale;          // 3d scaling of object along each dimension (applied prior to rotation)

  virtual void  SetColor(const String& clr);
  // set the color to given color name (standard web/html color names)

  virtual void  SetLine(float pos_x, float pos_y, float pos_z,
                        float size_x = 0.5, float size_y = 0.0, float size_z = 0.0,
                        float ln_width = 1.0, const String& color = "black");
  // set to a line with given coordinates -- coordinates are in the normalized coordinates of the view (typically 1x1x1)
  virtual void  SetArrow(float pos_x, float pos_y, float pos_z,
                         float size_x = 0.5, float size_y = 0.0, float size_z = 0.0,
                         float ln_width = 1.0, const String& color = "black",
                         float arrow_sz = 0.02);
  // add an arrow annotation -- coordinates are in the normalized coordinates of the view (typically 1x1x1) 
  virtual void  SetDoubleArrow(float pos_x, float pos_y, float pos_z,
                               float size_x = 0.5, float size_y = 0.0, float size_z = 0.0,
                               float ln_width = 1.0, const String& color = "black",
                               float arrow_sz = 0.02);
  // add a double-arrow annotation -- coordinates are in the normalized coordinates of the view (typically 1x1x1) 
  virtual void  SetRectangle(float pos_x, float pos_y, float pos_z,
                             float size_x = 0.5, float size_y = 0.5, float size_z = 0.0,
                             float ln_width = 1.0, const String& color = "black");
  // add a rectangle annotation -- coordinates are in the normalized coordinates of the view (typically 1x1x1) 
  virtual void  SetEllipse(float pos_x, float pos_y, float pos_z,
                           float size_x = 0.5, float size_y = 0.5, float size_z = 0.0,
                           float ln_width = 1.0, const String& color = "black");
  // add an ellipse annotation -- coordinates are in the normalized coordinates of the view (typically 1x1x1), and define the bounding box for the ellipse
  virtual void  SetCircle(float ctr_x, float ctr_y, float ctr_z,
                          float radius = 0.25,
                          float ln_width = 1.0, const String& color = "black");
  // add a circle annotation -- coordinates are in the normalized coordinates of the view (typically 1x1x1) (parameters converted to ellipse bounding box)
  virtual void  SetText(const String& text,
                        float pos_x, float pos_y, float pos_z,
                        float font_size = 0.05, const String& color = "black");
  // add a text annotation -- coordinates are in the normalized coordinates of the view (typically 1x1x1) for lower left start of text
  virtual void  SetObject(const String& obj_file_path,
                          float pos_x, float pos_y, float pos_z,
                          const String& color = "black");
  // add an object (3D inventor file) annotation -- coordinates are in the normalized coordinates of the view (typically 1x1x1)

  virtual void  UpdateDisplay();
  // #BUTTON force update the display to reflect any changes -- should happen automatically but sometimes needs a nudge

  virtual void  UpdateGeom();
  // #IGNORE update geometry -- some types re-translate scale and rotation into coordinates

  TA_SIMPLE_BASEFUNS(T3Annotation);
protected:
  override void         UpdateAfterEdit_impl();

private:
  void Initialize();
  void Destroy()     { CutLinks(); }
};

#endif // T3Annotation_h