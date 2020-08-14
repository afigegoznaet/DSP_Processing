#ifndef COLORMAP_HPP
#define COLORMAP_HPP
#include "../IOTypes/WavFile.hpp"
#include <mutex>
#include <QFutureWatcher>
#include <condition_variable>
class SpectrumAnalyzer;
class DataProcessor;
class RawFile;
class ColorMap : public QObject {
	using s_clock = std::chrono::steady_clock;
	using time_point = std::chrono::time_point<s_clock>;
	Q_OBJECT
public:
	explicit ColorMap(QObject *parent);
	int getSamplingFrequency() { return inputFrequency; }
	int getFileLengthInSamples() { return rawFile->size() / bytesPerSample; }
signals:
	void calculateSpectrum(std::vector<double> buffer);
	void setWindowFunction(int);
	void finished(const std::vector<std::vector<double>> &);
	void averageRuns(QString);
public slots:
	void setSampleRate(int freq);
	void setBitDepth(int bytes);
	void calculationComplete(const std::vector<double> &spectrum);
	void startProcessing();
	void currentFileChanged(const QString &fName);
	void processData(bool flag) { dataProcessingEnabled = flag; }

private:
	void fillFFTBuff(int &pos);
	void parseSlice(int &pos);
	void generateSpectrumMap();
	void calculateAverages();

private:
	bool							 colorMapReady{true};
	bool							 dataProcessingEnabled{false};
	bool							 makeColorMap{true};
	SpectrumAnalyzer *				 analizer;
	DataProcessor *					 dataProcessor;
	std::vector<double>				 buf1;
	std::vector<double>				 buf2;
	std::vector<double> *			 curBuf{};
	QByteArray						 fileData;
	WavFile *						 rawFile{};
	QString							 fileName;
	int								 bytesPerSample = 2;
	int								 inputFrequency = 48000;
	int								 specNum{200};
	long long						 divisor = 0;
	std::vector<std::vector<double>> slicesResult;

	std::mutex			sliceMutex;
	std::mutex			avgMutex;
	std::vector<double> avgBlock;
	QFuture<void>		executor;

	std::condition_variable			 cv;
	std::chrono::time_point<s_clock> startBlock;
	std::chrono::time_point<s_clock> startTotal;
};

#endif // COLORMAP_HPP
