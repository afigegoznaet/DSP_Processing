#include "SpectrumAnalyserThread.hpp"
#include <QtMath>
#include <limits>
#include <float.h>
// const int SpectrumLengthSamples = 1024;
SpectrumAnalyserThread::SpectrumAnalyserThread(QObject *parent)
	: QObject(parent)
#ifndef DISABLE_FFT

	  ,
	  m_fft(new FFTRealWrapper)
#endif
	  ,
	  m_numSamples(SpectrumLengthSamples),
	  m_windowFunction(DefaultWindowFunction),
	  m_SquareWindow(SpectrumLengthSamples, 0.0),
	  m_HannWindow(SpectrumLengthSamples, 0.0),
	  m_HammWindow(SpectrumLengthSamples, 0.0),
	  m_BHWindow(SpectrumLengthSamples, 0.0),
	  m_NutallWindow(SpectrumLengthSamples, 0.0),
	  m_TriangularWindow(SpectrumLengthSamples, 0.0),
	  m_input(SpectrumLengthSamples, 0.0), m_output(SpectrumLengthSamples, 0.0),
	  m_spectrum(SpectrumLengthSamples)
#ifdef SPECTRUM_ANALYSER_SEPARATE_THREAD
	  ,
	  m_thread(new QThread(this))
#endif
{
#ifdef SPECTRUM_ANALYSER_SEPARATE_THREAD
	// moveToThread() cannot be called on a QObject with a parent
	setParent(0);
	moveToThread(m_thread);
	m_thread->start();
#endif
	calculateWindows();
#ifdef DISABLE_FFT
	fftPlan = fftw_plan_r2r_1d(SpectrumLengthSamples, in, out, FFTW_REDFT10,
							   FFTW_ESTIMATE);
#endif
}

SpectrumAnalyserThread::~SpectrumAnalyserThread() {
#ifndef DISABLE_FFT
	delete m_fft;
#else
	fftw_destroy_plan(fftPlan);
#endif
}

void SpectrumAnalyserThread::setWindowFunction(WindowFunction type) {
	m_windowFunction = type;
}

void SpectrumAnalyserThread::calculateWindows() {
	const float a0 = 0.35875f;
	const float a1 = 0.48829f;
	const float a2 = 0.14128f;
	const float a3 = 0.01168f;

	const float b0 = 0.355768f;
	const float b1 = 0.487396f;
	const float b2 = 0.144232f;
	const float b3 = 0.012604f;

	for (int i = 0; i < m_numSamples; ++i) {
		m_SquareWindow[i] = 1.0;
		m_HannWindow[i] = 0.5 * (1 - qCos((2 * M_PI * i) / (m_numSamples - 1)));
		m_HammWindow[i] =
			25.0 / 46.0
			- (1 - 25.0 / 46.0) * qCos((2 * M_PI * i) / (m_numSamples - 1));


		m_BHWindow[i] = a0 - (a1 * cos((2.0f * M_PI * i) / (m_numSamples - 1)))
						+ (a2 * cos((4.0f * M_PI * i) / (m_numSamples - 1)))
						- (a3 * cos((6.0f * M_PI * i) / (m_numSamples - 1)));

		m_NutallWindow[i] =
			b0 - (b1 * cos((2.0f * M_PI * i) / (m_numSamples - 1)))
			+ (b2 * cos((4.0f * M_PI * i) / (m_numSamples - 1)))
			- (b3 * cos((6.0f * M_PI * i) / (m_numSamples - 1)));
	}

	double step = 2.0f / m_numSamples;
	m_TriangularWindow[0] = m_TriangularWindow[m_numSamples - 1] = DBL_MIN;
	for (int i = 1; i < m_numSamples / 2; ++i)
		m_TriangularWindow[i] = m_TriangularWindow[m_numSamples - 1 - i] =
			m_TriangularWindow[i - 1] + step;
}

void SpectrumAnalyserThread::calculateSpectrum(QVector<DataType> *buffer) {
	QVector<DataType> m_window;

	switch (m_windowFunction) {
	default:
		m_window = m_SquareWindow;
		break;
	case HannWindow:
		m_window = m_HannWindow;
		break;
	case HammWindow:
		m_window = m_HammWindow;
		break;
	case BlackmanHarrisWindow:
		m_window = m_BHWindow;
		break;
	case NutallWindow:
		m_window = m_NutallWindow;
		break;
	case TriangularWindow:
		m_window = m_TriangularWindow;
		break;
	}
	const double *ptr = buffer->constData();

	for (int i = 0; i < m_numSamples; ++i) {
#ifndef DISABLE_FFT
		m_input[i] = ptr[i] * m_window[i];
#else
		in[i] = ptr[i] * m_window[i];
#endif
	}

// Calculate the FFT
#ifndef DISABLE_FFT
	m_fft->calculateFFT(m_output.data(), m_input.data());
#else
	fftw_execute(fftPlan);
#endif
	// m_fft->calculateFFT(m_output.data(), buffer->data());
	// Analyze output to obtain amplitude and phase for each frequency
	qreal fraction = (m_numSamples / 2);
	for (int i = 0; i < m_numSamples / 2; ++i) {
		// Calculate frequency of this complex sample
		// m_spectrum[i].frequency = qreal(i * inputFrequency) / fraction;
#ifndef DISABLE_FFT
		const qreal real = m_output[i];
		qreal		imag = m_output[m_numSamples / 2 + i];
#else
		const qreal real = out[i];
		qreal		imag = out[m_numSamples / 2 + i];
#endif
		const qreal magnitude = qSqrt(real * real + imag * imag);
		qreal		amplitude = 20 * log10(magnitude / fraction);
		m_spectrum[i] = amplitude;
	}
	// qDebug()<<"Max: "<<maxAmp;
	// qDebug()<<"Min: "<<minAmp;
	emit calculationComplete(m_spectrum);
}
