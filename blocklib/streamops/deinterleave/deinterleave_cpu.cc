/* -*- c++ -*- */
/*
 * Copyright 2022 Josh Morman
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "deinterleave_cpu.h"
#include "deinterleave_cpu_gen.h"

#include <algorithm>

namespace gr {
namespace streamops {

deinterleave_cpu::deinterleave_cpu(block_args args) : INHERITED_CONSTRUCTORS
{
    if (args.itemsize > 0) {
        d_size_bytes = args.itemsize * args.blocksize;
        set_output_multiple(args.blocksize);
    }
    set_relative_rate(1.0 / args.nstreams);
}

work_return_code_t
deinterleave_cpu::work(std::vector<block_work_input_sptr>& work_input,
                       std::vector<block_work_output_sptr>& work_output)
{
    auto blocksize = pmtf::get_as<size_t>(*this->param_blocksize);
    auto itemsize = work_input[0]->buffer->item_size();

    // Since itemsize can be set after construction
    if (d_size_bytes == 0) {
        d_size_bytes = itemsize * blocksize;
        set_output_multiple(blocksize);
        return work_return_code_t::WORK_OK;
    }

    // Forecasting
    auto nstreams = pmtf::get_as<size_t>(*this->param_nstreams);
    auto noutput_items = block_work_output::min_n_items(work_output);
    auto ninput_items = work_input[0]->n_items;
    auto min_output = blocksize * (ninput_items / (blocksize * nstreams));
    if (min_output < 1) {
        return work_return_code_t::WORK_INSUFFICIENT_INPUT_ITEMS;
    }
    noutput_items = std::min(noutput_items, min_output);
    ninput_items = noutput_items * nstreams;

    auto in = work_input[0]->items<uint8_t>();
    int count = 0, totalcount = noutput_items * nstreams;
    unsigned int skip = 0;
    unsigned int acc = 0;
    while (count < totalcount) {
        auto out = work_output[d_current_output]->items<uint8_t>();
        memcpy(out + skip * d_size_bytes, in, d_size_bytes);
        in += d_size_bytes;
        // produce(d_current_output, blocksize);
        work_output[d_current_output]->n_produced += blocksize;
        d_current_output = (d_current_output + 1) % nstreams;

        // accumulate times through the loop; increment skip after a
        // full pass over the output streams.
        // This is separate than d_current_output since we could be in
        // the middle of a loop when we exit.
        acc++;
        if (acc >= nstreams) {
            skip++;
            acc = 0;
        }

        // Keep track of our loop counter
        count += blocksize;
    }
    consume_each(totalcount, work_input);
    return work_return_code_t::WORK_OK;
}


} // namespace streamops
} // namespace gr