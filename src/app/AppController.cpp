#include "app/AppController.h"

#include "app/LinkGuard.h"

#include "app/SessionStore.h"

#include "app/DocumentLimits.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QSet>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImage>
#include <QPdfDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QUrl>
#include <QVector>
#include <QtConcurrent>

#include <array>
#include <memory>

AppController::AppController(SessionStore *store, bool verbose, QObject *parent)
    : QObject(parent), m_store(store), m_verbose(verbose) {
  m_docChangeDebounce.setSingleShot(true);
  m_docChangeDebounce.setInterval(400);
  QObject::connect(&m_docWatcher, &QFileSystemWatcher::fileChanged, this,
                   [this](const QString &path) {
                     m_pendingChangedPath = path;
                     m_docChangeDebounce.start();
                   });
  QObject::connect(&m_docChangeDebounce, &QTimer::timeout, this, [this]() {
    const QString path = m_pendingChangedPath;
    if (path.isEmpty()) {
      return;
    }
    armDocumentWatch(path);
    emit documentFileChanged(path);
  });
}

void AppController::setCurrentIndex(int index) {
  if (index == m_currentIndex) {
    return;
  }
  if (index < -1 || index >= m_tabs.rowCount()) {
    return;
  }
  m_currentIndex = index;
  emit currentIndexChanged();
}

QStringList AppController::recents() const {
  return m_store ? m_store->recents() : QStringList{};
}

QString AppController::canonicalLocalFile(const QString &path) {
  return QFileInfo(path).canonicalFilePath();
}

QString AppController::localPath(const QUrl &url) const {
  if (url.isLocalFile()) {
    return url.toLocalFile();
  }
  if (url.scheme().isEmpty() || url.scheme() == QLatin1String("file")) {
    return url.path();
  }
  return {};
}

QUrl AppController::fileUrl(const QString &path) const {
  return QUrl::fromLocalFile(path);
}

void AppController::appendTab(const QString &path) {
  const QString canon = canonicalLocalFile(path);
  if (canon.isEmpty() || !QFileInfo::exists(canon)) {
    emit openFailed(path, QCoreApplication::translate("AppController",
                                           "file not found"));
    return;
  }
  const QFileInfo info(canon);
  if (!canon.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
    emit openFailed(path, QCoreApplication::translate("AppController",
                                                      "not a PDF"));
    return;
  }
  if (info.size() > omapdf::kMaxFileBytes) {
    emit openFailed(path, QCoreApplication::translate("AppController",
                                           "file too large"));
    return;
  }
  {
    QFile probe(canon);
    std::array<char, 5> magic{};
    if (!probe.open(QIODevice::ReadOnly)
        || probe.read(magic.data(), static_cast<qint64>(magic.size())) < 5
        || qstrncmp(magic.data(), "%PDF-", 5) != 0) {
      emit openFailed(path, QCoreApplication::translate("AppController",
                                                        "not a PDF"));
      return;
    }
  }
  const int existing = m_tabs.indexOfPath(canon);
  if (existing >= 0) {
    setCurrentIndex(existing);
    return;
  }
  const int row = m_tabs.append(canon, QFileInfo(canon).fileName());
  emit tabsChanged();
  syncDocumentWatches();
  setCurrentIndex(row);
}

void AppController::openPaths(const QStringList &paths) {
  for (const QString &path : paths) {
    appendTab(path);
  }
}

void AppController::openUrl(const QUrl &url) {
  const QString path = localPath(url);
  if (path.isEmpty()) {
    emit openFailed(url.toString(), QCoreApplication::translate("AppController",
                                                    "only local files"));
    return;
  }
  appendTab(path);
}

void AppController::closeTab(int index) {
  if (index < 0 || index >= m_tabs.rowCount()) {
    return;
  }
  m_tabs.removeAt(index);
  emit tabsChanged();
  syncDocumentWatches();
  if (m_tabs.rowCount() == 0) {
    setCurrentIndex(-1);
  } else if (m_currentIndex >= m_tabs.rowCount()) {
    setCurrentIndex(m_tabs.rowCount() - 1);
  } else if (m_currentIndex > index) {
    setCurrentIndex(m_currentIndex - 1);
  } else {
    emit currentIndexChanged();
  }
}

void AppController::openRecent(const QString &path) { appendTab(path); }

void AppController::noteOpened(const QString &path) {
  if (!m_store) {
    return;
  }
  const QString canon = canonicalLocalFile(path);
  if (canon.isEmpty() || !QFileInfo::exists(canon)) {
    return;
  }
  m_store->pushRecent(canon);
  emit recentsChanged();
}

void AppController::setTabTitle(int index, const QString &title) {
  if (!m_tabs.setTitle(index, title)) {
    return;
  }
  emit tabsChanged();
}

QString AppController::tabPathAt(int index) const {
  return m_tabs.pathAt(index);
}

QString AppController::tabTitleAt(int index) const {
  return m_tabs.titleAt(index);
}

QVariantMap AppController::loadState(const QString &path) const {
  return m_store ? m_store->documentState(path) : QVariantMap{};
}

void AppController::saveState(const QString &path, double zoom, int page,
                              double scrollY, bool dimmed) {
  if (m_store) {
    m_store->saveDocumentState(path, zoom, page, scrollY, dimmed);
  }
}

QString AppController::annotColor() const {
  return m_store ? m_store->annotColor() : QStringLiteral("#f6c177");
}

void AppController::setAnnotColor(const QString &color) {
  if (m_store) {
    m_store->setAnnotColor(color);
  }
}

void AppController::copyText(const QString &text) const {
  if (text.isEmpty()) {
    return;
  }
  QGuiApplication::clipboard()->setText(text);
}

void AppController::copyTextToSelection(const QString &text) const {
  if (text.isEmpty()) {
    return;
  }
  QGuiApplication::clipboard()->setText(text, QClipboard::Selection);
}

void AppController::printPdf(const QString &path) {
  if (m_printBusy) {
    emit statusMessage(QCoreApplication::translate("AppController",
                                                   "print already in progress"));
    return;
  }
  QPdfDocument doc;
  if (doc.load(path) != QPdfDocument::Error::None) {
    emit statusMessage(QCoreApplication::translate("AppController",
                                             "print failed: cannot load"));
    return;
  }
  auto printer = std::make_shared<QPrinter>(QPrinter::HighResolution);
  QPrintDialog dialog(printer.get());
  dialog.setWindowTitle(QCoreApplication::translate("AppController", "Print"));
  dialog.setMinMax(1, doc.pageCount());
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  const int fromPage = printer->fromPage() > 0 ? printer->fromPage() - 1 : 0;
  const int toPage =
      printer->toPage() > 0 ? printer->toPage() - 1 : doc.pageCount() - 1;
  const QRectF target = printer->pageRect(QPrinter::DevicePixel);
  const int pageCount = doc.pageCount();

  m_printBusy = true;
  emit statusMessage(
      QCoreApplication::translate("AppController", "printing…"));

  auto *watcher = new QFutureWatcher<QVector<QImage>>(this);
  connect(watcher, &QFutureWatcher<QVector<QImage>>::finished, this,
          [this, printer, watcher, target]() {
            const QVector<QImage> images = watcher->result();
            watcher->deleteLater();
            if (images.isEmpty()) {
              m_printBusy = false;
              emit statusMessage(QCoreApplication::translate(
                  "AppController", "print failed: cannot load"));
              return;
            }
            QPainter painter(printer.get());
            if (!painter.isActive()) {
              m_printBusy = false;
              emit statusMessage(QCoreApplication::translate(
                  "AppController", "print failed: painter"));
              return;
            }
            bool first = true;
            for (const QImage &image : images) {
              if (!first) {
                printer->newPage();
              }
              first = false;
              if (!image.isNull()) {
                painter.drawImage(target.topLeft(), image);
              }
            }
            m_printBusy = false;
            emit statusMessage(QCoreApplication::translate("AppController",
                                                           "sent to printer"));
          });

  watcher->setFuture(QtConcurrent::run(
      [path, fromPage, toPage, pageCount, target]() -> QVector<QImage> {
        QPdfDocument workerDoc;
        if (workerDoc.load(path) != QPdfDocument::Error::None) {
          return {};
        }
        QVector<QImage> images;
        images.reserve(qMax(0, toPage - fromPage + 1));
        for (int i = fromPage; i <= toPage && i < pageCount; ++i) {
          const QSizeF pagePoints = workerDoc.pagePointSize(i);
          const qreal scale =
              qMin(target.width() / pagePoints.width(),
                   target.height() / pagePoints.height());
          const QSize renderSize(qRound(pagePoints.width() * scale),
                                 qRound(pagePoints.height() * scale));
          images.push_back(workerDoc.render(i, renderSize));
        }
        return images;
      }));
}

void AppController::requestLinkConfirm(const QUrl &url) {
  m_pendingLink = url;
  emit pendingLinkChanged();
}

void AppController::setLinkGuard(LinkGuard *guard) { m_linkGuard = guard; }

void AppController::confirmPendingLink(bool accept) {
  const QUrl url = m_pendingLink;
  m_pendingLink = QUrl();
  emit pendingLinkChanged();
  if (accept && url.isValid()) {
    if (m_linkGuard) {
      m_linkGuard->openConfirmed(url);
    } else {
      QDesktopServices::openUrl(url);
    }
  }
}

void AppController::armDocumentWatch(const QString &path) {
  if (path.isEmpty() || !QFile::exists(path)) {
    return;
  }
  if (!m_docWatcher.files().contains(path)) {
    m_docWatcher.addPath(path);
  }
}

void AppController::syncDocumentWatches() {
  const QStringList watched = m_docWatcher.files();
  if (!watched.isEmpty()) {
    m_docWatcher.removePaths(watched);
  }
  for (int i = 0; i < m_tabs.rowCount(); ++i) {
    armDocumentWatch(m_tabs.pathAt(i));
  }
}
