#include "StreamConsumer.hpp"
#include "StreamReader.hpp"

#include "../FFTStuff/DataProcessor.hpp"
#include "../FFTStuff/SpectrumAnalyzer.hpp"


StreamConsumer::StreamConsumer(StreamReader *dataSource, bool base,
							   QObject *parent)
	: RawFile(parent) {
	if (!base)
		dataSource->addListener(this);

	dataProcessor = new DataProcessor(this);
	connect(this, SIGNAL(setWindowFunction(int)), dataProcessor,
			SIGNAL(setWindowFunction(int)));
}

void StreamConsumer::setSampleRate(int freq) {
	inputFrequency = freq;
	dataProcessor->setSampleRate(freq);
}
void StreamConsumer::setBitDepth(int bytes) {
	bytesPerSample = bytes;
	dataProcessor->setBitDepth(bytes);
}


bool StreamConsumer::open(QIODevice::OpenMode flags) {
	stopReading = false;
	setOpenMode(QIODevice::ReadWrite);

	return internalBuffer.open(flags);
}


qint64 StreamConsumer::readData(char *data, qint64 len) {
	if (internalBuffer.pos() == internalBuffer.size())
		emit finishedReading();
	auto sz = internalBuffer.size();
	auto pos = internalBuffer.pos();
	auto res = internalBuffer.read(data, len);
	return res;
}


qint64 StreamConsumer::bytesAvailable() const {
	// qDebug()<<"bytes available";
	return internalBuffer.bytesAvailable();
}

void StreamConsumer::setData(const std::vector<char> &tmpBuf) {
	internalBuffer.close();
	internalBuffer.setData(
		QByteArray::fromRawData(reinterpret_cast<const char *>(&tmpBuf[0]),
								static_cast<int>(tmpBuf.size())));
	auto sz = internalBuffer.size();
	internalBuffer.open(QIODevice::ReadWrite);
	//qDebug() << sz;
}

void StreamConsumer::writeData(const std::vector<char> &tmpBuf) {
	internalBuffer.seek(0);
	internalBuffer.write(reinterpret_cast<const char *>(&tmpBuf[0]),
						 static_cast<int>(tmpBuf.size()));
	internalBuffer.seek(0);
}
