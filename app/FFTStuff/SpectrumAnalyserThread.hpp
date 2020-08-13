#ifndef FFTREALWRAPPER_HPP
#define FFTREALWRAPPER_HPP
#include <QObject>
#include "../3rdparty/fftreal/FFTRealFixLenParam.h"
#include "../3rdparty/fftreal/fftreal_wrapper.h"

//#define DISABLE_FFT
enum WindowFunction {
	NoWindow,
	HannWindow,
	HammWindow,
	BlackmanHarrisWindow,
	NutallWindow,
	TriangularWindow
};


static constexpr auto DefaultWindowFunction = HannWindow;


using DataType = FFTRealFixLenParam::DataType;

template <int N>
class PowerOfTwo {
public:
	static const int Result = PowerOfTwo<N - 1>::Result * 2;
};


template <>
class PowerOfTwo<0> {
public:
	static const int Result = 1;
};
constexpr int SpectrumLengthSamples = PowerOfTwo<FFTLengthPowerOfTwo>::Result;

class FFTRealWrapper;


class SpectrumAnalyserThread : public QObject {
	Q_OBJECT


public:
	SpectrumAnalyserThread(QObject *parent);
	~SpectrumAnalyserThread();

public slots:
	void setWindowFunction(WindowFunction type);
	void calculateSpectrum(QVector<DataType> *buffer);

signals:
	void calculationComplete(const std::vector<double> &spectrum);

private:
	void calculateWindows();

private:
#ifndef DISABLE_FFT
	FFTRealWrapper *m_fft;
#else
	fftw_plan fftPlan;
	double	  in[SpectrumLengthSamples];
	double	  out[SpectrumLengthSamples];
#endif

	const int m_numSamples;

	WindowFunction m_windowFunction;


	QVector<DataType>	m_SquareWindow;
	QVector<DataType>	m_HannWindow;
	QVector<DataType>	m_HammWindow;
	QVector<DataType>	m_BHWindow;
	QVector<DataType>	m_NutallWindow;
	QVector<DataType>	m_TriangularWindow;
	QVector<DataType>	m_input;
	QVector<DataType>	m_output;
	std::vector<double> m_spectrum;

	QThread *m_thread;
};

#endif // FFTREALWRAPPER_HPP
