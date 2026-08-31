#pragma once

#include <QJsonDocument>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class SessionStore : public QObject {
  Q_OBJECT

public:
  explicit SessionStore(QObject *parent = nullptr);

  [[nodiscard]] QStringList recents() const;
  void pushRecent(const QString &path);

  [[nodiscard]] QVariantMap documentState(const QString &path) const;
  void saveDocumentState(const QString &path, double zoom, int page,
                         double scrollY, bool dimmed);
  [[nodiscard]] QString annotColor() const;
  void setAnnotColor(const QString &color);

  [[nodiscard]] static QString stateRoot();

private:
  [[nodiscard]] QString recentsPath() const;
  [[nodiscard]] QString prefsPath() const;
  [[nodiscard]] QString docStatePath(const QString &path) const;
  void ensureDirs() const;
  [[nodiscard]] QJsonDocument readJson(const QString &path) const;
  bool writeJson(const QString &path, const QJsonDocument &doc) const;

  static constexpr int kMaxRecents = 20;
};
