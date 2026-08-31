#include "app/ThemeBridge.h"

#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QRegularExpression>
#include <QtGlobal>

namespace {

QString themeDirPath() {
  return QDir::homePath() + QStringLiteral("/.local/state/omarchy/current/theme");
}

QString userShellPath() {
  return QDir::homePath() + QStringLiteral("/.config/omarchy/shell.toml");
}

void mergeMaps(std::unordered_map<QString, QString> &dst,
               const std::unordered_map<QString, QString> &src) {
  for (const auto &[k, v] : src) {
    dst.insert_or_assign(k, v);
  }
}

} // namespace

ThemeBridge::ThemeBridge(QObject *parent) : QObject(parent) {
  m_themeDir = themeDirPath();
  watchThemeDir();
  reload();
}

QColor ThemeBridge::withAlpha(const QColor &color, qreal alpha) const {
  QColor out = color;
  out.setAlphaF(qBound(0.0, alpha, 1.0));
  return out;
}

std::expected<std::unordered_map<QString, QString>, QString>
ThemeBridge::parseTomlFlat(const QString &text) {
  std::unordered_map<QString, QString> out;
  static const QRegularExpression quoted(
      QStringLiteral("^\\s*([A-Za-z0-9_-]+)\\s*=\\s*\"([^\"]*)\"\\s*(?:#.*)?"));
  static const QRegularExpression bare(
      QStringLiteral("^\\s*([A-Za-z0-9_-]+)\\s*=\\s*([^#]+?)\\s*(?:#.*)?$"));

  for (const QString &line : text.split(QLatin1Char('\n'))) {
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#')) ||
        trimmed.startsWith(QLatin1Char('['))) {
      continue;
    }
    if (const auto m = quoted.match(line); m.hasMatch()) {
      out.insert_or_assign(m.captured(1), m.captured(2));
      continue;
    }
    if (const auto m = bare.match(line); m.hasMatch()) {
      out.insert_or_assign(m.captured(1), m.captured(2).trimmed());
    }
  }
  if (out.empty()) {
    return std::unexpected(QStringLiteral("no keys parsed"));
  }
  return out;
}

QColor ThemeBridge::resolveToken(const QString &token,
                                 const QColor &fallback) const {
  const QString t = token.trimmed().toLower();
  if (t == QLatin1String("foreground") || t == QLatin1String("text")) {
    return m_foreground;
  }
  if (t == QLatin1String("background")) {
    return m_background;
  }
  if (t == QLatin1String("accent")) {
    return m_accent;
  }
  if (t == QLatin1String("urgent") || t == QLatin1String("red")) {
    return m_urgent;
  }
  if (t == QLatin1String("muted")) {
    return m_muted;
  }
  if (t == QLatin1String("selection")) {
    return m_selection;
  }
  if (t.startsWith(QLatin1Char('#'))) {
    const QColor c(token.trimmed());
    return c.isValid() ? c : fallback;
  }
  const QColor c(token.trimmed());
  return c.isValid() ? c : fallback;
}

void ThemeBridge::readAlpha(const std::unordered_map<QString, QString> &map,
                            const QString &key, qreal &dst) {
  const auto it = map.find(key);
  if (it == map.end()) {
    return;
  }
  bool ok = false;
  const qreal v = it->second.toDouble(&ok);
  if (ok) {
    dst = qBound(0.0, v, 1.0);
  }
}

void ThemeBridge::readInt(const std::unordered_map<QString, QString> &map,
                          const QString &key, int &dst, int minValue) {
  const auto it = map.find(key);
  if (it == map.end()) {
    return;
  }
  bool ok = false;
  const int v = it->second.toInt(&ok);
  if (ok && v >= minValue) {
    dst = v;
  }
}

void ThemeBridge::applyColors(const std::unordered_map<QString, QString> &map) {
  auto colorOr = [&](const QString &key, const QColor &fb) {
    const auto it = map.find(key);
    if (it == map.end()) {
      return fb;
    }
    const QColor c(it->second);
    return c.isValid() ? c : fb;
  };
  m_foreground = colorOr(QStringLiteral("foreground"), m_foreground);
  m_background = colorOr(QStringLiteral("background"), m_background);
  m_accent = colorOr(QStringLiteral("accent"), m_accent);
  m_urgent = colorOr(QStringLiteral("red"), m_urgent);
  if (map.contains(QStringLiteral("urgent"))) {
    m_urgent = colorOr(QStringLiteral("urgent"), m_urgent);
  }
  m_muted = colorOr(QStringLiteral("muted"), m_muted);
  m_selection = colorOr(QStringLiteral("selection"), m_selection);
  m_darkBackground =
      colorOr(QStringLiteral("dark_background"), m_darkBackground);
  m_darkerBackground =
      colorOr(QStringLiteral("darker_background"), m_darkerBackground);
  m_lighterBackground =
      colorOr(QStringLiteral("lighter_background"), m_lighterBackground);
  m_darkForeground =
      colorOr(QStringLiteral("dark_foreground"), m_darkForeground);
  if (const auto it = map.find(QStringLiteral("mode")); it != map.end()) {
    m_mode = it->second;
  }
  m_hoverColor = m_foreground;
  m_selectedColor = m_foreground;
  m_focusColor = m_foreground;
  m_normalBorder = m_foreground;
  m_hoverBorder = m_foreground;
  m_selectedBorder = m_foreground;
  m_focusBorder = m_foreground;
}

void ThemeBridge::applyShell(const std::unordered_map<QString, QString> &map) {
  auto token = [&](const QString &key, QColor &dst) {
    const auto it = map.find(key);
    if (it != map.end()) {
      dst = resolveToken(it->second, dst);
    }
  };
  token(QStringLiteral("hover-cursor-color"), m_hoverColor);
  token(QStringLiteral("selected-color"), m_selectedColor);
  token(QStringLiteral("focus-color"), m_focusColor);
  token(QStringLiteral("normal-color"), m_normalBorder);
  token(QStringLiteral("normal-border"), m_normalBorder);
  token(QStringLiteral("hover-cursor-border"), m_hoverBorder);
  token(QStringLiteral("selected-border"), m_selectedBorder);
  token(QStringLiteral("focus-border"), m_focusBorder);

  readAlpha(map, QStringLiteral("normal-fill-alpha"), m_normalFillAlpha);
  readAlpha(map, QStringLiteral("hover-cursor-fill-alpha"), m_hoverFillAlpha);
  readAlpha(map, QStringLiteral("selected-fill-alpha"), m_selectedFillAlpha);
  readAlpha(map, QStringLiteral("focus-fill-alpha"), m_focusFillAlpha);
  readAlpha(map, QStringLiteral("pressed-fill-alpha"), m_pressedFillAlpha);
  readAlpha(map, QStringLiteral("normal-border-alpha"), m_normalBorderAlpha);
  readAlpha(map, QStringLiteral("hover-cursor-border-alpha"), m_hoverBorderAlpha);
  readAlpha(map, QStringLiteral("selected-border-alpha"), m_selectedBorderAlpha);
  readAlpha(map, QStringLiteral("focus-border-alpha"), m_focusBorderAlpha);

  readInt(map, QStringLiteral("normal-border-width"), m_controlBorderWidth, 0);
  readInt(map, QStringLiteral("base-size"), m_fontBaseSize, 1);

  qreal spacingScale = 1.0;
  if (const auto it = map.find(QStringLiteral("scale")); it != map.end()) {
    bool ok = false;
    const qreal v = it->second.toDouble(&ok);
    if (ok && v > 0.0) {
      spacingScale = v;
    }
  }
  m_spaceSm = qMax(1, qRound(4 * spacingScale));
  m_spaceMd = qMax(1, qRound(6 * spacingScale));
  m_spaceLg = qMax(1, qRound(8 * spacingScale));
  readInt(map, QStringLiteral("sm"), m_spaceSm, 1);
  readInt(map, QStringLiteral("md"), m_spaceMd, 1);
  readInt(map, QStringLiteral("lg"), m_spaceLg, 1);
}

void ThemeBridge::loadHyprRounding() {
  QProcess proc;
  proc.start(QStringLiteral("hyprctl"),
             {QStringLiteral("-j"), QStringLiteral("getoption"),
              QStringLiteral("decoration:rounding")});
  if (!proc.waitForFinished(500)) {
    return;
  }
  const QByteArray out = proc.readAllStandardOutput();
  static const QRegularExpression re(QStringLiteral("\"int\"\\s*:\\s*(\\d+)"));
  if (const auto m = re.match(QString::fromUtf8(out)); m.hasMatch()) {
    m_cornerRadius = m.captured(1).toInt();
  }
}

void ThemeBridge::reload() {
  const QString colorsPath = m_themeDir + QStringLiteral("/colors.toml");
  QFile colorsFile(colorsPath);
  if (!colorsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_ready = false;
    emit paletteChanged();
    return;
  }
  const auto colors = parseTomlFlat(QString::fromUtf8(colorsFile.readAll()));
  if (!colors) {
    m_ready = false;
    emit paletteChanged();
    return;
  }
  applyColors(*colors);

  std::unordered_map<QString, QString> shell;
  const QString themeShell = m_themeDir + QStringLiteral("/shell.toml");
  if (QFile f(themeShell); f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (const auto parsed = parseTomlFlat(QString::fromUtf8(f.readAll()))) {
      shell = *parsed;
    }
  }
  if (QFile f(userShellPath()); f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (const auto parsed = parseTomlFlat(QString::fromUtf8(f.readAll()))) {
      mergeMaps(shell, *parsed);
    }
  }
  applyShell(shell);
  loadHyprRounding();
  m_ready = true;
  emit paletteChanged();
}

void ThemeBridge::watchThemeDir() {
  auto *watcher = new QFileSystemWatcher(this);
  const auto arm = [this, watcher]() {
    if (QDir(m_themeDir).exists()) {
      if (!watcher->directories().contains(m_themeDir)) {
        watcher->addPath(m_themeDir);
      }
      for (const char *name : {"colors.toml", "shell.toml"}) {
        const QString p = m_themeDir + QLatin1Char('/') + QLatin1String(name);
        if (QFile::exists(p) && !watcher->files().contains(p)) {
          watcher->addPath(p);
        }
      }
    }
    const QString userShell = userShellPath();
    if (QFile::exists(userShell) && !watcher->files().contains(userShell)) {
      watcher->addPath(userShell);
    }
  };
  arm();
  const auto onChange = [this, arm](const QString &) {
    reload();
    arm();
  };
  QObject::connect(watcher, &QFileSystemWatcher::fileChanged, this, onChange);
  QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, this,
                   onChange);
}
