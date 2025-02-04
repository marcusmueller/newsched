/* -*- c++ -*- */
/*
 * Copyright 2005-2011 Free Software Foundation, Inc.
 * Copyright 2021 Josh Morman
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "throttle_cpu.h"
#include "throttle_cpu_gen.h"
#include <thread>

namespace gr {
namespace streamops {

throttle_cpu::throttle_cpu(block_args args)
    : INHERITED_CONSTRUCTORS, d_ignore_tags(args.ignore_tags)
{
    set_sample_rate(args.samples_per_sec);
}

void throttle_cpu::set_sample_rate(double rate)
{
    // changing the sample rate performs a reset of state params
    d_start = std::chrono::steady_clock::now();
    d_total_samples = 0;
    d_sample_rate = rate;
    d_sample_period = std::chrono::duration<double>(1 / rate);
}

bool throttle_cpu::start()
{
    d_start = std::chrono::steady_clock::now();
    d_total_samples = 0;
    return block::start();
}

work_return_code_t throttle_cpu::work(std::vector<block_work_input_sptr>& work_input,
                                      std::vector<block_work_output_sptr>& work_output)
{
    if (d_sleeping) {
        produce_each(0, work_output);
        consume_each(0, work_input);
        return work_return_code_t::WORK_OK;
    }

    // copy all samples output[i] <= input[i]
    auto in = work_input[0]->items<uint8_t>();
    auto out = work_output[0]->items<uint8_t>();

    auto noutput_items = work_output[0]->n_items;

    d_total_samples += noutput_items;

    auto now = std::chrono::steady_clock::now();
    auto expected_time = d_start + d_sample_period * d_total_samples;
    int n = noutput_items;
    if (expected_time > now) {
        auto limit_duration =
            std::chrono::duration<double>(std::numeric_limits<long>::max());

        d_sleeping = true;
        std::thread t([this, expected_time, now]() {
                d_debug_logger->debug(
                "Throttle sleeping {}",
                std::chrono::duration_cast<std::chrono::milliseconds>(expected_time - now)
                    .count());
            std::this_thread::sleep_until(expected_time);
            this->p_scheduler->push_message(
                std::make_shared<scheduler_action>(scheduler_action_t::NOTIFY_INPUT));
            this->d_sleeping = false;
        });
        t.detach();

        n = 0;
        d_total_samples -= noutput_items;
        produce_each(0, work_output);
        consume_each(0, work_input);
        return work_return_code_t::WORK_OK;
    }

    // TODO: blocks like throttle shouldn't need to do a memcpy, but this would have to be
    // fixed in the buffering model and a special port type
    if (n) {
        std::memcpy(out, in, n * work_output[0]->buffer->item_size());
    }
    work_output[0]->n_produced = n;

    d_debug_logger->debug( "Throttle produced {}", n);
    return work_return_code_t::WORK_OK;
}


} // namespace streamops
} // namespace gr