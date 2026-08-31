#include "app/AnnotStore.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <tuple>

namespace {

constexpr int kMaxUndo = 64;
constexpr int kJsonVersion = 1;

QString sanitizeColor(const QString &color) {
  if (color.size() == 7 && color.startsWith(QLatin1Char('#'))) {
    return color.toLower();
  }
  return QStringLiteral("#f6c177");
}

QVariantList nestedQuads(const QList<QPolygonF> &quads) {
  QVariantList out;
  for (const QPolygonF &poly : quads) {
    QVariantList pts;
    for (const QPointF &pt : poly) {
      pts.push_back(QVariantList{pt.x(), pt.y()});
    }
    out.push_back(pts);
  }
  return out;
}

QJsonObject annotJson(const QString &id, const QString &type, int page,
                      const QString &color, const QString &text,
                      const QList<QPolygonF> &quads, const QPointF &point) {
  QJsonObject o;
  o.insert(QStringLiteral("id"), id);
  o.insert(QStringLiteral("type"), type);
  o.insert(QStringLiteral("page"), page);
  o.insert(QStringLiteral("color"), color);
  o.insert(QStringLiteral("text"), text);
  o.insert(QStringLiteral("quads"),
           QJsonArray::fromVariantList(nestedQuads(quads)));
  if (type == QLatin1String("note")) {
    o.insert(QStringLiteral("point"), QJsonArray{point.x(), point.y()});
  }
  return o;
}

QVariantMap annotItem(const QString &id, const QString &type, int page,
                      const QString &color, const QString &text,
                      const QList<QPolygonF> &quads, const QPointF &point) {
  QVariantMap entry;
  entry.insert(QStringLiteral("id"), id);
  entry.insert(QStringLiteral("type"), type);
  entry.insert(QStringLiteral("page"), page);
  entry.insert(QStringLiteral("color"), color);
  entry.insert(QStringLiteral("text"), text);
  entry.insert(QStringLiteral("quads"), nestedQuads(quads));
  if (type == QLatin1String("note")) {
    entry.insert(QStringLiteral("x"), point.x());
    entry.insert(QStringLiteral("y"), point.y());
  }
  return entry;
}

} // namespace

AnnotStore::AnnotStore(QObject *parent) : QAbstractListModel(parent) {
  QDir().mkpath(annotRoot());
}

QString AnnotStore::annotRoot() {
  return QDir::homePath() + QStringLiteral("/.local/state/omapdf/annots");
}

QStringList AnnotStore::palette() const {
  return {QStringLiteral("#f6c177"), QStringLiteral("#9ece6a"),
          QStringLiteral("#7aa2f7"), QStringLiteral("#bb9af7")};
}

void AnnotStore::setActiveColor(const QString &color) {
  const QString c = sanitizeColor(color);
  if (c == m_activeColor) {
    return;
  }
  m_activeColor = c;
  emit activeColorChanged();
}

int AnnotStore::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(m_annots.size());
}

QHash<int, QByteArray> AnnotStore::roleNames() const {
  return {
      {IdRole, "annotId"},   {TypeRole, "type"}, {PageRole, "page"},
      {ColorRole, "color"},  {TextRole, "text"}, {QuadsRole, "quads"},
  };
}

QVariant AnnotStore::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(m_annots.size())) {
    return {};
  }
  const Annot &a = m_annots[static_cast<size_t>(index.row())];
  switch (role) {
  case IdRole:
    return a.id;
  case TypeRole:
    return a.type;
  case PageRole:
    return a.page;
  case ColorRole:
    return a.color;
  case TextRole:
    return a.text;
  case QuadsRole:
    return quadsToVariant(a.quads);
  default:
    return {};
  }
}

QString AnnotStore::contentHash(const QString &path) const {
  return QString::fromLatin1(fileContentHash(path));
}

QByteArray AnnotStore::fileContentHash(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  QCryptographicHash hash(QCryptographicHash::Sha256);
  while (!file.atEnd()) {
    hash.addData(file.read(1 << 20));
  }
  return hash.result().toHex();
}

QString AnnotStore::sidecarPath() const {
  const QByteArray pathHash =
      QCryptographicHash::hash(m_path.toUtf8(), QCryptographicHash::Sha256)
          .toHex();
  return annotRoot() + QLatin1Char('/') + QString::fromLatin1(pathHash) +
         QStringLiteral(".json");
}

QString AnnotStore::hashSidecarPath() const {
  if (m_contentHash.isEmpty()) {
    return {};
  }
  return annotRoot() + QLatin1Char('/') +
         QString::fromLatin1(m_contentHash) + QStringLiteral(".byhash.json");
}

void AnnotStore::setDirty(bool dirty) {
  if (m_dirty == dirty) {
    return;
  }
  m_dirty = dirty;
  emit dirtyChanged();
}

void AnnotStore::invalidatePageCaches() {
  m_polyByPage.clear();
  m_noteByPage.clear();
}

void AnnotStore::pushUndo() {
  m_undo.push_back(m_annots);
  if (m_undo.size() > kMaxUndo) {
    m_undo.erase(m_undo.begin());
  }
  m_redo.clear();
}

bool AnnotStore::canUndo() const { return !m_undo.empty(); }

bool AnnotStore::canRedo() const { return !m_redo.empty(); }

void AnnotStore::undo() {
  if (m_undo.empty()) {
    return;
  }
  beginResetModel();
  m_redo.push_back(std::move(m_annots));
  m_annots = std::move(m_undo.back());
  m_undo.pop_back();
  endResetModel();
  invalidatePageCaches();
  setDirty(true);
}

void AnnotStore::redo() {
  if (m_redo.empty()) {
    return;
  }
  beginResetModel();
  m_undo.push_back(std::move(m_annots));
  m_annots = std::move(m_redo.back());
  m_redo.pop_back();
  endResetModel();
  invalidatePageCaches();
  setDirty(true);
}

void AnnotStore::load(const QString &path, const QString &contentHash) {
  beginResetModel();
  m_path = path;
  m_contentHash = contentHash.toLatin1();
  m_annots.clear();
  m_undo.clear();
  m_redo.clear();
  endResetModel();
  invalidatePageCaches();
  emit pathChanged();
  setDirty(false);

  QByteArray bytes;
  QFile pathFile(sidecarPath());
  if (pathFile.open(QIODevice::ReadOnly)) {
    bytes = pathFile.readAll();
  } else {
    const QString byHash = hashSidecarPath();
    if (!byHash.isEmpty()) {
      QFile hashFile(byHash);
      if (hashFile.open(QIODevice::ReadOnly)) {
        bytes = hashFile.readAll();
      }
    }
  }
  if (bytes.isEmpty()) {
    return;
  }
  auto parsed = parseJson(bytes);
  if (!parsed) {
    return;
  }
  beginResetModel();
  m_annots = std::move(*parsed);
  endResetModel();
  invalidatePageCaches();
}

std::expected<std::vector<AnnotStore::Annot>, QString>
AnnotStore::parseJson(const QByteArray &bytes) {
  const auto doc = QJsonDocument::fromJson(bytes);
  if (!doc.isObject()) {
    return std::unexpected(QStringLiteral("not an object"));
  }
  const QJsonObject root = doc.object();
  const QJsonArray arr = root.value(QStringLiteral("annots")).toArray();
  std::vector<Annot> out;
  out.reserve(static_cast<size_t>(arr.size()));
  for (const auto &v : arr) {
    if (!v.isObject()) {
      continue;
    }
    const QJsonObject o = v.toObject();
    Annot a;
    a.id = o.value(QStringLiteral("id")).toString();
    if (a.id.isEmpty()) {
      a.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    a.type = o.value(QStringLiteral("type")).toString(QStringLiteral("highlight"));
    a.page = o.value(QStringLiteral("page")).toInt(0);
    a.color = sanitizeColor(o.value(QStringLiteral("color")).toString());
    a.text = o.value(QStringLiteral("text")).toString();
    const QJsonArray quads = o.value(QStringLiteral("quads")).toArray();
    for (const auto &qv : quads) {
      const QJsonArray pts = qv.toArray();
      QPolygonF poly;
      for (const auto &pv : pts) {
        const QJsonArray xy = pv.toArray();
        if (xy.size() >= 2) {
          poly << QPointF(xy.at(0).toDouble(), xy.at(1).toDouble());
        }
      }
      if (!poly.isEmpty()) {
        a.quads.push_back(poly);
      }
    }
    const QJsonArray pt = o.value(QStringLiteral("point")).toArray();
    if (pt.size() >= 2) {
      a.point = QPointF(pt.at(0).toDouble(), pt.at(1).toDouble());
    }
    out.push_back(std::move(a));
  }
  return out;
}

bool AnnotStore::writeBytes(const QString &path, const QByteArray &bytes) const {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  file.write(bytes);
  return true;
}

void AnnotStore::appendAnnot(Annot &&annot) {
  const int row = static_cast<int>(m_annots.size());
  beginInsertRows({}, row, row);
  m_annots.push_back(std::move(annot));
  endInsertRows();
  invalidatePageCaches();
  setDirty(true);
}

QByteArray AnnotStore::toJson() const {
  QJsonObject root;
  root.insert(QStringLiteral("version"), kJsonVersion);
  root.insert(QStringLiteral("path"), m_path);
  root.insert(QStringLiteral("contentHash"),
              QString::fromLatin1(m_contentHash));
  QJsonArray arr;
  for (const Annot &a : m_annots) {
    arr.append(annotJson(a.id, a.type, a.page, a.color, a.text, a.quads, a.point));
  }
  root.insert(QStringLiteral("annots"), arr);
  return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool AnnotStore::save() {
  if (m_path.isEmpty()) {
    return false;
  }
  QDir().mkpath(annotRoot());
  const QByteArray bytes = toJson();
  if (!writeBytes(sidecarPath(), bytes)) {
    return false;
  }
  const QString byHash = hashSidecarPath();
  if (!byHash.isEmpty()) {
    std::ignore = writeBytes(byHash, bytes);
  }
  m_undo.clear();
  m_redo.clear();
  setDirty(false);
  emit saved();
  return true;
}

void AnnotStore::discard() {
  const QString path = m_path;
  const QString hash = QString::fromLatin1(m_contentHash);
  load(path, hash);
}

void AnnotStore::addHighlight(int page, const QString &text,
                              const QVariantList &quads) {
  if (page < 0 || quads.isEmpty()) {
    return;
  }
  pushUndo();
  Annot a;
  a.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  a.type = QStringLiteral("highlight");
  a.page = page;
  a.color = m_activeColor;
  a.text = text;
  a.quads = quadsFromVariant(quads);
  appendAnnot(std::move(a));
}

void AnnotStore::addNote(int page, qreal x, qreal y, const QString &text) {
  if (page < 0 || text.trimmed().isEmpty()) {
    return;
  }
  pushUndo();
  Annot a;
  a.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  a.type = QStringLiteral("note");
  a.page = page;
  a.color = m_activeColor;
  a.text = text.trimmed();
  a.point = QPointF(x, y);
  a.quads = {QPolygonF(QRectF(x - 6, y - 6, 12, 12))};
  appendAnnot(std::move(a));
}

void AnnotStore::removeAt(int row) {
  if (row < 0 || row >= static_cast<int>(m_annots.size())) {
    return;
  }
  pushUndo();
  beginRemoveRows({}, row, row);
  m_annots.erase(m_annots.begin() + row);
  endRemoveRows();
  invalidatePageCaches();
  setDirty(true);
}

QList<QPolygonF> AnnotStore::quadsFromVariant(const QVariantList &v) {
  QList<QPolygonF> out;
  for (const QVariant &item : v) {
    QPolygonF poly;
    if (item.canConvert<QPolygonF>()) {
      poly = item.value<QPolygonF>();
    } else if (item.typeId() == QMetaType::QVariantList ||
               item.canConvert<QVariantList>()) {
      const QVariantList pts = item.toList();
      for (const QVariant &pv : pts) {
        if (pv.canConvert<QPointF>()) {
          poly << pv.toPointF();
        } else {
          const QVariantList xy = pv.toList();
          if (xy.size() >= 2) {
            poly << QPointF(xy.at(0).toDouble(), xy.at(1).toDouble());
          }
        }
      }
    }
    if (!poly.isEmpty()) {
      out.push_back(poly);
    }
  }
  return out;
}

QVariantList AnnotStore::quadsToVariant(const QList<QPolygonF> &q) {
  QVariantList out;
  for (const QPolygonF &poly : q) {
    out.push_back(QVariant::fromValue(poly));
  }
  return out;
}


QVariantList AnnotStore::items() const {
  QVariantList out;
  out.reserve(static_cast<int>(m_annots.size()));
  for (const Annot &a : m_annots) {
    out.push_back(annotItem(a.id, a.type, a.page, a.color, a.text, a.quads, a.point));
  }
  return out;
}

QVariantList AnnotStore::polygonsOnPage(int page) const {
  if (const auto it = m_polyByPage.constFind(page); it != m_polyByPage.cend()) {
    return it.value();
  }
  QVariantList out;
  for (const Annot &a : m_annots) {
    if (a.page != page || a.type != QLatin1String("highlight")) {
      continue;
    }
    QVariantMap entry;
    entry.insert(QStringLiteral("color"), a.color);
    entry.insert(QStringLiteral("paths"), quadsToVariant(a.quads));
    out.push_back(entry);
  }
  m_polyByPage.insert(page, out);
  return out;
}

QVariantList AnnotStore::notesOnPage(int page) const {
  if (const auto it = m_noteByPage.constFind(page); it != m_noteByPage.cend()) {
    return it.value();
  }
  QVariantList out;
  for (const Annot &a : m_annots) {
    if (a.page != page || a.type != QLatin1String("note")) {
      continue;
    }
    QVariantMap entry;
    entry.insert(QStringLiteral("color"), a.color);
    entry.insert(QStringLiteral("text"), a.text);
    entry.insert(QStringLiteral("x"), a.point.x());
    entry.insert(QStringLiteral("y"), a.point.y());
    out.push_back(entry);
  }
  m_noteByPage.insert(page, out);
  return out;
}
