#include "app/AppController.h"
#include "app/LinkGuard.h"
#include "app/LocaleTranslator.h"
#include "app/SessionStore.h"
#include "app/StructureEngine.h"
#include "app/ThemeBridge.h"

#include "app/DocumentLimits.h"
#include "version.h"

#include <QCommandLineParser>
#include <QApplication>
#include <QGuiApplication>
#include <QLocale>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <cstdlib>

int main(int argc, char *argv[]) {
  qputenv("QT_QUICK_CONTROLS_STYLE", QByteArrayLiteral("Basic"));
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

  QApplication app(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("omapdf"));
  QCoreApplication::setOrganizationName(QStringLiteral("omapdf"));
  QCoreApplication::setApplicationVersion(QStringLiteral(OMAPDF_VERSION));
  QQuickStyle::setStyle(QStringLiteral("Basic"));

  LocaleTranslator translator;
  if (QLocale::system().language() == QLocale::German &&
      translator.loadGerman()) {
    QCoreApplication::installTranslator(&translator);
  }

  QCommandLineParser parser;
  parser.setApplicationDescription(
      QStringLiteral("The fastest, most beautiful PDF reading engine for Linux"));
  parser.addHelpOption();
  parser.addVersionOption();
  QCommandLineOption verboseOpt(QStringLiteral("verbose"),
                                QStringLiteral("Verbose logging to stderr"));
  parser.addOption(verboseOpt);
  parser.addPositionalArgument(QStringLiteral("files"),
                               QStringLiteral("PDF files to open"),
                               QStringLiteral("[files...]"));
  parser.process(app);

  const bool verbose = parser.isSet(verboseOpt);
  if (!verbose) {
    QString rules = QString::fromLocal8Bit(qgetenv("QT_LOGGING_RULES"));
    if (!rules.isEmpty() && !rules.endsWith(QLatin1Char('\n'))) {
      rules += QLatin1Char('\n');
    }
    rules += QStringLiteral("qt.pdf.links.warning=false\n");
    QLoggingCategory::setFilterRules(rules);
  }
  SessionStore store;
  ThemeBridge theme;
  AppController controller(&store, verbose);
  StructureEngine structure;
  LinkGuard linkGuard(&controller);
  controller.setLinkGuard(&linkGuard);
  linkGuard.install();

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("theme"), &theme);
  engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
  engine.rootContext()->setContextProperty(QStringLiteral("structure"), &structure);
  engine.rootContext()->setContextProperty(QStringLiteral("omapdfMaxPageCount"),
                                           omapdf::kMaxPageCount);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);

  engine.loadFromModule(QStringLiteral("Omapdf"), QStringLiteral("Main"));

  const QStringList files = parser.positionalArguments();
  int failed = 0;
  if (!files.isEmpty()) {
    const int before = controller.tabs()->rowCount();
    controller.openPaths(files);
    failed = files.size() - (controller.tabs()->rowCount() - before);
    if (controller.tabs()->rowCount() == 0) {
      return 1;
    }
  }

  const int code = QApplication::exec();
  if (failed > 0 && controller.tabs()->rowCount() == 0) {
    return 1;
  }
  return code;
}
