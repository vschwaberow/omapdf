#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QPdfPageNavigator>
#include <QPdfPageRenderer>
#include <QPdfSearchModel>
#include <QPointF>
#include <QSignalSpy>
#include <QSize>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <ranges>
#include <vector>

class ReadingGateTest : public QObject {
  Q_OBJECT

private slots:
  void jumpP95UnderBudget();
  void searchKeystrokeUnderBudget();
  void scrollWindowFullPageRendersInk();
  void neighborWarmupThenFullPageUnderBudget();
  void fullPageRasterMatchesViewerCap();
  void firstPageInkUnderBudget();
};

static bool imageHasInk(const QImage &image) {
  if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
    return false;
  }
  const QImage img = image.convertToFormat(QImage::Format_RGB32);
  int dark = 0;
  constexpr int kStep = 8;
  for (int y = 0; y < img.height(); y += kStep) {
    const auto *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
    for (int x = 0; x < img.width(); x += kStep) {
      const QRgb p = line[x];
      if (qRed(p) < 250 || qGreen(p) < 250 || qBlue(p) < 250) {
        if (++dark >= 8) {
          return true;
        }
      }
    }
  }
  return false;
}

static qint64 p95Micros(std::vector<qint64> samples) {
  std::ranges::sort(samples);
  const auto idx =
      static_cast<size_t>(std::ceil(0.95 * static_cast<double>(samples.size()))) -
      1;
  return samples[std::min(idx, samples.size() - 1)];
}

static bool writeTextPdf(const QString &path, int pageCount,
                         const QByteArray &token) {
  if (pageCount <= 0) {
    return false;
  }

  QByteArray kids;
  for (int i = 0; i < pageCount; ++i) {
    const int pageId = 4 + (i * 2) + 1;
    kids += QByteArray::number(pageId) + " 0 R ";
  }

  QByteArray out;
  std::vector<int> xref;
  auto emitObj = [&](const QByteArray &obj) {
    xref.push_back(out.size());
    out += obj;
    if (!obj.endsWith('\n')) {
      out += '\n';
    }
  };

  out += "%PDF-1.4\n";
  emitObj("1 0 obj<< /Type /Catalog /Pages 2 0 R >>endobj\n");
  emitObj("2 0 obj<< /Type /Pages /Kids [" + kids + "] /Count " +
          QByteArray::number(pageCount) + " >>endobj\n");
  emitObj("3 0 obj<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>endobj\n");

  int nextId = 4;
  for (int i = 0; i < pageCount; ++i) {
    const int contentId = nextId++;
    const int pageId = nextId++;
    const QByteArray stream =
        "BT /F1 12 Tf 72 720 Td (Page " + QByteArray::number(i + 1) + " " +
        token + ") Tj ET";
    emitObj(QByteArray::number(contentId) + " 0 obj<< /Length " +
            QByteArray::number(stream.size()) + " >>stream\n" + stream +
            "\nendstream\nendobj\n");
    emitObj(QByteArray::number(pageId) +
            " 0 obj<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
            "/Resources << /Font << /F1 3 0 R >> >> /Contents " +
            QByteArray::number(contentId) + " 0 R >>endobj\n");
  }

  const int xrefPos = out.size();
  out += "xref\n0 " + QByteArray::number(static_cast<int>(xref.size()) + 1) +
         "\n";
  out += "0000000000 65535 f \n";
  for (int off : xref) {
    out += QByteArray::number(off).rightJustified(10, '0') + " 00000 n \n";
  }
  out += "trailer<< /Size " +
         QByteArray::number(static_cast<int>(xref.size()) + 1) +
         " /Root 1 0 R >>\nstartxref\n" + QByteArray::number(xrefPos) +
         "\n%%EOF\n";

  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  return f.write(out) == out.size();
}

// Matches OmapdfMultiPageView PdfPageImage applySourceSize cap.
static QSize viewerPageRasterSize(QSizeF pagePointSize, qreal renderScale,
                                  qreal dpr) {
  constexpr int kMaxEdge = 4096;
  qreal w = pagePointSize.width() * renderScale * dpr;
  if (w > kMaxEdge) {
    w = kMaxEdge;
  }
  const qreal aspect =
      pagePointSize.height() / qMax(qreal(1), pagePointSize.width());
  const int iw = qMax(1, qRound(w));
  const int ih = qMax(1, qRound(iw * aspect));
  return QSize(iw, ih);
}

void ReadingGateTest::jumpP95UnderBudget() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("gate_200.pdf"));
  QVERIFY(writeTextPdf(path, 200, QByteArrayLiteral("omapdf-gate")));

  QPdfDocument doc;
  QCOMPARE(doc.load(path), QPdfDocument::Error::None);
  QCOMPARE(doc.pageCount(), 200);

  QPdfPageNavigator nav;

  std::vector<qint64> samples;
  samples.reserve(200);
  QElapsedTimer timer;
  for (int page = 0; page < 200; ++page) {
    timer.restart();
    nav.jump(page, QPointF(0, 0), 0);
    const auto size = doc.pagePointSize(page);
    QVERIFY(size.isValid());
    samples.push_back(timer.nsecsElapsed() / 1000);
  }

  const double p95ms = static_cast<double>(p95Micros(samples)) / 1000.0;
  qInfo("BH2 jump p95=%.3f ms (budget 16 ms)", p95ms);
  QVERIFY2(p95ms < 16.0, qPrintable(QStringLiteral("jump p95 %1 ms").arg(p95ms)));
}

void ReadingGateTest::searchKeystrokeUnderBudget() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("gate_200.pdf"));
  QVERIFY(writeTextPdf(path, 200, QByteArrayLiteral("omapdf-gate")));

  QPdfDocument doc;
  QCOMPARE(doc.load(path), QPdfDocument::Error::None);

  QPdfSearchModel search;
  search.setDocument(&doc);

  const QString needle = QStringLiteral("omapdf-gate");
  std::vector<qint64> samples;
  QElapsedTimer timer;
  QString typed;
  for (QChar ch : needle) {
    typed.append(ch);
    timer.restart();
    search.setSearchString(typed);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    samples.push_back(timer.nsecsElapsed() / 1000);
  }

  const double p95ms = static_cast<double>(p95Micros(samples)) / 1000.0;
  qInfo("BH2 search keystroke p95=%.3f ms (budget 50 ms)", p95ms);
  QVERIFY2(p95ms < 50.0,
           qPrintable(QStringLiteral("search keystroke p95 %1 ms").arg(p95ms)));

  QSignalSpy spy(&search, &QPdfSearchModel::countChanged);
  if (search.count() == 0) {
    QVERIFY(spy.wait(10000));
  }
  QVERIFY(search.count() >= 1);
}

void ReadingGateTest::scrollWindowFullPageRendersInk() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("gate_200.pdf"));
  QVERIFY(writeTextPdf(path, 200, QByteArrayLiteral("omapdf-gate")));

  QPdfDocument doc;
  QCOMPARE(doc.load(path), QPdfDocument::Error::None);
  QCOMPARE(doc.pageCount(), 200);

  QPdfPageRenderer renderer;
  renderer.setDocument(&doc);
  renderer.setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);

  constexpr int kWindow = 4;
  constexpr int kStride = 2;
  const QSize pageSize =
      viewerPageRasterSize(doc.pagePointSize(0), 1.0, 1.0);
  std::vector<qint64> windowSamples;
  QElapsedTimer timer;

  for (int startPage = 0; startPage + kWindow <= 200; startPage += kStride) {
    int remaining = kWindow;
    bool imagesOk = true;
    QEventLoop loop;
    const auto conn = QObject::connect(
        &renderer, &QPdfPageRenderer::pageRendered, &loop,
        [&](int, QSize, const QImage &image, QPdfDocumentRenderOptions, quint64) {
          if (!imageHasInk(image)) {
            imagesOk = false;
          }
          if (--remaining == 0) {
            loop.quit();
          }
        });
    timer.restart();
    for (int i = 0; i < kWindow; ++i) {
      renderer.requestPage(startPage + i, pageSize);
    }
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    loop.exec();
    QObject::disconnect(conn);
    QVERIFY2(imagesOk, "blank or empty full-page raster");
    QCOMPARE(remaining, 0);
    windowSamples.push_back(timer.nsecsElapsed() / 1000);
  }

  const double p95ms = static_cast<double>(p95Micros(windowSamples)) / 1000.0;
  qInfo("AZ2 scroll-window full-page render+ink p95=%.3f ms over %zu windows "
        "(4 pages @ %dx%d)",
        p95ms, windowSamples.size(), pageSize.width(), pageSize.height());
  QVERIFY2(p95ms < 1000.0,
           qPrintable(QStringLiteral("scroll window p95 %1 ms").arg(p95ms)));
}

void ReadingGateTest::neighborWarmupThenFullPageUnderBudget() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("gate_200.pdf"));
  QVERIFY(writeTextPdf(path, 200, QByteArrayLiteral("omapdf-gate")));

  QPdfDocument doc;
  QCOMPARE(doc.load(path), QPdfDocument::Error::None);

  QPdfPageRenderer renderer;
  renderer.setDocument(&doc);
  renderer.setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);

  constexpr int kWindow = 4;
  constexpr int kStride = 8;
  const QSize warmSize(360, 480);
  const QSize pageSize =
      viewerPageRasterSize(doc.pagePointSize(0), 1.0, 1.0);
  std::vector<qint64> samples;
  QElapsedTimer timer;

  for (int startPage = 0; startPage + kWindow <= 200; startPage += kStride) {
    int remaining = kWindow;
    QEventLoop warm;
    const auto warmConn = QObject::connect(
        &renderer, &QPdfPageRenderer::pageRendered, &warm,
        [&](int, QSize, const QImage &, QPdfDocumentRenderOptions, quint64) {
          if (--remaining == 0) {
            warm.quit();
          }
        });
    for (int i = 0; i < kWindow; ++i) {
      renderer.requestPage(startPage + i, warmSize);
    }
    QTimer::singleShot(10000, &warm, &QEventLoop::quit);
    warm.exec();
    QObject::disconnect(warmConn);
    QCOMPARE(remaining, 0);

    remaining = kWindow;
    bool imagesOk = true;
    QEventLoop loop;
    const auto conn = QObject::connect(
        &renderer, &QPdfPageRenderer::pageRendered, &loop,
        [&](int, QSize size, const QImage &image, QPdfDocumentRenderOptions,
            quint64) {
          if (size != pageSize) {
            return;
          }
          if (!imageHasInk(image)) {
            imagesOk = false;
          }
          if (--remaining == 0) {
            loop.quit();
          }
        });
    timer.restart();
    for (int i = 0; i < kWindow; ++i) {
      renderer.requestPage(startPage + i, pageSize);
    }
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    loop.exec();
    QObject::disconnect(conn);
    QVERIFY2(imagesOk, "blank full-page after warmup");
    QCOMPARE(remaining, 0);
    samples.push_back(timer.nsecsElapsed() / 1000);
  }

  const double p95ms = static_cast<double>(p95Micros(samples)) / 1000.0;
  qInfo("A1 neighbor-warmup then full-page p95=%.3f ms over %zu windows", p95ms,
        samples.size());
  QVERIFY2(p95ms < 1000.0,
           qPrintable(QStringLiteral("warmup+full page p95 %1 ms").arg(p95ms)));
}

void ReadingGateTest::fullPageRasterMatchesViewerCap() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("gate_cap.pdf"));
  QVERIFY(writeTextPdf(path, 2, QByteArrayLiteral("omapdf-cap")));

  QPdfDocument doc;
  QCOMPARE(doc.load(path), QPdfDocument::Error::None);

  const QSize capped =
      viewerPageRasterSize(QSizeF(3000, 4000), 2.0, 2.0);
  QVERIFY(capped.width() <= 4096);
  QVERIFY(capped.width() > 0);
  QVERIFY(capped.height() > 0);

  QPdfPageRenderer renderer;
  renderer.setDocument(&doc);
  renderer.setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);

  const QSize pageSize =
      viewerPageRasterSize(doc.pagePointSize(0), 1.0, 2.0);
  QVERIFY(pageSize.width() <= 4096);

  bool ok = false;
  QImage page;
  QEventLoop loop;
  const auto conn = QObject::connect(
      &renderer, &QPdfPageRenderer::pageRendered, &loop,
      [&](int, QSize, const QImage &image, QPdfDocumentRenderOptions, quint64) {
        page = image;
        ok = imageHasInk(image);
        loop.quit();
      });
  renderer.requestPage(0, pageSize);
  QTimer::singleShot(10000, &loop, &QEventLoop::quit);
  loop.exec();
  QObject::disconnect(conn);

  QVERIFY2(ok, "blank full-page viewer raster");
  QCOMPARE(page.size(), pageSize);
  qInfo("PdfPageImage-contract raster %dx%d with ink", page.width(),
        page.height());
}


void ReadingGateTest::firstPageInkUnderBudget() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("gate_cold.pdf"));
  QVERIFY(writeTextPdf(path, 20, QByteArrayLiteral("omapdf-cold")));

  QElapsedTimer wall;
  wall.start();

  QPdfDocument doc;
  QCOMPARE(doc.load(path), QPdfDocument::Error::None);

  QPdfPageRenderer renderer;
  renderer.setDocument(&doc);
  renderer.setRenderMode(QPdfPageRenderer::RenderMode::MultiThreaded);

  const QSize pageSize = viewerPageRasterSize(doc.pagePointSize(0), 1.0, 1.0);
  bool ok = false;
  QEventLoop loop;
  const auto conn = QObject::connect(
      &renderer, &QPdfPageRenderer::pageRendered, &loop,
      [&](int, QSize, const QImage &image, QPdfDocumentRenderOptions, quint64) {
        ok = imageHasInk(image);
        loop.quit();
      });
  renderer.requestPage(0, pageSize);
  QTimer::singleShot(10000, &loop, &QEventLoop::quit);
  loop.exec();
  QObject::disconnect(conn);

  const double ms = wall.nsecsElapsed() / 1e6;
  qInfo("Cold first-page ink %.3f ms (budget 500 ms) @ %dx%d", ms,
        pageSize.width(), pageSize.height());
  QVERIFY2(ok, "blank first page");
  QVERIFY2(ms < 500.0, qPrintable(QStringLiteral("cold start %1 ms").arg(ms)));
}

QTEST_MAIN(ReadingGateTest)
#include "test_reading_gate.moc"
