/* -*- c++ -*- */
/*
 * Copyright 2003,2008,2011,2012,2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <gnuradio/kernel/fft/fftw_fft.h>
// #include <gnuradio/sys_paths.h>
#include <fftw3.h>
#include <volk/volk.h>

#ifdef _WIN32 // http://www.fftw.org/install/windows.html#DLLwisdom
static void my_fftw_write_char(char c, void* f) { fputc(c, (FILE*)f); }
#define fftw_export_wisdom_to_file(f) fftw_export_wisdom(my_fftw_write_char, (void*)(f))
#define fftwf_export_wisdom_to_file(f) fftwf_export_wisdom(my_fftw_write_char, (void*)(f))
#define fftwl_export_wisdom_to_file(f) fftwl_export_wisdom(my_fftw_write_char, (void*)(f))

static int my_fftw_read_char(void* f) { return fgetc((FILE*)f); }
#define fftw_import_wisdom_from_file(f) fftw_import_wisdom(my_fftw_read_char, (void*)(f))
#define fftwf_import_wisdom_from_file(f) \
    fftwf_import_wisdom(my_fftw_read_char, (void*)(f))
#define fftwl_import_wisdom_from_file(f) \
    fftwl_import_wisdom(my_fftw_read_char, (void*)(f))
#include <fcntl.h>
#include <io.h>
#define O_NOCTTY 0
#define O_NONBLOCK 0
#endif //_WIN32

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>

#include <gnuradio/file_lock.h>
#include <gnuradio/prefs.h>

namespace fs = std::filesystem;

namespace gr {
namespace kernel {
namespace fft {
static std::mutex wisdom_thread_mutex;
gr::file_lock wisdom_lock;
static bool wisdom_lock_init_done = false; // Modify while holding 'wisdom_thread_mutex'

gr_complex* malloc_complex(int size)
{
    return (gr_complex*)volk_malloc(sizeof(gr_complex) * size, volk_get_alignment());
}

float* malloc_float(int size)
{
    return (float*)volk_malloc(sizeof(float) * size, volk_get_alignment());
}

double* malloc_double(int size)
{
    return (double*)volk_malloc(sizeof(double) * size, volk_get_alignment());
}

void free(void* b) { volk_free(b); }


std::mutex& planner::mutex()
{
    static std::mutex s_planning_mutex;

    return s_planning_mutex;
}

static std::string wisdom_filename()
{
    static fs::path path;
    path = fs::path(gr::prefs::appdata_path()) / ".gr_fftw_wisdom";
    return path.string();
}

static void wisdom_lock_init()
{
    if (wisdom_lock_init_done)
        return;

    const std::string wisdom_lock_file = wisdom_filename() + ".lock";
    // std::cerr << "Creating FFTW wisdom lockfile: " << wisdom_lock_file << std::endl;
    int fd =
        open(wisdom_lock_file.c_str(), O_WRONLY | O_CREAT | O_NOCTTY | O_NONBLOCK, 0666);
    if (fd < 0) {
        throw std::runtime_error("Failed to create FFTW wisdom lockfile: " +
                                 wisdom_lock_file);
    }
    close(fd);
    wisdom_lock = gr::file_lock(wisdom_lock_file);
    wisdom_lock_init_done = true;
}

static void lock_wisdom()
{
    wisdom_thread_mutex.lock();
    wisdom_lock_init();
    wisdom_lock.lock();
}

static void unlock_wisdom()
{
    // Assumes 'lock_wisdom' has already been called (i.e. this file_lock is valid)
    wisdom_lock.unlock();
    wisdom_thread_mutex.unlock();
}

static void import_wisdom()
{
    const std::string filename = wisdom_filename();
    FILE* fp = fopen(filename.c_str(), "r");
    if (fp != 0) {
        int r = fftwf_import_wisdom_from_file(fp);
        fclose(fp);
        if (!r) {
            gr::logger_ptr logger, debug_logger;
            gr::configure_default_loggers(logger, debug_logger, "fft::import_wisdom");
            logger->error("can't import wisdom from {}", filename.c_str());
        }
    }
}

static void config_threading(int nthreads)
{
#ifdef FFTW3F_THREADS
    static int fftw_threads_inited = 0;

    if (fftw_threads_inited == 0) {
        fftw_threads_inited = 1;
        fftwf_init_threads();
    }

    fftwf_plan_with_nthreads(nthreads);
#endif
}

static void export_wisdom()
{
    const std::string filename = wisdom_filename();
    FILE* fp = fopen(filename.c_str(), "w");
    if (fp != 0) {
        fftwf_export_wisdom_to_file(fp);
        fclose(fp);
    }
    else {
        gr::logger_ptr logger, debug_logger;
        gr::configure_default_loggers(logger, debug_logger, "fft::import_wisdom");
        logger->error("{}}: {}}", filename.c_str(), strerror(errno));
    }
}

// ----------------------------------------------------------------


template <class T, bool forward>
fftw_fft<T, forward>::fftw_fft(int fft_size, int nthreads)
    : d_nthreads(nthreads), d_inbuf(fft_size), d_outbuf(fft_size)
{
    gr::configure_default_loggers(d_logger, d_debug_logger, "fft_complex");
    // Hold global mutex during plan construction and destruction.
    std::scoped_lock lock(planner::mutex());

    static_assert(sizeof(fftwf_complex) == sizeof(gr_complex),
                  "The size of fftwf_complex is not equal to gr_complex");

    if (fft_size <= 0) {
        throw std::out_of_range("fft_impl_fftw: invalid fft_size");
    }

    config_threading(nthreads);
    lock_wisdom();
    import_wisdom(); // load prior wisdom from disk

    initialize_plan(fft_size);
    if (d_plan == NULL) {
        d_logger->error("creating plan failed");
        throw std::runtime_error("Creating fftw plan failed");
    }
    export_wisdom(); // store new wisdom to disk
    unlock_wisdom();
}

template <>
void fftw_fft<gr_complex, true>::initialize_plan(int fft_size)
{
    d_plan = fftwf_plan_dft_1d(fft_size,
                               reinterpret_cast<fftwf_complex*>(d_inbuf.data()),
                               reinterpret_cast<fftwf_complex*>(d_outbuf.data()),
                               FFTW_FORWARD,
                               FFTW_MEASURE);
}

template <>
void fftw_fft<gr_complex, false>::initialize_plan(int fft_size)
{
    d_plan = fftwf_plan_dft_1d(fft_size,
                               reinterpret_cast<fftwf_complex*>(d_inbuf.data()),
                               reinterpret_cast<fftwf_complex*>(d_outbuf.data()),
                               FFTW_BACKWARD,
                               FFTW_MEASURE);
}


template <>
void fftw_fft<float, true>::initialize_plan(int fft_size)
{
    d_plan = fftwf_plan_dft_r2c_1d(fft_size,
                                   d_inbuf.data(),
                                   reinterpret_cast<fftwf_complex*>(d_outbuf.data()),
                                   FFTW_MEASURE);
}

template <>
void fftw_fft<float, false>::initialize_plan(int fft_size)
{
    d_plan = fftwf_plan_dft_c2r_1d(fft_size,
                                   reinterpret_cast<fftwf_complex*>(d_inbuf.data()),
                                   d_outbuf.data(),
                                   FFTW_MEASURE);
}


template <class T, bool forward>
fftw_fft<T, forward>::~fftw_fft()
{
    // Hold global mutex during plan construction and destruction.
    std::scoped_lock lock(planner::mutex());

    fftwf_destroy_plan((fftwf_plan)d_plan);
}

template <class T, bool forward>
void fftw_fft<T, forward>::set_nthreads(int n)
{
    if (n <= 0) {
        throw std::out_of_range("gr::fft: invalid number of threads");
    }
    d_nthreads = n;

#ifdef FFTW3F_THREADS
    fftwf_plan_with_nthreads(d_nthreads);
#endif
}

template <class T, bool forward>
void fftw_fft<T, forward>::execute()
{
    fftwf_execute((fftwf_plan)d_plan);
}


template class fftw_fft<gr_complex, true>;
template class fftw_fft<gr_complex, false>;
template class fftw_fft<float, true>;
template class fftw_fft<float, false>;
} /* namespace fft */
} // namespace kernel
} // namespace gr