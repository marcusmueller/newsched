#pragma once

#include <iostream>
#include <memory>

#include <gnuradio/block.h>
#include <gnuradio/graph.h>
#include <gnuradio/scheduler.h>

namespace gr {

/**
 * @brief Domain Configuration
 *
 * A struct to contain all the necessary information about a domain:
 *  - Scheduler
 *  - Blocks
 *
 */
class domain_conf
{
public:
    domain_conf(scheduler_sptr sched, std::vector<node_sptr> blocks)
        : _sched(sched), _blocks(blocks)
    {
    }

    auto sched() { return _sched; }
    auto blocks() { return _blocks; }

private:
    scheduler_sptr _sched;
    std::vector<node_sptr> _blocks;
};

using domain_conf_vec = std::vector<domain_conf>;

} // namespace gr
