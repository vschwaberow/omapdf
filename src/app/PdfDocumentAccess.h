#pragma once

#include <QObject>
#include <QPdfDocument>

[[nodiscard]] inline QPdfDocument *pdfDocumentFrom(QObject *object) {
  if (object == nullptr) {
    return nullptr;
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
  return nested.isEmpty() ? nullptr : nested.constFirst();
}
