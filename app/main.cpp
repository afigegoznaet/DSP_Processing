#include "GUIStuff/MainWindow.hpp"
#include <QApplication>

#define ORG_NAME "WizeDSP LTD"
#define APP_NAME "Frequency chart and amplitude history"
#define WINDOW_TITLE APP_NAME

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	QCoreApplication::setOrganizationName(ORG_NAME);
	QCoreApplication::setApplicationName(APP_NAME);

	MainWindow w;

	w.show();

	return a.exec();
}
