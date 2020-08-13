#ifndef DATAPROCESSOR_HPP
#define DATAPROCESSOR_HPP
#include <QObject>
#include "Filters.hpp"
class SpectrumAnalyzer;
class DataProcessor : public QObject {
	Q_OBJECT
public:
	explicit DataProcessor(QObject *parent);

	void				setSampleRate(int freq);
	void				setBitDepth(int bytes);
	std::vector<double> reinterpret_data(const char *data, qint64 dataLen);
	void reinterpret_data(std::vector<double> &doublesVec, char *data);
	void processData(char *data, qint64 dataLen);

signals:
	void setWindowFunction(int);

private:
	SpectrumAnalyzer *analizer;
	BW_Filter		  filter;
	int				  bytesPerSample = 2;
	int				  inputFrequency = 48000;
};

#endif // DATAPROCESSOR_HPP
