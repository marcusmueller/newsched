#include <gnuradio/flowgraph.hh>
#include <gnuradio/graph_utils.hh>

#include <dlfcn.h>

namespace gr {


flowgraph::flowgraph(const std::string& name)
{
    set_alias(name);
}

size_t get_port_itemsize(port_sptr port)
{
    size_t size = 0;
    if (port->connected_ports().size() > 0) {
        auto cp = port->connected_ports()[0];
        // use data_size since this includes vector sizing
        size = cp->data_size();
    }
    return size;
}

std::string get_port_format_descriptor(port_sptr port)
{
    std::string fd = "";
    if (port->connected_ports().size() > 0) {
        auto cp = port->connected_ports()[0];
        // use data_size since this includes vector sizing
        fd = cp->format_descriptor();
    }
    return fd;
}


void flowgraph::check_connections(const graph_sptr& g)
{
    // Are all non-optional ports connected to something
    for (auto& node : g->calc_used_nodes()) {
        for (auto& port : node->output_ports()) {
            if (!port->optional() && port->connected_ports().size() == 0) {
                throw std::runtime_error("Nothing connected to " + node->name() + ": " + port->name());
            }
        }
        for (auto& port : node->input_ports()) {
            if (!port->optional()) {

                if (port->type() == port_type_t::STREAM) {

                    if (port->connected_ports().size() < 1) {
                        throw std::runtime_error("Nothing connected to " + node->name() + ": " + port->name());
                    } else if (port->connected_ports().size() > 1) {
                        throw std::runtime_error("More than 1 port connected to " +
                                                 port->alias());
                    }
                } else if (port->type() == port_type_t::MESSAGE) {
                    if (port->connected_ports().size() < 1) {
                        throw std::runtime_error("Nothing connected to " + node->name() + ": " + port->name());
                    }
                }
            }
        }
    }

    // Iteratively check the flowgraph for 0 size ports and adjust
    bool updated_sizes = true;
    while (updated_sizes) {
        updated_sizes = false;
        for (auto& node : g->calc_used_nodes()) {
            for (auto& port : node->output_ports()) {
                if (port->itemsize() == 0) {
                    // we need to propagate the itemsize from whatever it is connected to
                    auto newsize = get_port_itemsize(port);
                    auto newfd = get_port_format_descriptor(port);
                    port->set_itemsize(newsize);
                    port->set_format_descriptor(newfd);
                    updated_sizes = newsize > 0;
                }
            }
            for (auto& port : node->input_ports()) {
                if (port->itemsize() == 0) {
                    // we need to propagate the itemsize from whatever it is connected to
                    auto newsize = get_port_itemsize(port);
                    auto newfd = get_port_format_descriptor(port);
                    port->set_itemsize(newsize);
                    port->set_format_descriptor(newfd);
                    updated_sizes = newsize > 0;
                }
            }
        }
    }

    // Set any remaining 0 size ports to something
    size_t newsize = sizeof(gr_complex); // why not, does it matter
    for (auto& node : g->calc_used_nodes()) {
        for (auto& port : node->output_ports()) {
            if (port->itemsize() == 0) {
                port->set_itemsize(newsize);
                port->set_format_descriptor("Zf");
            }
        }
        for (auto& port : node->input_ports()) {
            if (port->itemsize() == 0) {
                port->set_itemsize(newsize);
                port->set_format_descriptor("Zf");
            }
        }
    }
}

} // namespace gr