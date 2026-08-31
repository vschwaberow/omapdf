#include "app/LocaleTranslator.h"

#include <QFile>
#include <QTest>

class LocaleTranslatorTest : public QObject {
  Q_OBJECT

private slots:
  void translatesGerman();
  void missesUnknown();
};

void LocaleTranslatorTest::translatesGerman() {
  QFile file(QStringLiteral(OMAPDF_SOURCE_DIR "/translations/omapdf_de.json"));
  QVERIFY(file.open(QIODevice::ReadOnly));
  LocaleTranslator tr;
  QVERIFY(tr.loadFromJson(file.readAll()));
  QCOMPARE(tr.translate("", "Save"), QStringLiteral("Speichern"));
  QCOMPARE(tr.translate("", "Cancel"), QStringLiteral("Abbrechen"));
  QCOMPARE(tr.translate("", "Open PDF"), QStringLiteral("PDF öffnen"));
}

void LocaleTranslatorTest::missesUnknown() {
  LocaleTranslator tr;
  QVERIFY(tr.loadFromJson(QByteArrayLiteral("{\"Save\":\"Speichern\"}")));
  QVERIFY(tr.translate("", "___missing___").isEmpty());
}

QTEST_MAIN(LocaleTranslatorTest)
#include "test_locale_translator.moc"
