#pragma once

#include "app/PdfDocumentBinding.h"
#include "selection/QtPdfTextSource.h"
#include "selection/Types.h"

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QPointF>
#include <QVariantList>
#include <QVariantMap>

class TextSelectionModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QObject *document READ document WRITE setDocument NOTIFY
                 documentChanged)
  Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectionChanged)
  Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
  Q_PROPERTY(int selectionPage READ selectionPage NOTIFY selectionChanged)
  Q_PROPERTY(QVariantMap spanPages READ spanPages NOTIFY selectionChanged)
  Q_PROPERTY(bool spanActive READ spanActive NOTIFY selectionChanged)
  Q_PROPERTY(QVariantList selectionGeometry READ selectionGeometry NOTIFY
                 selectionChanged)
  Q_PROPERTY(bool anchorValid READ anchorValid NOTIFY anchorChanged)
  Q_PROPERTY(int anchorPage READ anchorPage NOTIFY anchorChanged)
  Q_PROPERTY(QPointF anchorPoint READ anchorPoint NOTIFY anchorChanged)
  Q_PROPERTY(qreal renderScale READ renderScale WRITE setRenderScale NOTIFY
                 renderScaleChanged)

public:
  explicit TextSelectionModel(QObject *parent = nullptr);

  [[nodiscard]] QObject *document() const { return m_binding.source(); }
  void setDocument(QObject *document);

  [[nodiscard]] QString selectedText() const { return m_state.text; }
  [[nodiscard]] bool hasSelection() const { return !m_state.text.isEmpty(); }
  [[nodiscard]] int selectionPage() const { return m_state.anchorPage; }
  [[nodiscard]] QVariantMap spanPages() const;
  [[nodiscard]] bool spanActive() const { return m_state.multiPage(); }
  [[nodiscard]] QVariantList selectionGeometry() const;
  [[nodiscard]] bool anchorValid() const { return m_anchorValid; }
  [[nodiscard]] int anchorPage() const { return m_anchorPage; }
  [[nodiscard]] QPointF anchorPoint() const {
    return {m_anchorPoint.x, m_anchorPoint.y};
  }
  [[nodiscard]] qreal renderScale() const { return m_renderScale; }
  void setRenderScale(qreal scale);

  Q_INVOKABLE void clear();
  Q_INVOKABLE void setAnchor(int page, qreal x, qreal y);
  Q_INVOKABLE void setAnchorSnapped(int page, qreal x, qreal y);
  Q_INVOKABLE [[nodiscard]] bool updateTo(int page, qreal x, qreal y);
  Q_INVOKABLE [[nodiscard]] bool selectWordAt(int page, qreal x, qreal y);
  Q_INVOKABLE [[nodiscard]] bool selectLineAt(int page, qreal x, qreal y);
  Q_INVOKABLE [[nodiscard]] bool selectAllOnPage(int page);
  Q_INVOKABLE [[nodiscard]] QVariant captureForHighlight() const;
  Q_INVOKABLE [[nodiscard]] QPointF snapWordEdge(int page, qreal x, qreal y,
                                                 bool towardStart) const;

signals:
  void documentChanged();
  void selectionChanged();
  void anchorChanged();
  void renderScaleChanged();

private:
  void onPdfStatus(QPdfDocument::Status status);
  void applyState(omapdf::selection::SelectionState state);
  void refreshSource();
  [[nodiscard]] omapdf::selection::PagePoint pagePoint(qreal x,
                                                         qreal y) const;
  [[nodiscard]] static QVariantList boundsToVariant(
      const QList<QPolygonF> &bounds);

  PdfDocumentBinding m_binding;
  omapdf::selection::QtPdfTextSource m_source;
  omapdf::selection::SelectionState m_state;
  int m_anchorPage{-1};
  omapdf::selection::PagePoint m_anchorPoint{};
  bool m_anchorValid{false};
  qreal m_renderScale{1};
};
