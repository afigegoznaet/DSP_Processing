#include "DataProcessor.hpp"
#include "SpectrumAnalyzer.hpp"
#include <QFile>

struct threeBInt {
	int		   val : 24;
	threeBInt &operator=(int inval) {
		val = inval;
		return *this;
	}
}
#ifndef _WIN32
__attribute__((packed))
#endif
;


DataProcessor::DataProcessor(QObject *parent) : QObject(parent) {
	analizer = new SpectrumAnalyzer(this);
	analizer->setCalculateAmplitudes(false);
	connect(this, &DataProcessor::setWindowFunction, [&](int type) {
		analizer->setWindowFunction(WindowFunction(type));
	});
	filter.init(BW_FilterType::LOWPASS, inputFrequency, 1500, 0);
	QFile phaseResp("phase.txt");
	QFile freqResp("freq.txt");
	phaseResp.open(QIODevice::WriteOnly | QIODevice::Truncate);
	freqResp.open(QIODevice::WriteOnly | QIODevice::Truncate);

	for (size_t i = 0; i < filter.getFrequencyResponse().size(); i++) {
		phaseResp.write(QString::asprintf("%f\n", filter.getPhaseResponse()[i])
							.toLocal8Bit());
		freqResp.write(
			QString::asprintf("%f\n", filter.getFrequencyResponse()[i])
				.toLocal8Bit());
	}
	phaseResp.close();
	freqResp.close();
}

void DataProcessor::setSampleRate(int freq) {
	inputFrequency = freq;
	filter.init(BW_FilterType::HIPASS, inputFrequency, 500, 0);
}
void DataProcessor::setBitDepth(int bytes) { bytesPerSample = bytes; }

void DataProcessor::processData(char *data, qint64 dataLen) {
}

std::vector<double> DataProcessor::reinterpret_data(const char *data,
													qint64		dataLen) {
	std::vector<double> doublesVec(dataLen / bytesPerSample, 0);
	long long			divisor = 0;
	switch (bytesPerSample) {
	default:
		divisor = CHAR_MAX;
		break;
	case 2:
		divisor = SHRT_MAX;
		break;
	case 3:
		divisor |= 0x7fffff;
		break;
	case 4:
		divisor = INT_MAX;
		break;
	}

	auto charData = reinterpret_cast<const char *>(data);
	auto shortData = reinterpret_cast<const short *>(data);
	auto threeBInt = reinterpret_cast<const struct threeBInt *>(data);
	auto intData = reinterpret_cast<const int *>(data);

	for (size_t i = 0; i < doublesVec.size(); i++) {
		// qDebug()<<transferredBytes/sampleBitSize<<"_"<<i;
		switch (bytesPerSample) {
		default:
			doublesVec[i] = ((double)charData[i]) / divisor;
			break;
		case 2:
			doublesVec[i] = ((double)shortData[i]) / divisor;
			break;
		case 3:
			doublesVec[i] = ((double)threeBInt[i].val) / divisor;
			break;
		case 4:
			doublesVec[i] = ((double)intData[i]) / divisor;
			break;
		}
	}
	return doublesVec;
}

void DataProcessor::reinterpret_data(std::vector<double> &doublesVec,
									 char *				  data) {

	long long multiple = 0;
	switch (bytesPerSample) {
	default:
		multiple = CHAR_MAX;
		break;
	case 2:
		multiple = SHRT_MAX;
		break;
	case 3:
		multiple |= 0x7fffff;
		break;
	case 4:
		multiple = INT_MAX;
		break;
	}

	auto charData = reinterpret_cast<char *>(data);
	auto shortData = reinterpret_cast<short *>(data);
	auto threeBInt = reinterpret_cast<struct threeBInt *>(data);
	auto intData = reinterpret_cast<int *>(data);

	for (size_t i = 0; i < doublesVec.size(); i++) {
		switch (bytesPerSample) {
		default:
			charData[i] = doublesVec[i] * multiple;
			break;
		case 2:
			shortData[i] = doublesVec[i] * multiple;
			break;
		case 3:
			threeBInt[i] = doublesVec[i] * multiple;
			break;
		case 4:
			intData[i] = doublesVec[i] * multiple;
			break;
		}
	}
}
