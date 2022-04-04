/* -*- c++ -*- */
/*
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <gnuradio/streamops/nop_head.h>

namespace gr {
namespace streamops {

class nop_head_cpu : public nop_head
{
public:
    nop_head_cpu(const block_args& args);
    work_return_code_t
    work(std::vector<block_work_input_sptr>& work_input,
         std::vector<block_work_output_sptr>& work_output) override;

private:
    size_t d_nitems;
    size_t d_ncopied_items = 0;
};

} // namespace streamops
} // namespace gr