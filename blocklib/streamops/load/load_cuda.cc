/* -*- c++ -*- */
/*
 * Copyright 2021 Josh Morman
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "load_cuda.h"
#include "load_cuda_gen.h"

#include <gnuradio/helper_cuda.h>

#include <cuComplex.h>
#include <cuda.h>
#include <cuda_runtime.h>

#include "load_cuda.cuh"

namespace gr {
namespace streamops {

load_cuda::load_cuda(block_args args) : INHERITED_CONSTRUCTORS, d_load(args.iterations)

{
    load_cu::get_block_and_grid(&d_min_grid_size, &d_block_size);
    d_logger->info("minGrid: {}, blockSize: {}", d_min_grid_size, d_block_size);
    cudaStreamCreate(&d_stream);
}

work_return_code_t load_cuda::work(std::vector<block_work_input_sptr>& work_input,
                                   std::vector<block_work_output_sptr>& work_output)
{
    auto in = work_input[0]->items<uint8_t>();
    auto out = work_output[0]->items<uint8_t>();

    auto noutput_items = work_output[0]->n_items;
    auto itemsize = work_output[0]->buffer->item_size();
    int gridSize = (noutput_items * itemsize + d_block_size - 1) / d_block_size;
    load_cu::exec_kernel(in, out, gridSize, d_block_size, d_load, d_stream);
    checkCudaErrors(cudaPeekAtLastError());


    cudaStreamSynchronize(d_stream);


    // Tell runtime system how many output items we produced.
    produce_each(noutput_items, work_output);
    return work_return_code_t::WORK_OK;
}
} // namespace streamops
} // namespace gr
