/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "interfaces/radio.h"
#include "rtx/OpMode_APRS.hpp"
#include "protocols/APRS/packet.h"
#include "rtx/rtx.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

#if defined(PLATFORM_TTWRPLUS)
#include "drivers/baseband/AT1846S.h"
#endif

/**
 * \internal
 * On MD-UV3x0 radios the volume knob does not regulate the amplitude of the
 * analog signal towards the audio amplifier but it rather serves to provide a
 * digital value to be fed into the HR_C6000 lineout DAC gain. We thus have to
 * provide the helper function below to keep the real volume level consistent
 * with the knob position.
 */
#if defined(PLATFORM_TTWRPLUS)
void _setVolume()
{
    static uint8_t oldVolume = 0xFF;
    uint8_t volume = platform_getVolumeLevel();

    if (volume == oldVolume)
        return;

    // AT1846S volume control is 4 bit
    AT1846S::instance().setRxAudioGain(volume / 16, volume / 16);
    oldVolume = volume;
}
#endif

OpMode_APRS::OpMode_APRS() : rfSqlOpen(false), sqlOpen(false), enterRx(true)
{
}

OpMode_APRS::~OpMode_APRS()
{
}

void OpMode_APRS::enable()
{
    // When starting, close squelch and prepare for entering in RX mode.
    rfSqlOpen = false;
    sqlOpen = false;
    enterRx = true;
    makePkts = true;
}

void OpMode_APRS::disable()
{
    // Remove APRS packets that haven't been pulled
    rtxStatus_t status = rtx_getCurrentStatus();
    if (status.aprsPkts) {
        for (aprsPacket_t *pkt = status.aprsPkts; pkt; pkt = pkt->next)
            aprsPktFree(pkt);
    }

    // Clean shutdown.
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    audioPath_release(rxAudioPath);
    audioPath_release(txAudioPath);
    radio_disableRtx();
    rfSqlOpen = false;
    sqlOpen = false;
    enterRx = false;
}

void OpMode_APRS::update(rtxStatus_t *const status, const bool newCfg)
{
    (void)newCfg;

    if (makePkts) {
        // make some test packets
        status->aprsRecv = 10;
        status->aprsSaved = 10;
        aprsPacket_t *pkt = NULL;
        for (uint8_t i = 0; i < 10; i++) {
            pkt = (aprsPacket_t *)malloc(sizeof(aprsPacket_t));
            pkt->prev = NULL;
            pkt->next = NULL;
            // first address (dst)
            pkt->addresses = (aprsAddress_t *)malloc(sizeof(aprsAddress_t));
            pkt->addresses->addr[6] = '\0';
            sniprintf(pkt->addresses->addr, 6, "APRS%d", i);
            pkt->addresses->ssid = 0;
            // second address (src)
            pkt->addresses->next =
                (aprsAddress_t *)malloc(sizeof(aprsAddress_t));
            pkt->addresses->next->addr[6] = '\0';
            strncpy(pkt->addresses->next->addr, "N2BP", 6);
            pkt->addresses->next->ssid = 7;
            pkt->addresses->next->next = NULL;
            // info
            pkt->info = (char *)malloc(16);
            sniprintf(pkt->info, 16, ":Test packet %d", i);
            // timestamp
            pkt->ts = platform_getCurrentTime();
            status->aprsPkts = aprsPktsInsert(status->aprsPkts, pkt);
        }
        /*uint8_t testFrame[] = {
            0x9C, 0x94, 0x6E, 0xA0, 0x40, 0x40, 0xE0, 0x9C, 0x6E, 0x98,
            0x8A, 0x9A, 0x40, 0x61, 0x03, 0xF0, 0x54, 0x68, 0x65, 0x20,
            0x71, 0x75, 0x69, 0x63, 0x6B, 0x20, 0x62, 0x72, 0x6F, 0x77,
            0x6E, 0x20, 0x66, 0x6F, 0x78, 0x20, 0x6A, 0x75, 0x6D, 0x70,
            0x73, 0x20, 0x6F, 0x76, 0x65, 0x72, 0x20, 0x74, 0x68, 0x65,
            0x20, 0x6C, 0x61, 0x7A, 0x79, 0x20, 0x64, 0x6F, 0x67
        };
        aprsPacket_t *testPkt = aprsPktFromFrame(testFrame, 59);
        status->aprsPkts = testPkt;
        testPkt->next = status->aprsPkts;
        status->aprsPkts->prev = testPkt;
        testPkt->prev = NULL;
        status->aprsPkts = testPkt;*/
        makePkts = false;
    }

#if defined(PLATFORM_TTWRPLUS)
    // Set output volume by changing the HR_C6000 DAC gain
    _setVolume();
#endif

    // RX logic
    if (status->opStatus == RX) {
        // RF squelch mechanism
        // This turns squelch (0 to 15) into RSSI (-127.0dbm to -61dbm)
        rssi_t squelch = -127 + (status->sqlLevel * 66) / 15;
        rssi_t rssi = rtx_getRssi();

        // Provide a bit of hysteresis, only change state if the RSSI has
        // moved more than 1dBm on either side of the current squelch setting.
        if ((rfSqlOpen == false) && (rssi > (squelch + 1)))
            rfSqlOpen = true;
        if ((rfSqlOpen == true) && (rssi < (squelch - 1)))
            rfSqlOpen = false;

        // Local flags for current RF and tone squelch status
        bool rfSql = ((status->rxToneEn == 0) && (rfSqlOpen == true));
        bool toneSql = ((status->rxToneEn == 1)
                        && radio_checkRxDigitalSquelch());

        // Audio control
        if ((sqlOpen == false) && (rfSql || toneSql)) {
            rxAudioPath = audioPath_request(SOURCE_RTX, SINK_SPK, PRIO_RX);
            if (rxAudioPath > 0)
                sqlOpen = true;
        }

        if ((sqlOpen == true) && (rfSql == false) && (toneSql == false)) {
            audioPath_release(rxAudioPath);
            sqlOpen = false;
        }
    } else if ((status->opStatus == OFF) && enterRx) {
        radio_disableRtx();

        radio_enableRx();
        status->opStatus = RX;
        enterRx = false;
    }

    // TX logic
    if (platform_getPttStatus() && (status->opStatus != TX)
        && (status->txDisable == 0)) {
        audioPath_release(rxAudioPath);
        radio_disableRtx();

        txAudioPath = audioPath_request(SOURCE_MIC, SINK_RTX, PRIO_TX);
        radio_enableTx();

        status->opStatus = TX;
    }

    if (!platform_getPttStatus() && (status->opStatus == TX)) {
        audioPath_release(txAudioPath);
        radio_disableRtx();

        status->opStatus = OFF;
        enterRx = true;
        sqlOpen = false; // Force squelch to be redetected.
    }

    // Led control logic
    switch (status->opStatus) {
        case RX:
            if (radio_checkRxDigitalSquelch()) {
                platform_ledOn(
                    GREEN); // Red + green LEDs ("orange"): tone squelch open
                platform_ledOn(RED);
            } else if (rfSqlOpen) {
                platform_ledOn(GREEN); // Green LED only: RF squelch open
                platform_ledOff(RED);
            } else {
                platform_ledOff(GREEN);
                platform_ledOff(RED);
            }

            break;

        case TX:
            platform_ledOff(GREEN);
            platform_ledOn(RED);
            break;

        default:
            platform_ledOff(GREEN);
            platform_ledOff(RED);
            break;
    }

    // Sleep thread for 30ms for 33Hz update rate
    sleepFor(0u, 30u);
}

bool OpMode_APRS::rxSquelchOpen()
{
    return sqlOpen;
}
