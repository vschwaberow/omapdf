#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QColor>
#include <QPolygonF>
#include <QString>
#include <QUuid>
#include <QVariantList>
#include <QHash>
#include <expected>
#include <vector>

class AnnotStore : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
  Q_PROPERTY(QString path READ path NOTIFY pathChanged)
  Q_PROPERTY(QString activeColor READ activeColor WRITE setActiveColor NOTIFY
                 activeColorChanged)
  Q_PROPERTY(QStringList palette READ palette CONSTANT)

public:
  enum Role {
    IdRole = Qt::UserRole + 1,
    TypeRole,
    PageRole,
    ColorRole,
    TextRole,
    QuadsRole,
  };
  Q_ENUM(Role)

  explicit AnnotStore(QObject *parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] bool dirty() const { return m_dirty; }
  [[nodiscard]] QString path() const { return m_path; }
  [[nodiscard]] QString activeColor() const { return m_activeColor; }
  void setActiveColor(const QString &color);
  [[nodiscard]] QStringList palette() const;

  Q_INVOKABLE void load(const QString &path, const QString &contentHash);
  Q_INVOKABLE [[nodiscard]] bool save();
  Q_INVOKABLE void discard();
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();
  Q_INVOKABLE [[nodiscard]] bool canUndo() const;
  Q_INVOKABLE [[nodiscard]] bool canRedo() const;

  Q_INVOKABLE void addHighlight(int page, const QString &text,
                                const QVariantList &quads);
  Q_INVOKABLE void addNote(int page, qreal x, qreal y, const QString &text);
  Q_INVOKABLE void removeAt(int row);

  Q_INVOKABLE [[nodiscard]] QVariantList polygonsOnPage(int page) const;
  Q_INVOKABLE [[nodiscard]] QVariantList notesOnPage(int page) const;
  Q_INVOKABLE [[nodiscard]] QVariantList items() const;
  Q_INVOKABLE [[nodiscard]] QString contentHash(const QString &path) const;

  [[nodiscard]] static QString annotRoot();
  [[nodiscard]] static QByteArray fileContentHash(const QString &path);

signals:
  void dirtyChanged();
  void pathChanged();
  void activeColorChanged();
  void saved();

private:
  struct Annot {
    QString id;
    QString type;
    int page{0};
    QString color;
    QString text;
    QList<QPolygonF> quads;
    QPointF point;
  };

  void setDirty(bool dirty);
  void pushUndo();
  void appendAnnot(Annot &&annot);
  [[nodiscard]] bool writeBytes(const QString &path, const QByteArray &bytes) const;
  [[nodiscard]] QString sidecarPath() const;
  [[nodiscard]] QString hashSidecarPath() const;
  [[nodiscard]] static std::expected<std::vector<Annot>, QString>
  parseJson(const QByteArray &bytes);
  [[nodiscard]] QByteArray toJson() const;
  [[nodiscard]] static QList<QPolygonF> quadsFromVariant(const QVariantList &v);
  [[nodiscard]] static QVariantList quadsToVariant(const QList<QPolygonF> &q);
  void invalidatePageCaches();

  QString m_path;
  QByteArray m_contentHash;
  QString m_activeColor{QStringLiteral("#f6c177")};
  std::vector<Annot> m_annots;
  mutable QHash<int, QVariantList> m_polyByPage;
  mutable QHash<int, QVariantList> m_noteByPage;
  std::vector<std::vector<Annot>> m_undo;
  std::vector<std::vector<Annot>> m_redo;
  bool m_dirty{false};
};
