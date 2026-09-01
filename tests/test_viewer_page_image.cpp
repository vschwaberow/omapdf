#include <QFile>
#include <QTest>

class ViewerPageImageTest : public QObject {
  Q_OBJECT

private slots:
  void multipageViewUsesPdfPageImage();
};

void ViewerPageImageTest::multipageViewUsesPdfPageImage() {
  QFile f(QStringLiteral(OMAPDF_SOURCE_DIR "/qml/viewer/OmapdfMultiPageView.qml"));
  QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
  const QByteArray qml = f.readAll();
  QVERIFY2(qml.contains("PdfPageImage"), "viewer must use Qt PdfPageImage");
  QVERIFY2(!qml.contains("PageTileLayer"),
           "viewer must not use custom PageTileLayer");
  QVERIFY2(qml.contains("sharpenZoom"),
           "viewer must defer sourceSize via idle sharpenZoom timer");
  QVERIFY2(qml.contains("interval: 120"),
           "idle sharpen timer must be 120 ms");
}

QTEST_APPLESS_MAIN(ViewerPageImageTest)
#include "test_viewer_page_image.moc"
