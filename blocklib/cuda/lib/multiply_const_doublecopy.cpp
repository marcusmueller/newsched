#include <gnuradio/blocklib/cuda/multiply_const_doublecopy.hpp>

namespace gr {
namespace cuda {

extern void multiply_const_doublecopy_kernel_wrapper(int N, float k, const float* in, float* out);

template <class T>
work_return_code_t multiply_const_doublecopy<T>::work(std::vector<block_work_input>& work_input,
                                           std::vector<block_work_output>& work_output)
{
    multiply_const_doublecopy_kernel_wrapper(work_output[0].n_items, d_k, (const T *)work_input[0].items, (T *)work_output[0].items);
    work_output[0].n_produced = work_output[0].n_items;
    return work_return_code_t::WORK_OK;
}


template class multiply_const_doublecopy<float>;

} // namespace cuda
} // namespace gr