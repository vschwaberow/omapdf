#pragma once

#include <QObject>
#include <QPdfDocument>
#include <QPointer>

[[nodiscard]] inline QPointer<QPdfDocument> pdfDocumentPtr(QObject *object) {
  if (object == nullptr) {
    return {};
  }
  const QPointer<QObject> owner(object);
  if (owner.isNull()) {
    return {};
  }
  if (auto *doc = qobject_cast<QPdfDocument *>(object)) {
    return doc;
  }
  const auto direct =
      object->findChildren<QPdfDocument *>(Qt::FindDirectChildrenOnly);
  if (!direct.isEmpty()) {
    return direct.constFirst();
  }
  const auto nested = object->findChildren<QPdfDocument *>();
  if (nested.isEmpty()) {
    return {};
  }
  return nested.constFirst();
}
