#include "app/ThemeBridge.h"

#include <QTest>

class ThemeBridgeTest : public QObject {
  Q_OBJECT

private slots:
  void parsesQuotedColors();
  void parsesBareShellKeys();
  void parsesControlAlphas();
  void withAlphaClamps();
  void rejectsEmpty();
};

void ThemeBridgeTest::parsesQuotedColors() {
  const QString toml = R"(
mode = "dark"
accent = "#7aa2f7"
background = "#1a1b26"
foreground = "#a9b1d6"
)";
  const auto parsed = ThemeBridge::parseTomlFlat(toml);
  QVERIFY(parsed.has_value());
  QCOMPARE(parsed->at(QStringLiteral("accent")), QStringLiteral("#7aa2f7"));
  QCOMPARE(parsed->at(QStringLiteral("mode")), QStringLiteral("dark"));
}

void ThemeBridgeTest::parsesBareShellKeys() {
  const QString toml = R"(
normal-fill-alpha = 0.04
base-size = 14
hover-cursor-color = foreground
)";
  const auto parsed = ThemeBridge::parseTomlFlat(toml);
  QVERIFY(parsed.has_value());
  QCOMPARE(parsed->at(QStringLiteral("base-size")), QStringLiteral("14"));
  QCOMPARE(parsed->at(QStringLiteral("hover-cursor-color")),
           QStringLiteral("foreground"));
}

void ThemeBridgeTest::parsesControlAlphas() {
  const QString toml = R"(
hover-cursor-fill-alpha = 0.08
selected-fill-alpha = 0.18
focus-border-alpha = 0.25
normal-border-width = 1
scale = 1.0
)";
  const auto parsed = ThemeBridge::parseTomlFlat(toml);
  QVERIFY(parsed.has_value());
  QCOMPARE(parsed->at(QStringLiteral("selected-fill-alpha")),
           QStringLiteral("0.18"));
  QCOMPARE(parsed->at(QStringLiteral("normal-border-width")),
           QStringLiteral("1"));
}

void ThemeBridgeTest::withAlphaClamps() {
  ThemeBridge theme;
  const QColor base = theme.withAlpha(QColor(QStringLiteral("#7aa2f7")), 2.0);
  QCOMPARE(base.alphaF(), 1.0);
  const QColor zero = theme.withAlpha(QColor(QStringLiteral("#7aa2f7")), -1.0);
  QCOMPARE(zero.alphaF(), 0.0);
}

void ThemeBridgeTest::rejectsEmpty() {
  const auto parsed =
      ThemeBridge::parseTomlFlat(QStringLiteral("# only comment\n"));
  QVERIFY(!parsed.has_value());
}

QTEST_MAIN(ThemeBridgeTest)
#include "test_theme_bridge.moc"
