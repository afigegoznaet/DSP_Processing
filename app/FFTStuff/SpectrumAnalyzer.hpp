#ifndef FFTREALWRAPPER_HPP
#define FFTREALWRAPPER_HPP
#include <QObject>
#include "../3rdparty/fftreal/FFTRealFixLenParam.h"
#include "../3rdparty/fftreal/fftreal_wrapper.h"

//#define DISABLE_FFT

#ifdef DISABLE_FFT
#include <fftw3.h>
#endif

enum WindowFunction {
	NoWindow,
	HannWindow,
	HammWindow,
	BlackmanHarrisWindow,
	NutallWindow,
	TriangularWindow
};


static constexpr auto DefaultWindowFunction = HannWindow;


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


class SpectrumAnalyzer : public QObject {
	Q_OBJECT


public:
	SpectrumAnalyzer(QObject *parent);
	~SpectrumAnalyzer();

	std::vector<double> &	   getRawSpectrum() { return spectrum; }
	std::vector<double> &	   getSignal() { return signal; }
	const std::vector<double> &getWindowFunction() const;
	void setCalculateAmplitudes(bool mode) { calculateAmplitudes = mode; }

public slots:
	void setWindowFunction(WindowFunction type);
	void calculateSpectrum(std::vector<double> buffer);
	void calculateSignalFromSpectrum();

signals:
	void calculationComplete(const std::vector<double> &spectrum);

protected:
	void		 calculateWindows();
	virtual void initWrapper();

private:
#ifndef DISABLE_FFT
	FFTRealWrapper *fft{};
#else
	fftw_plan fftPlan;
	fftw_plan ifftPlan;
#endif

	const int numSamples;

	WindowFunction windowFunction;
	bool		   calculateAmplitudes = true;

	std::vector<double> wSquareWindow;
	std::vector<double> wHannWindow;
	std::vector<double> wHammWindow;
	std::vector<double> wBHWindow;
	std::vector<double> wNutallWindow;
	std::vector<double> wTriangularWindow;

	std::vector<double> signal;
	std::vector<double> spectrum;
	std::vector<double> frequencyAmplitudes;
	std::atomic_bool	writeToFile{true};
	std::atomic_int		numberOfSpecs = 1;

	QThread *thread;
};

#endif // FFTREALWRAPPER_HPP
