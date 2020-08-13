#ifndef FT2STREAMREADER_HPP
#define FT2STREAMREADER_HPP

#include <QBuffer>
#include "WavFile.hpp"
#include <vector>
#include <QFuture>

class DataProcessor;
class QFile;
class OuterStream;
class StreamConsumer;
class StreamReader : public QObject {
	Q_OBJECT
public:
	explicit StreamReader(QObject *parent = nullptr);
	void   addListener(StreamConsumer *listener);
	void   open();
	void   close();
	qint64 size() const;

signals:
	void transferredBytes(quint16);
	void endOfData();
	void bytesRead(qint64);
	void setWindowFunction(int);

public slots:
	void processData(bool flag) { dataProcessingEnabled = flag; }
	void setSampleRate(int freq);
	void setBitDepth(int bytes);
	void readStream();
	void currentFileChanged(const QString &text);

private:
	DataProcessor *				  dataProcessor;
	std::vector<char>			  tmpBuf;
	std::vector<StreamConsumer *> listeners;
	QFuture<void>				  streamReader;
	QFile *						  dataFile{};
	// OuterStream *					 stream;
	QString fileName;

	qint64 rxSize{};
	qint64 sizeTransferred{};

	int	 bytesPerSample = 2;
	int	 inputFrequency = 48000;
	bool dataProcessingEnabled{false};
};

#endif // FT2STREAMREADER_HPP
