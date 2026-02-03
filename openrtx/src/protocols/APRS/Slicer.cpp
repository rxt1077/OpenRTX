/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/Slicer.hpp"

using namespace APRS;

Slicer::Slicer()
{
}

Slicer::~Slicer()
{
}

int8_t Slicer::operator()(const int16_t input)
{
    int8_t result = -1;

    // increment clock
    clock++;
    // check for symbol center
    if (clock >= Slicer::ROLLOVER_THRESHOLD) {
        // at or past symbol center, reset clock
        clock -= SAMPLES_PER_SYMBOL;
        // make a bit decision
        if (input >= 0)
            // this is a '1' bit
            result = 1;
        else
            // otherwise it's a '0' bit
            result = 0;
    }
    // check for zero-crossing in sample stream
    if ((lastSample ^ input) < 0) {
        // the clock should be zero at the crossing, if it's off by too much
        if (abs(clock) >= NUDGE_THRESH) {
            if (clock > 0)
                // if it's ahead nudge it back
                clock -= NUDGE;
            else
                // if it's behind nudge it forward
                clock += NUDGE;
        }
    }
    lastSample = input;
    return result;
}
