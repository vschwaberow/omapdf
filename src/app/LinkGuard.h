#pragma once

#include <QObject>
#include <QUrl>

class AppController;

class LinkGuard : public QObject {
  Q_OBJECT

public:
  LinkGuard(AppController *controller, QObject *parent = nullptr);
  void install();
  void openConfirmed(const QUrl &url);

public slots:
  void handleUrl(const QUrl &url);

private:
  AppController *m_controller{nullptr};
};
