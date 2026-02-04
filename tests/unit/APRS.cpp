/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/dsp.h"
#include "protocols/APRS/constants.h"
#include "protocols/APRS/Demodulator.hpp"
#include "protocols/APRS/Slicer.hpp"
#include "protocols/APRS/HDLC.hpp"
#include "protocols/APRS/packet.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr const char *TEST_FILE = "../tests/unit/assets/aprs.raw";
constexpr const char *TEST_PACKETS[] = {
    "N2BP-7>APRS::N2BP-7   :Testing 1", "N2BP-7>APRS::N2BP-7   :Testing 2",
    "N2BP-7>APRS::N2BP-7   :Testing 3", "N2BP-7>APRS::N2BP-7   :Testing 4",
    "N2BP-7>APRS::N2BP-7   :Testing 5", "N2BP-7>APRS::N2BP-7   :Testing 6",
    "N2BP-7>APRS::N2BP-7   :Testing 7", "N2BP-7>APRS::N2BP-7   :Testing 8",
    "N2BP-7>APRS::N2BP-7   :Testing 9", "N2BP-7>APRS::N2BP-7   :Testing 10"
};

/**
 * @brief Fills a buffer with the TNC2 text representation of an APRS packet
 *
 * @param pkt pointer to an aprsPacket_t
 * @param output pointer to char buffer to fill
 */
void strFromPkt(aprsPacket_t *pkt, char *output)
{
    char ssid[4];

    aprsAddress_t *dst = pkt->addresses;
    aprsAddress_t *src = pkt->addresses->next;
    output[0] = '\0';
    strcat(output, src->addr);
    if (src->ssid) {
        sprintf(ssid, "-%d", src->ssid);
        strcat(output, ssid);
    }
    strcat(output, ">");
    strcat(output, dst->addr);
    if (src->next) {
        for (aprsAddress_t *address = src->next; address;
             address = address->next) {
            strcat(output, ",");
            strcat(output, address->addr);
            if (address->ssid) {
                sprintf(ssid, "-%d", address->ssid);
                strcat(output, ssid);
            }
            if (address->commandHeard)
                strcat(output, "*");
        }
    }
    strcat(output, ":");
    strcat(output, pkt->info);
}

int main()
{
    // Test APRS Demodulator, Slicer, Decoder, and packet on sample data
    printf("Testing APRS\n");

    FILE *file = fopen(TEST_FILE, "rb");
    if (!file) {
        printf("Error opening APRS baseband file\n");
        return -1;
    }
    int16_t *baseband = (int16_t *)malloc(APRS_BUF_SIZE * sizeof(int16_t));
    if (!baseband) {
        printf("Error allocating memory for baseband buffer\n");
        return -1;
    }
#ifdef APRS_DEBUG
    FILE *demodFile = fopen("APRS_demodulator_output.csv", "w");
    if (!demodFile) {
        printf("Error opening APRS demodulator output file\n");
        return -1;
    }
    FILE *decisionsFile = fopen("APRS_slicer_decisions.csv", "w");
    if (!decisionsFile) {
        printf("Error opening APRS slicer decisions file\n");
        return -1;
    }
#endif

    struct dcBlock dcBlock;
    dsp_resetState(dcBlock);
    APRS::Demodulator demod;
    APRS::Slicer slice;
    APRS::Decoder decode;
    size_t readItems = 0;
    size_t pktNum = 0;
    bool errorFlag = false;
    while ((readItems = fread(baseband, 2, APRS_BUF_SIZE, file)) > 0) {
        for (size_t i = 0; i < readItems; i++) {
            // remove DC bias and scale down the baseband audio by 4
            int16_t sample = dsp_dcBlockFilter(&dcBlock, baseband[i]) >> 2;

            // demodulate
            sample = demod(sample);
#ifdef APRS_DEBUG
            fprintf(demodFile, "%ld,%d\n", i, sample);
#endif

            // slice
            int8_t bit = slice(sample);
            if (bit < 0) { // there's typically 1 bit for every 8 samples
#ifdef APRS_DEBUG
                fprintf(decisionsFile, "%ld,%d\n", i, bit);
#endif
                continue;
            }

            std::vector<uint8_t> *frame = decode(bit);
            if (frame) {
                char pktStr[256];
                aprsPacket_t *pkt = aprsPktFromFrame(frame->data(),
                                                     frame->size());
                delete frame;
                strFromPkt(pkt, pktStr);
                printf("Frame %ld: %s\n", pktNum, pktStr);
                aprsPktFree(pkt);
                /*if (strcmp(TEST_PACKETS[pktNum], pktStr)) {
                    printf("Packet %ld does not match:\n", pktNum);
                    printf("    Expected \"%s\"\n", TEST_PACKETS[pktNum]);
                    printf("     Decoded \"%s\"\n", pktStr);
                    errorFlag = true;
                }*/
                pktNum++;
            }
        }
    }
    fclose(file);
#ifdef APRS_DEBUG
    fclose(demodFile);
    fclose(decisionsFile);
#endif
    free(baseband);

    if (errorFlag) {
        return -1;
    }
    if (pktNum != 10) {
        printf("%ld frames decoded, expected 10\n", pktNum);
        return -1;
    }
    return 0;
}
