#pragma once

#include "app/PdfDocumentAccess.h"
#include "app/ScopedConnection.h"

#include <QObject>
#include <QPdfDocument>
#include <QPointer>

class PdfDocumentBinding {
public:
  void clear() {
    m_status.reset();
    m_pdf.clear();
    m_source.clear();
  }

  template <typename Receiver>
  void bind(QObject *source, Receiver *receiver,
            void (Receiver::*slot)(QPdfDocument::Status)) {
    clear();
    m_source = source;
    if (m_source.isNull()) {
      return;
    }
    m_pdf = pdfDocumentPtr(m_source.data());
    if (m_pdf.isNull()) {
      return;
    }
    m_status.reset(QObject::connect(m_pdf.data(), &QPdfDocument::statusChanged,
                                    receiver, slot));
  }

  [[nodiscard]] QObject *source() const noexcept { return m_source.data(); }
  [[nodiscard]] QPdfDocument *document() const noexcept { return m_pdf.data(); }
  [[nodiscard]] bool isNull() const noexcept { return m_pdf.isNull(); }
  [[nodiscard]] bool isReady() const {
    return !m_pdf.isNull() &&
           m_pdf->status() == QPdfDocument::Status::Ready;
  }

private:
  QPointer<QObject> m_source;
  QPointer<QPdfDocument> m_pdf;
  ScopedConnection m_status;
};
