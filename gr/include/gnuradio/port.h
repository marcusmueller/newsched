#pragma once

#include <gnuradio/api.h>
#include <gnuradio/buffer.h>
#include <gnuradio/neighbor_interface.h>
#include <gnuradio/parameter_types.h>
#include <gnuradio/port_interface.h>
#include <gnuradio/scheduler_message.h>
#include <algorithm>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <utility>


namespace gr {

enum class GR_RUNTIME_API port_type_t { STREAM, MESSAGE };

enum class GR_RUNTIME_API port_direction_t {
    INPUT,
    OUTPUT,
    BIDIRECTONAL //?? can it be done
};

class block;

/**
 * @brief Base class for all ports
 *
 * Holds the necessary information to describe the port to the runtime
 *
 */
class GR_RUNTIME_API port_base : public port_interface,
                                 public std::enable_shared_from_this<port_base>
{

public:
    using sptr = std::shared_ptr<port_base>;
    static sptr make(const std::string& name,
                     const port_direction_t direction,
                     const param_type_t data_type = param_type_t::CFLOAT,
                     const port_type_t port_type = port_type_t::STREAM,
                     const std::vector<size_t>& shape = std::vector<size_t>{ 1 },
                     const bool optional = false,
                     const int multiplicity = 1);

    port_base(const std::string& name,
              const port_direction_t direction,
              const param_type_t data_type = param_type_t::CFLOAT,
              const port_type_t port_type = port_type_t::STREAM,
              const std::vector<size_t>& shape = std::vector<size_t>{ 1 },
              const bool optional = false,
              const int multiplicity = 1);
    port_base(const std::string& name,
              const port_direction_t direction,
              const size_t itemsize,
              const port_type_t port_type = port_type_t::STREAM,
              const bool optional = false,
              const int multiplicity = 1);

    ~port_base() override = default;

    std::string name() { return _name; }
    std::string alias() { return _alias; }
    void set_alias(const std::string& alias) { _alias = alias; }
    void set_index(int val) { _index = val; }
    int index() { return _index; }
    port_type_t type() { return _port_type; }
    param_type_t data_type() { return _data_type; }
    port_direction_t direction() { return _direction; }
    size_t data_size() { return _datasize; }
    size_t itemsize() { return _itemsize; }
    void set_itemsize(size_t itemsize) { _itemsize = itemsize; }
    std::vector<size_t> shape() { return _shape; }
    sptr base() { return shared_from_this(); }
    bool optional() { return _optional; }
    auto& connected_ports() { return _connected_ports; }

    void set_parent_intf(neighbor_interface_sptr intf) { _parent_intf = intf; }
    std::string format_descriptor();
    void set_format_descriptor(const std::string& fd) { _format_descriptor = fd; }
    void set_buffer(buffer_sptr buffer) { _buffer = buffer; }
    buffer_sptr buffer() { return _buffer; }
    void set_buffer_reader(buffer_reader_sptr rdr) { _buffer_reader = rdr; }
    buffer_reader_sptr buffer_reader() { return _buffer_reader; }

    void notify_connected_ports(scheduler_message_sptr msg);
    // Inbound messages
    void push_message(scheduler_message_sptr msg) override;
    void connect(port_interface_sptr other_port);
    void disconnect(port_interface_sptr other_port);

protected:
    std::string _name;
    std::string _alias;
    port_direction_t _direction;
    param_type_t _data_type;
    port_type_t _port_type;
    int _index = -1;           // how does this get set??
    std::vector<size_t> _shape; // allow for matrices to be sent naturally across ports
    // empty shape refers to a scalar, shape=[n] same as vlen=n
    bool _optional;
    int _multiplicity; // port can be replicated as in grc
    size_t _datasize;
    size_t _itemsize; // data size across all shape
    std::string _format_descriptor = "";

    std::vector<port_interface_sptr> _connected_ports;
    neighbor_interface_sptr _parent_intf = nullptr;
    buffer_sptr _buffer = nullptr;
    buffer_reader_sptr _buffer_reader = nullptr;

    block* _parent_block = nullptr;
};

using port_sptr = port_base::sptr;
using port_vector_t = std::vector<port_sptr>;


/**
 * @brief Typed port class
 *
 * Wraps the port_base class with a type to take care of all the sizing and lower level
 * properties
 *
 * @tparam T datatype to instantiate the base port class
 */
template <class T>
class GR_RUNTIME_API port : public port_base
{
public:
    static std::shared_ptr<port<T>> make(const std::string& name,
                                         const port_direction_t direction,
                                         const std::vector<size_t>& shape = { 1 },
                                         const bool optional = false,
                                         const int multiplicity = 1);
    port(const std::string& name,
         const port_direction_t direction,
         const std::vector<size_t>& shape = { 1 },
         const bool optional = false,
         const int multiplicity = 1);
};


/**
 * @brief Untyped port class
 *
 * Wraps the port base class but only populates stream size info.  To be used in case of
 * e.g. head block where the underlying datatype is not used, just copied byte for byte
 *
 */
class GR_RUNTIME_API untyped_port : public port_base
{
public:
    static std::shared_ptr<untyped_port> make(const std::string& name,
                                              const port_direction_t direction,
                                              const size_t itemsize,
                                              const bool optional = false,
                                              const int multiplicity = 1);
    untyped_port(const std::string& name,
                 const port_direction_t direction,
                 const size_t itemsize,
                 const bool optional = false,
                 const int multiplicity = 1);
};


/**
 * @brief Message port class
 *
 * Wraps the port_base class to provide a message port where streaming parameters are
 * absent and the type is MESSAGE
 *
 */
class GR_RUNTIME_API message_port : public port_base
{
private: //
    message_port_callback_fcn _callback_fcn;

public:
    using sptr = std::shared_ptr<message_port>;
    static sptr make(const std::string& name,
                     const port_direction_t direction,
                     const bool optional = true,
                     const int multiplicity = 1);
    message_port(const std::string& name,
                 const port_direction_t direction,
                 const bool optional = false,
                 const int multiplicity = 1);


    message_port_callback_fcn callback() { return _callback_fcn; }
    void register_callback(message_port_callback_fcn fcn) { _callback_fcn = fcn; }
    void post(pmtf::pmt msg);
    void push_message(scheduler_message_sptr msg) override;
};
using message_port_sptr = message_port::sptr;

} // namespace gr
