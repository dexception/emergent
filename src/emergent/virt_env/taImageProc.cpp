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

#include "taImageProc.h"
#include <float_Matrix>
#include <taVector2i>
#include <taVector2f>
#include <taVector3i>
#include <taMath_float>
#include <float_Array>
#include <Random>
#include <int_Matrix>

#include <taMisc>

TA_BASEFUNS_CTORS_DEFN(taImageProc);

void taImageProc::Initialize() {
}

void taImageProc::Destroy() {
}

bool taImageProc::GetBorderColor_float(float_Matrix& img_data,
                                       float& r, float& g, float& b, float& a,
                                       bool median) {
  if(img_data.dims() == 3) {    // an rgb guy
    return GetBorderColor_float_rgb(img_data, r, g, b, a, median);
  }
  else {
    bool rval = GetBorderColor_float_grey(img_data, r, median);
    g = r; b = r; a = 1.0f;
    return rval;
  }
}

bool taImageProc::GetBorderColor_float_rgb(float_Matrix& img_data,
                                           float& r, float& g, float& b, float& a,
                                           bool median) {
  if(img_data.dims() != 3) {
    taMisc::Error("taImageProc::GetBorderColor_float_rgb", "image must have 3 dims: x, y, color");
    return false; // err
  }
  int nclrs = img_data.dim(2);
  float clrs[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // coverity food
  for(int i=0;i<nclrs;i++) {
    float_Matrix* cmp = img_data.GetFrameSlice(i);
    taBase::Ref(cmp);
    GetBorderColor_float_grey(*cmp, clrs[i], median);
    taBase::unRefDone(cmp);
  }
  r = clrs[0]; g = clrs[1]; b = clrs[2];
  if(nclrs == 4) a = clrs[3];
  else	a = 1.0f;
  return true;
}

bool taImageProc::GetBorderColor_float_grey(float_Matrix& img_data, float& grey,
                                            bool median) {
  if(img_data.dims() != 2) {
    taMisc::Error("taImageProc::GetBorderColor_float_grey", "image must have 2 dims: x, y");
    return false; // err
  }
  taVector2i img_size(img_data.dim(0), img_data.dim(1));
  if(median) {
    float_Array edges;
    edges.SetSize(img_size.x * 2 + img_size.y * 2);
    int idx = 0;
    for(int x=0;x<img_size.x;x++) {
      edges.FastEl(idx++) = img_data.FastEl2d(x, img_size.y-1);
      edges.FastEl(idx++) = img_data.FastEl2d(x, 0);
    }
    for(int y=0;y<img_size.y;y++) {
      edges.FastEl(idx++) = img_data.FastEl2d(img_size.x-1, y);
      edges.FastEl(idx++) = img_data.FastEl2d(0, y);
    }
    edges.Sort();
    grey = edges.FastEl(edges.size / 2);
  }
  else {
    float tavg = 0.0f;
    float bavg = 0.0f;
    for(int x=0;x<img_size.x;x++) {
      tavg += img_data.FastEl2d(x, img_size.y-1);
      bavg += img_data.FastEl2d(x, 0);
    }
    tavg /= (float)(img_size.x);
    bavg /= (float)(img_size.x);

    float lavg = 0.0f;
    float ravg = 0.0f;
    for(int y=0;y<img_size.y;y++) {
      ravg += img_data.FastEl2d(img_size.x-1, y);
      lavg += img_data.FastEl2d(0, y);
    }
    lavg /= (float)(img_size.y);
    ravg /= (float)(img_size.y);

    grey = 0.25f * (tavg + bavg + lavg + ravg);
  }
  return true;
}

bool taImageProc::RenderBorder_float(float_Matrix& img_data) {
  if(img_data.dims() == 3) {    // an rgb guy
    int nclrs = img_data.dim(2);
    for(int i=0;i<nclrs;i++) {
      float_Matrix* cmp = img_data.GetFrameSlice(i);
      taBase::Ref(cmp);
      RenderBorder_float(*cmp);
      taBase::unRefDone(cmp);
    }
    return true;
  }
  float grey;
  GetBorderColor_float_grey(img_data, grey);

  taVector2i img_size(img_data.dim(0), img_data.dim(1));

  for(int x=0;x<img_size.x;x++) {
    img_data.FastEl2d(x, img_size.y-1) = grey;
    img_data.FastEl2d(x, 0) = grey;
  }
  for(int y=1;y<img_size.y-1;y++) {
    img_data.FastEl2d(img_size.x-1, y) = grey;
    img_data.FastEl2d(0, y) = grey;
  }
  return true;
}

bool taImageProc::FadeEdgesToBorder_float(float_Matrix& img_data, int fade_width) {
  if(img_data.dims() == 3) {    // an rgb guy
    int nclrs = img_data.dim(2);
    for(int i=0;i<nclrs;i++) {
      float_Matrix* cmp = img_data.GetFrameSlice(i);
      taBase::Ref(cmp);
      FadeEdgesToBorder_float(*cmp, fade_width);
      taBase::unRefDone(cmp);
    }
    return true;
  }
  taVector2i img_size(img_data.dim(0), img_data.dim(1));
  float oavg = img_data.FastEl2d(0,0); // assuming already has renderborder called
  for(int wd=1; wd<=fade_width;wd++) {
    float pct = (float)wd / (float)(fade_width+1);
    float pct_c = 1.0f - pct;
    float oavgadd = pct_c * oavg;
    for(int x=wd;x<img_size.x-wd;x++) {
      float& tv = img_data.FastEl2d(x, img_size.y-1-wd);
      float& bv = img_data.FastEl2d(x, wd);
      tv = oavgadd + pct * tv;
      bv = oavgadd + pct * bv;
    }
    for(int y=wd+1;y<img_size.y-wd-1;y++) {
      float& rv = img_data.FastEl2d(img_size.x-1-wd, y);
      float& lv = img_data.FastEl2d(wd, y);
      rv = oavgadd + pct * rv;
      lv = oavgadd + pct * lv;
    }
  }
  return true;
}

bool taImageProc::RenderOccluderBorderColor_float(float_Matrix& img_data,
                                                  float llx, float lly, float urx, float ury) {
  if(img_data.dims() == 3) {    // an rgb guy
    int nclrs = img_data.dim(2);
    for(int i=0;i<nclrs;i++) {
      float_Matrix* cmp = img_data.GetFrameSlice(i);
      taBase::Ref(cmp);
      RenderOccluderBorderColor_float(*cmp, llx, lly, urx, ury);
      taBase::unRefDone(cmp);
    }
    return true;
  }
  taVector2i img_size(img_data.dim(0), img_data.dim(1));
  taVector2i ll;
  ll.x = (int)(img_size.x * llx);  ll.y = (int)(img_size.y * lly);
  taVector2i ur;
  ur.x = (int)(img_size.x * urx);  ur.y = (int)(img_size.y * ury);
  float oavg = img_data.FastEl2d(0,0); // assuming already has renderborder called
  for(int y = ll.y; y < ur.y; y++) {
    for(int x = ll.x; x < ur.x; x++) {
      img_data.FastEl2d(x, y) = oavg;
    }
  }
  return true;
}

bool taImageProc::RenderFill(float_Matrix& img_data, float r, float g, float b, float a) {
  taVector2i img_size(img_data.dim(0), img_data.dim(1));
  float clrs[4] = {r,g,b,a};
  int nclrs = 1;
  if(img_data.dims() == 3) {
    nclrs = img_data.dim(2);
  }

  for(int yi = 0; yi < img_size.y; yi++) {
    for(int xi = 0; xi < img_size.x; xi++) {
      if(nclrs > 1) {
	for(int cl=0; cl < nclrs; cl++) {
	  img_data.FastEl3d(xi,yi,cl) = clrs[cl];
	}
      }
      else {
	img_data.FastEl2d(xi,yi) = r; // just use r channel for gray
      }
    }
  }
  return true;
}

bool taImageProc::RGBToGrey(float_Matrix& grey_data, const float_Matrix& rgb_data) {
  taVector2i img_size(rgb_data.dim(0), rgb_data.dim(1));
  grey_data.SetGeom(2, img_size.x, img_size.y);
  int nclrs = 1;
  if(rgb_data.dims() == 3) {
    nclrs = rgb_data.dim(2);
  }
  float norm = 1.0f / (float)nclrs;
  for(int yi = 0; yi < img_size.y; yi++) {
    for(int xi = 0; xi < img_size.x; xi++) {
      float grey = 0.0f;
      if(nclrs > 1) {
	for(int cl=0; cl < nclrs; cl++) {
	  grey += rgb_data.FastEl3d(xi,yi,cl);
	}
        grey *= norm;
      }
      else {
	grey = rgb_data.FastEl2d(xi,yi);
      }
      grey_data.FastEl2d(xi,yi) = grey;
    }
  }
  return true;
}

bool taImageProc::GreyToRGB(float_Matrix& rgb_data, const float_Matrix& grey_data,
                            bool alpha) {
  taVector2i img_size(grey_data.dim(0), grey_data.dim(1));
  int nclrs = 3;
  if(alpha)
    nclrs++;
  rgb_data.SetGeom(3, img_size.x, img_size.y, nclrs);
  for(int yi = 0; yi < img_size.y; yi++) {
    for(int xi = 0; xi < img_size.x; xi++) {
      float grey = grey_data.FastEl2d(xi, yi);
      for(int cl=0; cl < 3; cl++) {
        rgb_data.FastEl3d(xi,yi,cl) = grey;
      }
      if(nclrs == 4) {
        rgb_data.FastEl3d(xi,yi,3) = 1.0f;
      }
    }
  }
  return true;
}

bool taImageProc::TranslateImagePix_float(float_Matrix& xlated_img, float_Matrix& orig_img,
                                          int move_x, int move_y, EdgeMode edge) {

  taVector2i img_size(orig_img.dim(0), orig_img.dim(1));
  taVector2f img_ctr = taVector2f(img_size) / 2.0f;
  taVector2i img_off = taVector2i(move_x, move_y);

  int nclrs = 1;
  if(orig_img.dims() == 3) { // rgb
    nclrs = orig_img.dim(2);
    xlated_img.SetGeom(3, img_size.x, img_size.y, nclrs);
  }
  else {
    xlated_img.SetGeom(2, img_size.x, img_size.y);
  }

  bool wrap = (edge == WRAP);

  taVector2i ic;
  if(wrap) {
    for(int ny = 0; ny < img_size.y; ny++) {
      ic.y = ny - img_off.y;
      for(int nx = 0; nx < img_size.x; nx++) {
        ic.x = nx - img_off.x;
        ic.Wrap(img_size);      // d/c on half wrap
        if(nclrs > 1) {
          for(int i=0;i<nclrs;i++)
            xlated_img.FastEl3d(nx, ny, i) = orig_img.FastEl3d(ic.x, ic.y, i);
        }
        else {
          xlated_img.FastEl2d(nx, ny) = orig_img.FastEl2d(ic.x, ic.y);
        }
      }
    }
  }
  else {
    for(int ny = 0; ny < img_size.y; ny++) {
      ic.y = ny - img_off.y;
      for(int nx = 0; nx < img_size.x; nx++) {
        ic.x = nx - img_off.x;
        if(ic.Clip(img_size)) {
          if(edge == CLIP) continue; // don't bail for BORDER
        }
        if(nclrs > 1) {
          for(int i=0;i<nclrs;i++)
            xlated_img.FastEl3d(nx, ny, i) = orig_img.FastEl3d(ic.x, ic.y, i);
        }
        else {
          xlated_img.FastEl2d(nx, ny) = orig_img.FastEl2d(ic.x, ic.y);
        }
      }
    }
  }
  return true;
}

bool taImageProc::TranslateImage_float(float_Matrix& xlated_img, float_Matrix& orig_img,
                                       float move_x, float move_y, EdgeMode edge) {

  taVector2f deltas(move_x, move_y);
  taVector2i img_size(orig_img.dim(0), orig_img.dim(1));
  taVector2f img_ctr = taVector2f(img_size) / 2.0f;
  taVector2i img_off = taVector2i(deltas * img_ctr);

  return TranslateImagePix_float(xlated_img, orig_img, img_off.x, img_off.y, edge);
}

bool taImageProc::ScaleImage_float(float_Matrix& scaled_img, float_Matrix& orig_img,
                                   float scale, EdgeMode edge) {
  if(scale < .01f) {
    taMisc::Error("Can't scale below .01.");
    return false;
  }
  if(scale > 100.0f) {
    taMisc::Error("Can't scale above 100.");
    return false;
  }
  taVector2i img_size(orig_img.dim(0), orig_img.dim(1));
  taVector2i scaled_size;
  scaled_size.x = 2 + (int)(scale * (img_size.x-2)); // keep border in there
  scaled_size.y = 2 + (int)(scale * (img_size.y-2));

  int nclrs = 1;
  if(orig_img.dims() == 3) { // rgb
    nclrs = orig_img.dim(2);
    scaled_img.SetGeom(3, scaled_size.x, scaled_size.y, nclrs);
  }
  else {
    scaled_img.SetGeom(2, scaled_size.x, scaled_size.y);
  }
  float half_sc;
  if(scale < 1.0f)
    half_sc = .5f / scale; // how much on each side of the pixel to include in avg
  else
    half_sc = .5f * scale;

  // scale > 1
  //   o o o    3
  //  /|X|X|\   connections
  // s s s s s  5

  int   half_int = (int)floor(half_sc);
  float extra = half_sc - (float)half_int;
  int n_orig_pix = half_int + 1; // number of pixels to get from original image for each scaled pixel
  //    int tot_org_pix = 2 * n_orig_pix + 1;
  float_Matrix sc_ary(false);
  sc_ary.SetGeom(2, n_orig_pix*2 + 1, n_orig_pix*2 + 1);
  int x, y;
  for(y=-n_orig_pix; y<= n_orig_pix; y++) {
    float ysc = extra;
    if((y >= -half_int) && (y <= half_int))
      ysc = 1.0f;
    for(x=-n_orig_pix; x<= n_orig_pix; x++) {
      float xsc = extra;
      if((x >= -half_int) && (x <= half_int))
        xsc = 1.0f;
      sc_ary.Set(ysc * xsc, x + n_orig_pix, y + n_orig_pix);
    }
  }

  taMath_float::vec_norm_sum(&sc_ary);          // make it add to 1

  bool wrap = (edge == WRAP);

  taVector2i ic;
  if(wrap) {
    for(y=0; y<scaled_size.y; y++) {
      int oyc = (int)floor(.5f + ((float)y / scale));
      for(x=0; x<scaled_size.x; x++) {
        int oxc = (int)floor(.5f + ((float)x / scale));
        float avgs[4] = {};	// init to 0
        int oxi, oyi;
        for(oyi=-n_orig_pix; oyi<=n_orig_pix; oyi++) {
          ic.y = oyc + oyi;
          for(oxi=-n_orig_pix;oxi<=n_orig_pix;oxi++) {
            ic.x = oxc + oxi;
            ic.Wrap(img_size);       // d/c on half wrap
            float sc = sc_ary.FastEl2d(oxi + n_orig_pix, oyi + n_orig_pix);
            if(nclrs > 1) {
              for(int cl=0; cl < nclrs; cl++) {
                avgs[cl] += sc * orig_img.FastEl3d(ic.x, ic.y, cl);
              }
            }
            else {
              avgs[0] += sc * orig_img.FastEl2d(ic.x, ic.y);
            }
          }
        }
        if(nclrs > 1) {
          for(int cl=0; cl < nclrs; cl++) {
            scaled_img.FastEl3d(x, y, cl) = avgs[cl];
          }
        }
        else {
          scaled_img.FastEl2d(x, y) = avgs[0];
        }
      }
    }
  }
  else {
    for(y=0; y<scaled_size.y; y++) {
      int oyc = (int)floor(.5f + ((float)y / scale));
      for(x=0; x<scaled_size.x; x++) {
        int oxc = (int)floor(.5f + ((float)x / scale));
        float avgs[4] = {};	// init to 0
        int oxi, oyi;
        for(oyi=-n_orig_pix; oyi<=n_orig_pix; oyi++) {
          ic.y = oyc + oyi;
          for(oxi=-n_orig_pix;oxi<=n_orig_pix;oxi++) {
            ic.x = oxc + oxi;
            if(ic.Clip(img_size)) {
              if(edge == CLIP) continue; // don't bail for BORDER
            }
            float sc = sc_ary.FastEl2d(oxi + n_orig_pix, oyi + n_orig_pix);
            if(nclrs > 1) {
              for(int cl=0; cl < nclrs; cl++) {
                avgs[cl] += sc * orig_img.FastEl3d(ic.x, ic.y, cl);
              }
            }
            else {
              avgs[0] += sc * orig_img.FastEl2d(ic.x, ic.y);
            }
          }
        }
        if(nclrs > 1) {
          for(int cl=0; cl < nclrs; cl++) {
            scaled_img.FastEl3d(x, y, cl) = avgs[cl];
          }
        }
        else {
          scaled_img.FastEl2d(x, y) = avgs[0];
        }
      }
    }
  }
  return true;
}

// get pixel coordinates (pc1, 2) with norm weights (pw1, 2) for given floating coordinate coord
void taImageProc::GetWeightedPixels_float(float coord, int size, int* pc, float* pw) {
  //   |  .|   |  get from two closest pixels..
  pc[0] = (int)floor(coord);
  float xfrac = coord - floor(coord);
  if(xfrac < .5f) {
    pw[0] = xfrac;
    pc[1] = pc[0]-1; pw[1] = 1.0f - xfrac;
  }
  else if(xfrac > .5f) {
    pw[0] = 1.0f - xfrac;
    pc[1] = pc[0]+1; pw[1] = xfrac;
  }
  else {
    pw[0] = pw[1] = .5f;
    pc[1] = pc[0];
  }
  if(pc[0] < 0) pc[0] = 0; if(pc[0] >= size) pc[0] = size-1;
  if(pc[1] < 0) pc[1] = 0; if(pc[1] >= size) pc[1] = size-1;
}


bool taImageProc::RotateImage_float(float_Matrix& rotated_img, float_Matrix& orig_img,
                                    float rotate, EdgeMode edge) {
  taVector2i img_size(orig_img.dim(0), orig_img.dim(1));

  int nclrs = 1;
  if(orig_img.dims() == 3) { // rgb
    nclrs = orig_img.dim(2);
    rotated_img.SetGeom(3, img_size.x, img_size.y, nclrs);
  }
  else {
    rotated_img.SetGeom(2, img_size.x, img_size.y);
  }
  taVector2f ctr = ((taVector2f) img_size) / 2.0f;

  rotate *= 2.0f * taMath_float::pi; // convert to radians
  float rot_sin = sin(rotate);
  float rot_cos = cos(rotate);

  bool wrap = (edge == WRAP);

  float_Matrix sc_ary(false);  sc_ary.SetGeom(2, 2, 2);

  int x,y;                      // coords in new image
  for(y=0;y<img_size.y;y++) {
    for(x=0;x<img_size.x;x++) {
      float cx = ((float) x) - ctr.x;
      float cy = ((float) y) - ctr.y;

      float org_x = cx * rot_cos + cy * rot_sin + ctr.x;
      float org_y = cy * rot_cos - cx * rot_sin + ctr.y;

      int pcx[2];
      int pcy[2];
      float pwx[2];
      float pwy[2];

      GetWeightedPixels_float(org_x, img_size.x, pcx, pwx);
      GetWeightedPixels_float(org_y, img_size.y, pcy, pwy);

      int oxi, oyi;
      for(oyi=0;oyi<2;oyi++) {
        for(oxi=0;oxi<2;oxi++) {
          sc_ary.FastEl2d(oxi, oyi) = pwx[oxi] + pwy[oyi];
        }
      }
      taMath_float::vec_norm_sum(&sc_ary);

      float avgs[4] = {};	// init to 0
      taVector2i ic;
      if(wrap) {
        for(oyi=0;oyi<2;oyi++) {
          ic.y = pcy[oyi];
          for(oxi=0;oxi<2;oxi++) {
            ic.x = pcx[oxi];
            ic.Wrap(img_size);      // d/c on half wrap
            if(nclrs > 1) {
              for(int cl=0; cl < nclrs; cl++) {
                avgs[cl] += sc_ary.FastEl2d(oxi, oyi) * orig_img.FastEl3d(ic.x, ic.y, cl);
              }
            }
            else {
              avgs[0] += sc_ary.FastEl2d(oxi, oyi) * orig_img.FastEl2d(ic.x, ic.y);
            }
          }
        }
      }
      else {
        for(oyi=0;oyi<2;oyi++) {
          ic.y = pcy[oyi];
          for(oxi=0;oxi<2;oxi++) {
            ic.x = pcx[oxi];
            if(ic.Clip(img_size)) {
              if(edge == CLIP) continue; // don't bail for BORDER
            }
            if(nclrs > 1) {
              for(int cl=0; cl < nclrs; cl++) {
                avgs[cl] += sc_ary.FastEl2d(oxi, oyi) * orig_img.FastEl3d(ic.x, ic.y, cl);
              }
            }
            else {
              avgs[0] += sc_ary.FastEl2d(oxi, oyi) * orig_img.FastEl2d(ic.x, ic.y);
            }
          }
        }
      }
      if(nclrs > 1) {
	for(int cl=0; cl < nclrs; cl++) {
	  rotated_img.FastEl3d(x,y,cl) = avgs[cl];
	}
      }
      else {
        rotated_img.FastEl2d(x,y) = avgs[0];
      }
    }
  }
  return true;
}

bool taImageProc::CropImage_float(float_Matrix& crop_img, float_Matrix& orig_img,
                                  int crop_width, int crop_height, EdgeMode edge) {
  taVector2i img_size(orig_img.dim(0), orig_img.dim(1));
  taVector2i crop_size(crop_width, crop_height);
  if(crop_size.x < 0) crop_size.x = img_size.x;
  if(crop_size.y < 0) crop_size.y = img_size.y;

  taVector2i img_ctr = img_size / 2;
  taVector2i crop_ctr = crop_size / 2;
  taVector2i img_off = img_ctr - crop_ctr; // offset for 0,0 pixel of cropped image, in orig_img

  int nclrs = 1;
  if(orig_img.dims() == 3) { // rgb
    nclrs = orig_img.dim(2);
    crop_img.SetGeom(3, crop_size.x, crop_size.y, nclrs);
  }
  else {
    crop_img.SetGeom(2, crop_size.x, crop_size.y);
  }

  bool wrap = (edge == WRAP);

  taVector2i ic;
  if(wrap) {
    for(int ny = 0; ny < crop_size.y; ny++) {
      ic.y = img_off.y + ny;
      for(int nx = 0; nx < crop_size.x; nx++) {
        ic.x = img_off.x + nx;
        ic.Wrap(img_size);      // d/c on half wrap
        if(nclrs > 1) {
          for(int i=0;i<nclrs;i++)
            crop_img.FastEl3d(nx, ny, i) = orig_img.FastEl3d(ic.x, ic.y, i);
        }
        else {
          crop_img.FastEl2d(nx, ny) = orig_img.FastEl2d(ic.x, ic.y);
        }
      }
    }
  }
  else {
    for(int ny = 0; ny < crop_size.y; ny++) {
      ic.y = img_off.y + ny;
      for(int nx = 0; nx < crop_size.x; nx++) {
        ic.x = img_off.x + nx;
        if(ic.Clip(img_size)) {
          if(edge == CLIP) continue; // don't bail for BORDER
        }
        if(nclrs > 1) {
          for(int i=0;i<nclrs;i++)
            crop_img.FastEl3d(nx, ny, i) = orig_img.FastEl3d(ic.x, ic.y, i);
        }
        else {
          crop_img.FastEl2d(nx, ny) = orig_img.FastEl2d(ic.x, ic.y);
        }
      }
    }
  }
  return true;
}

bool taImageProc::TransformImage_float(float_Matrix& xformed_img, float_Matrix& orig_img,
                                       float move_x, float move_y, float rotate,
                                       float scale, int crop_width, int crop_height,
                                       EdgeMode edge)
{
  float_Matrix* use_img = &orig_img;
  float_Matrix xlate_img(false);        taBase::Ref(xlate_img);
  float_Matrix rot_img(false);          taBase::Ref(rot_img);
  float_Matrix sc_img(false);           taBase::Ref(sc_img);

  // render border after each xform to keep edges clean..
  if(edge == BORDER) taImageProc::RenderBorder_float(*use_img);
  if((move_x != 0.0f) || (move_y != 0.0f)) {
    taImageProc::TranslateImage_float(xlate_img, *use_img, move_x, move_y, edge);
    use_img = &xlate_img;
    if(edge == BORDER) taImageProc::RenderBorder_float(*use_img);
  }
  if(rotate != 0.0f) {
    taImageProc::RotateImage_float(rot_img, *use_img, rotate, edge);
    use_img = &rot_img;
    if(edge == BORDER) taImageProc::RenderBorder_float(*use_img);
  }
  if(scale != 1.0f) {
    taImageProc::RenderBorder_float(*use_img); // always borderize prior to scaling to avoid probs
    taImageProc::ScaleImage_float(sc_img, *use_img, scale, BORDER); // border!
    use_img = &sc_img;
    if(edge == BORDER) taImageProc::RenderBorder_float(*use_img);
  }
  if(crop_width < 0 && crop_height < 0) {
    xformed_img = *use_img;     // todo: this is somewhat inefficient
  }
  else {
    taImageProc::CropImage_float(xformed_img, *use_img, crop_width, crop_height, edge);
  }
  return true;
}

bool taImageProc::SampleImageWindow_float(float_Matrix& out_img, float_Matrix& in_img,
                                          int win_width, int win_height,
                                          float ctr_x, float ctr_y,
                                          float rotate, float scale,
                                          EdgeMode edge) {
  taVector2i img_size(in_img.dim(0), in_img.dim(1));
  taVector2i win_size(win_width, win_height);

  taVector2f img_ctr(img_size.x * ctr_x, img_size.y * ctr_y);
  taVector2f win_ctr = ((taVector2f) win_size) / 2.0f;

  if(edge == BORDER) taImageProc::RenderBorder_float(in_img);

  rotate *= 2.0f * taMath_float::pi; // convert to radians
  float rot_sin = sin(rotate);
  float rot_cos = cos(rotate);

  int nclrs = 1;
  if(in_img.dims() == 3) { // rgb
    nclrs = in_img.dim(2);
    out_img.SetGeom(3, win_size.x, win_size.y, nclrs);
  }
  else {
    out_img.SetGeom(2, win_size.x, win_size.y);
  }

  float half_sc;
  if(scale < 1.0f)
    half_sc = .5f / scale; // how much on each side of the pixel to include in avg
  else
    half_sc = .5f * scale;

  // scale > 1
  //   o o o    3
  //  /|X|X|\   connections
  // s s s s s  5

  int   half_int = (int)floor(half_sc);
  float extra = half_sc - (float)half_int;
  int n_orig_pix = half_int + 1; // number of pixels to get from original image for each scaled pixel
  // make this static so we don't end up re-doing it every time
  static float_Matrix sc_ary(false);
  static int last_dim = 0;
  const int cur_dim = n_orig_pix*2 + 1;
  if(last_dim != cur_dim) {
    sc_ary.SetGeom(2, cur_dim, cur_dim);
    int x, y;
    for(y=-n_orig_pix; y<= n_orig_pix; y++) {
      float ysc = extra;
      if((y >= -half_int) && (y <= half_int))
        ysc = 1.0f;
      for(x=-n_orig_pix; x<= n_orig_pix; x++) {
        float xsc = extra;
        if((x >= -half_int) && (x <= half_int))
          xsc = 1.0f;
        sc_ary.Set(ysc * xsc, x + n_orig_pix, y + n_orig_pix);
      }
    }
    taMath_float::vec_norm_sum(&sc_ary);          // make it add to 1
    last_dim = cur_dim;
  }

  bool wrap = (edge == WRAP);
  if(wrap) {
    if(nclrs == 1) {
      SampleImageWindow_float_wrap_mono(out_img, in_img, sc_ary, win_size, img_size,
                                        win_ctr, img_ctr, n_orig_pix, rotate, scale);
    }
    else {
      SampleImageWindow_float_wrap_rgb(out_img, in_img, sc_ary, win_size, img_size,
                                        win_ctr, img_ctr, n_orig_pix, rotate, scale);
    }
  }     
  else {
    if(nclrs == 1) {
      SampleImageWindow_float_clip_mono(out_img, in_img, sc_ary, win_size, img_size,
                                        win_ctr, img_ctr, n_orig_pix, rotate, scale, edge);
    }
    else {
      SampleImageWindow_float_clip_rgb(out_img, in_img, sc_ary, win_size, img_size,
                                       win_ctr, img_ctr, n_orig_pix, rotate, scale, edge);
    }
  }

  return true;
}


bool taImageProc::SampleImageWindow_float_wrap_mono
(float_Matrix& out_img, float_Matrix& in_img, float_Matrix& sc_ary,
 taVector2i& win_size, taVector2i& img_size, taVector2f& win_ctr, taVector2f& img_ctr,
 int n_orig_pix, float rotate, float scale) {
  float rot_sin = sin(rotate);
  float rot_cos = cos(rotate);

  int pcx[2];    int pcy[2];
  float pwx[2];  float pwy[2];
  static float_Matrix rot_ary(false);
  rot_ary.SetGeom(2, 2, 2);

  taVector2i wc;
  for(wc.y=0; wc.y<win_size.y; wc.y++) {
    for(wc.x=0; wc.x<win_size.x; wc.x++) {
      taVector2f wcd = ((taVector2f)wc) - win_ctr; // delta from ctr in window
      taVector2f icd = wcd / scale;                    // scaled delta from ctr in img
      taVector2f icr(icd.x * rot_cos + icd.y * rot_sin, // rotated
                     icd.y * rot_cos - icd.x * rot_sin);
      taVector2i icc((int)floor(.5f + icr.x + img_ctr.x), // img center coord
                     (int)floor(.5f + icr.y + img_ctr.y));

      float avgs[4] = {};	// init to 0

      taVector2i oc;             // offsets
      for(oc.y=-n_orig_pix; oc.y<=n_orig_pix; oc.y++) {
        for(oc.x=-n_orig_pix;oc.x<=n_orig_pix;oc.x++) {
          taVector2i ic = icc + oc;
          ic.Wrap(img_size);      // d/c on half wrap
          float sc = sc_ary.FastEl2d(oc.x + n_orig_pix, oc.y + n_orig_pix);

          if(rotate != 0.0f) {
            // now, for each scaling fuzzy-sampled point, correct for rotational aliasing..
            GetWeightedPixels_float(ic.x, img_size.x, pcx, pwx);
            GetWeightedPixels_float(ic.y, img_size.y, pcy, pwy);

            int oxi, oyi;
            for(oyi=0;oyi<2;oyi++) {
              for(oxi=0;oxi<2;oxi++) {
                rot_ary.FastEl2d(oxi, oyi) = pwx[oxi] + pwy[oyi];
              }
            }
            taMath_float::vec_norm_sum(&rot_ary);

            float r_avgs[4] = {};
            taVector2i ric;
            for(oyi=0;oyi<2;oyi++) {
              ric.y = pcy[oyi];
              for(oxi=0;oxi<2;oxi++) {
                ric.x = pcx[oxi];
                ric.Wrap(img_size);      // d/c on half wrap
                float rsc = rot_ary.FastEl2d(oxi, oyi);
                r_avgs[0] += rsc * in_img.FastEl2d(ric.x, ric.y);
              }
            }
            avgs[0] += sc * r_avgs[0];
          }
          else {
            avgs[0] += sc * in_img.FastEl2d(ic.x, ic.y);
          }
        }
      }
      out_img.FastEl2d(wc.x, wc.y) = avgs[0];
    }
  }
  return true;
}

bool taImageProc::SampleImageWindow_float_wrap_rgb
(float_Matrix& out_img, float_Matrix& in_img, float_Matrix& sc_ary,
 taVector2i& win_size, taVector2i& img_size, taVector2f& win_ctr, taVector2f& img_ctr,
 int n_orig_pix, float rotate, float scale) {

  float rot_sin = sin(rotate);
  float rot_cos = cos(rotate);

  int pcx[2];    int pcy[2];
  float pwx[2];  float pwy[2];

  static float_Matrix rot_ary(false);
  rot_ary.SetGeom(2, 2, 2);

  const int nclrs = in_img.dim(2);

  taVector2i wc;
  for(wc.y=0; wc.y<win_size.y; wc.y++) {
    for(wc.x=0; wc.x<win_size.x; wc.x++) {
      taVector2f wcd = ((taVector2f)wc) - win_ctr; // delta from ctr in window
      taVector2f icd = wcd / scale;                    // scaled delta from ctr in img
      taVector2f icr(icd.x * rot_cos + icd.y * rot_sin, // rotated
                     icd.y * rot_cos - icd.x * rot_sin);
      taVector2i icc((int)floor(.5f + icr.x + img_ctr.x), // img center coord
                     (int)floor(.5f + icr.y + img_ctr.y));

      float avgs[4] = {};	// init to 0

      taVector2i oc;             // offsets
      for(oc.y=-n_orig_pix; oc.y<=n_orig_pix; oc.y++) {
        for(oc.x=-n_orig_pix;oc.x<=n_orig_pix;oc.x++) {
          taVector2i ic = icc + oc;
          ic.Wrap(img_size);      // d/c on half wrap
          float sc = sc_ary.FastEl2d(oc.x + n_orig_pix, oc.y + n_orig_pix);

          if(rotate != 0.0f) {
            // now, for each scaling fuzzy-sampled point, correct for rotational aliasing..
            GetWeightedPixels_float(ic.x, img_size.x, pcx, pwx);
            GetWeightedPixels_float(ic.y, img_size.y, pcy, pwy);

            int oxi, oyi;
            for(oyi=0;oyi<2;oyi++) {
              for(oxi=0;oxi<2;oxi++) {
                rot_ary.FastEl2d(oxi, oyi) = pwx[oxi] + pwy[oyi];
              }
            }
            taMath_float::vec_norm_sum(&rot_ary);

            float r_avgs[4] = {};
            taVector2i ric;
            for(oyi=0;oyi<2;oyi++) {
              ric.y = pcy[oyi];
              for(oxi=0;oxi<2;oxi++) {
                ric.x = pcx[oxi];
                ric.Wrap(img_size);      // d/c on half wrap
                float rsc = rot_ary.FastEl2d(oxi, oyi);

                for(int cl=0; cl < nclrs; cl++) {
                  r_avgs[cl] += rsc * in_img.FastEl3d(ric.x, ric.y, cl);
                }
              }
            }
            for(int cl=0; cl < nclrs; cl++) {
              avgs[cl] += sc * r_avgs[cl];
            }
          }
          else {
            for(int cl=0; cl < nclrs; cl++) {
              avgs[cl] += sc * in_img.FastEl3d(ic.x, ic.y, cl);
            }
          }
        }
      }
      for(int cl=0; cl < nclrs; cl++) {
        out_img.FastEl3d(wc.x, wc.y, cl) = avgs[cl];
      }
    }
  }
  return true;
}

bool taImageProc::SampleImageWindow_float_clip_mono
(float_Matrix& out_img, float_Matrix& in_img, float_Matrix& sc_ary,
 taVector2i& win_size, taVector2i& img_size, taVector2f& win_ctr, taVector2f& img_ctr,
 int n_orig_pix, float rotate, float scale, EdgeMode edge) {
  float rot_sin = sin(rotate);
  float rot_cos = cos(rotate);

  int pcx[2];    int pcy[2];
  float pwx[2];  float pwy[2];

  static float_Matrix rot_ary(false);
  rot_ary.SetGeom(2, 2, 2);

  taVector2i wc;
  for(wc.y=0; wc.y<win_size.y; wc.y++) {
    for(wc.x=0; wc.x<win_size.x; wc.x++) {
      taVector2f wcd = ((taVector2f)wc) - win_ctr; // delta from ctr in window
      taVector2f icd = wcd / scale;                    // scaled delta from ctr in img
      taVector2f icr(icd.x * rot_cos + icd.y * rot_sin, // rotated
                     icd.y * rot_cos - icd.x * rot_sin);
      taVector2i icc((int)floor(.5f + icr.x + img_ctr.x), // img center coord
                     (int)floor(.5f + icr.y + img_ctr.y));

      float avgs[4] = {};	// init to 0

      taVector2i oc;             // offsets
      for(oc.y=-n_orig_pix; oc.y<=n_orig_pix; oc.y++) {
        for(oc.x=-n_orig_pix;oc.x<=n_orig_pix;oc.x++) {
          taVector2i ic = icc + oc;
          if(ic.Clip(img_size)) {
            if(edge == CLIP) continue; // don't bail for BORDER
          }
          float sc = sc_ary.FastEl2d(oc.x + n_orig_pix, oc.y + n_orig_pix);

          if(rotate != 0.0f) {
            // now, for each scaling fuzzy-sampled point, correct for rotational aliasing..
            GetWeightedPixels_float(ic.x, img_size.x, pcx, pwx);
            GetWeightedPixels_float(ic.y, img_size.y, pcy, pwy);

            int oxi, oyi;
            for(oyi=0;oyi<2;oyi++) {
              for(oxi=0;oxi<2;oxi++) {
                rot_ary.FastEl2d(oxi, oyi) = pwx[oxi] + pwy[oyi];
              }
            }
            taMath_float::vec_norm_sum(&rot_ary);

            float r_avgs[4] = {};
            taVector2i ric;
            for(oyi=0;oyi<2;oyi++) {
              ric.y = pcy[oyi];
              for(oxi=0;oxi<2;oxi++) {
                ric.x = pcx[oxi];
                if(ric.Clip(img_size)) {
                  if(edge == CLIP) continue; // don't bail for BORDER
                }
                float rsc = rot_ary.FastEl2d(oxi, oyi);

                r_avgs[0] += rsc * in_img.FastEl2d(ric.x, ric.y);
              }
            }
            avgs[0] += sc * r_avgs[0];
          }
          else {
            avgs[0] += sc * in_img.FastEl2d(ic.x, ic.y);
          }
        }
      }
      out_img.FastEl2d(wc.x, wc.y) = avgs[0];
    }
  }
  return true;
}

bool taImageProc::SampleImageWindow_float_clip_rgb
(float_Matrix& out_img, float_Matrix& in_img, float_Matrix& sc_ary,
 taVector2i& win_size, taVector2i& img_size, taVector2f& win_ctr, taVector2f& img_ctr,
 int n_orig_pix, float rotate, float scale, EdgeMode edge) {
  float rot_sin = sin(rotate);
  float rot_cos = cos(rotate);

  int pcx[2];    int pcy[2];
  float pwx[2];  float pwy[2];

  static float_Matrix rot_ary(false);
  rot_ary.SetGeom(2, 2, 2);

  const int nclrs = in_img.dim(2);

  taVector2i wc;
  for(wc.y=0; wc.y<win_size.y; wc.y++) {
    for(wc.x=0; wc.x<win_size.x; wc.x++) {
      taVector2f wcd = ((taVector2f)wc) - win_ctr; // delta from ctr in window
      taVector2f icd = wcd / scale;                    // scaled delta from ctr in img
      taVector2f icr(icd.x * rot_cos + icd.y * rot_sin, // rotated
                     icd.y * rot_cos - icd.x * rot_sin);
      taVector2i icc((int)floor(.5f + icr.x + img_ctr.x), // img center coord
                     (int)floor(.5f + icr.y + img_ctr.y));

      float avgs[4] = {};	// init to 0

      taVector2i oc;             // offsets
      for(oc.y=-n_orig_pix; oc.y<=n_orig_pix; oc.y++) {
        for(oc.x=-n_orig_pix;oc.x<=n_orig_pix;oc.x++) {
          taVector2i ic = icc + oc;
          if(ic.Clip(img_size)) {
            if(edge == CLIP) continue; // don't bail for BORDER
          }
          float sc = sc_ary.FastEl2d(oc.x + n_orig_pix, oc.y + n_orig_pix);

          if(rotate != 0.0f) {
            // now, for each scaling fuzzy-sampled point, correct for rotational aliasing..
            GetWeightedPixels_float(ic.x, img_size.x, pcx, pwx);
            GetWeightedPixels_float(ic.y, img_size.y, pcy, pwy);

            int oxi, oyi;
            for(oyi=0;oyi<2;oyi++) {
              for(oxi=0;oxi<2;oxi++) {
                rot_ary.FastEl2d(oxi, oyi) = pwx[oxi] + pwy[oyi];
              }
            }
            taMath_float::vec_norm_sum(&rot_ary);

            float r_avgs[4] = {};
            taVector2i ric;
            for(oyi=0;oyi<2;oyi++) {
              ric.y = pcy[oyi];
              for(oxi=0;oxi<2;oxi++) {
                ric.x = pcx[oxi];
                if(ric.Clip(img_size)) {
                  if(edge == CLIP) continue; // don't bail for BORDER
                }
                float rsc = rot_ary.FastEl2d(oxi, oyi);

                for(int cl=0; cl < nclrs; cl++) {
                  r_avgs[cl] += rsc * in_img.FastEl3d(ric.x, ric.y, cl);
                }
              }
            }
            for(int cl=0; cl < nclrs; cl++) {
              avgs[cl] += sc * r_avgs[cl];
            }
          }
          else {
            for(int cl=0; cl < nclrs; cl++) {
              avgs[cl] += sc * in_img.FastEl3d(ic.x, ic.y, cl);
            }
          }
        }
      }
      for(int cl=0; cl < nclrs; cl++) {
        out_img.FastEl3d(wc.x, wc.y, cl) = avgs[cl];
      }
    }
  }
  return true;
}

bool taImageProc::AttentionFilter(float_Matrix& mat, float radius_pct) {
  if(mat.dims() == 3) {    // an rgb guy
    int nclrs = mat.dim(2);
    for(int i=0;i<nclrs;i++) {
      float_Matrix* cmp = mat.GetFrameSlice(i);
      taBase::Ref(cmp);
      AttentionFilter(*cmp, radius_pct);
      taBase::unRefDone(cmp);
    }
    return true;
  }
  taVector2i img_size(mat.dim(0), mat.dim(1));
  taVector2i img_ctr = img_size / 2;

  float max_radius = taMath_float::max(img_ctr.x, img_ctr.y);
  float scale_x = max_radius / (float)img_ctr.x;
  float scale_y = max_radius / (float)img_ctr.y;
  float radius = radius_pct * max_radius;
  float r_sq = radius * radius;

  for (int y = 0; y < img_size.y; y++) {
    for (int x = 0; x < img_size.x; x++) {
      float dist_x = scale_x * float(x - img_ctr.x);
      float dist_y = scale_y * float(y - img_ctr.y);
      float dist_sq = dist_x * dist_x + dist_y * dist_y;
      if (dist_sq > r_sq) {
        float mult = (float) r_sq / (float) dist_sq;
        mat.FastEl2d(x,y) *= mult;
      }
    }
  }
  int idx;
  float max_v = taMath_float::vec_max(&mat, idx);
  if(max_v > .01f)
    taMath_float::vec_norm_max(&mat);
  return true;
}

// todo: handle rgb
bool taImageProc::Blur(float_Matrix& img, int kernel_size) {
  bool rval = false;
  
  if (img.dims() > 2) {
    taMisc::Error("taImageProc::Blur", "image must be greyscale");
    return false;
  }
  if (kernel_size < 3) {
    taMisc::Error("taImageProc::Blur", "minimum window size is 3");
    return false;
  }
  
  // create the kernel - always square - equal weighting throughout
  float_Matrix kernel = float_Matrix(2, kernel_size, kernel_size);
  for(int yi=0; yi<kernel_size; yi++) {
    for(int xi=0; xi<kernel_size; xi++) {
      kernel.FastEl2d(xi, yi) = 1;
    }
  }
  rval = taMath_float::vec_norm_sum(&kernel, 1.0);
  
  // create matrix for result of the convolution
  float_Matrix* out_matrix;
  taVector2i img_size(img.dim(0), img.dim(1));
  out_matrix = new float_Matrix(2, img_size.x, img_size.y);
  
  int offset = (kernel_size-1)/2;
  
  for(int i=0; i<img.Frames(); i++) {
    for(int j=0; j<img.FrameSize(); j++) {
      float sum = 0.0;
      float dnorm = 0.0;
      for(int m=0; m<kernel_size; m++) {
        for(int n=0; n<kernel_size; n++) {
          int iIdx = i+n-offset;
          int jIdx = j+m-offset;
          if (iIdx < 0 || iIdx >= img.Frames() || jIdx < 0 || jIdx >= img.FrameSize()) {
            dnorm += kernel.FastEl2d(n,m);
          }
          else {
            sum += kernel.FastEl2d(n,m)*img.FastEl2d(iIdx, jIdx);
          }
        }
      }
      if(dnorm > 0.0 && dnorm < 1.0) { // renorm
        sum /= (1.0 - dnorm);
      }
      out_matrix->FastEl2d(i,j) = sum;
    }
  }
  
  img = out_matrix;
  return rval;
}

bool taImageProc::SetGaussianKernel(float_Matrix* kernel, float sigma) {
  // Generate a kernel by sampling the Gaussian function
  int kernel_size = kernel->dim(0);
  int uc, vc;
  float g, sum;
  sum = 0;
  for(int u=0; u<kernel_size; u++) {
    for(int v=0; v<kernel_size; v++) {
      // Center the Gaussian sample so max is at u,v = 10,10
      uc = u - (kernel_size-1)/2;
      vc = v - (kernel_size-1)/2;
      // Calculate and save
      g = exp(-(uc*uc+vc*vc)/(2*sigma*sigma));
      sum += g;
      kernel->FastEl2d(u,v) = g;
    }
  }
  // Normalize it
  for(int u=0; u<kernel_size; u++) {
    for(int v=0; v<kernel_size; v++) {
      kernel->FastEl2d(u,v) /= sum;
    }
  }
  return true;
}

bool taImageProc::BlobBlurOcclude(float_Matrix& img, float pct_occlude,
                                  float circ_radius, float gauss_sig,
                                  EdgeMode edge, bool use_border_clr) {
  taVector2i img_size(img.dim(0), img.dim(1));

  float gauss_eff = gauss_sig * (float)img_size.x;
  float radius_eff = circ_radius * (float)img_size.x;
  int filt_half = (int)radius_eff + (int)(gauss_eff * 2);
  int filt_wd = filt_half * 2;
  taVector2i ntot = (img_size / filt_half) + 1;
  int totblob = ntot.Product();
  int nblob = (int) (2.5f * pct_occlude * (float)totblob + 0.5f);
  if(pct_occlude == 1.0f) nblob *= 2; // really nuke it for sure!

  float_Matrix filt_wt;
  filt_wt.SetGeom(2, filt_wd, filt_wd);
  float ctr = (float)(filt_wd-1) * .5f;
  for(int yi=0; yi < filt_wd; yi++) {
    float y = (float)yi - ctr;
    for(int xi=0; xi < filt_wd; xi++) {
      float x = (float)xi - ctr;
      float d = taMath_float::hypot(x,y);
      float fv = 1.0f;
      if(d > radius_eff) {
        float gd = (d-radius_eff) / gauss_eff;
        fv = expf(-(gd * gd)/2.0f);
      }
      filt_wt.FastEl2d(xi, yi) = fv;
    }
  }

  float_Matrix filt_cnv;
  filt_cnv.CopyFrom(&filt_wt);
  taMath_float::vec_norm_sum(&filt_cnv, 1.0f); // conv = sum norm
  taMath_float::vec_norm_max(&filt_wt, 1.0f);  // weights = max norm

  int nclrs = 1;
  if(img.dims() == 3) { // rgb
    nclrs = img.dim(2);
  }

  bool wrap = (edge == WRAP);

  float brd_clr[4];

  if(use_border_clr) {
    GetBorderColor_float(img, brd_clr[0], brd_clr[1], brd_clr[2], brd_clr[3]);
  }

  taVector2i ic;
  taVector2i icw;
  for(int blob=0; blob < nblob; blob++) {
    ic.x = Random::IntMinMax(0, img_size.x);
    ic.y = Random::IntMinMax(0, img_size.y);

    if(nclrs > 1) {
      for(int rgb=0; rgb<nclrs; rgb++) {
        float clr = 0.0f;
        if(use_border_clr) {
          clr = brd_clr[rgb];
        }
        else {
          for(int yi=0; yi< filt_wd; yi++) {
            for(int xi=0; xi< filt_wd; xi++) {
              icw.x = ic.x + xi - filt_half;
              icw.y = ic.y + yi - filt_half;
              icw.WrapClip(wrap, img_size); // use edges if clipping
              float iv = img.FastEl3d(icw.x, icw.y, rgb);
              clr += filt_cnv.FastEl2d(xi, yi) * iv;
            }
          }
        }

        for(int yi=0; yi< filt_wd; yi++) {
          for(int xi=0; xi< filt_wd; xi++) {
            icw.x = ic.x + xi - filt_half;
            icw.y = ic.y + yi - filt_half;
            if(icw.WrapClip(wrap, img_size)) {
              if(!wrap) continue;
            }
            float& iv = img.FastEl3d(icw.x, icw.y, rgb);
            float wt = filt_wt.FastEl2d(xi, yi);
            float nw_iv = (1.0f - wt) * iv + wt * clr;
            iv = nw_iv;
          }
        }
      }
    }
    else {
      float clr = 0.0f;
      if(use_border_clr) {
        clr = brd_clr[0];
      }
      else {
        for(int yi=0; yi< filt_wd; yi++) {
          for(int xi=0; xi< filt_wd; xi++) {
            icw.x = ic.x + xi - filt_half;
            icw.y = ic.y + yi - filt_half;
            icw.WrapClip(wrap, img_size); // use edges if clipping
            float iv = img.FastEl2d(icw.x, icw.y);
            clr += filt_cnv.FastEl2d(xi, yi) * iv;
          }
        }
      }

      for(int yi=0; yi< filt_wd; yi++) {
        for(int xi=0; xi< filt_wd; xi++) {
          icw.x = ic.x + xi - filt_half;
          icw.y = ic.y + yi - filt_half;
          if(icw.WrapClip(wrap, img_size)) {
            if(!wrap) continue;
          }
          float& iv = img.FastEl2d(icw.x, icw.y);
          float wt = filt_wt.FastEl2d(xi, yi);
          float nw_iv = (1.0f - wt) * iv + wt * clr;
          iv = nw_iv;
        }
      }
    }
  }
  return true;
}

bool taImageProc::BubbleMask(float_Matrix& img, int n_bubbles, float bubble_sig,
			     float_Matrix* foreground, int_Matrix* bubble_coords) {
  // floor value for mask
  float floor_thr=pow(10.0f, -8.0f);

  // get the img size -- need for lots of stuff
  taVector2i img_size(img.dim(0), img.dim(1));

  // create the mask and temporary mask -- if bubble_coords is specified, init it for saving coords
  float_Matrix mask;
  mask.SetGeom(2, img_size.x, img_size.y);
  float_Matrix mask_tmp;
  mask_tmp.SetGeom(2, img_size.x, img_size.y);
  for(int yi=0; yi< img_size.y; yi++) {
    for(int xi=0; xi< img_size.x; xi++) {
      mask.FastEl2d(xi, yi) = 0.0f;
      mask_tmp.FastEl2d(xi, yi) = 0.0f;
    }
  }
 
  // for saving bubble coordinates
  if(bubble_coords != NULL) {
    bubble_coords->SetGeom(2, 2, n_bubbles);
  }

  // ndgrid from matlab
  float_Matrix ndgridx;
  ndgridx.SetGeom(2, img_size.x, img_size.y);
  float_Matrix ndgridy;
  ndgridy.SetGeom(2, img_size.x, img_size.y);
  for(int yi=0; yi< img_size.y; yi++) {
    for(int xi=0; xi< img_size.x; xi++) {
      ndgridx.FastEl2d(xi, yi) = (float)xi;
      ndgridy.FastEl2d(xi, yi) = (float)yi;
    }
  }

  // apply bubbles to the mask
  for(int bubble=0; bubble < n_bubbles; bubble++) {
    // random center
    int xc = Random::IntMinMax(0, img_size.x-1);
    int yc = Random::IntMinMax(0, img_size.y-1);

    // save the bubble coords
    if(bubble_coords != NULL) {
      bubble_coords->FastEl2d(0, bubble) = xc;
      bubble_coords->FastEl2d(1, bubble) = yc;
    }

    for(int yi=0; yi< img_size.y; yi++) {
      for(int xi=0; xi< img_size.x; xi++) {
	float &mask_iv = mask_tmp.FastEl2d(xi, yi);
	float ndgridx_val = ndgridx.FastEl2d(xi, yi);
	float ndgridy_val = ndgridy.FastEl2d(xi, yi);
	// key formula -- note that bubble_sig is in normalized image coords here, assumes image is square (uses img_size.x)
	mask_iv = expf(-(pow(ndgridx_val-xc,2.0f) + pow(ndgridy_val-yc,2.0f))/2.0f/pow(bubble_sig*float(img_size.x),2.0f));
      }
    }

    // normalize mask each time, and combine with all masks using max
    taMath_float::vec_norm_max(&mask_tmp);
    for(int yi=0; yi< img_size.y; yi++) {
      for(int xi=0; xi< img_size.x; xi++) {
	float &mask_iv = mask.FastEl2d(xi, yi);
	float mask_tmp_iv = mask_tmp.FastEl2d(xi, yi);
	mask_iv = MAX(mask_iv, mask_tmp_iv);
      }
    }
  }

  // floor anything in mask below floor thresh
  taMath_float::vec_threshold_low(&mask, floor_thr, 0.0f);

  // combine the image and foreground using the mask as a weighting matrix  
  // if foreground is null, just use a blank image with border color
  float_Matrix fg;
  if(foreground == NULL) {
    float brd_clr[4];
    GetBorderColor_float(img, brd_clr[0], brd_clr[1], brd_clr[2], brd_clr[3]);
    fg.SetGeom(2, img_size.x, img_size.y);
    for(int yi=0; yi< img_size.y; yi++) {
      for(int xi=0; xi< img_size.x; xi++) {
	fg.FastEl2d(xi, yi) = brd_clr[0];
      }
    }
  }
  for(int yi=0; yi< img_size.y; yi++) {
    for(int xi=0; xi< img_size.x; xi++) {
      float &img_iv = img.FastEl2d(xi, yi);
      float mask_iv = mask.FastEl2d(xi, yi);
      float foreground_iv;
      if(foreground == NULL) {  foreground_iv = fg.FastEl2d(xi, yi); }
      else { foreground_iv = foreground->FastEl2d(xi, yi); }
      img_iv = mask_iv*img_iv + (1.0f-mask_iv)*foreground_iv;
    }
  }
  
  return true;
}

bool taImageProc::ReplaceColor(float_Matrix& img,
                               float oc_r, float oc_g, float oc_b, float oc_a,
                               float nc_r, float nc_g, float nc_b, float nc_a,
                               float tol, bool repl_nonmatch) {
  taVector2i img_size(img.dim(0), img.dim(1));

  float ocs[4];
  ocs[0] = oc_r; ocs[1] = oc_g; ocs[2] = oc_b; ocs[3] = oc_a;
  float ncs[4];
  ncs[0] = nc_r; ncs[1] = nc_g; ncs[2] = nc_b; ncs[3] = nc_a;
  
  // different processing depending on whether image is rgb or gray
  if(img.dims() == 3) { // rgb or rgba
    int nclrs = img.dim(2);
    for(int yi=0; yi< img_size.y; yi++) {
      for(int xi=0; xi< img_size.x; xi++) {
        bool match = true;
        for(int cl=0; cl < nclrs; cl++) {
          float val = img.FastEl3d(xi, yi, cl);
          if(fabs(val - ocs[cl]) > tol) {
            match = false;
            break;
          }
        }
        if(repl_nonmatch)
          match = !match;
        if(match) {
          for(int cl=0; cl < nclrs; cl++) {
            float& val = img.FastEl3d(xi, yi, cl);
            val = ncs[cl];
          }
        }
      }
    }
  }
  else { // grayscale
    for(int yi=0; yi< img_size.y; yi++) {
      for(int xi=0; xi< img_size.x; xi++) {      	        	  	
        float& val = img.FastEl2d(xi, yi);
        bool match = true;
        if(fabs(val - oc_r) > tol) {
          match = false;
        }
        if(repl_nonmatch)
          match = !match;
        if(match) {
          val = nc_r;
        }
      }
    }
  }
  return true;
}

bool taImageProc::AdjustContrast(float_Matrix& img, float new_contrast, float bg_color) {
  taVector2i img_size(img.dim(0), img.dim(1));
  
  if(bg_color > 1.0f) {	// hopefully user is smart enough not to use negative value aside from -1
    taMisc::Error("bg_color must be between 0 and 1");
    return false;
  }

  // if bg color not specified, use border color
  float brd_clr[4];
  if(bg_color == -1) {
    GetBorderColor_float(img, brd_clr[0], brd_clr[1], brd_clr[2], brd_clr[3]);  
  }
  else if(bg_color < 0.0f || bg_color > 1.0f) {
    taMisc::Error("bg_color must be between 0 and 1");
  }
  else {
    brd_clr[0] = bg_color;
    brd_clr[1] = bg_color;
    brd_clr[2] = bg_color;
    brd_clr[3] = 1.0f;
  }

  // different processing depending on whether image is rgb or gray
  if(img.dims() == 3) { // rgb or rgba
    int nclrs = img.dim(2);
    for(int yi=0; yi< img_size.y; yi++) {
      for(int xi=0; xi< img_size.x; xi++) {
        for(int cl=0; cl < 3; cl++) { // only use rgb for this loop
          float& val = img.FastEl3d(xi, yi, cl);
          val = ((val - brd_clr[cl])*new_contrast) + brd_clr[cl];
        }
      }
    }
  }
  else { // grayscale
    for(int yi=0; yi< img_size.y; yi++) {
      for(int xi=0; xi< img_size.x; xi++) {      	        	  	
        float& iv = img.FastEl2d(xi, yi);
        iv = ((iv-brd_clr[0])*new_contrast)+brd_clr[0]; // just use red channel
      }
    }
  }
  return true;
}

bool taImageProc::CompositeImages(float_Matrix& img1, float_Matrix& img2) {
  if(img1.dims() != 3) {
    taMisc::Error("img1 must be rgba format -- is only 2d greyscale");
    return false;
  }
  if(img1.dim(2) != 4) {
    taMisc::Error("img1 must be rgba format -- does not have 4 colors in outer dimension");
    return false;
  }

  if(img1.dim(0) != img2.dim(0) || img1.dim(1) != img2.dim(1)) {
    taMisc::Error("img1 must be same size as img2");
    return false;
  }

  int nclrs = 1; // grayscale
  if(img2.dims() == 3) {
    nclrs = img2.dim(2);
  }

  taVector2i img_size(img1.dim(0), img1.dim(1));

  for(int yi=0; yi< img_size.y; yi++) {
    for(int xi=0; xi< img_size.x; xi++) {
      const float i1alpha = img1.FastEl3d(xi, yi, 3); // much faster to cache these values!!
      const float i1alpha_c = 1.0f - img1.FastEl3d(xi, yi, 3);
      if(nclrs > 1) {
				for(int cl=0; cl < 3; cl++) { // only use rgb for this loop
				  float& i1clr = img1.FastEl3d(xi, yi, cl);
	 	 			i1clr = i1clr * i1alpha + img2.FastEl3d(xi,yi,cl) * i1alpha_c; // assume img2 has no alpha channel
				}
  		}
    	else {
				for(int cl=0; cl < 3; cl++) {
	  			float& i1clr = img1.FastEl3d(xi, yi, cl);
	  			i1clr = i1clr * i1alpha + img2.FastEl2d(xi,yi) * i1alpha_c; // // assume img2 has no alpha channel
				}
    	}
  	}
	}
  return true;
}

bool taImageProc::FlipY(float_Matrix& img1) {
  float * tmp_mtx = (float *)malloc(sizeof(float) * img1.dim(0)*img1.dim(1)*img1.dim(2));
  for (int yi = 0; yi < img1.dim(1); yi++) {
    for (int xi = 0; xi < img1.dim(0); xi++) {
      for (int ci = 0; ci < img1.dim(2); ci++) {
	tmp_mtx[yi*img1.dim(0)*img1.dim(2) + xi*img1.dim(2) + ci] = img1.FastEl3d(xi, yi, ci); 
      }
    }
  }
  for (int yi = 0; yi < img1.dim(1); yi++) {
    for (int xi = 0; xi < img1.dim(0); xi++) {
      for (int ci = 0; ci < img1.dim(2); ci++) {
	img1.FastEl3d(xi,img1.dim(1) - yi,ci) = tmp_mtx[yi*img1.dim(0)*img1.dim(2) + xi*img1.dim(2) + ci];
      }
    }
  }
  free(tmp_mtx);
  return true;
}

bool taImageProc::CompositeAndCenterImage(float_Matrix& img1, float_Matrix& img2) {
  if(img1.dims() != 3) {
    taMisc::Error("img1 must be rgba format -- is only 2d greyscale");
    return false;
  }
  if(img1.dim(2) != 4) {
    taMisc::Error("img1 must be rgba format -- does not have 4 colors in outer dimension");
    return false;
  }
  
  if(img1.dim(0) < img2.dim(0) || img1.dim(1) < img2.dim(1)) {
    taMisc::Error("img2 must be at least the size of img1");
    return false;
  }
  
  taVector2i img_1_size(img1.dim(0), img1.dim(1));
  taVector2i img_2_size(img2.dim(0), img2.dim(1));
  int x = img_1_size.x/2 - .5*img_2_size.x;
  int y = img_1_size.y/2 - .5*img_2_size.y;
  return CompositePartialImages(img1, x, y, img2);
}

bool taImageProc::CompositePartialImages(float_Matrix& img1, int x, int y, float_Matrix& img2) {
  if(img1.dims() != 3) {
    taMisc::Error("img1 must be rgba format -- is only 2d greyscale");
    return false;
  }
  if(img1.dim(2) != 4) {
    taMisc::Error("img1 must be rgba format -- does not have 4 colors in outer dimension");
    return false;
  }

  if(img1.dim(0) < img2.dim(0) || img1.dim(1) < img2.dim(1)) {
    taMisc::Error("img2 must be at least the size of img1");
    return false;
  }

  int nclrs = 1; // grayscale
  if(img2.dims() == 3) {
    nclrs = img2.dim(2);
  }

  taVector2i img_size(std::min(img2.dim(0), img1.dim(0) - x), std::min(img2.dim(1),img1.dim(1) - y));

  for(int yi=0; yi< img_size.y; yi++) {
    for(int xi=0; xi< img_size.x; xi++) {
      const float i1alpha = img2.FastEl3d(xi, yi, 3); // much faster to cache these values!!
      const float i1alpha_c = 1.0f - i1alpha;
      if(nclrs > 1) {
	for(int cl=0; cl < 3; cl++) { // only use rgb for this loop
	  float& i1clr = img1.FastEl3d(xi + x, yi + y, cl);
	  i1clr = i1clr * i1alpha_c + img2.FastEl3d(xi,yi,cl) * i1alpha;
	}
      }
      else {
	for(int cl=0; cl < 3; cl++) {
	  float& i1clr = img1.FastEl3d(xi + x, yi + y, cl);
	  i1clr = i1clr * i1alpha_c + img2.FastEl2d(xi,yi) * i1alpha; // // assume img2 has no alpha channel
	}
      }
    }
  }
  return true;
}

bool taImageProc::OverlayImages(float_Matrix& img1, float_Matrix& img2) {  
  if(img1.dims() != img2.dims()) {
    taMisc::Error("img1 and img2 must either both be grayscale or both be color");
    return false;
  }
  if(img1.dim(0) < img2.dim(0) || img1.dim(1) < img2.dim(1)) {
    taMisc::Error("img1 must be same size as or larger than img2");
    return false;
  }

  int nclrs = 1;
  if(img1.dims() == 3) {
    nclrs = img1.dim(2);
  }

  int xoff = (int)(img1.dim(0)*0.5f - img2.dim(0)*0.5f); // center of x dim, 0 if both are the same
  int yoff = (int)(img1.dim(1)*0.5f - img2.dim(1)*0.5f); // center of x dim, 0 if both are the same
  
  for(int yi=0; yi< img2.dim(1); yi++) {
    for(int xi=0; xi< img2.dim(0); xi++) {
      if(nclrs > 1) {
	for(int di=0; di < nclrs; di++) {
	  float& i1pix = img1.FastEl3d(xi+xoff, yi+yoff, di);
	  i1pix = img2.FastEl3d(xi, yi, di);
	}
      }
      else {
	float& i1pix = img1.FastEl2d(xi+xoff, yi+yoff);
	i1pix = img2.FastEl2d(xi, yi);
      }
    }
  }
  return true;
}
