#include "app/LocaleTranslator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

LocaleTranslator::LocaleTranslator(QObject *parent) : QTranslator(parent) {}

bool LocaleTranslator::loadFromJson(const QByteArray &json) {
  const auto doc = QJsonDocument::fromJson(json);
  if (!doc.isObject()) {
    return false;
  }
  m_map.clear();
  const QJsonObject obj = doc.object();
  for (auto it = obj.begin(); it != obj.end(); ++it) {
    m_map.insert(it.key(), it.value().toString());
  }
  return !m_map.isEmpty();
}

bool LocaleTranslator::loadGerman() {
  QFile file(QStringLiteral(":/i18n/omapdf_de.json"));
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  return loadFromJson(file.readAll());
}

QString LocaleTranslator::translate(const char *context, const char *sourceText,
                                    const char *disambiguation, int n) const {
  Q_UNUSED(context);
  Q_UNUSED(disambiguation);
  Q_UNUSED(n);
  if (!sourceText) {
    return {};
  }
  const QString key = QString::fromUtf8(sourceText);
  const auto it = m_map.constFind(key);
  if (it == m_map.cend()) {
    return {};
  }
  return *it;
}
