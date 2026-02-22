/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/fir.hpp"
#include "protocols/M17/M17DSP.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <random>
#include <chrono>

static constexpr size_t BLOCK_SIZE = 1920; // based on M17_FRAME_SAMPLES

int main()
{
    // Test a new Fir class implementation
    printf("Comparing Fir class with BlockFir class\n");

    // Generate random samples to simulate data for FIR
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0, 1.0);
    std::array<float, BLOCK_SIZE> testData;
    for (size_t i = 0; i < BLOCK_SIZE; i++)
        testData[i] = dist(gen);

    BlockFir< float, std::tuple_size< decltype(M17::rrc_taps_48k) >::value, BLOCK_SIZE > blockFir(M17::rrc_taps_48k);
    Fir< float, std::tuple_size< decltype(M17::rrc_taps_48k) >::value > fir(M17::rrc_taps_48k);

    float diff_sum = 0;
    size_t i;
    for (i = 0; i < 1000; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        for (auto &sample: testData) {
            fir(sample);
        }
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        int firMicroSeconds = duration.count();

        start = std::chrono::high_resolution_clock::now();
        blockFir(testData);
        stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        int blockFirMicroSeconds = duration.count();

        float diff = ((firMicroSeconds - blockFirMicroSeconds)*100)/blockFirMicroSeconds;
        printf("Fir class: %d µs, BlockFir class: %d µs, %.2f%% decrease\n",
                firMicroSeconds, blockFirMicroSeconds, diff);
        diff_sum += diff;
    }
    printf("%.2f%% average decrease\n", diff_sum / i);
}
