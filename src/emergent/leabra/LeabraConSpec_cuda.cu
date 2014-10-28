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

#include "LeabraConSpec_cuda.h"

LeabraConSpecCuda::LeabraConSpecCuda() {
  Initialize();
}

LeabraConSpecCuda::~LeabraConSpecCuda() {
  FreeCudaArrays();
}

void LeabraConSpecCuda::Initialize() {
  n_units = 0;
  own_cons_cnt = 0;
  ptr_cons_cnt = 0;
  own_units_x_cons = 0;
  ptr_units_x_cons = 0;

  own_cons_mem_h = NULL;
  own_cons_mem_d = NULL;
  ptr_cons_mem_h = NULL;
  ptr_cons_mem_d = NULL;

  units_h = NULL;
  units_d = NULL;
  con_mem_idxs_h = NULL;
  con_mem_idxs_d = NULL;
  con_allocs_h = NULL;
  con_allocs_d = NULL;
  con_sizes_h = NULL;
  con_sizes_d = NULL;
  unit_starts_h = NULL;

  cur_send_net_n = 0;
  cur_send_net_h = NULL;
  cur_send_net_d = NULL;
  send_net_acts_h = NULL;
  send_net_acts_d = NULL;
  send_netin_tmp_h = NULL;
  send_netin_tmp_d = NULL;
}

void LeabraConSpecCuda::FreeCudaArrays() {
  if(own_cons_mem_d)
    cudaFree(own_cons_mem_d);
  if(ptr_cons_mem_d)
    cudaFree(ptr_cons_mem_d);

  if(units_h)
    free(units_h);
  if(units_d)
    cudaFree(units_d);

  if(con_mem_idxs_h)
    free(con_mem_idxs_h);
  if(con_mem_idxs_d)
    cudaFree(con_mem_idxs_d);

  if(con_allocs_h)
    free(con_allocs_h);
  if(con_allocs_d)
    cudaFree(con_allocs_d);

  if(con_sizes_h)
    free(con_sizes_h);
  if(con_sizes_d)
    cudaFree(con_sizes_d);

  if(unit_starts_h)
    free(unit_starts_h);

  if(cur_send_net_h)
    free(cur_send_net_h);
  if(cur_send_net_d)
    cudaFree(cur_send_net_d);

  if(send_net_acts_h)
    free(send_net_acts_h);
  if(send_net_acts_d)
    cudaFree(send_net_acts_d);

  if(send_netin_tmp_d)
    cudaFree(send_netin_tmp_d);

  Initialize();
}

void LeabraConSpecCuda::AllocCudaArrays
(int n_un, int64_t own_cnt, int64_t ptr_cnt, int own_units_x, int ptr_units_x, 
 float* own_cons_mem, float* ptr_cons_mem, float* send_netin_tmp) {
  if(n_units != n_un || own_units_x != own_units_x_cons) {
    FreeCudaArrays();
  }

  if(n_un == 0 || own_units_x_cons == 0)
    return;

  n_units = n_un;
  own_cons_cnt = own_cnt;
  ptr_cons_cnt = ptr_cnt;
  own_units_x_cons = own_units_x;
  ptr_units_x_cons = ptr_units_x;

  own_cons_mem_h = own_cons_mem;
  ptr_cons_mem_h = ptr_cons_mem;
  send_netin_tmp_h = send_netin_tmp;

  units_h = (int*)malloc(own_units_x_cons * sizeof(int));
  cudaMalloc(&units_d, own_units_x_cons);

  con_mem_idxs_h = (int64_t*)malloc(own_units_x_cons * sizeof(int64_t));
  cudaMalloc(&con_mem_idxs_d, own_units_x_cons);

  con_allocs_h = (int*)malloc(own_units_x_cons * sizeof(int));
  cudaMalloc(&con_allocs_d, own_units_x_cons);

  con_sizes_h = (int*)malloc(own_units_x_cons * sizeof(int));
  cudaMalloc(&con_sizes_d, own_units_x_cons);

  unit_starts_h = (int*)malloc((n_units+1) * sizeof(int));

  cur_send_net_h = (int*)malloc(own_units_x_cons * sizeof(int));
  cudaMalloc(&cur_send_net_d, own_units_x_cons);

  send_net_acts_h = (float*)malloc(own_units_x_cons * sizeof(float));
  cudaMalloc(&send_net_acts_d, own_units_x_cons);

  cudaMalloc(&send_netin_tmp_d, (n_units+1));

  cudaMalloc(&own_cons_mem_d, own_cons_cnt);

  // conserve memory: not needed..
  //   cudaMalloc(&ptr_cons_mem_d, ptr_cons_cnt);
}

void LeabraConSpecCuda::UpdateOwnCons() {
  if(own_cons_mem_h && own_cons_mem_d) {
    cudaMemcpy(own_cons_mem_d, own_cons_mem_h, own_cons_cnt, cudaMemcpyHostToDevice);
  }
}

void LeabraConSpecCuda::UpdateUnitsXCons() {
  if(!units_h) return;

  cudaMemcpy(units_d, units_h, own_units_x_cons, cudaMemcpyHostToDevice);
  cudaMemcpy(con_mem_idxs_d, con_mem_idxs_h, own_units_x_cons, cudaMemcpyHostToDevice);
  cudaMemcpy(con_allocs_d, con_allocs_h, own_units_x_cons, cudaMemcpyHostToDevice);
  cudaMemcpy(con_sizes_d, con_sizes_h, own_units_x_cons, cudaMemcpyHostToDevice);
}

__global__ void Send_NetinDelta(float* A, float* B, float* C, int N) {
  int i = blockDim.x * blockIdx.x + threadIdx.x;
  if (i < N)
    C[i] = A[i] + B[i];
}

void LeabraConSpecCuda::Send_NetinDelta() {
  cudaMemcpy(cur_send_net_d, cur_send_net_h, cur_send_net_n, cudaMemcpyHostToDevice);
  cudaMemcpy(send_net_acts_d, send_net_acts_h, cur_send_net_n, cudaMemcpyHostToDevice);

//   int N = 256;
//   size_t size = N * sizeof(float);

//   // Invoke kernel
//   int threadsPerBlock = 256;
//   int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;

//   VecAdd<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, N);

  cudaMemcpy(send_netin_tmp_d, send_netin_tmp_h, n_units+1, cudaMemcpyDeviceToHost);
  // get results back from device
}
