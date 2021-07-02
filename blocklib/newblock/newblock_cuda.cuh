#include <cuComplex.h>
#include <cuComplex.h>
#include <cuda.h>
#include <cuda_runtime.h>

namespace gr {
namespace newmod {
namespace newblock {

template <typename T>
void exec_kernel(
    const T* in, T* out, int grid_size, int block_size, cudaStream_t stream);

template <typename T>
void get_block_and_grid(int* minGrid, int* minBlock);

} // namespace newblock
} // namespace newmod
} // namespace gr