#include "PicoStaticLib.hpp"

#include <windows.h>
// ÉJÅ[É\ÉãÇà⁄ìÆ
void setCursorPosition(int x, int y) {
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

#include <cmath>

double adc2mV(int16_t adc, int16_t range, double adcMax) {
	return double(adc) * channelInputRanges[range] / adcMax;
}

int16_t mV2adc(double mV, int16_t range, double adcMax) {
	return std::round(mV * adcMax / channelInputRanges[range]);
}

double getRange(int16_t range) {
	return channelInputRanges[range];
}


void getMaxMin(int32_t no_of_values, int16_t bufferA[], int16_t& minValue, int32_t& minIndex, int16_t& maxValue, int32_t& maxIndex) {
	minValue = bufferA[0];
	maxValue = bufferA[0];
	minIndex = 0;
	maxIndex = 0;
	for (int i = 1; i < no_of_values; ++i) {
		if (bufferA[i] < minValue) {
			minValue = bufferA[i];
			minIndex = i;
		}
		if (bufferA[i] > maxValue) {
			maxValue = bufferA[i];
			maxIndex = i;
		}
	}
}