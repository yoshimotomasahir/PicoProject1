#pragma once

#include <cstdint>
#include <iomanip>

void setCursorPosition(int x, int y);

#define space_pad(num) std::setfill(' ') << std::right << std::setw(num)

const double channelInputRanges[] = { 10., 20., 50., 100., 200., 500., 1000., 2000., 5000., 10000., 20000., 50000., 100000., 200000. };
void getMaxMin(int32_t no_of_values, int16_t bufferA[], int16_t& minValue, int32_t& minIndex, int16_t& maxValue, int32_t& maxIndex);
double adc2mV(int16_t adc, int16_t range, double adcMax = 32767.0);
int16_t mV2adc(double mV, int16_t range, double adcMax = 32767.0);
double getRange(int16_t range);
