#include "app/AnnotStore.h"

#include <QDir>
#include <QFile>
#include <QPolygonF>
#include <QTemporaryDir>
#include <QTest>

class AnnotStoreTest : public QObject {
  Q_OBJECT

private slots:
  void roundTripHighlight();
  void undoBeforeSave();
  void undoClearedAfterSave();
  void redoAfterUndo();
};

void AnnotStoreTest::roundTripHighlight() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  qputenv("HOME", dir.path().toUtf8());

  const QString pdfPath = dir.filePath(QStringLiteral("doc.pdf"));
  {
    QFile f(pdfPath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("%PDF-1.4 test");
  }

  AnnotStore store;
  store.load(pdfPath, store.contentHash(pdfPath));
  QVERIFY(!store.dirty());

  QPolygonF quad;
  quad << QPointF(10, 20) << QPointF(80, 20) << QPointF(80, 30) << QPointF(10, 30);
  QVariantList geometry;
  geometry.push_back(QVariant::fromValue(quad));
  store.addHighlight(0, QStringLiteral("hello"), geometry);
  QVERIFY(store.dirty());
  QCOMPARE(store.rowCount(), 1);
  QCOMPARE(store.data(store.index(0, 0), AnnotStore::TypeRole).toString(),
           QStringLiteral("highlight"));
  QVERIFY(store.save());
  QVERIFY(!store.dirty());

  AnnotStore loaded;
  loaded.load(pdfPath, loaded.contentHash(pdfPath));
  QCOMPARE(loaded.rowCount(), 1);
  QCOMPARE(loaded.data(loaded.index(0, 0), AnnotStore::TextRole).toString(),
           QStringLiteral("hello"));
  const QVariantList pagePolys = loaded.polygonsOnPage(0);
  QCOMPARE(pagePolys.size(), 1);
}

void AnnotStoreTest::undoBeforeSave() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  qputenv("HOME", dir.path().toUtf8());

  AnnotStore store;
  store.load(dir.filePath(QStringLiteral("a.pdf")), QStringLiteral("abc"));
  QVariantList geometry;
  QPolygonF quad(QRectF(0, 0, 10, 10));
  geometry.push_back(QVariant::fromValue(quad));
  store.addHighlight(1, QStringLiteral("x"), geometry);
  QCOMPARE(store.rowCount(), 1);
  QVERIFY(store.canUndo());
  store.undo();
  QCOMPARE(store.rowCount(), 0);
}

void AnnotStoreTest::undoClearedAfterSave() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  qputenv("HOME", dir.path().toUtf8());

  AnnotStore store;
  store.load(dir.filePath(QStringLiteral("b.pdf")), QStringLiteral("def"));
  QVariantList geometry;
  geometry.push_back(QVariant::fromValue(QPolygonF(QRectF(1, 1, 2, 2))));
  store.addHighlight(0, QStringLiteral("y"), geometry);
  QVERIFY(store.save());
  QVERIFY(!store.canUndo());
}


void AnnotStoreTest::redoAfterUndo() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  qputenv("HOME", dir.path().toUtf8());

  AnnotStore store;
  store.load(dir.filePath(QStringLiteral("c.pdf")), QStringLiteral("ghi"));
  QVariantList geometry;
  geometry.push_back(QVariant::fromValue(QPolygonF(QRectF(0, 0, 5, 5))));
  store.addHighlight(0, QStringLiteral("y"), geometry);
  QCOMPARE(store.rowCount(), 1);
  store.undo();
  QCOMPARE(store.rowCount(), 0);
  QVERIFY(store.canRedo());
  store.redo();
  QCOMPARE(store.rowCount(), 1);
  QCOMPARE(store.data(store.index(0, 0), AnnotStore::TextRole).toString(),
           QStringLiteral("y"));
}

QTEST_MAIN(AnnotStoreTest)
#include "test_annot_store.moc"
