#include <QApplication>
#include <QCommandLineParser>
#include <QWebEngineView>

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication app(argc, argv);
	QString url = "http://www.google.com";
	int width = 800;
	int height = 800;

	QCommandLineParser parser;
	parser.addVersionOption();

	QCommandLineOption urlOption(QStringList() << "u" << "url",
			QCoreApplication::translate("main", "Destination URL"),
			QCoreApplication::translate("main", "url"));
	parser.addOption(urlOption);

	QCommandLineOption widthOption(QStringList() << "w" << "width",
			QCoreApplication::translate("main", "Width"),
			QCoreApplication::translate("main", "width"));
	parser.addOption(widthOption);

	QCommandLineOption heightOption(QStringList() << "h" << "height",
			QCoreApplication::translate("main", "Height"),
			QCoreApplication::translate("main", "height"));
	parser.addOption(heightOption);

	parser.process(app);

	if (parser.isSet(urlOption))
		url = parser.value(urlOption);
	if (parser.isSet(widthOption))
		width = parser.value(widthOption).toInt();
	if (parser.isSet(heightOption))
		height = parser.value(heightOption).toInt();

	QWebEngineView view;
	view.setUrl(QUrl(url));
	view.resize(width, height);
	view.show();

	return app.exec();
}
