#include "ui_MainWindow.h"
#include "MainWindow.hpp"
#include "IOTypes/XYSeriesIODevice.hpp"
#include "IOTypes/StreamReader.hpp"
#include "IOTypes/FrequencyAnalizerIODevice.hpp"
#include "FFTStuff/SpectrumAnalyzer.hpp"
#include <QDebug>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QSettings>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QLogValueAxis>
#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/Q3DTheme>
#include <QtDataVisualization/QLogValue3DAxisFormatter>
#include <QMessageBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QtMath>
#include <FFTStuff/ColorMap.hpp>
#include <QtConcurrent/QtConcurrent>

const int	sampleCountX = 160;
const int	sampleCountZ = 160;
const float sampleMin = -1000.0f;
const float sampleMax = 1000.0f;

#define X_SAMPLES 1024
#define CHART_POINTS 2048

using namespace QtDataVisualization;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow), amplitudes(new QLineSeries),
	  frequencies(new QLineSeries),
	  /*frequencyStats(new QtDataVisualization::QSurface3DSeries),*/
	  frequencySeries(nullptr), audioFile(nullptr) {


	qRegisterMetaType<std::vector<std::vector<double>>>(
		"std::vector<std::vector<double>>");
	/***
	 * Restore UI
	 * */
	ui->setupUi(this);

	/***
	 * End restore UI
	 * */


	// auto chart1 = ui->frequencyChart->chart();
	// chart1->addSeries(frequencies);
	// auto chart3 = ui->timeAplitudeChart->chart();

	amplitudes->setUseOpenGL(true);
	frequencies->setUseOpenGL(true);
	//	frequencyStats->setUseOpenGL(true);
	// chart3->addSeries(amplitudes);


	ui->verticalSplitter->setStyleSheet("margin-left: 10px");
	ui->horizontalSplitter->setStyleSheet(
		"QSplitter::handle:horizontal { \
										   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, \
											   stop:0 #eee, stop:1 #ccc); \
										   border: 1px solid #777; \
										   width: 5px; \
										   margin-top: 2px; \
										   margin-bottom: 2px; \
										   border-radius: 4px; \
										   }");

	ui->horizontalSplitter->setCollapsible(0, false);
	setupControls();
	setupStreams();

	setupAmplitudeChart();
	setupFrequencyChart();
	setupFrequencyStats();
	// ui->frequencyStatistics->hide();
	loadSettings();
}

void MainWindow::loadSettings() {
	QSettings settings;
	settings.beginGroup("MainWindow");

	resize(settings.value("size", QSize(800, 600)).toSize());
	move(settings.value("pos", QPoint(200, 200)).toPoint());
	QByteArray verticalSplitterSizes =
		settings.value("verticalSplitterSizes").toByteArray();
	QByteArray horizontalSplitterSizes =
		settings.value("horizontalSplitterSizes").toByteArray();
	bool maximized = settings.value("maximized", false).toBool();
	if (maximized)
		setWindowState(Qt::WindowMaximized);
	settings.endGroup();
	if (!ui->verticalSplitter->restoreState(verticalSplitterSizes))
		ui->verticalSplitter->setSizes({400, 400});
	if (!ui->horizontalSplitter->restoreState(horizontalSplitterSizes))
		ui->horizontalSplitter->setSizes({300, 300});
}

MainWindow::~MainWindow() {
	saveSettings();
	delete ui;
}

void MainWindow::setupControls() {
	int		rates[] = {8000, 16000, 32000, 44100, 48000, 96000, 384000};
	int		depths[] = {8, 16, 24, 32};
	QString inputs[] = {
		":/raw_pcm_song1.raw", ":/sounds/440.raw",
		":/sounds/440-square.raw", ":/sounds/440-sawtooth.raw",
		":/sounds/400-800-1600.raw", ":/sounds/400-9500.raw",
		":/sounds/7500.raw", ":/raw_pcm_song2.raw",
		":/raw_pcm_song3.raw", ":/raw_pcm_song4.raw",
		":/raw_pcm_song5.raw", ":/raw_pcm_song6.raw"};

	QString windows[] = {"Square", "Hanning", "Hamming",
						 "Blackman–Harris", "Nutall", "Triangular"};

	for (const auto &item : rates)
		ui->sampleRate->addItem(QString::number(item), item);

	for (const auto &item : depths)
		ui->bitDepth->addItem(QString::number(item), item);

	for (const auto &item : inputs)
		ui->inputStream->addItem(item);

	for (const auto &item : windows)
		ui->windowTypes->addItem(item);
}

void MainWindow::setupAmplitudeChart() {
	auto *chart = ui->timeAplitudeChart->chart();

	QValueAxis *axisX = new QValueAxis;
	QValueAxis *axisY = new QValueAxis;

	axisX->setRange(0, X_SAMPLES);
	axisX->setLabelFormat("%g");
	axisX->setTitleText("Data Points");

	axisY->setRange(-1, 1);
	axisY->setTitleText("Audio level");


	chart->addAxis(axisX, Qt::AlignBottom);
	chart->addAxis(axisY, Qt::AlignLeft);
	chart->addSeries(amplitudes);
	amplitudes->attachAxis(axisX);
	amplitudes->attachAxis(axisY);

	chart->legend()->hide();
	chart->setTitle("Input data");
}


void MainWindow::setupFrequencyChart() {
	auto chart = ui->frequencyChart->chart();
	// chart->addSeries(frequencies);
	QCategoryAxis *axisX = new QCategoryAxis;
	axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
	// axisX->setRange(0,  ui->sampleRate->currentData().toInt() / 2);
	axisX->setRange(0, CHART_POINTS);
	axisX->append("0", 0);
	axisX->append("440", 37);
	axisX->append("6000", 512);
	axisX->append("7500", 640);
	axisX->append("12000", 1024);
	axisX->append("18000", 1536);
	axisX->append("24000", 2048);

	axisX->setLabelFormat("%g");
	axisX->setTitleText("Frequency");

	QCategoryAxis *axisY = new QCategoryAxis;
	axisY->setRange(-120, 72);
	axisY->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
	axisY->append("-120 Db", -120);
	axisY->append("-72 Db", -72);
	axisY->append("-36 Db", -36);
	axisY->append("0 Db", 0);
	axisY->append("36 Db", 36);
	axisY->append("72 Db", 72);
	axisY->setTitleText("Decibels");
	chart->legend()->hide();
	ui->frequencyChart->chart()->setTitle("FFT deduced data");


	chart->addAxis(axisX, Qt::AlignBottom);
	chart->addAxis(axisY, Qt::AlignLeft);
	chart->addSeries(frequencies);
	frequencies->attachAxis(axisX);
	frequencies->attachAxis(axisY);
}


void MainWindow::setupFrequencyStats() {
	graph = new Q3DSurface();
	auto xAxis = new QValue3DAxis;
	auto yAxis = new QValue3DAxis;
	auto zAxis = new QValue3DAxis;
	xAxis->setTitle("X");
	yAxis->setTitle("Y");
	zAxis->setTitle("Z");
	graph->setAxisX(xAxis);
	graph->setAxisY(yAxis);
	graph->setAxisZ(zAxis);
	QWidget *container = QWidget::createWindowContainer(graph);

	sqrtSinProxy = new QSurfaceDataProxy(this);
	frequencyStats = new QSurface3DSeries(sqrtSinProxy);

	if (!graph->hasContext()) {
		QMessageBox msgBox;
		msgBox.setText("Couldn't initialize the OpenGL context.");
		msgBox.exec();
		return;
	}

	QWidget *	 widget = ui->frequencyStatistics;
	QHBoxLayout *hLayout = new QHBoxLayout(widget);
	QVBoxLayout *vLayout = new QVBoxLayout();
	hLayout->addWidget(container, 1);
	hLayout->addLayout(vLayout);
	vLayout->setAlignment(Qt::AlignTop);
	fillSqrtSinProxy();
	graph->addSeries(frequencyStats);
	setRainbowGradient();
}

void MainWindow::on_sampleRate_currentIndexChanged(int) {
	ui->startStopButton->setChecked(false);
	// ui->time->setText(ui->sampleRate->currentText() + ": not yet implemented,
	// using default");
	if (!frequencySeries)
		return;
	frequencySeries->setSampleRate(ui->sampleRate->currentData().toInt());
	amplitudeSeries->setSampleRate(ui->sampleRate->currentData().toInt());
	audioFile->setSampleRate(ui->sampleRate->currentData().toInt());
	colorMap->setSampleRate(ui->sampleRate->currentData().toInt());
	streamReader->setSampleRate(ui->sampleRate->currentData().toInt());

	QCategoryAxis *axisX = new QCategoryAxis;
	axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
	// axisX->setRange(0,  ui->sampleRate->currentData().toInt() / 2);
	axisX->setRange(0, CHART_POINTS);
	axisX->append("0", 0);

	int sampleRate = ui->sampleRate->currentData().toInt();
	qDebug() << QString::number(440 * 4096
								/ ui->sampleRate->currentData().toInt());

	switch (sampleRate) {
	case 8000:
		axisX->append("73", 73 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("440",
					  440 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("1000", 512);
		axisX->append("2000", 1024);
		axisX->append("3000", 1536);
		axisX->append("4000", 2048);
		break;
	case 16000:
		axisX->append("110",
					  110 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("440",
					  440 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("2000", 512);
		axisX->append("4000", 1024);
		axisX->append("6000", 1536);
		axisX->append("8000", 2048);
		[[fallthrough]];
	case 32000:
		axisX->append("440",
					  440 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("4000", 512);
		axisX->append("8000", 1024);
		axisX->append("12000", 1536);
		axisX->append("16000", 2048);
		break;
	case 48000:
		axisX->append("440",
					  440 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("6000", 512);
		axisX->append("7500",
					  7500 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("12000", 1024);
		axisX->append("18000", 1536);
		axisX->append("24000", 2048);
		break;
	case 96000:
		axisX->append("880",
					  880 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("12000", 512);
		axisX->append("19000",
					  9500 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("24000", 1024);
		axisX->append("36000", 1536);
		axisX->append("48000", 2048);
		break;
	case 384000:
		axisX->append("35200",
					  35200 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("48000",
					  48000 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("96000",
					  96000 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("144000",
					  144000 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("192000",
					  192000 * 4096 / ui->sampleRate->currentData().toInt());
		break;
	default: // 44100
		axisX->append("404",
					  404 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("5512", 512);
		axisX->append("9500",
					  9500 * 4096 / ui->sampleRate->currentData().toInt());
		axisX->append("11025", 1024);
		axisX->append("16357", 1536);
		axisX->append("22050", 2048);
	}


	axisX->setLabelFormat("%g");
	axisX->setTitleText("Frequency");

	for (const auto &axis : frequencies->attachedAxes()) {
		if (Qt::AlignBottom == axis->alignment()) {
			frequencies->detachAxis(axis);
			axis->deleteLater();
		}
	}
	ui->frequencyChart->chart()->addAxis(axisX, Qt::AlignBottom);
	frequencies->attachAxis(axisX);
}

void MainWindow::on_bitDepth_currentIndexChanged(int) {
	ui->startStopButton->setChecked(false);
	// ui->time->setText(ui->bitDepth->currentText() + ": not yet implemented,
	// using default");
	if (!frequencySeries)
		return;
	frequencySeries->setBitDepth(ui->bitDepth->currentData().toInt() / 8);
	amplitudeSeries->setBitDepth(ui->bitDepth->currentData().toInt() / 8);
	audioFile->setBitDepth(ui->bitDepth->currentData().toInt() / 8);
	colorMap->setBitDepth(ui->bitDepth->currentData().toInt() / 8);
	streamReader->setBitDepth(ui->bitDepth->currentData().toInt() / 8);
}

void MainWindow::on_playSoundCheckBox_toggled(bool checked) {
	if (!ui->startStopButton->isChecked())
		return;
	if (checked)
		audioOutput->setVolume(.9);
	else
		audioOutput->setVolume(0.000);
}


void MainWindow::on_startStopButton_toggled(bool checked) {
	if (checked) {
		audioFile->open(QIODevice::ReadWrite);
		audioFile->readHeader();
		streamReader->open();
		setupAudio();
		if (!ui->playSoundCheckBox->isChecked())
			audioOutput->setVolume(0.000);
		audioOutput->start(audioFile);
		qDebug() << "Audio buffer: " << audioOutput->bufferSize();
		ui->startStopButton->setText("Stop");
	} else {
		audioOutput->stop();
		audioOutput->reset();
		qDebug() << audioOutput->state();
		ui->startStopButton->setText("Play");
		audioFile->close();
		// m_analysisFile->close();
		streamReader->close();
	}
}

void MainWindow::setupAudio() {

	const auto deviceInfo = QAudioDeviceInfo::defaultOutputDevice();

	QAudioFormat format;
	format.setSampleRate(ui->sampleRate->currentData().toInt());
	format.setChannelCount(1);
	format.setSampleSize(ui->bitDepth->currentData().toInt());
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);

	if (!deviceInfo.isFormatSupported(format)) {
		qWarning() << "Default format not supported - trying to use nearest";
		format = deviceInfo.nearestFormat(format);
	}

	audioOutput = new QAudioOutput(deviceInfo, format, this);
	// m_audioOutput->setBufferSize(24000);
}

void MainWindow::bytesRead(qint64 bytes) {
	if (!bytes)
		ui->timeLine->setText("0.0");

	auto time = double(bytes)
				/ (ui->sampleRate->currentText().toInt()
				   * ui->bitDepth->currentText().toInt() / 8);
	ui->timeLine->setText(QString::number(time));
}
void MainWindow::setupStreams() {

	streamReader = new StreamReader(this);
	connect(
		streamReader, &StreamReader::endOfData, this,
		[&] { ui->startStopButton->setChecked(false); }, Qt::QueuedConnection);
	audioFile = new StreamConsumer(streamReader, false, this);

	connect(streamReader, SIGNAL(bytesRead(qint64)), this,
			SLOT(bytesRead(qint64)), Qt::QueuedConnection);
	connect(ui->inputStream, SIGNAL(currentIndexChanged(const QString &)),
			streamReader, SLOT(currentFileChanged(const QString &)));

	connect(ui->inputStream, SIGNAL(currentIndexChanged(const QString &)),
			streamReader, SLOT(currentFileChanged(const QString &)));
	connect(ui->inputStream, SIGNAL(currentIndexChanged(const QString &)), this,
			SLOT(currentFileChanged(const QString &)));
	colorMap = new ColorMap(this);
	connect(ui->inputStream, SIGNAL(currentIndexChanged(const QString &)),
			colorMap, SLOT(currentFileChanged(const QString &)));
	connect(colorMap,
			SIGNAL(finished(const std::vector<std::vector<double>> &)), this,
			SLOT(setColorMap(const std::vector<std::vector<double>> &)),
			Qt::QueuedConnection);

	connect(colorMap, SIGNAL(averageRuns(QString)), this,
			SLOT(showStats(QString)));
	connect(
		ui->colorMapStart, &QPushButton::clicked, colorMap, [this] {
			ui->colorMapStart->setEnabled(false);
			QtConcurrent::run(colorMap, &ColorMap::startProcessing);
		},
		Qt::QueuedConnection);


	amplitudeSeries =
		new XYSeriesIODevice(amplitudes, streamReader, X_SAMPLES, false, this);
	frequencySeries =
		new FrequencyAnalizerIODevice(frequencies, streamReader, 4096, this);
	connect(ui->windowTypes, SIGNAL(currentIndexChanged(int)), frequencySeries,
			SIGNAL(setWindowFunction(int)));
	connect(ui->windowTypes, SIGNAL(currentIndexChanged(int)), colorMap,
			SIGNAL(setWindowFunction(int)));
	connect(ui->windowTypes, SIGNAL(currentIndexChanged(int)), streamReader,
			SIGNAL(setWindowFunction(int)));

	connect(ui->processDataCheckBox, SIGNAL(toggled(bool)), colorMap,
			SLOT(processData(bool)));
	connect(ui->processDataCheckBox, SIGNAL(toggled(bool)), streamReader,
			SLOT(processData(bool)));

	amplitudeSeries->open(QIODevice::ReadWrite);
	frequencySeries->open(QIODevice::ReadWrite);

	ui->inputStream->setCurrentIndex(1);
	ui->windowTypes->setCurrentIndex(1);
	ui->sampleRate->setCurrentIndex(4);
	ui->bitDepth->setCurrentIndex(1);
	// initialize();
}

void MainWindow::saveSettings() {
	QSettings settings;

	settings.beginGroup("MainWindow");
	settings.setValue("size", size());
	settings.setValue("pos", pos());
	settings.setValue("verticalSplitterSizes",
					  ui->verticalSplitter->saveState());
	settings.setValue("horizontalSplitterSizes",
					  ui->horizontalSplitter->saveState());
	settings.setValue("maximized", isMaximized());
	settings.endGroup();
}


void MainWindow::on_inputStream_currentIndexChanged(int index) {
	ui->startStopButton->setChecked(false);
	ui->sampleRate->setCurrentIndex(4);

	if (0 == index)
		ui->bitDepth->setCurrentIndex(3);
	else
		ui->bitDepth->setCurrentIndex(1);

	if (index > 6)
		ui->sampleRate->setCurrentIndex(3);
}

void MainWindow::fillSqrtSinProxy() {
	float stepX = (sampleMax - sampleMin) / float(sampleCountX - 1);
	float stepZ = (sampleMax - sampleMin) / float(sampleCountZ - 1);

	QSurfaceDataArray *dataArray = new QSurfaceDataArray;
	dataArray->reserve(sampleCountZ);
	for (int i = 0; i < sampleCountZ; i++) {
		QSurfaceDataRow *newRow = new QSurfaceDataRow(sampleCountX);
		// Keep values within range bounds, since just adding step can cause
		// minor drift due to the rounding errors.
		float z = qMin(sampleMax, (i * stepZ + sampleMin));
		int	  index = 0;
		for (int j = 0; j < sampleCountX; j++) {
			float x = qMin(sampleMax, (j * stepX + sampleMin));
			float y = x * x - z * z;
			(*newRow)[index++].setPosition(QVector3D(x, y, z));
		}
		*dataArray << newRow;
	}
	frequencyStats->setDrawMode(QSurface3DSeries::DrawSurfaceAndWireframe);
	frequencyStats->setFlatShadingEnabled(true);
	sqrtSinProxy->resetArray(dataArray);
}

void MainWindow::changeTheme(int theme) {
	graph->activeTheme()->setType(Q3DTheme::Theme(theme));
}

void MainWindow::setRainbowGradient() {

	QLinearGradient gr;
	gr.setColorAt(0.0, Qt::darkBlue);
	gr.setColorAt(1 / 6., Qt::blue);
	gr.setColorAt(2 / 6., Qt::cyan);
	gr.setColorAt(3 / 6., Qt::green);
	gr.setColorAt(4 / 6., Qt::yellow);
	gr.setColorAt(5 / 6., {255, 165, 0});
	gr.setColorAt(1.0, Qt::red);

	graph->seriesList().at(0)->setBaseGradient(gr);
	graph->seriesList().at(0)->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
}

void MainWindow::currentFileChanged(const QString &file) { qDebug() << file; }

void MainWindow::setColorMap(
	const std::vector<std::vector<double>> &colorMapVec) {

	ui->colorMapStart->setEnabled(true);
	graph->axisX()->setFormatter(new QLogValue3DAxisFormatter(this));

	QSurfaceDataArray *dataArray = new QSurfaceDataArray;
	dataArray->reserve(colorMapVec.size());
	for (size_t i = 0; i < colorMapVec.size(); i++) {
		QSurfaceDataRow *newRow = new QSurfaceDataRow(colorMapVec[i].size());

		// qDebug() << colorMapVec[i].size() / colorMapVec.size();
		float z = (double(i) * SpectrumLengthSamples)
				  / ui->sampleRate->currentText().toInt();
		int index = 0;
		for (ulong j = 0; j < colorMapVec[i].size(); j++) {
			float x = j * ui->sampleRate->currentText().toInt()
					  / SpectrumLengthSamples;
			float y = colorMapVec[i][j];
			(*newRow)[index++].setPosition(QVector3D(x, y, z));
		}
		*dataArray << newRow;
	}
	qDebug() << "Max Z:"
			 << (double(colorMapVec.size()) * SpectrumLengthSamples)
					/ ui->sampleRate->currentText().toInt();


	auto proxy = new QSurfaceDataProxy(this);
	auto fff = new QSurface3DSeries(proxy);
	proxy->resetArray(dataArray);
	fff->setDrawMode(QSurface3DSeries::DrawSurface);
	fff->setFlatShadingEnabled(true);

	graph->removeSeries(frequencyStats);
	graph->addSeries(fff);

	// auto timeAxis = graph->axisZ();

	// auto formatter = new AxisFormatter(this);
	// formatter->setSpectrumRate(ui->sampleRate->currentData().toInt()/
	// double(SpectrumLengthSamples));

	// timeAxis->setFormatter(formatter);
	if (!ui->logFreqCheckBox->isChecked())
		graph->axisX()->setFormatter(new QValue3DAxisFormatter(this));

	frequencyStats->deleteLater();
	sqrtSinProxy->deleteLater();
	sqrtSinProxy = proxy;
	frequencyStats = fff;
	setRainbowGradient();
	graph->setHorizontalAspectRatio(.75);
}

void MainWindow::showStats(QString stats) {
	qDebug() << stats;
	ui->statsLabel->setText(stats);
}

void MainWindow::on_logFreqCheckBox_toggled(bool checked) {
	auto frequencyAxis = graph->axisX();
	if (checked)
		frequencyAxis->setFormatter(new QLogValue3DAxisFormatter(this));
	else
		frequencyAxis->setFormatter(new QValue3DAxisFormatter(this));
	setRainbowGradient();
	qDebug() << "Max X: " << frequencyAxis->max();
}
