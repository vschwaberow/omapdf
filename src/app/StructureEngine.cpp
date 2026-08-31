#include "app/StructureEngine.h"

#include "app/DocumentLimits.h"

#include <QColor>
#include <QPolygonF>
#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <unistd.h>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFExc.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFWriter.hh>
#include <set>
#include <QFutureWatcher>
#include <QtConcurrent>

namespace fs = std::filesystem;

namespace {

QVariantMap resultOk() {
  return QVariantMap{{QStringLiteral("ok"), true},
                     {QStringLiteral("error"), QString()}};
}

QVariantMap resultErr(const QString &message) {
  return QVariantMap{{QStringLiteral("ok"), false},
                     {QStringLiteral("error"), message}};
}

template <typename Body>
std::expected<void, QString> catchQpdf(Body &&body) {
  try {
    return body();
  } catch (QPDFExc const &ex) {
    return std::unexpected(QString::fromUtf8(ex.what()));
  } catch (std::exception const &ex) {
    return std::unexpected(QString::fromUtf8(ex.what()));
  }
}

template <typename Body>
std::expected<void, QString> withOpenedPdf(const QString &canonical,
                                          Body &&body) {
  return catchQpdf([&]() -> std::expected<void, QString> {
    QPDF pdf;
    pdf.processFile(canonical.toUtf8().constData());
    QPDFPageDocumentHelper helper(pdf);
    auto pages = helper.getAllPages();
    if (static_cast<int>(pages.size()) > omapdf::kMaxPageCount) {
      return std::unexpected(QStringLiteral("too many pages"));
    }
    return body(pdf, helper, pages);
  });
}

} // namespace

StructureEngine::StructureEngine(QObject *parent) : QObject(parent) {}


void StructureEngine::setBusy(bool busy) {
  if (m_busy == busy) {
    return;
  }
  m_busy = busy;
  emit busyChanged();
}

void StructureEngine::runAsync(std::function<QVariantMap()> job) {
  if (m_busy) {
    emit opFinished(errMap(QStringLiteral("structure op already in progress")));
    return;
  }
  setBusy(true);
  auto *watcher = new QFutureWatcher<QVariantMap>(this);
  connect(watcher, &QFutureWatcher<QVariantMap>::finished, this,
          [this, watcher]() {
            const QVariantMap result = watcher->result();
            watcher->deleteLater();
            setBusy(false);
            emit opFinished(result);
          });
  watcher->setFuture(QtConcurrent::run(std::move(job)));
}

void StructureEngine::rotateAsync(const QString &path, int page, int degrees) {
  runAsync([path, page, degrees]() {
    return StructureEngine{}.rotate(path, page, degrees);
  });
}

void StructureEngine::removePagesAsync(const QString &path,
                                       const QVariantList &pages) {
  runAsync([path, pages]() {
    return StructureEngine{}.removePages(path, pages);
  });
}

void StructureEngine::reorderAsync(const QString &path,
                                   const QVariantList &order) {
  runAsync([path, order]() {
    return StructureEngine{}.reorder(path, order);
  });
}

void StructureEngine::extractAsync(const QString &path,
                                   const QVariantList &pages,
                                   const QString &dest) {
  runAsync([path, pages, dest]() {
    return StructureEngine{}.extract(path, pages, dest);
  });
}

void StructureEngine::mergeAsync(const QStringList &sources,
                                 const QString &dest) {
  runAsync([sources, dest]() {
    return StructureEngine{}.merge(sources, dest);
  });
}

void StructureEngine::exportAnnotsAsync(const QString &path, const QString &dest,
                                        const QVariantList &annots) {
  runAsync([path, dest, annots]() {
    return StructureEngine{}.exportAnnots(path, dest, annots);
  });
}


QVariantMap StructureEngine::okMap() { return resultOk(); }

QVariantMap StructureEngine::errMap(const QString &message) {
  return resultErr(message);
}

std::expected<QString, QString>
StructureEngine::hardenLocalPath(const QString &path, bool mustExist) {
  if (path.isEmpty()) {
    return std::unexpected(QStringLiteral("empty path"));
  }
  if (path.contains(QLatin1Char('\0'))) {
    return std::unexpected(QStringLiteral("invalid path"));
  }
  const QFileInfo info(path);
  const QString canon =
      mustExist ? info.canonicalFilePath() : info.absoluteFilePath();
  if (canon.isEmpty()) {
    return std::unexpected(QStringLiteral("path not found"));
  }
  if (mustExist) {
    if (!info.isFile()) {
      return std::unexpected(QStringLiteral("not a file"));
    }
    if (info.size() > omapdf::kMaxFileBytes) {
      return std::unexpected(QStringLiteral("file too large"));
    }
  } else {
    const QFileInfo parent(QFileInfo(canon).absolutePath());
    if (!parent.exists() || !parent.isDir()) {
      return std::unexpected(QStringLiteral("destination directory missing"));
    }
  }
  return canon;
}

std::expected<std::vector<int>, QString>
StructureEngine::normalizePages(const QVariantList &pages, int pageCount) {
  if (pageCount <= 0 || pageCount > omapdf::kMaxPageCount) {
    return std::unexpected(QStringLiteral("invalid page count"));
  }
  std::set<int> unique;
  for (const QVariant &v : pages) {
    bool ok = false;
    const int page = v.toInt(&ok);
    if (!ok || page < 0 || page >= pageCount) {
      return std::unexpected(QStringLiteral("page out of range"));
    }
    unique.insert(page);
  }
  if (unique.empty()) {
    return std::unexpected(QStringLiteral("no pages"));
  }
  return std::vector<int>(unique.begin(), unique.end());
}

std::expected<void, QString> StructureEngine::writeAtomic(QPDF &pdf,
                                                         const QString &dest) {
  const fs::path destPath = dest.toStdString();
  const fs::path tmpPath =
      destPath.string() + ".omapdf-tmp-" + std::to_string(::getpid());
  try {
    QPDFWriter writer(pdf, tmpPath.string().c_str());
    writer.setStaticID(true);
    writer.setDeterministicID(true);
    writer.write();
  } catch (QPDFExc const &ex) {
    std::error_code ignore{};
    fs::remove(tmpPath, ignore);
    return std::unexpected(QString::fromUtf8(ex.what()));
  } catch (std::exception const &ex) {
    std::error_code ignore{};
    fs::remove(tmpPath, ignore);
    return std::unexpected(QString::fromUtf8(ex.what()));
  }
  std::error_code ec;
  fs::rename(tmpPath, destPath, ec);
  if (ec) {
    std::error_code ignore{};
    fs::remove(tmpPath, ignore);
    return std::unexpected(QString::fromStdString(ec.message()));
  }
  return {};
}

std::expected<void, QString> StructureEngine::writeBlankPdf(const QString &path,
                                                           int pageCount) {
  if (pageCount <= 0 || pageCount > omapdf::kMaxPageCount) {
    return std::unexpected(QStringLiteral("invalid page count"));
  }
  auto dest = hardenLocalPath(path, false);
  if (!dest) {
    return std::unexpected(dest.error());
  }
  return catchQpdf([&]() -> std::expected<void, QString> {
    QPDF pdf;
    pdf.emptyPDF();
    auto helper = QPDFPageDocumentHelper(pdf);
    for (int i = 0; i < pageCount; ++i) {
      QPDFObjectHandle page = QPDFObjectHandle::newDictionary();
      page.replaceKey("/Type", QPDFObjectHandle::newName("/Page"));
      page.replaceKey("/MediaBox",
                      QPDFObjectHandle::newArray(
                          QPDFObjectHandle::Rectangle(0, 0, 612, 792)));
      page.replaceKey("/Resources", QPDFObjectHandle::newDictionary());
      page.replaceKey("/Contents", QPDFObjectHandle::newStream(&pdf, " "));
      helper.addPage(QPDFPageObjectHelper(page), false);
    }
    return writeAtomic(pdf, *dest);
  });
}

std::expected<void, QString>
StructureEngine::rotateFile(const QString &path, int page, int degrees) {
  if (degrees % 90 != 0) {
    return std::unexpected(QStringLiteral("degrees must be multiple of 90"));
  }
  auto src = hardenLocalPath(path, true);
  if (!src) {
    return std::unexpected(src.error());
  }
  return withOpenedPdf(*src, [&](QPDF &pdf, QPDFPageDocumentHelper &,
                                 auto &pages) -> std::expected<void, QString> {
    if (page < 0 || page >= static_cast<int>(pages.size())) {
      return std::unexpected(QStringLiteral("page out of range"));
    }
    pages[static_cast<size_t>(page)].rotatePage(degrees, true);
    return writeAtomic(pdf, *src);
  });
}

std::expected<void, QString>
StructureEngine::removePagesFile(const QString &path,
                                 const std::vector<int> &pages) {
  auto src = hardenLocalPath(path, true);
  if (!src) {
    return std::unexpected(src.error());
  }
  return withOpenedPdf(
      *src, [&](QPDF &pdf, QPDFPageDocumentHelper &helper,
                auto &all) -> std::expected<void, QString> {
        const int count = static_cast<int>(all.size());
        if (pages.empty()) {
          return std::unexpected(QStringLiteral("no pages"));
        }
        if (static_cast<int>(pages.size()) >= count) {
          return std::unexpected(QStringLiteral("cannot delete all pages"));
        }
        for (auto it = pages.rbegin(); it != pages.rend(); ++it) {
          if (*it < 0 || *it >= count) {
            return std::unexpected(QStringLiteral("page out of range"));
          }
          helper.removePage(all[static_cast<size_t>(*it)]);
        }
        return writeAtomic(pdf, *src);
      });
}

std::expected<void, QString>
StructureEngine::reorderFile(const QString &path,
                             const std::vector<int> &order) {
  auto src = hardenLocalPath(path, true);
  if (!src) {
    return std::unexpected(src.error());
  }
  return withOpenedPdf(
      *src, [&](QPDF &pdf, QPDFPageDocumentHelper &helper,
                auto &all) -> std::expected<void, QString> {
        const int count = static_cast<int>(all.size());
        if (static_cast<int>(order.size()) != count) {
          return std::unexpected(QStringLiteral("order size mismatch"));
        }
        std::vector<bool> seen(static_cast<size_t>(count), false);
        std::vector<QPDFPageObjectHelper> reordered;
        reordered.reserve(static_cast<size_t>(count));
        for (int idx : order) {
          if (idx < 0 || idx >= count || seen[static_cast<size_t>(idx)]) {
            return std::unexpected(QStringLiteral("invalid order"));
          }
          seen[static_cast<size_t>(idx)] = true;
          reordered.push_back(all[static_cast<size_t>(idx)]);
        }
        for (auto it = all.rbegin(); it != all.rend(); ++it) {
          helper.removePage(*it);
        }
        for (auto &page : reordered) {
          helper.addPage(page, false);
        }
        return writeAtomic(pdf, *src);
      });
}

std::expected<void, QString>
StructureEngine::extractFile(const QString &path, const std::vector<int> &pages,
                             const QString &dest) {
  auto src = hardenLocalPath(path, true);
  if (!src) {
    return std::unexpected(src.error());
  }
  auto out = hardenLocalPath(dest, false);
  if (!out) {
    return std::unexpected(out.error());
  }
  if (*src == *out) {
    return std::unexpected(QStringLiteral("destination equals source"));
  }
  return withOpenedPdf(
      *src, [&](QPDF &, QPDFPageDocumentHelper &,
                auto &all) -> std::expected<void, QString> {
        const int count = static_cast<int>(all.size());
        auto normalized = normalizePages([&] {
          QVariantList list;
          for (int p : pages) {
            list.push_back(p);
          }
          return list;
        }(),
                                         count);
        if (!normalized) {
          return std::unexpected(normalized.error());
        }

        QPDF outPdf;
        outPdf.emptyPDF();
        auto outHelper = QPDFPageDocumentHelper(outPdf);
        for (int page : *normalized) {
          outHelper.addPage(all[static_cast<size_t>(page)], false);
        }
        return writeAtomic(outPdf, *out);
      });
}

std::expected<void, QString>
StructureEngine::mergeFiles(const std::vector<QString> &sources,
                            const QString &dest) {
  if (sources.size() < 2) {
    return std::unexpected(QStringLiteral("need at least two sources"));
  }
  auto out = hardenLocalPath(dest, false);
  if (!out) {
    return std::unexpected(out.error());
  }
  return catchQpdf([&]() -> std::expected<void, QString> {
    QPDF outPdf;
    outPdf.emptyPDF();
    auto outHelper = QPDFPageDocumentHelper(outPdf);
    int totalPages = 0;
    std::vector<std::unique_ptr<QPDF>> keepAlive;
    keepAlive.reserve(sources.size());
    for (const QString &source : sources) {
      auto src = hardenLocalPath(source, true);
      if (!src) {
        return std::unexpected(src.error());
      }
      if (*src == *out) {
        return std::unexpected(QStringLiteral("destination equals source"));
      }
      auto pdf = std::make_unique<QPDF>();
      pdf->processFile(src->toUtf8().constData());
      auto helper = QPDFPageDocumentHelper(*pdf);
      auto pages = helper.getAllPages();
      totalPages += static_cast<int>(pages.size());
      if (totalPages > omapdf::kMaxPageCount) {
        return std::unexpected(QStringLiteral("too many pages"));
      }
      for (auto &page : pages) {
        outHelper.addPage(page, false);
      }
      keepAlive.push_back(std::move(pdf));
    }
    return writeAtomic(outPdf, *out);
  });
}

QVariantMap StructureEngine::rotate(const QString &path, int page,
                                    int degrees) const {
  auto result = rotateFile(path, page, degrees);
  return result ? okMap() : errMap(result.error());
}

QVariantMap StructureEngine::removePages(const QString &path,
                                         const QVariantList &pages) const {
  std::vector<int> list;
  list.reserve(static_cast<size_t>(pages.size()));
  for (const QVariant &v : pages) {
    bool ok = false;
    const int page = v.toInt(&ok);
    if (!ok) {
      return errMap(QStringLiteral("invalid page"));
    }
    list.push_back(page);
  }
  auto result = removePagesFile(path, list);
  return result ? okMap() : errMap(result.error());
}

QVariantMap StructureEngine::reorder(const QString &path,
                                     const QVariantList &order) const {
  std::vector<int> pages;
  pages.reserve(static_cast<size_t>(order.size()));
  for (const QVariant &v : order) {
    bool ok = false;
    const int page = v.toInt(&ok);
    if (!ok) {
      return errMap(QStringLiteral("invalid order"));
    }
    pages.push_back(page);
  }
  auto result = reorderFile(path, pages);
  return result ? okMap() : errMap(result.error());
}

QVariantMap StructureEngine::extract(const QString &path,
                                     const QVariantList &pages,
                                     const QString &dest) const {
  std::vector<int> list;
  list.reserve(static_cast<size_t>(pages.size()));
  for (const QVariant &v : pages) {
    bool ok = false;
    const int page = v.toInt(&ok);
    if (!ok) {
      return errMap(QStringLiteral("invalid page"));
    }
    list.push_back(page);
  }
  auto result = extractFile(path, list, dest);
  return result ? okMap() : errMap(result.error());
}

QVariantMap StructureEngine::merge(const QStringList &sources,
                                   const QString &dest) const {
  std::vector<QString> list;
  list.reserve(static_cast<size_t>(sources.size()));
  for (const QString &s : sources) {
    list.push_back(s);
  }
  auto result = mergeFiles(list, dest);
  return result ? okMap() : errMap(result.error());
}

namespace {

QPDFObjectHandle colorArray(const QString &hex) {
  QColor c(hex);
  if (!c.isValid()) {
    c = QColor(QStringLiteral("#f6c177"));
  }
  return QPDFObjectHandle::newArray(std::vector<QPDFObjectHandle>{
      QPDFObjectHandle::newReal(c.redF()),
      QPDFObjectHandle::newReal(c.greenF()),
      QPDFObjectHandle::newReal(c.blueF()),
  });
}

QPDFObjectHandle::Rectangle boundsOf(const QList<QPolygonF> &quads, QPointF fallback) {
  if (quads.isEmpty()) {
    return QPDFObjectHandle::Rectangle(fallback.x(), fallback.y(),
                                       fallback.x() + 18, fallback.y() + 18);
  }
  qreal minX = quads.front().boundingRect().left();
  qreal minY = quads.front().boundingRect().top();
  qreal maxX = quads.front().boundingRect().right();
  qreal maxY = quads.front().boundingRect().bottom();
  for (const QPolygonF &poly : quads) {
    const QRectF b = poly.boundingRect();
    minX = std::min(minX, b.left());
    minY = std::min(minY, b.top());
    maxX = std::max(maxX, b.right());
    maxY = std::max(maxY, b.bottom());
  }
  return QPDFObjectHandle::Rectangle(minX, minY, maxX, maxY);
}

QList<QPolygonF> quadsFromItem(const QVariantMap &item) {
  QList<QPolygonF> out;
  const QVariantList quads = item.value(QStringLiteral("quads")).toList();
  for (const QVariant &qv : quads) {
    QPolygonF poly;
    for (const QVariant &pv : qv.toList()) {
      const QVariantList xy = pv.toList();
      if (xy.size() >= 2) {
        poly << QPointF(xy.at(0).toDouble(), xy.at(1).toDouble());
      }
    }
    if (!poly.isEmpty()) {
      out.push_back(poly);
    }
  }
  return out;
}

} // namespace

std::expected<void, QString>
StructureEngine::exportAnnotsFile(const QString &path, const QString &dest,
                                  const QVariantList &annots) {
  auto src = hardenLocalPath(path, true);
  if (!src) {
    return std::unexpected(src.error());
  }
  auto out = hardenLocalPath(dest, false);
  if (!out) {
    return std::unexpected(out.error());
  }
  return catchQpdf([&]() -> std::expected<void, QString> {
    QPDF pdf;
    pdf.processFile(src->toUtf8().constData());
    auto helper = QPDFPageDocumentHelper(pdf);
    auto pages = helper.getAllPages();
    const int count = static_cast<int>(pages.size());
    if (count > omapdf::kMaxPageCount) {
      return std::unexpected(QStringLiteral("too many pages"));
    }

    for (const QVariant &v : annots) {
      const QVariantMap item = v.toMap();
      bool pageOk = false;
      const int page = item.value(QStringLiteral("page")).toInt(&pageOk);
      if (!pageOk) {
        return std::unexpected(QStringLiteral("annot page missing"));
      }
      if (page < 0 || page >= count) {
        return std::unexpected(QStringLiteral("annot page out of range"));
      }
      const QString type = item.value(QStringLiteral("type")).toString();
      const QString color = item.value(QStringLiteral("color")).toString();
      const QString text = item.value(QStringLiteral("text")).toString();
      QPDFObjectHandle annot = QPDFObjectHandle::newDictionary();
      annot.replaceKey("/Type", QPDFObjectHandle::newName("/Annot"));
      annot.replaceKey("/C", colorArray(color));
      annot.replaceKey("/F", QPDFObjectHandle::newInteger(4));
      if (!text.isEmpty()) {
        annot.replaceKey("/Contents",
                         QPDFObjectHandle::newUnicodeString(text.toStdString()));
      }

      if (type == QLatin1String("highlight")) {
        const QList<QPolygonF> quads = quadsFromItem(item);
        if (quads.isEmpty()) {
          continue;
        }
        annot.replaceKey("/Subtype", QPDFObjectHandle::newName("/Highlight"));
        const auto rect = boundsOf(quads, {});
        annot.replaceKey("/Rect", QPDFObjectHandle::newArray(rect));
        std::vector<QPDFObjectHandle> qp;
        for (const QPolygonF &poly : quads) {
          QPolygonF ordered = poly;
          if (ordered.size() < 4) {
            const QRectF b = ordered.boundingRect();
            ordered = QPolygonF(QRectF(b));
          }
          const QRectF b = ordered.boundingRect();
          const QPointF ul(b.left(), b.bottom());
          const QPointF ur(b.right(), b.bottom());
          const QPointF ll(b.left(), b.top());
          const QPointF lr(b.right(), b.top());
          for (const QPointF &p : {ul, ur, ll, lr}) {
            qp.push_back(QPDFObjectHandle::newReal(p.x()));
            qp.push_back(QPDFObjectHandle::newReal(p.y()));
          }
        }
        annot.replaceKey("/QuadPoints", QPDFObjectHandle::newArray(qp));
      } else if (type == QLatin1String("note")) {
        const qreal x = item.value(QStringLiteral("x")).toDouble();
        const qreal y = item.value(QStringLiteral("y")).toDouble();
        annot.replaceKey("/Subtype", QPDFObjectHandle::newName("/Text"));
        annot.replaceKey("/Name", QPDFObjectHandle::newName("/Comment"));
        annot.replaceKey("/Rect",
                         QPDFObjectHandle::newArray(QPDFObjectHandle::Rectangle(
                             x, y, x + 18, y + 18)));
      } else {
        continue;
      }

      QPDFObjectHandle pageObj = pages[static_cast<size_t>(page)].getObjectHandle();
      if (!pageObj.hasKey("/Annots") || !pageObj.getKey("/Annots").isArray()) {
        pageObj.replaceKey("/Annots", QPDFObjectHandle::newArray());
      }
      pageObj.getKey("/Annots").appendItem(pdf.makeIndirectObject(annot));
    }

    return writeAtomic(pdf, *out);
  });
}

QVariantMap StructureEngine::exportAnnots(const QString &path, const QString &dest,
                                          const QVariantList &annots) const {
  auto result = exportAnnotsFile(path, dest, annots);
  return result ? okMap() : errMap(result.error());
}
