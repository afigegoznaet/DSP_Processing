#include "ColorMap.hpp"
#include "IOTypes/WavFile.hpp"
#include <QtConcurrent/QtConcurrent>
#include "DataProcessor.hpp"
#include "SpectrumAnalyzer.hpp"
#include <chrono>
struct threeBInt {
	int val : 24;
}
#ifndef _WIN32
__attribute__((packed))
#endif
;

ColorMap::ColorMap(QObject *parent)
	: QObject(parent), buf1(SpectrumLengthSamples, 0),
	  buf2(SpectrumLengthSamples, 0) {
	dataProcessor = new DataProcessor(this);
	analizer = new SpectrumAnalyzer(this);
	connect(this, SIGNAL(calculateSpectrum(std::vector<double>)), analizer,
			SLOT(calculateSpectrum(std::vector<double>)));
	connect(analizer, SIGNAL(calculationComplete(const std::vector<double> &)),
			this, SLOT(calculationComplete(const std::vector<double> &)));
	connect(this, &ColorMap::setWindowFunction, [&](int type) {
		analizer->setWindowFunction(WindowFunction(type));
	});
	connect(this, SIGNAL(setWindowFunction(int)), dataProcessor,
			SIGNAL(setWindowFunction(int)));
	// connect(&watcher, &QFutureWatcher<void>::finished, this, [this] { emit
	// finished(slicesResult); });
	curBuf = &buf1;
}

void ColorMap::setSampleRate(int freq) {
	inputFrequency = freq;
	dataProcessor->setSampleRate(freq);
}
void ColorMap::setBitDepth(int bytes) {
	bytesPerSample = bytes;
	dataProcessor->setBitDepth(bytes);
}

void ColorMap::fillFFTBuff(int &pos) {
	if (dataProcessingEnabled)
		dataProcessor->processData(fileData.data() + pos * bytesPerSample,
								   SpectrumLengthSamples * bytesPerSample);
	auto doubles =
		dataProcessor->reinterpret_data(fileData.data() + pos * bytesPerSample,
										SpectrumLengthSamples * bytesPerSample);
	pos += SpectrumLengthSamples;

	emit calculateSpectrum(std::move(doubles));
	if (curBuf == &buf1)
		curBuf = &buf2;
	else
		curBuf = &buf1;
}

void ColorMap::parseSlice(int &pos) {
	std::unique_lock locker(sliceMutex);

	startBlock = s_clock::now();
	if (pos >= fileData.size() / bytesPerSample) {
		locker.unlock();
	}
	fillFFTBuff(pos);
	cv.wait_for(locker, std::chrono::seconds(10));
}
void ColorMap::generateSpectrumMap() {
	int pos = 0;
	for (int i = 0; i < specNum && makeColorMap; i++) {
		parseSlice(pos);
	}
	emit finished(slicesResult);
	calculateAverages();
}

void ColorMap::calculateAverages() {
	int avgBlockRun = std::accumulate(avgBlock.begin(), avgBlock.end(), 0)
					  / (avgBlock.size());
	auto totalRun = (s_clock::now() - startTotal).count();
	auto stats = QString::asprintf(
		"Average block processing time: %dns\nTotal running time: %ldns, %fs",
		avgBlockRun, totalRun, double(totalRun) / 1000000000);
	avgMutex.unlock();
	emit averageRuns(stats);
	makeColorMap = true;
}

void ColorMap::startProcessing() {
	specNum = fileData.size() / (bytesPerSample * SpectrumLengthSamples);
	avgMutex.lock();
	startTotal = s_clock::now();
	avgBlock.clear();
	slicesResult.clear();

	generateSpectrumMap();
}

void ColorMap::calculationComplete(const std::vector<double> &spectra) {
	avgBlock.push_back((s_clock::now() - startBlock).count());

	slicesResult.emplace_back(spectra);
	cv.notify_one();
}
void ColorMap::currentFileChanged(const QString &fName) {
	makeColorMap = false;
	avgMutex.lock();
	if (rawFile) {
		rawFile->close();
		rawFile->deleteLater();
	}
	rawFile = new RawFile(this);
	rawFile->setBitDepth(bytesPerSample);
	rawFile->setSampleRate(inputFrequency);
	rawFile->open(fName);
	fileData = QByteArray::fromRawData(
		reinterpret_cast<char *>(rawFile->map(0, rawFile->size())),
		rawFile->size());
	makeColorMap = true;
	avgMutex.unlock();
}
