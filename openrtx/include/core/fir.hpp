/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FIR_H
#define FIR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

/**
 * Class for FIR filter with configurable coefficients.
 * Adapted from the original implementation by Rob Riggs, Mobilinkd LLC.
 */
template < typename T, size_t N >
class Fir
{
public:

    /**
     * Constructor.
     *
     * @param taps: reference to a std::array of floating poing values representing
     * the FIR filter coefficients.
     */
    Fir(const std::array< T, N >& taps) : taps(taps), pos(0)
    {
        reset();
    }

    /**
     * Destructor.
     */
    ~Fir() { }

    /**
     * Perform one step of the FIR filter, computing a new output value given
     * the input value and the history of previous input values.
     *
     * @param input: FIR input value for the current time step.
     * @return FIR output as a function of the current and past input values.
     */
    T operator()(const T& input)
    {
        T acc = 0;
        size_t i;

        pos = (pos == 0 ? N - 1 : pos - 1);
        hist[pos] = input;
        hist[pos + N] = input;

        // Unroll loop by 4
        for (i = 0; i < (N & ~size_t(3)); i += 4) {
            acc += hist[pos + i] * taps[i];
            acc += hist[pos + i + 1] * taps[i + 1];
            acc += hist[pos + i + 2] * taps[i + 2];
            acc += hist[pos + i + 3] * taps[i + 3];
        }

        // Remaining taps
        for (; i < N; i++)
            acc += hist[pos + i] * taps[i];

        return acc;
    }

    /**
     * Reset FIR history, clearing the memory of past values.
     */
    void reset()
    {
        hist.fill(0);
        pos = 0;
    }

    void print() {
        for (const auto& tap: taps) {
            printf("taps: %d ", tap);
        }
        printf("\n");
    }

private:

    const std::array< T, N >& taps;    ///< FIR filter coefficients.
    std::array< T, 2 * N >    hist;    ///< History of past inputs.
    size_t                    pos;     ///< Current position in history.
};

/**
 * @brief A finite impulse response class that works with blocks of data.
 *
 * Performs linear convolution on chunks of input using the overlap add
 * algorithm found here:
 * https://blog.robertelder.org/overlap-add-overlap-save/
 * Convolution code is based on the following:
 * https://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_Ch6.pdf
 * This class is stateful and can operate continuously on a stream of input.
 */
template < typename T, size_t FirSize, size_t DataSize >
class BlockFir
{
public:
    /**
     * Constructor.
     *
     * @param taps: reference to a std::array of floating poing values representing
     * the FIR filter coefficients.
     */
    BlockFir(const std::array< T, FirSize >& taps) : taps(taps) {
        prevOverlap.fill(0);
    }

    const std::array< T, DataSize >& operator()(const std::array< T, DataSize >& input) {
        size_t i, j;
        T sample;

        // zero out the output and newOverlap buffers
        output.fill(0);
        newOverlap.fill(0);

        // based on TABLE 6-1 'CONVOLUTION USING THE INPUT SIDE ALGORITHM'
        for (i = 0; i < (DataSize + FirSize - 1); i++) {
            if (i < DataSize)
                sample = input[i];
            else
                // pad the end of the signal with zeroes
                sample = 0;
            for (j = 0; j < FirSize; j++) {
                if ((i + j) < DataSize)
                    output[i + j] += sample * taps[j];
                else {
                    if ((i + j - DataSize) < (FirSize - 1))
                        // the end of our convolution should go in the new_overlap buffer
                        newOverlap[i + j - DataSize] += sample * taps[j];
                }
            }
        }

        // add the previous overlap to the front of the output and swap
        // the overlap buffers
        for (i = 0; i < (FirSize - 1); i++) {
            output[i] += prevOverlap[i];
            prevOverlap[i] = newOverlap[i];
        }

        return output;
    }

    void print() {
        printf("taps: ");
        for (const auto& tap: taps) {
            printf("%d ", tap);
        }
        printf("\n");
    }

private:
    const std::array< T, FirSize >& taps;    ///< FIR filter coefficients.
    std::array< T, FirSize - 1> prevOverlap; ///< buffer to hold previous overlap
    std::array< T, FirSize - 1> newOverlap;  ///< buffer to hold new overlap
    std::array< T, DataSize > output;        ///< buffer to hold output
};

#endif /* DSP_H */
