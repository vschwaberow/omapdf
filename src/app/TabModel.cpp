#include "app/TabModel.h"

TabModel::TabModel(QObject *parent) : QAbstractListModel(parent) {}

int TabModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(m_rows.size());
}

QVariant TabModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(m_rows.size())) {
    return {};
  }
  const Row &row = m_rows[static_cast<size_t>(index.row())];
  switch (role) {
  case PathRole:
    return row.path;
  case TitleRole:
    return row.title;
  case TabIdRole:
    return row.id;
  default:
    return {};
  }
}

QHash<int, QByteArray> TabModel::roleNames() const {
  return {
      {PathRole, "path"},
      {TitleRole, "title"},
      {TabIdRole, "tabId"},
  };
}

int TabModel::append(const QString &path, const QString &title) {
  const int row = static_cast<int>(m_rows.size());
  beginInsertRows({}, row, row);
  m_rows.push_back(Row{m_nextId++, path, title});
  endInsertRows();
  emit countChanged();
  return row;
}

void TabModel::removeAt(int row) {
  if (row < 0 || row >= static_cast<int>(m_rows.size())) {
    return;
  }
  beginRemoveRows({}, row, row);
  m_rows.erase(m_rows.begin() + row);
  endRemoveRows();
  emit countChanged();
}

bool TabModel::setTitle(int row, const QString &title) {
  if (row < 0 || row >= static_cast<int>(m_rows.size()) || title.isEmpty()) {
    return false;
  }
  Row &entry = m_rows[static_cast<size_t>(row)];
  if (entry.title == title) {
    return false;
  }
  entry.title = title;
  const QModelIndex idx = index(row);
  emit dataChanged(idx, idx, {TitleRole});
  return true;
}

int TabModel::indexOfPath(const QString &path) const {
  for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
    if (m_rows[static_cast<size_t>(i)].path == path) {
      return i;
    }
  }
  return -1;
}

QString TabModel::pathAt(int row) const {
  if (row < 0 || row >= static_cast<int>(m_rows.size())) {
    return {};
  }
  return m_rows[static_cast<size_t>(row)].path;
}

QString TabModel::titleAt(int row) const {
  if (row < 0 || row >= static_cast<int>(m_rows.size())) {
    return {};
  }
  return m_rows[static_cast<size_t>(row)].title;
}

int TabModel::idAt(int row) const {
  if (row < 0 || row >= static_cast<int>(m_rows.size())) {
    return -1;
  }
  return m_rows[static_cast<size_t>(row)].id;
}

QVariantList TabModel::snapshot() const {
  QVariantList out;
  out.reserve(static_cast<int>(m_rows.size()));
  for (const Row &row : m_rows) {
    out.push_back(QVariantMap{
        {QStringLiteral("path"), row.path},
        {QStringLiteral("title"), row.title},
        {QStringLiteral("tabId"), row.id},
    });
  }
  return out;
}
