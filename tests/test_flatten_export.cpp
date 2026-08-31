#include "app/StructureEngine.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>

class FlattenExportTest : public QObject {
  Q_OBJECT

private slots:
  void exportsHighlightAndNote();
};

void FlattenExportTest::exportsHighlightAndNote() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString src = dir.filePath(QStringLiteral("src.pdf"));
  const QString dest = dir.filePath(QStringLiteral("out.pdf"));
  QVERIFY(StructureEngine::writeBlankPdf(src, 2).has_value());

  QVariantMap highlight;
  highlight.insert(QStringLiteral("type"), QStringLiteral("highlight"));
  highlight.insert(QStringLiteral("page"), 0);
  highlight.insert(QStringLiteral("color"), QStringLiteral("#f6c177"));
  highlight.insert(QStringLiteral("text"), QStringLiteral("hello"));
  QVariantList quad;
  quad.push_back(QVariantList{10.0, 20.0});
  quad.push_back(QVariantList{80.0, 20.0});
  quad.push_back(QVariantList{80.0, 30.0});
  quad.push_back(QVariantList{10.0, 30.0});
  highlight.insert(QStringLiteral("quads"), QVariantList{QVariant(quad)});

  QVariantMap note;
  note.insert(QStringLiteral("type"), QStringLiteral("note"));
  note.insert(QStringLiteral("page"), 1);
  note.insert(QStringLiteral("color"), QStringLiteral("#7aa2f7"));
  note.insert(QStringLiteral("text"), QStringLiteral("note"));
  note.insert(QStringLiteral("x"), 40.0);
  note.insert(QStringLiteral("y"), 50.0);

  auto exported =
      StructureEngine::exportAnnotsFile(src, dest, {highlight, note});
  if (!exported.has_value())
    QFAIL(qPrintable(exported.error()));

  QPDF pdf;
  pdf.processFile(dest.toUtf8().constData());
  auto pages = QPDFPageDocumentHelper(pdf).getAllPages();
  QCOMPARE(static_cast<int>(pages.size()), 2);
  QCOMPARE(static_cast<int>(pages[0].getAnnotations("/Highlight").size()), 1);
  QCOMPARE(static_cast<int>(pages[1].getAnnotations("/Text").size()), 1);
}

QTEST_MAIN(FlattenExportTest)
#include "test_flatten_export.moc"
