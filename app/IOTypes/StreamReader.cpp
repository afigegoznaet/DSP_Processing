#include "StreamReader.hpp"
#include <QDebug>
#include <QFile>
#include <QtConcurrent/QtConcurrent>
#include <mutex>
#include "XYSeriesIODevice.hpp"

#include "../FFTStuff/DataProcessor.hpp"
#include "../FFTStuff/SpectrumAnalyzer.hpp"

StreamReader::StreamReader(QObject *parent) : QObject(parent) {
	// stream = new OuterStream(this);
	tmpBuf.resize(SpectrumLengthSamples * bytesPerSample);
	dataProcessor = new DataProcessor(this);
	connect(this, SIGNAL(setWindowFunction(int)), dataProcessor,
			SIGNAL(setWindowFunction(int)));
}

void StreamReader::setSampleRate(int freq) {
	inputFrequency = freq;
	dataProcessor->setSampleRate(freq);
}
void StreamReader::setBitDepth(int bytes) {
	bytesPerSample = bytes;
	dataProcessor->setBitDepth(bytes);

	tmpBuf.resize(SpectrumLengthSamples * bytesPerSample);
	for (const auto &listener : listeners) {
		listener->setData(tmpBuf);
	}
}

void StreamReader::open() {
	dataFile = new QFile(fileName, this);
	dataFile->open(QIODevice::ReadOnly);
	qDebug() << "File size: " << dataFile->size();
}
qint64 StreamReader::size() const { return dataFile->size(); }

void StreamReader::close() {
	dataFile->close();
	memset(tmpBuf.data(), 0, SpectrumLengthSamples * bytesPerSample);
}

void StreamReader::addListener(StreamConsumer *listener) {
	auto ampGraph = qobject_cast<XYSeriesIODevice *>(listener);
	if (ampGraph) {
		connect(this, SIGNAL(transferredBytes(quint16)), ampGraph,
				SLOT(showData(quint16)), Qt::QueuedConnection);
		// return;
	}
	listeners.push_back(listener);
	listener->setData(tmpBuf);
	// qDebug()<<"Data address: "<<&tmpBuf[0];
	connect(listeners[0], SIGNAL(finishedReading()), this, SLOT(readStream()),
			Qt::ConnectionType(Qt::UniqueConnection));
}

void StreamReader::currentFileChanged(const QString &text) { fileName = text; }

void StreamReader::readStream() {

	// std::lock_guard<std::mutex> locker(rwMutex);

	sizeTransferred = dataFile->read(reinterpret_cast<char *>(tmpBuf.data()),
									 SpectrumLengthSamples * bytesPerSample);
	if (dataProcessingEnabled)
		dataProcessor->processData(tmpBuf.data(), sizeTransferred);
	if (dataFile->bytesAvailable() < 2) {
		emit endOfData();
		emit bytesRead(0);
	}

	emit bytesRead(sizeTransferred);

	for (auto listener : listeners) {
		if (listener->readyToRead())
			listener->resetBuffer();
	}
	emit transferredBytes(sizeTransferred);
}
