#include <iostream>
#include <thread>
#include <chrono>
#include "ps2000.h"
#include <PicoStaticLib.hpp>

int main() {

	// デバイスをオープン
	int16_t handle = ps2000_open_unit();
	if (handle <= 0) {
		std::cerr << "デバイスのオープンに失敗しました。" << std::endl;
		return 1;
	}

	// サンプル数と電圧レンジとサンプリング間隔を設定
	const int32_t sampleCount = 1250; // 取得するサンプル数
	int16_t range = PS2000_100MV; // 電圧レンジ設定

	// チャンネル設定
	ps2000_set_channel(handle, PS2000_CHANNEL_A, 1, PS2000_DC_VOLTAGE, range);
	ps2000_set_channel(handle, PS2000_CHANNEL_B, 1, PS2000_DC_VOLTAGE, range);

	// トリガー(なし)
	ps2000_set_trigger(handle, PS2000_NONE, 0, 0, 0, 0);

	// サンプル間隔を指定(最小1ms)
	int16_t  sample_interval_ms = 1;
	ps2000_run_streaming(handle, sample_interval_ms, sampleCount, 0);

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
	setCursorPosition(x1 + 6, 0); std::cout << getRange(range);
	setCursorPosition(x1, 4); std::cout << space_pad(11) << sample_interval_ms * 1000.0 * 2 / 1e6;
	setCursorPosition(x1, 5); std::cout << space_pad(11) << sample_interval_ms * 1000.0;
	setCursorPosition(x2, 5); std::cout << space_pad(11) << 1.e6 / (sample_interval_ms * 1000.0);

	int16_t bufferA[sampleCount];
	int16_t bufferB[sampleCount];
	int16_t		overflow;
	int32_t nValues;
	auto start0 = std::chrono::high_resolution_clock::now();
	auto start = std::chrono::high_resolution_clock::now();

	uint32_t totalSamples = 0;

	while (true) {
		nValues = ps2000_get_values(handle, bufferA, bufferB, NULL, NULL, &overflow, sampleCount);

		if (nValues > 0) {
			totalSamples += nValues;

			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			auto duration0 = std::chrono::duration_cast<std::chrono::microseconds>(end - start0);
			start = std::chrono::high_resolution_clock::now();

			int16_t minValueA, maxValueA, minValueB, maxValueB;
			int32_t minIndexA, maxIndexA, minIndexB, maxIndexB;
			getMaxMin(nValues, bufferA, minValueA, minIndexA, maxValueA, maxIndexA);
			getMaxMin(nValues, bufferB, minValueB, minIndexB, maxValueB, maxIndexB);

			setCursorPosition(x1, 1); std::cout << space_pad(11) << adc2mV(maxValueA, range);
			setCursorPosition(x2, 1); std::cout << space_pad(11) << adc2mV(maxValueB, range);
			setCursorPosition(x1, 2); std::cout << space_pad(11) << adc2mV(minValueA, range);
			setCursorPosition(x2, 2); std::cout << space_pad(11) << adc2mV(minValueB, range);

			setCursorPosition(x1, 3);  std::cout << space_pad(11) << totalSamples;
			setCursorPosition(x2, 3);  std::cout << space_pad(11) << nValues;

			setCursorPosition(x1, 6); std::cout << space_pad(11) << 0;
			setCursorPosition(x1, 7); std::cout << space_pad(11) << duration.count();
			setCursorPosition(x2, 7); std::cout << space_pad(11) << 1e6 / duration.count() * nValues;

			setCursorPosition(x1, 8); std::cout << space_pad(11) << duration0.count();
			setCursorPosition(x2, 8); std::cout << space_pad(11) << 1e6 / duration0.count() * totalSamples;
			
			setCursorPosition(0, 9); //カーソルの位置を末尾に
			std::cout << std::flush;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10ms 待つ
	}

	// デバイスを停止
	ps2000_stop(handle);

	return 0;
}