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

#ifndef TiledSubGpRFPrjnSpec_h
#define TiledSubGpRFPrjnSpec_h 1

// parent includes:
#include <ProjectionSpec>

// member includes:
#include <taVector2i>
#include <MinMaxRange>
#include <int_Array>

// declare all other types mentioned but not required to include:

eTypeDef_Of(TiledSubGpRFPrjnSpec);

class E_API TiledSubGpRFPrjnSpec : public ProjectionSpec {
  // Tiled receptive field projection spec for topographic tiled group-to-group connections, with a sub-tiling of unit groups within the larger receptive field tiling, to divide the larger problem into more managable subsets: connects entire receiving layer unit groups with overlapping tiled regions of sending layer groups -- if init_wts is on, gaussian topographic weights are initialized
INHERITED(ProjectionSpec)
public:
  enum InitWtsType {
    GAUSSIAN,                    // standard gaussian weight distribution
  };
  
  taVector2i	send_gp_size;		// number of groups in the sending receptive field
  taVector2i	send_gp_skip;		// number of groups to skip per each recv group (typically 1/2 of the size for nice overlap)
  taVector2i	send_gp_start;		// starting offset for sending groups -- for wrapping this was previously automatically set to -send_gp_skip (else 0), but you can now set this to anything you want
  taVector2i	send_subgp_size;        // number of sub-groups in the sending receptive field -- these sub-groups are nested within the larger topographic receiptive field tiling
  taVector2i	recv_subgp_size;        // number of sub-groups in the receiving receptive field -- these sub-groups are nested within the larger topographic receiptive field tiling
  bool		wrap;			// if true, then connectivity has a wrap-around structure so it starts at -gp_skip (wrapped to right/top) and goes +gp_skip past the right/top edge (wrapped to left/bottom)
  bool		reciprocal;		// if true, make the appropriate reciprocal connections for a backwards projection from recv to send
  InitWtsType   wts_type;               // #CONDSHOW_ON_init_wts how to initialize the random initial weights
  float		gauss_sig;		// #CONDSHOW_ON_init_wts&&wts_type:GAUSSIAN gaussian sigma (width), in normalized units where entire distance across sending layer is 1.0
  float         gauss_ctr_mv;           // #CONDSHOW_ON_init_wts&&wts_type:GAUSSIAN how much the center of the gaussian moves with respect to the position of the receiving unit within its unit group -- 1.0 = centers span the entire range of the receptive field
  MinMaxRange	wt_range;
  // #CONDSHOW_ON_init_wts range of weakest (min) to strongest (max) weight values generated -- for bimodal, min and max are the two means of the bimodal distribution
  float         p_high;
  // #CONDSHOW_ON_init_wts&&wts_type:BIMODAL_PERMUTED probability of generating high values for bimodal permuted case

  taVector2i 	trg_recv_geom;	// #READ_ONLY #SHOW target receiving layer gp geometry -- computed from send and rf_width, move by TrgRecvFmSend button, or given by TrgSendFmRecv
  taVector2i 	trg_send_geom;	// #READ_ONLY #SHOW target sending layer geometry -- computed from recv and rf_width, move by TrgSendFmRecv button, or given by TrgRecvFmSend
  int_Array     high_low_wts;   // #HIDDEN #NO_SAVE for bimodal permuted case, gives order of high and low weight values

  void	Init_Weights_Prjn(Projection* prjn, ConGroup* cg, Network* net,
                          int thr_no) override;

  virtual void	Init_Weights_Gaussian(Projection* prjn, ConGroup* cg, Network* net,
                                      int thr_no);
  // gaussian initial weights

  void  Connect_impl(Projection* prjn, bool make_cons) override;

  virtual void	Connect_UnitGroup(Projection* prjn, Layer* recv_lay, Layer* send_lay,
				  int rgpidx, int sgpidx, bool make_cons);
  // #IGNORE connect one unit group to another -- rgpidx = recv unit group idx, sgpidx = send unit group idx

  virtual bool	TrgRecvFmSend(int send_x, int send_y);
  // #BUTTON compute target recv layer geometry based on given sending layer geometry -- updates trg_recv_geom and trg_send_geom members, including fixing send to be an appropriate even multiple of rf_move -- returns true if send values provided result are same "good" ones that come out the end
  virtual bool	TrgSendFmRecv(int recv_x, int recv_y);
  // #BUTTON compute target recv layer geometry based on given sending layer geometry -- updates trg_recv_geom and trg_send_geom members, including fixing recv to be an appropriate even multiple of rf_move --  -- returns true if send values provided result are same "good" ones that come out the end

  TA_SIMPLE_BASEFUNS(TiledSubGpRFPrjnSpec);
protected:
  void  UpdateAfterEdit_impl() override;
  
private:
  void Initialize();
  void Destroy()     { };
};

#endif // TiledSubGpRFPrjnSpec_h
