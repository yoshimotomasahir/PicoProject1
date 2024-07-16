#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include "ps2000.h"

#define space_pad(num) std::setfill(' ') << std::right << std::setw(num)

const double channelInputRanges[] = { 10., 20., 50., 100., 200., 500., 1000., 2000., 5000., 10000., 20000., 50000., 100000., 200000. };
double adc2mV(int16_t adc, int16_t range, double adcMax = 32767.0) {
	return double(adc) * channelInputRanges[range] / adcMax;
}

int16_t mV2adc(double mV, int16_t range, double adcMax = 32767.0) {
	return std::round(mV * adcMax / channelInputRanges[range]);
}

double getRange(int16_t range) {
	return channelInputRanges[range];
}

#include <windows.h>
// カーソルを移動
void setCursorPosition(int x, int y) {
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
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

#define BUFFER_SIZE 	1024
int main() {

	// デバイスをオープン
	int16_t handle = ps2000_open_unit();
	if (handle <= 0) {
		std::cerr << "デバイスのオープンに失敗しました。" << std::endl;
		return 1;
	}

	// サンプル数と電圧レンジとサンプリング間隔を設定
	const int32_t sampleCount = 1250; // 取得するサンプル数
	int16_t oversample = 1;     // オーバーサンプル (??)
	int16_t range = PS2000_100MV; // 電圧レンジ設定
	int16_t timeUnits = PS2000_NS; // 時間単位 (ns)
	int16_t timeBase = 3;
	double timeInterval = std::pow(2, timeBase) * 10; // ns

	// チャンネル設定
	ps2000_set_channel(handle, PS2000_CHANNEL_A, 1, PS2000_DC_VOLTAGE, range);
	ps2000_set_channel(handle, PS2000_CHANNEL_B, 1, PS2000_DC_VOLTAGE, range);

	// 初期表示
	std::cout << "Voltage range: (+/-)       [mV]\n";
	std::cout << "Max voltageAB:             [mV]             [mV] \n";
	std::cout << "Min voltageAB:             [mV]             [mV] \n";
	std::cout << "Samples      :              \n";
	std::cout << "Sample rate  :             [MS/s]\n";
	std::cout << "Time interval:             [us]             [Hz]\n";
	std::cout << "Start   time :             [us]\n";
	std::cout << "End     time :             [us]             [Hz]\n";
	std::cout << "Elapsed time :             [us]             [Hz]\n";
	std::cout << std::flush;
	int x1 = 15;
	int x2 = 32;

	// トリガー(なし)
	ps2000_set_trigger(handle, PS2000_NONE, 0, 0, 0, 0);

	int16_t  sample_interval_ms = 1;
	ps2000_run_streaming(handle, sample_interval_ms, sampleCount, 0);
	setCursorPosition(x1, 4); std::cout << space_pad(11) << sample_interval_ms * 1000.0 * 2 / 1e6;
	setCursorPosition(x1, 5); std::cout << space_pad(11) << sample_interval_ms * 1000.0;
	setCursorPosition(x2, 5); std::cout << space_pad(11) << 1.e6 / (sample_interval_ms * 1000.0);

	int16_t bufferA[sampleCount];
	int16_t bufferB[sampleCount];
	int16_t		overflow;
	int32_t no_of_values;
	auto start0 = std::chrono::high_resolution_clock::now();
	auto start = std::chrono::high_resolution_clock::now();

	uint32_t totalSamples = 0;

	while (true) {
		no_of_values = ps2000_get_values(handle, bufferA, bufferB, NULL, NULL, &overflow, BUFFER_SIZE);

		if (no_of_values > 0) {
			totalSamples += no_of_values;

			setCursorPosition(x1, 3);  std::cout << space_pad(11) << no_of_values;
			setCursorPosition(0, 9);
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			auto duration0 = std::chrono::duration_cast<std::chrono::microseconds>(end - start0);
			setCursorPosition(x1, 6); std::cout << space_pad(11) << 0;
			setCursorPosition(x1, 7); std::cout << space_pad(11) << duration.count();
			setCursorPosition(x2, 7); std::cout << space_pad(11) << 1e6 / duration.count() * no_of_values;
			start = std::chrono::high_resolution_clock::now();

			int16_t minValueA, maxValueA, minValueB, maxValueB;
			int32_t minIndexA, maxIndexA, minIndexB, maxIndexB;
			getMaxMin(no_of_values, bufferA, minValueA, minIndexA, maxValueA, maxIndexA);
			getMaxMin(no_of_values, bufferB, minValueB, minIndexB, maxValueB, maxIndexB);
			setCursorPosition(x1 + 6, 0); std::cout << getRange(range);

			setCursorPosition(x1, 1); std::cout << space_pad(11) << adc2mV(maxValueA, range);
			setCursorPosition(x2, 1); std::cout << space_pad(11) << adc2mV(maxValueB, range);
			setCursorPosition(x1, 2); std::cout << space_pad(11) << adc2mV(minValueA, range);
			setCursorPosition(x2, 2); std::cout << space_pad(11) << adc2mV(minValueB, range);

			setCursorPosition(x1, 8); std::cout << space_pad(11) << duration0.count();
			setCursorPosition(x2, 8); std::cout << space_pad(11) << 1e6 / duration0.count() * totalSamples;
			setCursorPosition(0, 9);
			std::cout << std::flush;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10ms 待つ
	}

	// デバイスを停止
	ps2000_stop(handle);

	return 0;
}