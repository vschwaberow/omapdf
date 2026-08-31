#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <vector>

class TabModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Role {
    PathRole = Qt::UserRole + 1,
    TitleRole,
    TabIdRole,
  };
  Q_ENUM(Role)

  explicit TabModel(QObject *parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  int append(const QString &path, const QString &title);
  void removeAt(int row);
  bool setTitle(int row, const QString &title);
  [[nodiscard]] int indexOfPath(const QString &path) const;
  [[nodiscard]] QString pathAt(int row) const;
  [[nodiscard]] QString titleAt(int row) const;
  [[nodiscard]] int idAt(int row) const;
  [[nodiscard]] QVariantList snapshot() const;

signals:
  void countChanged();

private:
  struct Row {
    int id{0};
    QString path;
    QString title;
  };

  std::vector<Row> m_rows;
  int m_nextId{1};
};
