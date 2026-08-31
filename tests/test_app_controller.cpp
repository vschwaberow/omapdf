#include "app/AppController.h"
#include "app/SessionStore.h"
#include "app/StructureEngine.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

class AppControllerTest : public QObject {
  Q_OBJECT

private slots:
  void multiFileOpensOneTabEach();
  void rejectsNonLocalUrl();
  void rejectsNonPdfPath();
  void rejectsFakePdfExtension();
  void reopensExistingPathActivatesTab();
  void setTabTitleUpdates();
  void recentsOnlyAfterNoteOpened();
};

void AppControllerTest::multiFileOpensOneTabEach() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  AppController app(nullptr, false);

  QStringList paths;
  for (int i = 0; i < 3; ++i) {
    const QString path =
        dir.filePath(QStringLiteral("doc%1.pdf").arg(i));
    QVERIFY(StructureEngine::writeBlankPdf(path, 1).has_value());
    paths.push_back(path);
  }

  app.openPaths(paths);
  QCOMPARE(app.tabs()->rowCount(), 3);
  QCOMPARE(app.currentIndex(), 2);
}

void AppControllerTest::rejectsNonLocalUrl() {
  AppController app(nullptr, false);
  QSignalSpy spy(&app, &AppController::openFailed);
  app.openUrl(QUrl(QStringLiteral("https://example.com/a.pdf")));
  QCOMPARE(app.tabs()->rowCount(), 0);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("only local files"));
}

void AppControllerTest::reopensExistingPathActivatesTab() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  AppController app(nullptr, false);
  const QString a = dir.filePath(QStringLiteral("a.pdf"));
  const QString b = dir.filePath(QStringLiteral("b.pdf"));
  QVERIFY(StructureEngine::writeBlankPdf(a, 1).has_value());
  QVERIFY(StructureEngine::writeBlankPdf(b, 1).has_value());

  app.openPaths({a, b});
  QCOMPARE(app.tabs()->rowCount(), 2);
  QCOMPARE(app.currentIndex(), 1);

  app.openPaths({a});
  QCOMPARE(app.tabs()->rowCount(), 2);
  QCOMPARE(app.currentIndex(), 0);
}


void AppControllerTest::rejectsNonPdfPath() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  AppController app(nullptr, false);
  const QString path = dir.filePath(QStringLiteral("notes.txt"));
  QFile f(path);
  QVERIFY(f.open(QIODevice::WriteOnly));
  f.write("not pdf");
  f.close();

  QSignalSpy spy(&app, &AppController::openFailed);
  app.openPaths({path});
  QCOMPARE(app.tabs()->rowCount(), 0);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("not a PDF"));
}

void AppControllerTest::setTabTitleUpdates() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  AppController app(nullptr, false);
  const QString path = dir.filePath(QStringLiteral("a.pdf"));
  QVERIFY(StructureEngine::writeBlankPdf(path, 1).has_value());
  app.openPaths({path});
  QCOMPARE(app.tabs()->rowCount(), 1);
  app.setTabTitle(0, QStringLiteral("Chapter One"));
  QCOMPARE(app.tabs()->titleAt(0), QStringLiteral("Chapter One"));
}


void AppControllerTest::recentsOnlyAfterNoteOpened() {
  QTemporaryDir home;
  QVERIFY(home.isValid());
  const QByteArray oldHome = qgetenv("HOME");
  qputenv("HOME", home.path().toUtf8());

  SessionStore store;
  AppController app(&store, false);
  const QString path = home.filePath(QStringLiteral("a.pdf"));
  QVERIFY(StructureEngine::writeBlankPdf(path, 1).has_value());

  app.openPaths({path});
  QCOMPARE(app.tabs()->rowCount(), 1);
  QCOMPARE(app.recents().size(), 0);

  app.noteOpened(path);
  QCOMPARE(app.recents().size(), 1);
  QCOMPARE(app.recents().first(), QFileInfo(path).canonicalFilePath());

  if (oldHome.isNull()) {
    qunsetenv("HOME");
  } else {
    qputenv("HOME", oldHome);
  }
}


void AppControllerTest::rejectsFakePdfExtension() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  AppController app(nullptr, false);
  const QString path = dir.filePath(QStringLiteral("fake.pdf"));
  QFile f(path);
  QVERIFY(f.open(QIODevice::WriteOnly));
  f.write("not a real pdf payload");
  f.close();

  QSignalSpy spy(&app, &AppController::openFailed);
  app.openPaths({path});
  QCOMPARE(app.tabs()->rowCount(), 0);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("not a PDF"));
}

QTEST_MAIN(AppControllerTest)
#include "test_app_controller.moc"
