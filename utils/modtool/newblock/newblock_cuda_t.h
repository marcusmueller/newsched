/* -*- c++ -*- */
/*
 * Copyright <COPYRIGHT_YEAR> <COPYRIGHT_AUTHOR>
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <gnuradio/newmod/newblock.h>

#include <cuComplex.h>
#include <cuda.h>
#include <cuda_runtime.h>

namespace gr {
namespace newmod {

template <class T>
class newblock_cuda : public newblock<T>
{
public:
    newblock_cuda(const typename newblock<T>::block_args& args);
    
    virtual work_return_code_t work(std::vector<block_work_input_sptr>& work_input,
                                    std::vector<block_work_output_sptr>& work_output) override;

private:
    T d_k;
    size_t d_vlen;

    int d_block_size;
    int d_min_grid_size;
    cudaStream_t d_stream;
};


} // namespace newmod
} // namespace gr
