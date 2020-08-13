#-------------------------------------------------
#
# Project created by QtCreator 2018-07-31T23:26:22
#
#-------------------------------------------------
#message("DEPEND" + $$DEPENDPATH)
#message("INCLUDE" + $$INCLUDEPATH)

win32: SONAME=dll
else:  SONAME=so

QT       += charts multimedia core gui widgets datavisualization

TARGET = DSP_Processing

TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

SOURCES += \
    FFTStuff/ColorMap.cpp \
	FFTStuff/DataProcessor.cpp \
	FFTStuff/Filters.cpp \
    FFTStuff/SpectrumAnalyzer.cpp \
	GUIStuff/MainWindow.cpp \
	IOTypes/StreamConsumer.cpp \
	IOTypes/StreamReader.cpp \
	main.cpp \
	IOTypes/FrequencyAnalizerIODevice.cpp \
	IOTypes/WavFile.cpp \
	IOTypes/XYSeriesIODevice.cpp \
	#IOTypes/OuterStream.cpp

HEADERS += \
    FFTStuff/ColorMap.hpp \
	FFTStuff/DataProcessor.hpp \
	FFTStuff/Filters.hpp \
    FFTStuff/SpectrumAnalyzer.hpp \
	GUIStuff/MainWindow.hpp \
	IOTypes/FrequencyAnalizerIODevice.hpp \
	IOTypes/StreamConsumer.hpp \
	IOTypes/StreamReader.hpp \
	IOTypes/WavFile.hpp \
	IOTypes/XYSeriesIODevice.hpp
 	#IOTypes/OuterStream.hpp

FORMS += \
        MainWindow.ui

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

fftreal_dir = ../3rdparty/fftreal


# Dynamic linkage against FFTReal DLL
macx {
    # Link to fftreal framework
	LIBS += -F$${fftreal_dir}
	LIBS += -framework fftreal
} else {
    LIBS += -L..$${DSP_Processing_build_dir}
	LIBS += -lfftreal
}

INCLUDEPATH += $${fftreal_dir}

RESOURCES += \
	fifth_song.qrc \
	fourth_song.qrc \
    noise_analizer.qrc \
	sixth_song.qrc \
	the_other_song.qrc \
	the_song.qrc \
	440.qrc \
	third_song.qrc

#message("INCLUDEPATH" + $$INCLUDEPATH)
#message("LIBS" + $$LIBS)
#message("target" + ..$${debug})


# Deployment

DESTDIR = ..$${DSP_Processing_build_dir}
macx {
    # Relocate fftreal.framework into noise_analizer.app bundle
	framework_dir = ../noise_analizer.app/Contents/Frameworks
	framework_name = fftreal.framework/Versions/1/fftreal
	QMAKE_POST_LINK = \
	    mkdir -p $${framework_dir} &&\
		rm -rf $${framework_dir}/fftreal.framework &&\
		cp -R $${fftreal_dir}/fftreal.framework $${framework_dir} &&\
		install_name_tool -id @executable_path/../Frameworks/$${framework_name} \
		        $${framework_dir}/$${framework_name} &&\
		install_name_tool -change $${framework_name} \
		        @executable_path/../Frameworks/$${framework_name} \
				../noise_analizer.app/Contents/MacOS/noise_analizer
} else {
    linux-g++*: {
	    # Provide relative path from application to fftreal library
		QMAKE_LFLAGS += -Wl,--rpath=\\\$\$ORIGIN
		CONFIG(debug, debug|release) {
			QMAKE_CXXFLAGS+="-fsanitize=address"
			QMAKE_LFLAGS+=" -fsanitize=address"
		}
	}
}


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../fftw/bin/ -lfftw3
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../fftw/bin/ -lfftw3d
else:unix: LIBS += -L$$PWD/../fftw/bin/ -lfftw3

INCLUDEPATH += $$PWD/../fftw/include
DEPENDPATH += $$PWD/../fftw/include



#message("INCLUDEPATH" + $$INCLUDEPATH)
#message("LIBS" + $$LIBS)
