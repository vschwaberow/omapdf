#include "selection/Algorithms.h"
#include "selection/PdfTextSource.h"
#include "selection/Types.h"

#include <QTest>

#include <optional>
#include <utility>
#include <vector>

using omapdf::selection::PagePoint;
using omapdf::selection::PageSize;
using omapdf::selection::PdfTextSource;
using omapdf::selection::SelectionState;
using omapdf::selection::TextHit;

namespace {

constexpr double kGlyphW = 10.0;
constexpr double kGlyphH = 12.0;

struct GlyphBox {
  QChar ch;
  QRectF rect;
  int index{0};
};

class FakePdfTextSource final : public PdfTextSource {
public:
  void addPage(PageSize size, const QString &text) {
    Page page;
    page.size = size;
    int index = 0;
    double x = 0;
    double y = 0;
    for (const QChar ch : text) {
      if (ch == QLatin1Char('\n') || ch == QLatin1Char('\r')) {
        GlyphBox box;
        box.ch = ch;
        box.rect = QRectF(x, y, 0, kGlyphH);
        box.index = index++;
        page.glyphs.push_back(box);
        x = 0;
        y += kGlyphH;
        continue;
      }
      GlyphBox box;
      box.ch = ch;
      box.rect = QRectF(x, y, kGlyphW, kGlyphH);
      box.index = index++;
      page.glyphs.push_back(box);
      x += kGlyphW;
    }
    page.text = text;
    m_pages.push_back(std::move(page));
  }

  [[nodiscard]] int pageCount() const override {
    return static_cast<int>(m_pages.size());
  }

  [[nodiscard]] PageSize pageSize(int page) const override {
    if (!validPage(page)) {
      return {};
    }
    return m_pages[static_cast<size_t>(page)].size;
  }

  [[nodiscard]] TextHit selection(int page, PagePoint a,
                                    PagePoint b) const override {
    if (!validPage(page)) {
      return {};
    }
    const Page &p = m_pages[static_cast<size_t>(page)];
    const auto startOpt = indexAt(p, a);
    const auto endOpt = indexAt(p, b);
    if (!startOpt && !endOpt) {
      return {};
    }
    int start = startOpt.value_or(endOpt.value_or(0));
    int end = endOpt.value_or(startOpt.value_or(0));
    if (end < start) {
      std::swap(start, end);
    }
    end += 1;
    return hitFromRange(p, start, end - start);
  }

  [[nodiscard]] TextHit selectionAtIndex(int page, int startIndex,
                                           int maxLength) const override {
    if (!validPage(page) || maxLength <= 0) {
      return {};
    }
    return hitFromRange(m_pages[static_cast<size_t>(page)], startIndex,
                        maxLength);
  }

  [[nodiscard]] TextHit allText(int page) const override {
    if (!validPage(page)) {
      return {};
    }
    const Page &p = m_pages[static_cast<size_t>(page)];
    return hitFromRange(p, 0, static_cast<int>(p.glyphs.size()));
  }

private:
  struct Page {
    PageSize size{};
    QString text;
    std::vector<GlyphBox> glyphs;
  };

  [[nodiscard]] bool validPage(int page) const {
    return page >= 0 && page < pageCount();
  }

  [[nodiscard]] static std::optional<int> indexAt(const Page &page,
                                                  PagePoint pt) {
    for (const GlyphBox &g : page.glyphs) {
      if (g.ch == QLatin1Char('\n') || g.ch == QLatin1Char('\r')) {
        continue;
      }
      if (g.rect.contains(QPointF(pt.x, pt.y))) {
        return g.index;
      }
    }
    double best = 1e300;
    std::optional<int> bestIndex;
    for (const GlyphBox &g : page.glyphs) {
      if (g.ch == QLatin1Char('\n') || g.ch == QLatin1Char('\r')) {
        continue;
      }
      const QPointF c = g.rect.center();
      const double d =
          (c.x() - pt.x) * (c.x() - pt.x) + (c.y() - pt.y) * (c.y() - pt.y);
      if (d < best) {
        best = d;
        bestIndex = g.index;
      }
    }
    return bestIndex;
  }

  [[nodiscard]] static TextHit hitFromRange(const Page &page, int start,
                                              int length) {
    TextHit hit;
    if (start < 0 || length <= 0 ||
        start >= static_cast<int>(page.glyphs.size())) {
      return hit;
    }
    const int end =
        std::min(start + length, static_cast<int>(page.glyphs.size()));
    QString text;
    QRectF bounds;
    QList<QPolygonF> polys;
    for (int i = start; i < end; ++i) {
      const GlyphBox &g = page.glyphs[static_cast<size_t>(i)];
      text += g.ch;
      if (g.ch == QLatin1Char('\n') || g.ch == QLatin1Char('\r')) {
        continue;
      }
      if (!bounds.isValid()) {
        bounds = g.rect;
      } else {
        bounds = bounds.united(g.rect);
      }
      QPolygonF poly;
      poly << g.rect.topLeft() << g.rect.topRight() << g.rect.bottomRight()
           << g.rect.bottomLeft();
      polys.push_back(poly);
    }
    hit.valid = true;
    hit.text = text;
    hit.bounds = polys;
    hit.boundingRect = bounds;
    hit.startIndex = start;
    hit.endIndex = end;
    return hit;
  }

  std::vector<Page> m_pages;
};

} // namespace

class TextSelectionTest : public QObject {
  Q_OBJECT

private slots:
  void singlePageDrag();
  void crossPageForward();
  void crossPageBackward();
  void emptyHit();
  void wordAndLine();
  void selectAll();
  void outOfRangeSafe();
  void snapWordEdge();
};

void TextSelectionTest::singlePageDrag() {
  FakePdfTextSource src;
  src.addPage({200, 100}, QStringLiteral("hello world"));
  const SelectionState state =
      omapdf::selection::span(0, {5, 6}, 0, {45, 6}, src);
  QVERIFY(!state.empty());
  QCOMPARE(state.pages.size(), size_t{1});
  QCOMPARE(state.pages[0].page, 0);
  QCOMPARE(state.text, QStringLiteral("hello"));
}

void TextSelectionTest::crossPageForward() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  src.addPage({200, 50}, QStringLiteral("ccc"));
  const SelectionState state =
      omapdf::selection::span(0, {5, 6}, 2, {15, 6}, src);
  QCOMPARE(state.pages.size(), size_t{3});
  QVERIFY(state.multiPage());
  QCOMPARE(state.text, QStringLiteral("aaa\nbbb\ncc"));
  QCOMPARE(state.anchorPage, 0);
}

void TextSelectionTest::crossPageBackward() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  const SelectionState state =
      omapdf::selection::span(1, {15, 6}, 0, {5, 6}, src);
  QCOMPARE(state.pages.size(), size_t{2});
  QCOMPARE(state.anchorPage, 1);
  QVERIFY(state.text.contains(QStringLiteral("aa")));
  QVERIFY(state.text.contains(QStringLiteral("bb")));
}

void TextSelectionTest::emptyHit() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QString());
  const SelectionState state =
      omapdf::selection::span(0, {1, 1}, 0, {2, 2}, src);
  QVERIFY(state.text.isEmpty());
  QCOMPARE(state.pages.size(), size_t{1});
}

void TextSelectionTest::wordAndLine() {
  FakePdfTextSource src;
  src.addPage({400, 100}, QStringLiteral("one two\nthree"));
  const SelectionState word =
      omapdf::selection::wordAt(0, {45, 6}, src); // in "two"
  QCOMPARE(word.text, QStringLiteral("two"));
  const SelectionState line =
      omapdf::selection::lineAt(0, {5, 6}, src); // first line
  QCOMPARE(line.text, QStringLiteral("one two"));
}

void TextSelectionTest::selectAll() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("full page"));
  const SelectionState all = omapdf::selection::selectPageAll(0, src);
  QCOMPARE(all.text, QStringLiteral("full page"));
  QCOMPARE(all.pages.size(), size_t{1});
  QCOMPARE(all.pages[0].from.x, 0.0);
  QCOMPARE(all.pages[0].to.x, 200.0);
}

void TextSelectionTest::outOfRangeSafe() {
  FakePdfTextSource src;
  src.addPage({100, 50}, QStringLiteral("x"));
  QVERIFY(omapdf::selection::span(-1, {0, 0}, 0, {1, 1}, src).empty());
  QVERIFY(omapdf::selection::span(0, {0, 0}, 9, {1, 1}, src).empty());
  QVERIFY(omapdf::selection::wordAt(3, {0, 0}, src).empty());
  QVERIFY(omapdf::selection::selectPageAll(-1, src).empty());
  const PagePoint clamped =
      omapdf::selection::clamp({-5, 999}, src.pageSize(0));
  QCOMPARE(clamped.x, 0.0);
  QCOMPARE(clamped.y, 50.0);
}

void TextSelectionTest::snapWordEdge() {
  FakePdfTextSource src;
  src.addPage({400, 50}, QStringLiteral("alpha beta"));
  const PagePoint mid{45, 6}; // inside "alpha" (0-50)
  const PagePoint start =
      omapdf::selection::snapWordEdge(0, mid, true, src);
  const PagePoint end =
      omapdf::selection::snapWordEdge(0, mid, false, src);
  QVERIFY(start.x < end.x);
  QCOMPARE(start.x, 0.0);
  QCOMPARE(end.x, 50.0);
}

QTEST_MAIN(TextSelectionTest)
#include "test_text_selection.moc"
