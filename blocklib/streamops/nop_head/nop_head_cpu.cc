/* -*- c++ -*- */
/*
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "nop_head_cpu.h"
#include "nop_head_cpu_gen.h"

namespace gr {
namespace streamops {

nop_head_cpu::nop_head_cpu(const block_args& args)
    : INHERITED_CONSTRUCTORS, d_nitems(args.nitems)
{
}

work_return_code_t nop_head_cpu::work(std::vector<block_work_input_sptr>& work_input,
                                      std::vector<block_work_output_sptr>& work_output)
{

    if (d_ncopied_items >= d_nitems) {
        work_output[0]->n_produced = 0;
        return work_return_code_t::WORK_DONE; // Done!
    }

    unsigned n = std::min(d_nitems - d_ncopied_items, (uint64_t)work_output[0]->n_items);

    if (n == 0) {
        work_output[0]->n_produced = 0;
        return work_return_code_t::WORK_OK;
    }

    // Do Nothing

    d_ncopied_items += n;
    work_output[0]->n_produced = n;

    return work_return_code_t::WORK_OK;
}

} /* namespace streamops */
} /* namespace gr */
