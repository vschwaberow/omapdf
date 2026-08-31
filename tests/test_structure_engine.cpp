#include "app/StructureEngine.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>

class StructureEngineTest : public QObject {
  Q_OBJECT

private slots:
  void rotateRelative();
  void deletePage();
  void reorderPages();
  void extractAndMerge();
  void refusesDeleteAll();
  void asyncRotateEmitsFinished();
};

static int pageCountOf(const QString &path) {
  QPDF pdf;
  pdf.processFile(path.toUtf8().constData());
  return static_cast<int>(QPDFPageDocumentHelper(pdf).getAllPages().size());
}

static int pageRotate(const QString &path, int page) {
  QPDF pdf;
  pdf.processFile(path.toUtf8().constData());
  auto pages = QPDFPageDocumentHelper(pdf).getAllPages();
  const auto rot =
      pages.at(static_cast<size_t>(page)).getObjectHandle().getKey("/Rotate");
  if (rot.isNull()) {
    return 0;
  }
  return rot.getIntValueAsInt();
}

void StructureEngineTest::rotateRelative() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("a.pdf"));
  auto made = StructureEngine::writeBlankPdf(path, 2);
  if (!made.has_value())
    QFAIL(qPrintable(made.error()));
  auto rotated = StructureEngine::rotateFile(path, 0, 90);
  if (!rotated.has_value())
    QFAIL(qPrintable(rotated.error()));
  QCOMPARE(pageRotate(path, 0), 90);
  QCOMPARE(pageCountOf(path), 2);
}

void StructureEngineTest::deletePage() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("b.pdf"));
  QVERIFY(StructureEngine::writeBlankPdf(path, 3).has_value());
  auto removed = StructureEngine::removePagesFile(path, {1});
  if (!removed.has_value())
    QFAIL(qPrintable(removed.error()));
  QCOMPARE(pageCountOf(path), 2);
}

void StructureEngineTest::reorderPages() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("c.pdf"));
  QVERIFY(StructureEngine::writeBlankPdf(path, 3).has_value());
  QVERIFY(StructureEngine::rotateFile(path, 0, 90).has_value());
  QVERIFY(StructureEngine::rotateFile(path, 2, 180).has_value());
  auto reordered = StructureEngine::reorderFile(path, {2, 0, 1});
  if (!reordered.has_value())
    QFAIL(qPrintable(reordered.error()));
  QCOMPARE(pageRotate(path, 0), 180);
  QCOMPARE(pageRotate(path, 1), 90);
}

void StructureEngineTest::extractAndMerge() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString a = dir.filePath(QStringLiteral("src.pdf"));
  const QString extracted = dir.filePath(QStringLiteral("ex.pdf"));
  const QString other = dir.filePath(QStringLiteral("other.pdf"));
  const QString merged = dir.filePath(QStringLiteral("merged.pdf"));
  QVERIFY(StructureEngine::writeBlankPdf(a, 3).has_value());
  QVERIFY(StructureEngine::writeBlankPdf(other, 2).has_value());
  auto ex = StructureEngine::extractFile(a, {0, 2}, extracted);
  if (!ex.has_value())
    QFAIL(qPrintable(ex.error()));
  QCOMPARE(pageCountOf(extracted), 2);
  auto m = StructureEngine::mergeFiles({extracted, other}, merged);
  if (!m.has_value())
    QFAIL(qPrintable(m.error()));
  QCOMPARE(pageCountOf(merged), 4);
}

void StructureEngineTest::refusesDeleteAll() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("d.pdf"));
  QVERIFY(StructureEngine::writeBlankPdf(path, 2).has_value());
  auto removed = StructureEngine::removePagesFile(path, {0, 1});
  QVERIFY(!removed.has_value());
  QCOMPARE(pageCountOf(path), 2);
}


void StructureEngineTest::asyncRotateEmitsFinished() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("async.pdf"));
  QVERIFY(StructureEngine::writeBlankPdf(path, 2).has_value());

  StructureEngine engine;
  QSignalSpy spy(&engine, &StructureEngine::opFinished);
  QVERIFY(!engine.busy());
  engine.rotateAsync(path, 0, 90);
  QVERIFY(engine.busy());
  QVERIFY(spy.wait(5000));
  QCOMPARE(spy.count(), 1);
  const QVariantMap result = spy.at(0).at(0).toMap();
  QCOMPARE(result.value(QStringLiteral("ok")).toBool(), true);
  QVERIFY(!engine.busy());
  QCOMPARE(pageRotate(path, 0), 90);
}

QTEST_MAIN(StructureEngineTest)
#include "test_structure_engine.moc"
