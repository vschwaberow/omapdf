#include "app/SessionStore.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class SessionStoreTest : public QObject {
  Q_OBJECT

private slots:
  void init();
  void cleanup();
  void recentsCapAtTwenty();
  void documentStateRoundTrip();
  void annotColorRoundTrip();

private:
  QTemporaryDir m_home;
  QByteArray m_oldHome;
};

void SessionStoreTest::init() {
  QVERIFY(m_home.isValid());
  m_oldHome = qgetenv("HOME");
  qputenv("HOME", m_home.path().toUtf8());
}

void SessionStoreTest::cleanup() {
  if (m_oldHome.isNull()) {
    qunsetenv("HOME");
  } else {
    qputenv("HOME", m_oldHome);
  }
}

void SessionStoreTest::recentsCapAtTwenty() {
  SessionStore store;
  for (int i = 0; i < 25; ++i) {
    const QString path = m_home.filePath(QStringLiteral("f%1.pdf").arg(i));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("%PDF-1.4\n");
    f.close();
    store.pushRecent(path);
  }
  const QStringList recents = store.recents();
  QCOMPARE(recents.size(), 20);
  QCOMPARE(recents.first(), m_home.filePath(QStringLiteral("f24.pdf")));
  QVERIFY(!recents.contains(m_home.filePath(QStringLiteral("f0.pdf"))));
}

void SessionStoreTest::documentStateRoundTrip() {
  SessionStore store;
  const QString path = m_home.filePath(QStringLiteral("doc.pdf"));
  store.saveDocumentState(path, 1.5, 0, 420.5, true);
  const QVariantMap st = store.documentState(path);
  QCOMPARE(st.value(QStringLiteral("zoom")).toDouble(), 1.5);
  QCOMPARE(st.value(QStringLiteral("page")).toInt(), 0);
  QCOMPARE(st.value(QStringLiteral("scrollY")).toDouble(), 420.5);
  QCOMPARE(st.value(QStringLiteral("dimmed")).toBool(), true);
}

void SessionStoreTest::annotColorRoundTrip() {
  SessionStore store;
  QCOMPARE(store.annotColor(), QStringLiteral("#f6c177"));
  store.setAnnotColor(QStringLiteral("#7aa2f7"));
  QCOMPARE(store.annotColor(), QStringLiteral("#7aa2f7"));
  store.setAnnotColor(QStringLiteral("nope"));
  QCOMPARE(store.annotColor(), QStringLiteral("#7aa2f7"));
}

QTEST_MAIN(SessionStoreTest)
#include "test_session_store.moc"
