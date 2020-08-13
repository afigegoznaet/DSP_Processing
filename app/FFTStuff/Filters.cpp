#include "Filters.hpp"
#include <cmath>

void BW_Filter::init(BW_FilterType fType, unsigned long samplingRate,
					 double cutoffFreq, double bandWidth, int filtRespLength) {
	this->fType = fType;
	this->samplingRate = samplingRate;
	this->cutoffFreq = cutoffFreq;
	this->bandWidth = bandWidth;
	piDivSr = M_PI / samplingRate;
	twoPiDivSr = 2 * M_PI / samplingRate;
	switch (fType) {
	case (LOWPASS):
		updateLowPass();
		break;
	case (HIPASS):
		updateHighPass();
		break;
	case (BANDPASS):
		updateBandPass();
		break;
	case (BANDREJECT):
		updateBandReject();
		break;
	}
	freqResp.resize(filtRespLength, 0);
	phaseResp.resize(filtRespLength, 0);
	calculateFilterResponse();
}

void BW_Filter::updateLowPass() {
	double C;
	C = 1.0 / tan(piDivSr * cutoffFreq);
	coeffs.a0 = 1.0 / (1 + sqrt(2) * C + (C * C));
	coeffs.a1 = 2.0 * coeffs.a0;
	coeffs.a2 = coeffs.a0;
	coeffs.b1 = 2.0 * coeffs.a0 * (1.0 - (C * C));
	coeffs.b2 = coeffs.a0 * (1.0 - sqrt(2) * C + (C * C));
}

void BW_Filter::updateHighPass() {
	double C;
	C = tan(piDivSr * cutoffFreq);
	coeffs.a0 = 1.0 / (1 + sqrt(2) * C + (C * C));
	coeffs.a1 = -2.0 * coeffs.a0;
	coeffs.a2 = coeffs.a0;
	coeffs.b1 = 2.0 * coeffs.a0 * ((C * C) - 1.0);
	coeffs.b2 = coeffs.a0 * (1.0 - sqrt(2) * C + (C * C));
}

void BW_Filter::updateBandPass() {
	double C, D;

	C = 1.0 / tan(piDivSr * bandWidth);
	D = 2.0 * cos(twoPiDivSr * cutoffFreq);
	coeffs.a0 = 1.0 / (1.0 + C);
	coeffs.a1 = 0.0;
	coeffs.a2 = -coeffs.a0;
	coeffs.b1 = -coeffs.a0 * C * D;
	coeffs.b2 = coeffs.a0 * (C - 1.0);
}
void BW_Filter::updateBandReject() {
	double C, D;

	C = tan(piDivSr * bandWidth);
	D = 2.0 * cos(twoPiDivSr * cutoffFreq);
	coeffs.a0 = 1.0 / (1.0 + C);
	coeffs.a1 = -coeffs.a0 * D;
	;
	coeffs.a2 = coeffs.a0;
	coeffs.b1 = -coeffs.a0 * D;
	coeffs.b2 = coeffs.a0 * (1.0 - C);
}

void BW_Filter::calculateFilterResponse() {

	int sampleRate = freqResp.size();
	for (int i = 0; i < sampleRate; i++) {
		double imagX, imagY, realX, realY;
		imagX = -1
				* (coeffs.a1 * sin(i * M_PI / sampleRate)
				   + coeffs.a2 * sin(2 * i * M_PI / sampleRate));
		realX = (coeffs.a0 + coeffs.a1 * cos(i * M_PI / sampleRate)
				 + coeffs.a2 * cos(2 * i * M_PI / sampleRate));

		imagY = -1
				* (coeffs.b1 * sin(i * M_PI / sampleRate)
				   + coeffs.b2 * sin(2 * i * M_PI / sampleRate));
		realY = (1 + coeffs.b1 * cos(i * M_PI / sampleRate)
				 + coeffs.b2 * cos(2 * i * M_PI / sampleRate));
		phaseResp[i] = atan2(imagX, realX) - atan2(imagY, realY);

		freqResp[i] = sqrt((imagX * imagX + realX * realX)
						   / (imagY * imagY + realY * realY));
	}
}

int BW_Filter::getTick(int input) {
	double output = coeffs.a0 * input + coeffs.a1 * x1 + coeffs.a2 * x2
					- coeffs.b1 * y1 - coeffs.b2 * y2;
	x2 = x1;
	x1 = input;
	y2 = y1;
	y1 = output;
	output += 0.5;
	return (int)output;
}
