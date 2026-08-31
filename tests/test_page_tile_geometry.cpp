#include "app/PageTileGeometry.h"

#include <QTest>

class PageTileGeometryTest : public QObject {
  Q_OBJECT

private slots:
  void preferFullPage_typicalReadingPage();
  void preferFullPage_rejectsHugeRaster();
  void scaledPageSize_capsWidthEdge();
};

void PageTileGeometryTest::preferFullPage_typicalReadingPage() {
  QVERIFY(omapdf::page_tile::preferFullPage(QSize(1600, 2200)));
  QVERIFY(omapdf::page_tile::preferFullPage(QSize(4096, 4096)));
}

void PageTileGeometryTest::preferFullPage_rejectsHugeRaster() {
  QVERIFY(!omapdf::page_tile::preferFullPage(QSize()));
  QVERIFY(!omapdf::page_tile::preferFullPage(QSize(9000, 1000)));
  QVERIFY(!omapdf::page_tile::preferFullPage(QSize(5000, 5000)));
}

void PageTileGeometryTest::scaledPageSize_capsWidthEdge() {
  const QSize s = omapdf::page_tile::scaledPageSize(3000, 4000, 2.0);
  QVERIFY(s.width() <= omapdf::kMaxRenderEdgePx);
  QVERIFY(s.width() > 0);
  QVERIFY(s.height() > 0);
}

QTEST_APPLESS_MAIN(PageTileGeometryTest)
#include "test_page_tile_geometry.moc"
