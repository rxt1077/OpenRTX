/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/Demodulator.hpp"
#include <cmath>

using namespace APRS;

// these are in protocols/APRS/Demodulator.hpp
constexpr std::array<int16_t, Demodulator::BPF_TAP_SIZE> Demodulator::BPF_TAPS;
constexpr std::array<int16_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::MARK_CORRELATOR_I;
constexpr std::array<int16_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::MARK_CORRELATOR_Q;
constexpr std::array<int16_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::SPACE_CORRELATOR_I;
constexpr std::array<int16_t, Demodulator::CORRELATOR_SIZE>
    Demodulator::SPACE_CORRELATOR_Q;
constexpr std::array<int16_t, Demodulator::LPF_SIZE> Demodulator::LPF_TAPS;

Demodulator::Demodulator()
{
}

Demodulator::~Demodulator()
{
}

int16_t Demodulator::operator()(int16_t input)
{
    // Input band-pass filter
    int16_t filteredAudio = bpf(input);

    // Calculate mark and space correlations
    int16_t mark = abs(markI(filteredAudio)) + abs(markQ(filteredAudio));
    int16_t space = abs(spaceI(filteredAudio)) + abs(spaceQ(filteredAudio));
    int16_t diff = (mark - space) >> 3; // prevent overflow of int16_t

    // Output low-pass filter
    return lpf(diff);
}
