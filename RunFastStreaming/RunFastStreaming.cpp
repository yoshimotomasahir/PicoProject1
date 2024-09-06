#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cassert>

#include "ps2000.h"
#include <PicoStaticLib.hpp>
#include "INIReader.h"

uint64_t totalSamples = 0;
uint64_t subSamples = 0;

int16_t rangeA;
int16_t rangeB;

uint32_t bufferLength = 800 * 1000 * 1000;
std::unique_ptr<int16_t[]> bufferA;
std::unique_ptr<int16_t[]> bufferB;

std::string getCurrentTimestamp() {
	auto now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm;
	localtime_s(&now_tm ,&now_c);

	auto duration = now.time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;
	std::stringstream ss;
	ss << std::put_time(&now_tm, "%Y%m%dT%H%M%S") << '.' << std::setfill('0') << std::setw(3) << millis.count();
	return ss.str();
}

bool output_data;
std::string filePrefix;
int files = -1; //出力が完了したファイル数

void  PREF4 ps2000FastStreamingReady(int16_t** overviewBuffers,
	int16_t overflow,
	uint32_t triggeredAt,
	int16_t triggered,
	int16_t auto_stop,
	uint32_t nValues)
{
	assert(overflow == 0);

	uint64_t a = totalSamples % bufferLength;
	uint64_t b = (totalSamples + nValues) % bufferLength;

	if (output_data) {
		if (filePrefix == "") { filePrefix = "PS2000_" + getCurrentTimestamp() + "_"; }
		files = 0;
	}
	if (b > a) {
		memcpy_s(bufferA.get() + a, nValues * sizeof(int16_t), (void*)(overviewBuffers[0] + 0), nValues * sizeof(int16_t));
		memcpy_s(bufferB.get() + a, nValues * sizeof(int16_t), (void*)(overviewBuffers[2] + 0), nValues * sizeof(int16_t));

		if (output_data) {
			std::ofstream ofsA(filePrefix + "channelA_Range" + std::to_string(rangeA) + ".dat", std::ios::binary | std::ios::app);
			ofsA.write(reinterpret_cast<const char*>(bufferA.get() + a), nValues * sizeof(int16_t));
			ofsA.close();

			std::ofstream ofsB(filePrefix + "channelB_Range" + std::to_string(rangeB) + ".dat", std::ios::binary | std::ios::app);
			ofsB.write(reinterpret_cast<const char*>(bufferB.get() + a), nValues * sizeof(int16_t));
			ofsB.close();
		}
	}
	else {
		memcpy_s(bufferA.get() + a, (bufferLength - a) * sizeof(int16_t), (void*)(overviewBuffers[0]), (bufferLength - a) * sizeof(int16_t));
		memcpy_s(bufferB.get() + a, (bufferLength - a) * sizeof(int16_t), (void*)(overviewBuffers[2]), (bufferLength - a) * sizeof(int16_t));

		if (output_data) {
			std::ofstream ofsA1(filePrefix + "channelA_Range" + std::to_string(rangeA) + ".dat", std::ios::binary | std::ios::app);
			ofsA1.write(reinterpret_cast<const char*>(bufferA.get() + a), (bufferLength - a) * sizeof(int16_t));
			ofsA1.close();

			std::ofstream ofsB1(filePrefix + "channelB_Range" + std::to_string(rangeB) + ".dat", std::ios::binary | std::ios::app);
			ofsB1.write(reinterpret_cast<const char*>(bufferB.get() + a), (bufferLength - a) * sizeof(int16_t));
			ofsB1.close();
		}

		memcpy_s(bufferA.get() + 0, b * sizeof(int16_t), (void*)(overviewBuffers[0] + (bufferLength - a)), b * sizeof(int16_t));
		memcpy_s(bufferB.get() + 0, b * sizeof(int16_t), (void*)(overviewBuffers[2] + (bufferLength - a)), b * sizeof(int16_t));

		if (output_data) {
			files++;
			filePrefix = "PS2000_" + getCurrentTimestamp() + "_";

			std::ofstream ofsA2(filePrefix + "channelA_Range" + std::to_string(rangeA) + ".dat", std::ios::binary | std::ios::app);
			ofsA2.write(reinterpret_cast<const char*>(bufferA.get() + a), b * sizeof(int16_t));
			ofsA2.close();

			std::ofstream ofsB2(filePrefix + "channelB_Range" + std::to_string(rangeB) + ".dat", std::ios::binary | std::ios::app);
			ofsB2.write(reinterpret_cast<const char*>(bufferB.get() + a), b * sizeof(int16_t));
			ofsB2.close();
		}
	}
	totalSamples += nValues;
	subSamples = nValues;
}

int getRange(std::string str) {
	if (str == "10mV") {
		return PS2000_10MV;
	}
	else if (str == "20mV") {
		return PS2000_20MV;
	}
	else if (str == "50mV") {
		return PS2000_50MV;
	}
	else if (str == "100mV") {
		return PS2000_100MV;
	}
	else if (str == "200mV") {
		return PS2000_200MV;
	}
	else if (str == "500mV") {
		return PS2000_500MV;
	}
	else if (str == "1V") {
		return PS2000_1V;
	}
	else if (str == "2V") {
		return PS2000_2V;
	}
	else if (str == "5V") {
		return PS2000_5V;
	}
	else if (str == "10V") {
		return PS2000_10V;
	}
	else if (str == "20V") {
		return PS2000_20V;
	}
	else {
		std::cout << "Range not found." << std::endl;
		throw std::exception("Range not found."); 
	}
}

int main() {

	INIReader reader("config.ini");
	if (reader.ParseError() != 0) {
		std::ofstream ofsConfig("config.ini");
		ofsConfig << "[General]" << std::endl;
		ofsConfig << "sample_interval_us = 10" << std::endl;
		ofsConfig << "output_data = false" << std::endl;
		ofsConfig << "samples_per_file = 360000000" << std::endl;
		ofsConfig << "[ChannelA]" << std::endl;
		ofsConfig << "range = 100mV ; 50mV 100mV 200mV 500mV 1V 2V 5V 10V 20V" << std::endl;
		ofsConfig << "coupling = DC ; DC AC" << std::endl;
		ofsConfig << "[ChannelB]" << std::endl;
		ofsConfig << "range = 100mV ; 50mV 100mV 200mV 500mV 1V 2V 5V 10V 20V" << std::endl;
		ofsConfig << "coupling = DC ; DC AC" << std::endl;
	}

	// サンプル間隔を指定
	int sample_interval_us = reader.GetInteger("General", "sample_interval_us", 10);
	output_data = reader.GetBoolean("General", "output_data", false);

	// バッファー長(1ファイルに記録するサンプル数)
	bufferLength = reader.GetInteger("General", "samples_per_file", 360000000);

	// 電圧レンジ設定
	rangeA = getRange(reader.Get("ChannelA", "range", "100mV"));
	rangeB = getRange(reader.Get("ChannelB", "range", "100mV"));

	// カップリング
	int couplingA = reader.Get("ChannelA", "coupling", "DC") == "DC" ? 1 : 0;
	int couplingB = reader.Get("ChannelB", "coupling", "DC") == "DC" ? 1 : 0;


	// 動的にメモリを確保
	bufferA = std::unique_ptr<int16_t[]>(new int16_t[bufferLength]());
	bufferB = std::unique_ptr<int16_t[]>(new int16_t[bufferLength]());

	// デバイスをオープン
	int16_t handle = ps2000_open_unit();
	if (handle <= 0) {
		std::cerr << "デバイスのオープンに失敗しました。" << std::endl;
		return 1;
	}

	const int32_t sampleCount = 10000; // 取得するサンプル数

	// チャンネル設定
	ps2000_set_channel(handle, PS2000_CHANNEL_A, 1, couplingA, rangeA);
	ps2000_set_channel(handle, PS2000_CHANNEL_B, 1, couplingB, rangeB);

	// トリガー(なし)
	ps2000_set_trigger(handle, PS2000_NONE, 0, 0, 0, 0);

	uint32_t overviewBufferSize = 1000 * 1000;
	assert(bufferLength > overviewBufferSize);

	ps2000_run_streaming_ns(handle, sample_interval_us, PS2000_US, sampleCount, 0, 1, overviewBufferSize);

	// 初期表示
	std::cout << "Range        :    +-       [mV]    +-       [mV] \n";
	std::cout << "Max voltage  : A           [mV] B           [mV] \n";
	std::cout << "Min voltage  : A           [mV] B           [mV] \n";
	std::cout << "Samples      :             (tot)            (sub)\n";
	std::cout << "Sample rate  :             [MS/s]                \n";
	std::cout << "Time interval:             [us]             [Hz]\n";
	std::cout << "Start   time :             [us]\n";
	std::cout << "End     time :             [us]             [Hz]\n";
	std::cout << "Elapsed time :             [us]             [Hz]\n";
	std::cout << "Elapsed time :             [min]\n";
	std::cout << "Files*samples:                *                 \n";
	std::cout << std::flush;
	int x1 = 15;
	int x2 = 32;
	setCursorPosition(x1 + 0, 0); std::cout << reader.Get("ChannelA", "coupling", "DC");
	setCursorPosition(x2 + 0, 0); std::cout << reader.Get("ChannelB", "coupling", "DC");
	setCursorPosition(x1 + 6, 0); std::cout << getRange(rangeA);
	setCursorPosition(x2 + 6, 0); std::cout << getRange(rangeB);
	setCursorPosition(x1, 5); std::cout << space_pad(11) << sample_interval_us;
	setCursorPosition(x2, 5); std::cout << space_pad(11) << 1.e6 / sample_interval_us;

	if (!output_data) {
		setCursorPosition(x1, 10); std::cout << "No output data";
	}

	auto start0 = std::chrono::high_resolution_clock::now();
	auto start = std::chrono::high_resolution_clock::now();

	while (true) {
		ps2000_get_streaming_last_values(handle, ps2000FastStreamingReady);

		auto end = std::chrono::high_resolution_clock::now();
		auto duration0 = std::chrono::duration_cast<std::chrono::microseconds>(end - start0);
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		start = std::chrono::high_resolution_clock::now();

		int16_t minValueA, maxValueA, minValueB, maxValueB;
		int32_t minIndexA, maxIndexA, minIndexB, maxIndexB;
		getMaxMin(bufferLength, bufferA.get(), minValueA, minIndexA, maxValueA, maxIndexA);
		getMaxMin(bufferLength, bufferB.get(), minValueB, minIndexB, maxValueB, maxIndexB);

		setCursorPosition(x1, 1); std::cout << space_pad(11) << adc2mV(maxValueA, rangeA);
		setCursorPosition(x2, 1); std::cout << space_pad(11) << adc2mV(maxValueB, rangeB);
		setCursorPosition(x1, 2); std::cout << space_pad(11) << adc2mV(minValueA, rangeA);
		setCursorPosition(x2, 2); std::cout << space_pad(11) << adc2mV(minValueB, rangeB);

		setCursorPosition(x1, 3);  std::cout << space_pad(11) << totalSamples;
		setCursorPosition(x2, 3);  std::cout << space_pad(11) << subSamples;
		setCursorPosition(x1, 4);  std::cout << space_pad(11) << 1.e6 / duration0.count() * totalSamples * 2 / 1e6;

		setCursorPosition(x1, 6); std::cout << space_pad(11) << 0;
		setCursorPosition(x1, 7); std::cout << space_pad(11) << duration.count();
		setCursorPosition(x1, 8); std::cout << space_pad(11) << duration0.count();
		setCursorPosition(x2, 8); std::cout << space_pad(11) << 1e6 / duration0.count() * totalSamples;

		setCursorPosition(x1, 9); std::cout << space_pad(11) << duration0.count() / 1000000. / 60.;

		if (output_data) {
			setCursorPosition(x1, 10); std::cout << space_pad(11) << files;
			setCursorPosition(x2, 10); std::cout << space_pad(11) << bufferLength;
		}

		setCursorPosition(0, 11); //カーソルの位置を末尾に
		std::cout << std::flush;

		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10ms 待つ
	}

	// デバイスを停止
	ps2000_stop(handle);

	return 0;
}