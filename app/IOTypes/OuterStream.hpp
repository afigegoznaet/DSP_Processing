#ifndef OUTERSTREAM_HPP
#define OUTERSTREAM_HPP

#include <QtCore/QIODevice>

#include "FT2StreamReader.hpp"


class OuterStream : public QIODevice
{
	Q_OBJECT
public:
	//explicit XYSeriesIODevice(QXYSeries *series, int size = 2000, QObject *parent = nullptr);
	explicit OuterStream(QObject *parent = nullptr);
	bool open(OpenMode mode) override;
	void close() override;

	quint16 readExternalData(quint8 *data, quint16 maxlen);
protected:

	qint64 readData(char *data, qint64 maxSize) override{
		return readExternalData(reinterpret_cast<quint8*>(data), static_cast<quint16>(maxSize));
	}

	qint64 writeData(const char *, qint64) override{return 0;}

};


#endif // OUTERSTREAM_HPP
