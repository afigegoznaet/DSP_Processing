#ifndef STREAMCONSUMER_HPP
#define STREAMCONSUMER_HPP
#include "WavFile.hpp"

class StreamReader;
class DataProcessor;
class StreamConsumer : public RawFile {
	Q_OBJECT
public:
	using WavFile::open;
	explicit StreamConsumer(StreamReader *dataSource, bool base,
							QObject *parent = nullptr);
	~StreamConsumer() override { setOpenMode(QIODevice::NotOpen); }
	bool seek(qint64) override { return true; }
	bool open(QIODevice::OpenMode flags) override;

	qint64 readData(char *data, qint64 len) override;
	qint64 bytesAvailable() const override;

	void setData(const std::vector<char> &tmpBuf);
	void resetBuffer() { internalBuffer.seek(0); }
	using RawFile::writeData; // shut clang warnings up
	void   writeData(const std::vector<char> &tmpBuf);
	qint64 size() const override { return LLONG_MAX; }
	void   close() override { stopReading = true; }
	bool   readyToRead() const { return deviceReady; }
signals:
	void finishedReading();
	void setWindowFunction(int);
public slots:
	void setSampleRate(int freq) override;
	void setBitDepth(int bytes) override;

protected:
	DataProcessor *	 dataProcessor{nullptr};
	QBuffer			 internalBuffer;
	bool			 stopReading;
	std::atomic_bool deviceReady{true};
	int				 marker = 0;
};

#endif // STREAMCONSUMER_HPP
