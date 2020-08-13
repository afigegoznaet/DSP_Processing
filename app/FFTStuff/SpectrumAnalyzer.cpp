#include "SpectrumAnalyzer.hpp"
#include <QtMath>
#include <limits>
#include <float.h>
#include <QFile>

SpectrumAnalyzer::SpectrumAnalyzer(QObject *parent)
	: QObject(parent), numSamples(SpectrumLengthSamples),
	  windowFunction(DefaultWindowFunction),
	  wSquareWindow(SpectrumLengthSamples, 0.0),
	  wHannWindow(SpectrumLengthSamples, 0.0),
	  wHammWindow(SpectrumLengthSamples, 0.0),
	  wBHWindow(SpectrumLengthSamples, 0.0),
	  wNutallWindow(SpectrumLengthSamples, 0.0),
	  wTriangularWindow(SpectrumLengthSamples, 0.0),
	  signal(SpectrumLengthSamples, 0.0), spectrum(SpectrumLengthSamples, 0.0),
	  frequencyAmplitudes(SpectrumLengthSamples / 2, 0.0)
#ifdef SPECTRUANALYSER_SEPARATE_THREAD
	  ,
	  thread(new QThread(this))
#endif
{
#ifdef SPECTRUANALYSER_SEPARATE_THREAD
	// moveToThread() cannot be called on a QObject with a parent
	setParent(0);
	moveToThread(thread);
	thread->start();
#endif
	calculateWindows();
	initWrapper();
}

void SpectrumAnalyzer::initWrapper() {
#ifndef DISABLE_FFT
	fft = new FFTRealWrapper;
#else
	fftPlan = fftw_plan_r2r_1d(SpectrumLengthSamples, signal.data(),
							   spectrum.data(), FFTW_REDFT10, FFTW_ESTIMATE);
	ifftPlan = fftw_plan_r2r_1d(SpectrumLengthSamples, spectrum.data(),
								signal.data(), FFTW_REDFT01, FFTW_ESTIMATE);
#endif
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
#ifndef DISABLE_FFT
	delete fft;
#else
	fftw_destroy_plan(fftPlan);
#endif
}

void SpectrumAnalyzer::setWindowFunction(WindowFunction type) {
	windowFunction = type;
}

void SpectrumAnalyzer::calculateWindows() {
	const float a0 = 0.35875f;
	const float a1 = 0.48829f;
	const float a2 = 0.14128f;
	const float a3 = 0.01168f;

	const float b0 = 0.355768f;
	const float b1 = 0.487396f;
	const float b2 = 0.144232f;
	const float b3 = 0.012604f;

	for (int i = 0; i < numSamples; ++i) {
		wSquareWindow[i] = 1.0;
		wHannWindow[i] = 0.5 * (1 - qCos((2 * M_PI * i) / (numSamples - 1)));
		wHammWindow[i] =
			25.0 / 46.0
			- (1 - 25.0 / 46.0) * qCos((2 * M_PI * i) / (numSamples - 1));


		wBHWindow[i] = a0 - (a1 * cos((2.0f * M_PI * i) / (numSamples - 1)))
					   + (a2 * cos((4.0f * M_PI * i) / (numSamples - 1)))
					   - (a3 * cos((6.0f * M_PI * i) / (numSamples - 1)));

		wNutallWindow[i] = b0 - (b1 * cos((2.0f * M_PI * i) / (numSamples - 1)))
						   + (b2 * cos((4.0f * M_PI * i) / (numSamples - 1)))
						   - (b3 * cos((6.0f * M_PI * i) / (numSamples - 1)));
	}

	double step = 2.0f / numSamples;
	wTriangularWindow[0] = wTriangularWindow[numSamples - 1] = DBL_MIN;
	for (int i = 1; i < numSamples / 2; ++i)
		wTriangularWindow[i] = wTriangularWindow[numSamples - 1 - i] =
			wTriangularWindow[i - 1] + step;
}
const std::vector<double> &SpectrumAnalyzer::getWindowFunction() const {

	switch (windowFunction) {
	default:
		return wSquareWindow;
	case HannWindow:
		return wHannWindow;
	case HammWindow:
		return wHammWindow;
	case BlackmanHarrisWindow:
		return wBHWindow;
	case NutallWindow:
		return wNutallWindow;
	case TriangularWindow:
		return wTriangularWindow;
	}
}
void SpectrumAnalyzer::calculateSpectrum(std::vector<double> buffer) {
	const auto &  window = getWindowFunction();
	const double *ptr = buffer.data();

	for (size_t i = 0; i < buffer.size(); ++i) {
		signal[i] = ptr[i] * window[i];
	}
	for (int i = int(buffer.size()); i < numSamples; ++i) {
		signal[i] = 0;
	}
// Calculate the FFT
#ifndef DISABLE_FFT
	fft->calculateFFT(spectrum.data(), signal.data());
#else
	fftw_execute(fftPlan);
#endif
	if (!calculateAmplitudes)
		return;
	// Analyze output to obtain amplitude and phase for each frequency
	for (int i = 0; i < numSamples / 2; ++i) {

#ifndef DISABLE_FFT
		const qreal real = spectrum[i];
		qreal		imag = spectrum[numSamples / 2 + i];
#else
		const qreal real = spectrum[i];
		qreal		imag = spectrum[numSamples - i];
#endif
		const qreal magnitude = qSqrt(real * real + imag * imag);
		qreal		amplitude = 20 * log10(magnitude);
		frequencyAmplitudes[i] = amplitude;
	}
	// qDebug()<<"Max: "<<maxAmp;
	// qDebug()<<"Min: "<<minAmp;
	emit calculationComplete(frequencyAmplitudes);
}

QFile signalFile("signal.txt");

void SpectrumAnalyzer::calculateSignalFromSpectrum() {
	const auto &window = getWindowFunction();
#ifndef DISABLE_FFT
	fft->calculateIFFT(spectrum.data(), signal.data());
#else
	fftw_execute(ifftPlan);
#endif
	if (0.0 == signal[0])
		return;

	if (writeToFile && !signalFile.isOpen()) {
		signalFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
	}
	for (size_t i = 0; i < signal.size(); i++) {

		signal[i] /= window[i];
		signal[i] /= SpectrumLengthSamples;
		if (writeToFile) {
			signalFile.write(
				QString::asprintf("%f\n", signal[i]).toLocal8Bit());
		}
	}
	if (writeToFile) {
		if (numberOfSpecs--)
			return;
		signalFile.close();
		writeToFile = false;
	}
}
