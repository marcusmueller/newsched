/* -*- c++ -*- */
/*
 * Copyright 2004,2009,2010,2012,2018 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "multiply_const_cpu.h"
#include "multiply_const_cpu_gen.h"
#include <volk/volk.h>

namespace gr {
namespace math {

template <class T>
multiply_const_cpu<T>::multiply_const_cpu(
    const typename multiply_const<T>::block_args& args)
    : INHERITED_CONSTRUCTORS(T), d_vlen(args.vlen)
{
}

template <>
work_return_code_t
multiply_const_cpu<float>::work(std::vector<block_work_input_sptr>& work_input,
                                std::vector<block_work_output_sptr>& work_output)
{
    auto k = pmtf::get_as<float>(*this->param_k);

    auto in = work_input[0]->items<float>();
    auto out = work_output[0]->items<float>();
    int noi = work_output[0]->n_items * d_vlen;

    volk_32f_s32f_multiply_32f(out, in, k, noi);

    work_output[0]->n_produced = work_output[0]->n_items;
    return work_return_code_t::WORK_OK;
}

template <>
work_return_code_t
multiply_const_cpu<gr_complex>::work(std::vector<block_work_input_sptr>& work_input,
                                     std::vector<block_work_output_sptr>& work_output)
{
    auto k = pmtf::get_as<gr_complex>(*this->param_k);

    const auto in = work_input[0]->items<gr_complex>();
    auto out = work_output[0]->items<gr_complex>();
    int noi = work_output[0]->n_items * d_vlen;

    volk_32fc_s32fc_multiply_32fc(out, in, k, noi);

    work_output[0]->n_produced = work_output[0]->n_items;
    return work_return_code_t::WORK_OK;
}

template <class T>
work_return_code_t
multiply_const_cpu<T>::work(std::vector<block_work_input_sptr>& work_input,
                            std::vector<block_work_output_sptr>& work_output)
{
    auto k = pmtf::get_as<T>(*this->param_k);

    // Pre-generate these from modtool, for example
    auto iptr = work_input[0]->items<T>();
    auto optr = work_output[0]->items<T>();

    int size = work_output[0]->n_items * d_vlen;

    while (size >= 8) {
        *optr++ = *iptr++ * k;
        *optr++ = *iptr++ * k;
        *optr++ = *iptr++ * k;
        *optr++ = *iptr++ * k;
        *optr++ = *iptr++ * k;
        *optr++ = *iptr++ * k;
        *optr++ = *iptr++ * k;
        *optr++ = *iptr++ * k;
        size -= 8;
    }

    while (size-- > 0)
        *optr++ = *iptr++ * k;

    work_output[0]->n_produced = work_output[0]->n_items;
    work_input[0]->n_consumed = work_input[0]->n_items;
    return work_return_code_t::WORK_OK;
}

} /* namespace math */
} /* namespace gr */
