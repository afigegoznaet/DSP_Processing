#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

namespace QtDataVisualization {
	class QSurfaceDataProxy;
	class QSurface3DSeries;
	class Q3DSurface;
}
namespace QtCharts {
	class QLineSeries;
}

namespace Ui {
	class MainWindow;
}

class XYSeriesIODevice;
class QAudioOutput;
class StreamReader;
class StreamConsumer;
class FFTRealWrapper;
class FrequencyAnalizerIODevice;
class ColorMap;
class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void on_sampleRate_currentIndexChanged(int);

	void on_bitDepth_currentIndexChanged(int);

	void on_playSoundCheckBox_toggled(bool checked);

	void on_startStopButton_toggled(bool checked);

	void on_inputStream_currentIndexChanged(int index);
	void changeTheme(int theme);
	void setRainbowGradient();
	void currentFileChanged(const QString &);
	void setColorMap(const std::vector<std::vector<double>> &colorMap);
	void showStats(QString stats);

	void on_logFreqCheckBox_toggled(bool checked);
	void bytesRead(qint64 bytes);

private:
	void setupControls();
	void setupAmplitudeChart();
	void setupFrequencyStats();
	void setupFrequencyChart();
	void setupAudio();
	void setupStreams();
	void loadSettings();
	void saveSettings();
	void fillSqrtSinProxy();

	Ui::MainWindow *					   ui;
	QtCharts::QLineSeries *				   amplitudes;
	QtCharts::QLineSeries *				   frequencies;
	QtDataVisualization::QSurface3DSeries *frequencyStats;
	XYSeriesIODevice *					   amplitudeSeries;
	QAudioOutput *						   audioOutput;
	StreamReader *					   streamReader;

	FrequencyAnalizerIODevice *				frequencySeries;
	StreamConsumer *						audioFile;
	ColorMap *								colorMap;
	QtDataVisualization::QSurfaceDataProxy *sqrtSinProxy;
	QtDataVisualization::Q3DSurface *		graph;
};

#endif // MAINWINDOW_HPP
