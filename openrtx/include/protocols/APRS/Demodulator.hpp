/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_DEMODULATOR_H
#define APRS_DEMODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>
#include <array>
#include "core/fir.hpp"
#include "protocols/APRS/constants.h"

namespace APRS
{

class Demodulator
{
public:
    /**
     * @brief Constructs a Demodulator object
     */
    Demodulator();

    /**
     * @brief Destroys a Demodulator object
     */
    ~Demodulator();

    /**
     * @brief Demodulates baseband audio by mark and space frequencies
     *
     * Calculates the difference between the magnitude of the mark tone and
     * the magnitued of the space tone using FIR filter when fed one sample at
     * a time
     *
     * @param input A int16_t baseband audio sample
     *
     * @return The difference between the mark and space tones
     */
    int16_t operator()(const int16_t input);

private:
    // Based on pymodem: https://github.com/ninocarrillo/pymodem
    // See https://github.com/rxt1077/aprs_prototype/ for how these were derived

    // input band-pass FIR filter
    static constexpr size_t BPF_TAP_SIZE = 7;
    static constexpr std::array<int16_t, BPF_TAP_SIZE> BPF_TAPS = { -1, -1, 1,
                                                                    2,  1,  -1,
                                                                    -1 };
    Fir<int16_t, BPF_TAP_SIZE> bpf{ BPF_TAPS };

    // mark/space correlators
    static constexpr size_t CORRELATOR_SIZE = 8;
    static constexpr std::array<int16_t, CORRELATOR_SIZE> MARK_CORRELATOR_I = {
        8, 5, 0, -5, -8, -5, 0, 5
    };
    static constexpr std::array<int16_t, CORRELATOR_SIZE> MARK_CORRELATOR_Q = {
        0, 5, 8, 5, 0, -5, -8, -5
    };
    static constexpr std::array<int16_t, CORRELATOR_SIZE> SPACE_CORRELATOR_I = {
        8, 1, -7, -3, 6, 4, -5, -6
    };
    static constexpr std::array<int16_t, CORRELATOR_SIZE> SPACE_CORRELATOR_Q = {
        0, 7, 2, -7, -3, 6, 5, -4
    };
    Fir<int16_t, CORRELATOR_SIZE> markI{ MARK_CORRELATOR_I };
    Fir<int16_t, CORRELATOR_SIZE> markQ{ MARK_CORRELATOR_Q };
    Fir<int16_t, CORRELATOR_SIZE> spaceI{ SPACE_CORRELATOR_I };
    Fir<int16_t, CORRELATOR_SIZE> spaceQ{ SPACE_CORRELATOR_Q };

    // output low-pass FIR filter
    static constexpr size_t LPF_SIZE = 4;
    static constexpr std::array<int16_t, LPF_SIZE> LPF_TAPS = { 1, 2, 2, 1 };
    Fir<int16_t, LPF_SIZE> lpf{ LPF_TAPS };
};

} /* APRS */

#endif
