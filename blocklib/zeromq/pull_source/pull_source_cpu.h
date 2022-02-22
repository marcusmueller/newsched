#pragma once

#include "base.h"
#include <gnuradio/zeromq/pull_source.h>

namespace gr {
namespace zeromq {

class pull_source_cpu : public virtual pull_source, public virtual base_source
{
public:
    pull_source_cpu(block_args args);
    virtual work_return_code_t
    work(std::vector<block_work_input_sptr>& work_input,
         std::vector<block_work_output_sptr>& work_output) override;
};

} // namespace zeromq
} // namespace gr