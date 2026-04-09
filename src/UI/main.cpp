#include "../Base/config.h"
#include "../Base/logging.h"
#include "MainWindow.h"
#include <QApplication>          
#include <QTranslator>          
#include <tbb/global_control.h>
#include <windows.h>

int main(int argc, char* argv[])
{
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	// 资源限制
	tbb::global_control tbb_limit(
		tbb::global_control::max_allowed_parallelism,
		8
	);

	Config::getInstance("../settings.ini");
	InitializeGlog(argv);

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	QApplication App(argc, argv);
	QTranslator translator;
	translator.load("C:\\Users\\WTG\\code\\OntheFlySfM\\src\\UI\\Translation_zh_CN.qm");
	App.installTranslator(&translator);

	MainWindow w;
	w.show();

	return App.exec();
}