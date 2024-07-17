#include <iostream>
#include <thread>
#include <chrono>
#include "ps2000.h"
#include <cassert>

#include <PicoStaticLib.hpp>


uint64_t totalSamples = 0;
uint64_t subSamples = 0;

const uint32_t bufferLength = 1000 * 100;
int16_t bufferA[bufferLength] = { 0 };
int16_t bufferB[bufferLength] = { 0 };

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

	if (b > a) {
		memcpy_s((void*)(bufferA + a), nValues * sizeof(int16_t), (void*)(overviewBuffers[0] + 0), nValues * sizeof(int16_t));
		memcpy_s((void*)(bufferB + a), nValues * sizeof(int16_t), (void*)(overviewBuffers[2] + 0), nValues * sizeof(int16_t));
	}
	else {
		memcpy_s((void*)(bufferA + a), (bufferLength - a) * sizeof(int16_t), (void*)(overviewBuffers[0]), (bufferLength - a) * sizeof(int16_t));
		memcpy_s((void*)(bufferB + a), (bufferLength - a) * sizeof(int16_t), (void*)(overviewBuffers[2]), (bufferLength - a) * sizeof(int16_t));

		memcpy_s((void*)(bufferA + 0), b * sizeof(int16_t), (void*)(overviewBuffers[0] + (bufferLength - a)), b * sizeof(int16_t));
		memcpy_s((void*)(bufferB + 0), b * sizeof(int16_t), (void*)(overviewBuffers[2] + (bufferLength - a)), b * sizeof(int16_t));
	}
	totalSamples += nValues;
	subSamples = nValues;
}

int main() {

	// �f�o�C�X���I�[�v��
	int16_t handle = ps2000_open_unit();
	if (handle <= 0) {
		std::cerr << "�f�o�C�X�̃I�[�v���Ɏ��s���܂����B" << std::endl;
		return 1;
	}

	// �T���v�����Ɠd�������W�ƃT���v�����O�Ԋu��ݒ�
	const int32_t sampleCount = 10000; // �擾����T���v����
	int16_t range = PS2000_100MV; // �d�������W�ݒ�

	// �`�����l���ݒ�
	ps2000_set_channel(handle, PS2000_CHANNEL_A, 1, PS2000_DC_VOLTAGE, range);
	ps2000_set_channel(handle, PS2000_CHANNEL_B, 1, PS2000_DC_VOLTAGE, range);

	// �g���K�[(�Ȃ�)
	ps2000_set_trigger(handle, PS2000_NONE, 0, 0, 0, 0);

	// �T���v���Ԋu���w��
	int16_t  sample_interval_us = 10;
	ps2000_run_streaming_ns(handle, sample_interval_us, PS2000_US, sampleCount, 0, 1, bufferLength);

	// �����\��
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
	setCursorPosition(x1, 5); std::cout << space_pad(11) << sample_interval_us;
	setCursorPosition(x2, 5); std::cout << space_pad(11) << 1.e6 / sample_interval_us;

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
		getMaxMin(bufferLength, bufferA, minValueA, minIndexA, maxValueA, maxIndexA);
		getMaxMin(bufferLength, bufferB, minValueB, minIndexB, maxValueB, maxIndexB);

		setCursorPosition(x1, 1); std::cout << space_pad(11) << adc2mV(maxValueA, range);
		setCursorPosition(x2, 1); std::cout << space_pad(11) << adc2mV(maxValueB, range);
		setCursorPosition(x1, 2); std::cout << space_pad(11) << adc2mV(minValueA, range);
		setCursorPosition(x2, 2); std::cout << space_pad(11) << adc2mV(minValueB, range);

		setCursorPosition(x1, 3);  std::cout << space_pad(11) << totalSamples;
		setCursorPosition(x2, 3);  std::cout << space_pad(11) << subSamples;
		setCursorPosition(x1, 4);  std::cout << space_pad(11) << 1.e6 / duration0.count() * totalSamples * 2 / 1e6;

		setCursorPosition(x1, 6); std::cout << space_pad(11) << 0;
		setCursorPosition(x1, 7); std::cout << space_pad(11) << duration.count();
		setCursorPosition(x1, 8); std::cout << space_pad(11) << duration0.count();
		setCursorPosition(x2, 8); std::cout << space_pad(11) << 1e6 / duration0.count() * totalSamples;

		setCursorPosition(0, 9); //�J�[�\���̈ʒu�𖖔���
		std::cout << std::flush;

		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10ms �҂�
	}

	// �f�o�C�X���~
	ps2000_stop(handle);

	return 0;
}