/* -*- c++ -*- */
/*
 * Copyright 2004,2013,2018 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <gnuradio/kernel/math/fxpt_nco.h>
#include <gnuradio/kernel/math/nco.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include <cmath>

using namespace gr::kernel::math;

static const float SIN_COS_TOLERANCE = 1e-5;

// static const float SIN_COS_FREQ = 5003;
static const float SIN_COS_FREQ = 4096;

static const int SIN_COS_BLOCK_SIZE = 100000;

static double max_d(double a, double b) { return fabs(a) > fabs(b) ? a : b; }


TEST(FxptNco, t0)
{
    nco<float, float> ref_nco;
    fxpt_nco new_nco;
    double max_error = 0, max_phase_error = 0;

    ref_nco.set_freq((float)(2 * GR_M_PI / SIN_COS_FREQ));
    new_nco.set_freq((float)(2 * GR_M_PI / SIN_COS_FREQ));

    EXPECT_TRUE(std::abs(ref_nco.get_freq() - new_nco.get_freq()) <= SIN_COS_TOLERANCE);

    for (int i = 0; i < SIN_COS_BLOCK_SIZE; i++) {
        float ref_sin = ref_nco.sin();
        float new_sin = new_nco.sin();
        // printf ("i = %6d\n", i);
        EXPECT_TRUE(std::abs(ref_sin - new_sin) <= SIN_COS_TOLERANCE);

        max_error = max_d(max_error, ref_sin - new_sin);

        float ref_cos = ref_nco.cos();
        float new_cos = new_nco.cos();
        EXPECT_TRUE(std::abs(ref_cos - new_cos) <= SIN_COS_TOLERANCE);

        max_error = max_d(max_error, ref_cos - new_cos);

        ref_nco.step();
        new_nco.step();

        EXPECT_TRUE(std::abs(ref_nco.get_phase() - new_nco.get_phase()) <=
                    SIN_COS_TOLERANCE);

        max_phase_error =
            max_d(max_phase_error, ref_nco.get_phase() - new_nco.get_phase());
    }
}

TEST(FxptNco, t1)
{
    nco<float, float> ref_nco;
    fxpt_nco new_nco;
    std::vector<gr_complex> ref_block(SIN_COS_BLOCK_SIZE);
    std::vector<gr_complex> new_block(SIN_COS_BLOCK_SIZE);
    double max_error = 0;

    ref_nco.set_freq((float)(2 * GR_M_PI / SIN_COS_FREQ));
    new_nco.set_freq((float)(2 * GR_M_PI / SIN_COS_FREQ));

    EXPECT_TRUE(std::abs(ref_nco.get_freq() - new_nco.get_freq()) <= SIN_COS_TOLERANCE);

    ref_nco.sincos((gr_complex*)ref_block.data(), SIN_COS_BLOCK_SIZE);
    new_nco.sincos((gr_complex*)new_block.data(), SIN_COS_BLOCK_SIZE);

    for (int i = 0; i < SIN_COS_BLOCK_SIZE; i++) {
        EXPECT_TRUE(std::abs(ref_block[i].real() - new_block[i].real()) <=
                    SIN_COS_TOLERANCE);
        max_error = max_d(max_error, ref_block[i].real() - new_block[i].real());

        EXPECT_TRUE(std::abs(ref_block[i].imag() - new_block[i].imag()) <=
                    SIN_COS_TOLERANCE);
        max_error = max_d(max_error, ref_block[i].imag() - new_block[i].imag());
    }
    EXPECT_TRUE(std::abs(ref_nco.get_phase() - new_nco.get_phase()) <= SIN_COS_TOLERANCE);
}
