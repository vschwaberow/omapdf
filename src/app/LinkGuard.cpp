#include "app/LinkGuard.h"

#include "app/AppController.h"

#include <QDesktopServices>

LinkGuard::LinkGuard(AppController *controller, QObject *parent)
    : QObject(parent), m_controller(controller) {}

void LinkGuard::install() {
  QDesktopServices::setUrlHandler(QStringLiteral("http"), this, "handleUrl");
  QDesktopServices::setUrlHandler(QStringLiteral("https"), this, "handleUrl");
  QDesktopServices::setUrlHandler(QStringLiteral("mailto"), this, "handleUrl");
}

void LinkGuard::openConfirmed(const QUrl &url) {
  if (!url.isValid()) {
    return;
  }
  const QString scheme = url.scheme();
  QDesktopServices::unsetUrlHandler(scheme);
  QDesktopServices::openUrl(url);
  install();
}

void LinkGuard::handleUrl(const QUrl &url) {
  if (m_controller) {
    m_controller->requestLinkConfirm(url);
  }
}
