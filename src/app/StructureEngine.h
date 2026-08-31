#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <expected>
#include <functional>
#include <vector>

class StructureEngine : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
  explicit StructureEngine(QObject *parent = nullptr);

  [[nodiscard]] bool busy() const { return m_busy; }

  Q_INVOKABLE [[nodiscard]] QVariantMap rotate(const QString &path, int page,
                                               int degrees) const;
  Q_INVOKABLE [[nodiscard]] QVariantMap removePages(const QString &path,
                                                    const QVariantList &pages) const;
  Q_INVOKABLE [[nodiscard]] QVariantMap reorder(const QString &path,
                                                const QVariantList &order) const;
  Q_INVOKABLE [[nodiscard]] QVariantMap extract(const QString &path,
                                                const QVariantList &pages,
                                                const QString &dest) const;
  Q_INVOKABLE [[nodiscard]] QVariantMap merge(const QStringList &sources,
                                              const QString &dest) const;
  Q_INVOKABLE [[nodiscard]] QVariantMap exportAnnots(const QString &path,
                                                     const QString &dest,
                                                     const QVariantList &annots) const;

  Q_INVOKABLE void rotateAsync(const QString &path, int page, int degrees);
  Q_INVOKABLE void removePagesAsync(const QString &path,
                                    const QVariantList &pages);
  Q_INVOKABLE void reorderAsync(const QString &path, const QVariantList &order);
  Q_INVOKABLE void extractAsync(const QString &path, const QVariantList &pages,
                                const QString &dest);
  Q_INVOKABLE void mergeAsync(const QStringList &sources, const QString &dest);
  Q_INVOKABLE void exportAnnotsAsync(const QString &path, const QString &dest,
                                     const QVariantList &annots);

  [[nodiscard]] static std::expected<void, QString>
  rotateFile(const QString &path, int page, int degrees);
  [[nodiscard]] static std::expected<void, QString>
  removePagesFile(const QString &path, const std::vector<int> &pages);
  [[nodiscard]] static std::expected<void, QString>
  reorderFile(const QString &path, const std::vector<int> &order);
  [[nodiscard]] static std::expected<void, QString>
  extractFile(const QString &path, const std::vector<int> &pages,
              const QString &dest);
  [[nodiscard]] static std::expected<void, QString>
  mergeFiles(const std::vector<QString> &sources, const QString &dest);

  [[nodiscard]] static std::expected<void, QString>
  writeBlankPdf(const QString &path, int pageCount);
  [[nodiscard]] static std::expected<void, QString>
  exportAnnotsFile(const QString &path, const QString &dest,
                   const QVariantList &annots);

signals:
  void busyChanged();
  void opFinished(const QVariantMap &result);

private:
  [[nodiscard]] static QVariantMap okMap();
  [[nodiscard]] static QVariantMap errMap(const QString &message);
  [[nodiscard]] static std::expected<std::vector<int>, QString>
  normalizePages(const QVariantList &pages, int pageCount);
  [[nodiscard]] static std::expected<QString, QString>
  hardenLocalPath(const QString &path, bool mustExist);
  [[nodiscard]] static std::expected<void, QString>
  writeAtomic(class QPDF &pdf, const QString &dest);

  void setBusy(bool busy);
  void runAsync(std::function<QVariantMap()> job);

  bool m_busy{false};
};
