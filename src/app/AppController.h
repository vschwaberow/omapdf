#pragma once

#include "app/TabModel.h"

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

class SessionStore;
class ThemeBridge;
class LinkGuard;

class AppController : public QObject {
  Q_OBJECT
  Q_PROPERTY(TabModel *tabs READ tabs CONSTANT)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                 currentIndexChanged)
  Q_PROPERTY(QStringList recents READ recents NOTIFY recentsChanged)
  Q_PROPERTY(bool verbose READ verbose CONSTANT)
  Q_PROPERTY(QUrl pendingLink READ pendingLink NOTIFY pendingLinkChanged)

public:
  AppController(SessionStore *store, bool verbose, QObject *parent = nullptr);

  [[nodiscard]] TabModel *tabs() { return &m_tabs; }
  [[nodiscard]] const TabModel *tabs() const { return &m_tabs; }
  [[nodiscard]] int currentIndex() const { return m_currentIndex; }
  void setCurrentIndex(int index);
  [[nodiscard]] QStringList recents() const;
  [[nodiscard]] bool verbose() const { return m_verbose; }
  [[nodiscard]] QUrl pendingLink() const { return m_pendingLink; }

  Q_INVOKABLE void openPaths(const QStringList &paths);
  Q_INVOKABLE void openUrl(const QUrl &url);
  Q_INVOKABLE void closeTab(int index);
  Q_INVOKABLE void openRecent(const QString &path);
  Q_INVOKABLE void setTabTitle(int index, const QString &title);
  Q_INVOKABLE void noteOpened(const QString &path);
  Q_INVOKABLE QVariantMap loadState(const QString &path) const;
  Q_INVOKABLE void saveState(const QString &path, double zoom, int page,
                             double scrollY, bool dimmed);
  Q_INVOKABLE QString annotColor() const;
  Q_INVOKABLE void setAnnotColor(const QString &color);
  Q_INVOKABLE void printPdf(const QString &path);
  Q_INVOKABLE void copyText(const QString &text) const;
  Q_INVOKABLE void copyTextToSelection(const QString &text) const;
  Q_INVOKABLE void confirmPendingLink(bool accept);
  Q_INVOKABLE QString localPath(const QUrl &url) const;
  Q_INVOKABLE QUrl fileUrl(const QString &path) const;
  Q_INVOKABLE QString tabPathAt(int index) const;
  Q_INVOKABLE QString tabTitleAt(int index) const;

  void requestLinkConfirm(const QUrl &url);
  void setLinkGuard(LinkGuard *guard);

signals:
  void tabsChanged();
  void currentIndexChanged();
  void recentsChanged();
  void pendingLinkChanged();
  void openFailed(const QString &path, const QString &reason);
  void statusMessage(const QString &message);
  void documentFileChanged(const QString &path);

private:
  void appendTab(const QString &path);
  void syncDocumentWatches();
  void armDocumentWatch(const QString &path);
  [[nodiscard]] static QString canonicalLocalFile(const QString &path);

  SessionStore *m_store{nullptr};
  bool m_verbose{false};
  bool m_printBusy{false};
  TabModel m_tabs;
  int m_currentIndex{-1};
  QUrl m_pendingLink;
  LinkGuard *m_linkGuard{nullptr};
  QFileSystemWatcher m_docWatcher;
  QTimer m_docChangeDebounce;
  QString m_pendingChangedPath;
};
