#pragma once

#include <QHash>
#include <QString>
#include <QTranslator>

class LocaleTranslator : public QTranslator {
  Q_OBJECT

public:
  explicit LocaleTranslator(QObject *parent = nullptr);

  [[nodiscard]] bool loadGerman();
  [[nodiscard]] bool loadFromJson(const QByteArray &json);
  [[nodiscard]] QString translate(const char *context, const char *sourceText,
                                  const char *disambiguation = nullptr,
                                  int n = -1) const override;

private:
  QHash<QString, QString> m_map;
};
