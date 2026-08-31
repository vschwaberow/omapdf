#include "app/SessionStore.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>

SessionStore::SessionStore(QObject *parent) : QObject(parent) { ensureDirs(); }

QString SessionStore::stateRoot() {
  return QDir::homePath() + QStringLiteral("/.local/state/omapdf");
}

void SessionStore::ensureDirs() const {
  QDir().mkpath(stateRoot());
  QDir().mkpath(stateRoot() + QStringLiteral("/docs"));
  QDir().mkpath(stateRoot() + QStringLiteral("/annots"));
}

QString SessionStore::recentsPath() const {
  return stateRoot() + QStringLiteral("/recents.json");
}

QString SessionStore::docStatePath(const QString &path) const {
  const QByteArray hash =
      QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256)
          .toHex();
  return stateRoot() + QStringLiteral("/docs/") +
         QString::fromLatin1(hash) + QStringLiteral(".json");
}

QJsonDocument SessionStore::readJson(const QString &path) const {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  return QJsonDocument::fromJson(file.readAll());
}

bool SessionStore::writeJson(const QString &path,
                             const QJsonDocument &doc) const {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  file.write(doc.toJson(QJsonDocument::Compact));
  return true;
}

QStringList SessionStore::recents() const {
  const auto doc = readJson(recentsPath());
  if (!doc.isArray()) {
    return {};
  }
  QStringList out;
  for (const auto &v : doc.array()) {
    const QString p = v.toString();
    if (!p.isEmpty() && QFile::exists(p)) {
      out.push_back(p);
    }
  }
  return out;
}

void SessionStore::pushRecent(const QString &path) {
  QStringList list = recents();
  list.removeAll(path);
  list.prepend(path);
  while (list.size() > kMaxRecents) {
    list.removeLast();
  }
  QJsonArray arr;
  for (const QString &p : list) {
    arr.append(p);
  }
  writeJson(recentsPath(), QJsonDocument(arr));
}

QVariantMap SessionStore::documentState(const QString &path) const {
  const auto doc = readJson(docStatePath(path));
  if (!doc.isObject()) {
    return {};
  }
  const QJsonObject o = doc.object();
  return QVariantMap{
      {QStringLiteral("zoom"), o.value(QStringLiteral("zoom")).toDouble(0.0)},
      {QStringLiteral("page"), o.value(QStringLiteral("page")).toInt(0)},
      {QStringLiteral("scrollY"),
       o.value(QStringLiteral("scrollY")).toDouble(0.0)},
      {QStringLiteral("dimmed"),
       o.value(QStringLiteral("dimmed")).toBool(false)},
  };
}

void SessionStore::saveDocumentState(const QString &path, double zoom, int page,
                                     double scrollY, bool dimmed) {
  QJsonObject o;
  o.insert(QStringLiteral("path"), path);
  o.insert(QStringLiteral("zoom"), zoom);
  o.insert(QStringLiteral("page"), page);
  o.insert(QStringLiteral("scrollY"), scrollY);
  o.insert(QStringLiteral("dimmed"), dimmed);
  writeJson(docStatePath(path), QJsonDocument(o));
}

QString SessionStore::prefsPath() const {
  return stateRoot() + QStringLiteral("/prefs.json");
}

QString SessionStore::annotColor() const {
  constexpr auto kDefault = QLatin1String("#f6c177");
  const auto doc = readJson(prefsPath());
  if (!doc.isObject()) {
    return QString(kDefault);
  }
  const QString color =
      doc.object().value(QStringLiteral("annotColor")).toString().toLower();
  if (color.size() == 7 && color.startsWith(QLatin1Char('#'))) {
    return color;
  }
  return QString(kDefault);
}

void SessionStore::setAnnotColor(const QString &color) {
  const QString c = color.toLower();
  if (c.size() != 7 || !c.startsWith(QLatin1Char('#'))) {
    return;
  }
  QJsonObject o;
  const auto existing = readJson(prefsPath());
  if (existing.isObject()) {
    o = existing.object();
  }
  if (o.value(QStringLiteral("annotColor")).toString() == c) {
    return;
  }
  o.insert(QStringLiteral("annotColor"), c);
  writeJson(prefsPath(), QJsonDocument(o));
}
