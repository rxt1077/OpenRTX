/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_HDLC_H
#define APRS_HDLC_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstddef>
#include <cstdint>
#include <vector>
#include "protocols/APRS/constants.h"

namespace APRS
{

class Decoder
{
public:
    /**
     * @brief Construct a Decoder
     */
    Decoder();

    /**
     * @brief Destroy a Decoder
     */
    ~Decoder();

    /**
     * @brief Performs HDLC decoding
     *
     * Performs stateful HDLC decoding: NRZI decoding, bit-unstuffing,
     * CRC checking, etc.
     *
     * @param bit A uint8_t bit from the slicer
     *
     * @return A pointer to byte vector of a valid HDLC frame, or NULL if no
     * frame has been decoded yet 
     */
    std::vector<uint8_t> *operator()(uint8_t bit);

    /**
     * @brief Checks the CRC of the recieved bytes
     *
     * Checks the CRC of the received bytes to see if they are a valid
     * frame. The last two bytes received are the CRC.
     *
     * @return true if the CRC is valid, false otherwise
     */
    bool checkCRC();

private:
    uint8_t onesCount = 0;
    uint8_t prevBit = 0;
    uint8_t workingByte = 0;
    uint8_t bitPos = 0;
    std::vector<uint8_t> *currentFrame = NULL;
};

} /* APRS */

#endif
