#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <gnuradio/buffer.h>

namespace gr {

class buffer_cpu_simple_reader;
class buffer_cpu_simple : public buffer
{
private:
    std::vector<uint8_t> _buffer;

public:
    using sptr = std::shared_ptr<buffer_cpu_simple>;
    buffer_cpu_simple(size_t num_items,
                      size_t item_size,
                      std::shared_ptr<buffer_properties> buf_properties);

    static buffer_sptr make(size_t num_items,
                            size_t item_size,
                            std::shared_ptr<buffer_properties> buffer_properties);

    void* read_ptr(size_t index) override;
    void* write_ptr() override;

    void post_write(int num_items) override;

    std::shared_ptr<buffer_reader>
    add_reader(std::shared_ptr<buffer_properties> buf_props, size_t itemsize) override;
};

class buffer_cpu_simple_reader : public buffer_reader
{
public:
    buffer_cpu_simple_reader(buffer_sptr buffer,
                             std::shared_ptr<buffer_properties> buf_props,
                             size_t itemsize,
                             size_t read_index = 0)
        : buffer_reader(buffer, buf_props, itemsize, read_index)
    {
    }

    void post_read(int num_items) override;
};

} // namespace gr
