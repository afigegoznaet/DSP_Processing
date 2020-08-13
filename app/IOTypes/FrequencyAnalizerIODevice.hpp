#ifndef FREQUENCYANALIZERIODEVICE_HPP
#define FREQUENCYANALIZERIODEVICE_HPP

#include "XYSeriesIODevice.hpp"

class SpectrumAnalyzer;
class DataProcessor;
class FrequencyAnalizerIODevice : public XYSeriesIODevice {
	Q_OBJECT
public:
	explicit FrequencyAnalizerIODevice(QXYSeries *	 series,
									   StreamReader *streamReader,
									   int			 size = 4000,
									   QObject *	 parent = nullptr);
signals:
	void calculateSpectrum(std::vector<double> buffer);
	void setWindowFunction(int);
public slots:
	void showData(quint16) override;
	void calculationComplete(const std::vector<double> &spectrum);

private:
	SpectrumAnalyzer *	 analizer;
	std::vector<double>	 buf1;
	std::vector<double>	 buf2;
	std::vector<double> *curBuf;
};

#endif // FREQUENCYANALIZERIODEVICE_HPP
