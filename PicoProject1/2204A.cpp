#include <iostream>
#include <thread>
#include <chrono>
#include "ps2000.h"

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
// �J�[�\�����ړ�
void setCursorPosition(int x, int y) {
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
int main() {

	// �f�o�C�X���I�[�v��
	int16_t handle = ps2000_open_unit();
	if (handle <= 0) {
		std::cerr << "�f�o�C�X�̃I�[�v���Ɏ��s���܂����B" << std::endl;
		return 1;
	}

	// �T���v�����Ɠd�������W�ƃT���v�����O�Ԋu��ݒ�
	const int32_t sampleCount = 1250; // �擾����T���v����
	int16_t oversample = 1;     // �I�[�o�[�T���v�� (??)
	int16_t range = PS2000_100MV; // �d�������W�ݒ�
	int16_t timeUnits = PS2000_NS; // ���ԒP�� (ns)
	int16_t timeBase = 3;
	double timeInterval = std::pow(2, timeBase) * 10; // ns

	// �`�����l���ݒ�
	ps2000_set_channel(handle, PS2000_CHANNEL_A, 1, PS2000_DC_VOLTAGE, range);
	ps2000_set_channel(handle, PS2000_CHANNEL_B, 0, PS2000_DC_VOLTAGE, range);

	// �����\��
	std::cout << "Voltage range: (+/-)       [mV]\n";
	std::cout << "Max voltage  :             [mV]            [us] \n";
	std::cout << "Min voltage  :             [mV]            [us] \n";
	std::cout << "Samples      :              \n";
	std::cout << "Sample rate  :             [MS/s]\n";
	std::cout << "Time interval:             [us]\n";
	std::cout << "Start   time :             [us]\n";
	std::cout << "End     time :             [us]            [Hz]\n";
	std::cout << "Elapsed time :             [us]            [Hz]\n";
	std::cout << std::flush;
	int x1 = 15;
	int x2 = 32;

	// �g���K�[
	// delay: �g�`�擾�J�n�ʒu [%]
	// auto_trigger_ms: �����g���K�[ [ms]
	ps2000_set_trigger(handle, PS2000_CHANNEL_A, mV2adc(-20, range), PS2000_FALLING, -20, 1000);

	// �f�[�^�擾�̃��[�v
	while (true) {
		auto start = std::chrono::high_resolution_clock::now();

		// �f�[�^�擾�J�n
		ps2000_run_block(handle, sampleCount, timeBase, oversample, NULL);

		// �f�[�^�擾�̊�����҂�
		while (ps2000_ready(handle) == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 1ms �҂�
		}

		// �f�[�^�o�b�t�@������
		int32_t times[sampleCount];
		int16_t bufferA[sampleCount];

		// �f�[�^���擾
		ps2000_get_times_and_values(handle, times, bufferA, NULL, NULL, NULL, NULL, timeUnits, sampleCount);


		// �ŏ��l�E�ő�l���擾
		int16_t minValue = bufferA[0];
		int16_t maxValue = bufferA[0];
		int32_t minIndex = 0;
		int32_t maxIndex = 0;
		for (int i = 1; i < sampleCount; ++i) {
			if (bufferA[i] < minValue) {
				minValue = bufferA[i];
				minIndex = i;
			}
			if (bufferA[i] > maxValue) {
				maxValue = bufferA[i];
				maxIndex = i;
			}
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		// �\�����X�V
		int j = 0;
		setCursorPosition(x1 + 6, 0); std::cout << getRange(range);
		j = maxIndex;
		setCursorPosition(x2, 1); std::cout << times[j] / 1000.0;
		setCursorPosition(x1, 1); std::cout << adc2mV(bufferA[j], range);
		j = minIndex;
		setCursorPosition(x2, 2); std::cout << times[j] / 1000.0;
		setCursorPosition(x1, 2); std::cout << adc2mV(bufferA[j], range);
		setCursorPosition(x1, 3); std::cout << sampleCount;
		setCursorPosition(x1, 4); std::cout << 1e9 / timeInterval / 1e6;
		setCursorPosition(x1, 5); std::cout << timeInterval / 1000.0;
		double timeRange = (times[sampleCount - 1] - times[0]); // ns
		setCursorPosition(x1, 6); std::cout << times[0] / 1000.0;
		setCursorPosition(x1, 7); std::cout << times[sampleCount - 1] / 1000.0;
		setCursorPosition(x2, 7); std::cout << 1e9 / timeRange;
		setCursorPosition(x1, 8); std::cout << duration.count();
		setCursorPosition(x2, 8); std::cout << 1e6 / duration.count();
		setCursorPosition(0, 9);
		std::cout << std::flush;
	}

	// �f�o�C�X���N���[�Y
	ps2000_close_unit(handle);

	return 0;
}