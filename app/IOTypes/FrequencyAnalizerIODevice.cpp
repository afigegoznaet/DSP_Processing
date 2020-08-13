#include "FrequencyAnalizerIODevice.hpp"
#include <QtCharts/QXYSeries>
#include "StreamReader.hpp"

#include "../FFTStuff/SpectrumAnalyzer.hpp"
#include "../FFTStuff/DataProcessor.hpp"

FrequencyAnalizerIODevice::FrequencyAnalizerIODevice(QXYSeries *   series,
													 StreamReader *streamReader,
													 int size, QObject *parent)
	: XYSeriesIODevice(series, streamReader, size, true, parent),
	  buf1(SpectrumLengthSamples, 0), buf2(SpectrumLengthSamples, 0) {
	streamReader->addListener(this);
	analizer = new SpectrumAnalyzer(this);
	connect(this, SIGNAL(calculateSpectrum(std::vector<double>)), analizer,
			SLOT(calculateSpectrum(std::vector<double>)));
	connect(analizer, SIGNAL(calculationComplete(const std::vector<double> &)),
			this, SLOT(calculationComplete(const std::vector<double> &)));
	connect(this, &FrequencyAnalizerIODevice::setWindowFunction, [&](int type) {
		analizer->setWindowFunction(WindowFunction(type));
	});
	curBuf = &buf1;
	buffer.resize(SpectrumLengthSamples / 2);
	// buffer.reserve(SpectrumLengthSamples / 2);
	// for (int i = 0; i < SpectrumLengthSamples / 2; ++i)
	// buffer.append(QPointF(i, 0));
}

void FrequencyAnalizerIODevice::showData(quint16 transferredBytes) {
	if (!deviceReady)
		return;

	//qDebug() << internalBuffer.size();
	auto doubles = dataProcessor->reinterpret_data(internalBuffer.data().data(),
												   transferredBytes);

	emit calculateSpectrum(std::move(doubles));
}

void FrequencyAnalizerIODevice::calculationComplete(
	const std::vector<double> &spectrum) {
	if (!deviceReady)
		return;

	for (unsigned long i = 0; i < spectrum.size(); i++) {
		buffer[i].setY(spectrum[i]);
	}

	deviceReady = false;
	series->replace(buffer);
	deviceReady = true;
}
