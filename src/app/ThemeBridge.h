#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <expected>
#include <unordered_map>

class ThemeBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY(QColor foreground READ foreground NOTIFY paletteChanged)
  Q_PROPERTY(QColor background READ background NOTIFY paletteChanged)
  Q_PROPERTY(QColor accent READ accent NOTIFY paletteChanged)
  Q_PROPERTY(QColor urgent READ urgent NOTIFY paletteChanged)
  Q_PROPERTY(QColor muted READ muted NOTIFY paletteChanged)
  Q_PROPERTY(QColor selection READ selection NOTIFY paletteChanged)
  Q_PROPERTY(QColor darkBackground READ darkBackground NOTIFY paletteChanged)
  Q_PROPERTY(QColor darkerBackground READ darkerBackground NOTIFY paletteChanged)
  Q_PROPERTY(QColor lighterBackground READ lighterBackground NOTIFY paletteChanged)
  Q_PROPERTY(QColor darkForeground READ darkForeground NOTIFY paletteChanged)
  Q_PROPERTY(QColor hoverColor READ hoverColor NOTIFY paletteChanged)
  Q_PROPERTY(QColor selectedColor READ selectedColor NOTIFY paletteChanged)
  Q_PROPERTY(QColor focusColor READ focusColor NOTIFY paletteChanged)
  Q_PROPERTY(QColor normalBorder READ normalBorder NOTIFY paletteChanged)
  Q_PROPERTY(QColor hoverBorder READ hoverBorder NOTIFY paletteChanged)
  Q_PROPERTY(QColor selectedBorder READ selectedBorder NOTIFY paletteChanged)
  Q_PROPERTY(QColor focusBorder READ focusBorder NOTIFY paletteChanged)
  Q_PROPERTY(qreal normalFillAlpha READ normalFillAlpha NOTIFY paletteChanged)
  Q_PROPERTY(qreal hoverFillAlpha READ hoverFillAlpha NOTIFY paletteChanged)
  Q_PROPERTY(qreal selectedFillAlpha READ selectedFillAlpha NOTIFY paletteChanged)
  Q_PROPERTY(qreal focusFillAlpha READ focusFillAlpha NOTIFY paletteChanged)
  Q_PROPERTY(qreal pressedFillAlpha READ pressedFillAlpha NOTIFY paletteChanged)
  Q_PROPERTY(qreal normalBorderAlpha READ normalBorderAlpha NOTIFY paletteChanged)
  Q_PROPERTY(qreal hoverBorderAlpha READ hoverBorderAlpha NOTIFY paletteChanged)
  Q_PROPERTY(qreal selectedBorderAlpha READ selectedBorderAlpha NOTIFY paletteChanged)
  Q_PROPERTY(qreal focusBorderAlpha READ focusBorderAlpha NOTIFY paletteChanged)
  Q_PROPERTY(int controlBorderWidth READ controlBorderWidth NOTIFY paletteChanged)
  Q_PROPERTY(int fontBaseSize READ fontBaseSize NOTIFY paletteChanged)
  Q_PROPERTY(int cornerRadius READ cornerRadius NOTIFY paletteChanged)
  Q_PROPERTY(int spaceSm READ spaceSm NOTIFY paletteChanged)
  Q_PROPERTY(int spaceMd READ spaceMd NOTIFY paletteChanged)
  Q_PROPERTY(int spaceLg READ spaceLg NOTIFY paletteChanged)
  Q_PROPERTY(QString mode READ mode NOTIFY paletteChanged)
  Q_PROPERTY(bool ready READ ready NOTIFY paletteChanged)

public:
  explicit ThemeBridge(QObject *parent = nullptr);

  [[nodiscard]] QColor foreground() const { return m_foreground; }
  [[nodiscard]] QColor background() const { return m_background; }
  [[nodiscard]] QColor accent() const { return m_accent; }
  [[nodiscard]] QColor urgent() const { return m_urgent; }
  [[nodiscard]] QColor muted() const { return m_muted; }
  [[nodiscard]] QColor selection() const { return m_selection; }
  [[nodiscard]] QColor darkBackground() const { return m_darkBackground; }
  [[nodiscard]] QColor darkerBackground() const { return m_darkerBackground; }
  [[nodiscard]] QColor lighterBackground() const { return m_lighterBackground; }
  [[nodiscard]] QColor darkForeground() const { return m_darkForeground; }
  [[nodiscard]] QColor hoverColor() const { return m_hoverColor; }
  [[nodiscard]] QColor selectedColor() const { return m_selectedColor; }
  [[nodiscard]] QColor focusColor() const { return m_focusColor; }
  [[nodiscard]] QColor normalBorder() const { return m_normalBorder; }
  [[nodiscard]] QColor hoverBorder() const { return m_hoverBorder; }
  [[nodiscard]] QColor selectedBorder() const { return m_selectedBorder; }
  [[nodiscard]] QColor focusBorder() const { return m_focusBorder; }
  [[nodiscard]] qreal normalFillAlpha() const { return m_normalFillAlpha; }
  [[nodiscard]] qreal hoverFillAlpha() const { return m_hoverFillAlpha; }
  [[nodiscard]] qreal selectedFillAlpha() const { return m_selectedFillAlpha; }
  [[nodiscard]] qreal focusFillAlpha() const { return m_focusFillAlpha; }
  [[nodiscard]] qreal pressedFillAlpha() const { return m_pressedFillAlpha; }
  [[nodiscard]] qreal normalBorderAlpha() const { return m_normalBorderAlpha; }
  [[nodiscard]] qreal hoverBorderAlpha() const { return m_hoverBorderAlpha; }
  [[nodiscard]] qreal selectedBorderAlpha() const { return m_selectedBorderAlpha; }
  [[nodiscard]] qreal focusBorderAlpha() const { return m_focusBorderAlpha; }
  [[nodiscard]] int controlBorderWidth() const { return m_controlBorderWidth; }
  [[nodiscard]] int fontBaseSize() const { return m_fontBaseSize; }
  [[nodiscard]] int cornerRadius() const { return m_cornerRadius; }
  [[nodiscard]] int spaceSm() const { return m_spaceSm; }
  [[nodiscard]] int spaceMd() const { return m_spaceMd; }
  [[nodiscard]] int spaceLg() const { return m_spaceLg; }
  [[nodiscard]] QString mode() const { return m_mode; }
  [[nodiscard]] bool ready() const { return m_ready; }

  Q_INVOKABLE [[nodiscard]] QColor withAlpha(const QColor &color, qreal alpha) const;

  [[nodiscard]] static std::expected<std::unordered_map<QString, QString>, QString>
  parseTomlFlat(const QString &text);

  Q_INVOKABLE void reload();

signals:
  void paletteChanged();

private:
  void applyColors(const std::unordered_map<QString, QString> &map);
  void applyShell(const std::unordered_map<QString, QString> &map);
  [[nodiscard]] QColor resolveToken(const QString &token,
                                    const QColor &fallback) const;
  void watchThemeDir();
  void loadHyprRounding();
  static void readAlpha(const std::unordered_map<QString, QString> &map,
                        const QString &key, qreal &dst);
  static void readInt(const std::unordered_map<QString, QString> &map,
                      const QString &key, int &dst, int minValue);

  QColor m_foreground{QStringLiteral("#a9b1d6")};
  QColor m_background{QStringLiteral("#1a1b26")};
  QColor m_accent{QStringLiteral("#7aa2f7")};
  QColor m_urgent{QStringLiteral("#f7768e")};
  QColor m_muted{QStringLiteral("#414868")};
  QColor m_selection{QStringLiteral("#292e42")};
  QColor m_darkBackground{QStringLiteral("#13141c")};
  QColor m_darkerBackground{QStringLiteral("#0e0e14")};
  QColor m_lighterBackground{QStringLiteral("#24283b")};
  QColor m_darkForeground{QStringLiteral("#565f89")};
  QColor m_hoverColor{QStringLiteral("#a9b1d6")};
  QColor m_selectedColor{QStringLiteral("#a9b1d6")};
  QColor m_focusColor{QStringLiteral("#a9b1d6")};
  QColor m_normalBorder{QStringLiteral("#a9b1d6")};
  QColor m_hoverBorder{QStringLiteral("#a9b1d6")};
  QColor m_selectedBorder{QStringLiteral("#a9b1d6")};
  QColor m_focusBorder{QStringLiteral("#a9b1d6")};
  qreal m_normalFillAlpha{0.04};
  qreal m_hoverFillAlpha{0.08};
  qreal m_selectedFillAlpha{0.18};
  qreal m_focusFillAlpha{0.08};
  qreal m_pressedFillAlpha{0.22};
  qreal m_normalBorderAlpha{0.4};
  qreal m_hoverBorderAlpha{0.25};
  qreal m_selectedBorderAlpha{1.0};
  qreal m_focusBorderAlpha{0.25};
  int m_controlBorderWidth{1};
  int m_fontBaseSize{14};
  int m_cornerRadius{0};
  int m_spaceSm{4};
  int m_spaceMd{6};
  int m_spaceLg{8};
  QString m_mode{QStringLiteral("dark")};
  bool m_ready{false};
  QString m_themeDir;
};
