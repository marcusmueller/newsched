/* -*- c++ -*- */
/*
 * Copyright 2010,2012,2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <gnuradio/kernel/fft/fftw_fft.h>
#include <gnuradio/kernel/api.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/logger.h>
#include <volk/volk_alloc.hh>
#include <vector>

namespace gr {
namespace kernel {
namespace filter {

/*!
 * \brief Fast FFT filter with float input, float output and float taps
 * \ingroup filter_blk
 *
 * \details
 * This block performs fast convolution using the
 * overlap-and-save algorithm. The filtering is performand in
 * the frequency domain instead of the time domain (see
 * gr::filter::kernel::fir_filter_fff). For an input signal x
 * and filter coefficients (taps) t, we compute y as:
 *
 * \code
 *    y = ifft(fft(x)*fft(t))
 * \endcode
 *
 * This kernel computes the FFT of the taps when they are set to
 * only perform this operation once. The FFT of the input signal
 * x is done every time.
 *
 * Because this is designed as a very low-level kernel
 * operation, it is designed for speed and avoids certain checks
 * in the filter() function itself. The filter function expects
 * that the input signal is a multiple of d_nsamples in the
 * class that's computed internally to be as fast as
 * possible. The function set_taps will return the value of
 * nsamples that can be used externally to check this
 * boundary. Notice that all implementations of the fft_filter
 * GNU Radio blocks (e.g., gr::filter::fft_filter_fff) use this
 * value of nsamples to compute the value to call
 * gr::block::set_output_multiple that ensures the scheduler
 * always passes this block the right number of samples.
 */
template <class T, class TAPS_T>
class fft_filter
{
private:
    int d_ntaps;
    int d_nsamples;
    int d_fftsize; // fftsize = ntaps + nsamples - 1
    int d_decimation;
    std::unique_ptr<fft::fftw_fft<T, true>> d_fwdfft; // forward "plan"
    std::unique_ptr<fft::fftw_fft<T, false>> d_invfft; // inverse "plan"
    int d_nthreads;                              // number of FFTW threads to use
    std::vector<T> d_tail; // state carried between blocks for overlap-add
    std::vector<TAPS_T> d_taps; // stores time domain taps
    volk::vector<gr_complex> d_xformed_taps; // Fourier xformed taps

    void compute_sizes(int ntaps);
    int tailsize() const { return d_ntaps - 1; }

    gr::logger_ptr d_logger, d_debug_logger;

public:
    /*!
     * \brief Construct an FFT filter for float vectors with the given taps and decimation
     * rate.
     *
     * This is the basic implementation for performing FFT filter for fast convolution
     * in other blocks (e.g., gr::filter::fft_filter_fff).
     *
     * \param decimation The decimation rate of the filter (int)
     * \param taps       The filter taps (vector of float)
     * \param nthreads   The number of threads for the FFT to use (int)
     */
    fft_filter(int decimation, const std::vector<TAPS_T>& taps, int nthreads = 1);

    // Disallow copy.
    //
    // This prevents accidentally doing needless copies, not just of fft_filter_xxx,
    // but every block that contains one.
    fft_filter(const fft_filter&) = delete;
    fft_filter& operator=(const fft_filter&) = delete;
    fft_filter(fft_filter&&) = default;
    fft_filter& operator=(fft_filter&&) = default;

    /*!
     * \brief Set new taps for the filter.
     *
     * Sets new taps and resets the class properties to handle different sizes
     * \param taps       The filter taps (complex)
     */
    int set_taps(const std::vector<TAPS_T>& taps);

    /*!
     * \brief Set number of threads to use.
     */
    void set_nthreads(int n);

    /*!
     * \brief Returns the taps.
     */
    std::vector<TAPS_T> taps() const;

    /*!
     * \brief Returns the number of taps in the filter.
     */
    unsigned int ntaps() const;

    /*!
     * \brief Get number of threads being used.
     */
    int nthreads() const;

    /*!
     * \brief Perform the filter operation
     *
     * \param nitems  The number of items to produce
     * \param input   The input vector to be filtered
     * \param output  The result of the filter operation
     */
    int filter(int nitems, const T* input, T* output);
};

} /* namespace filter */
} /* namespace kernel */
} /* namespace gr */
