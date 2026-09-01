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
  void reverseSinglePageDrag();
  void zeroLengthSelection();
  void isWordCharBoundaries();
  void wordAtNbspBoundary();
  void wordAtPunctuationAttached();
  void lineAtSecondLine();
  void lineAtEmptyPage();
  void crossPageEmptyMiddle();
  void crossPageAdjacentOnly();
  void backwardCrossPageTextOrder();
  void selectAllEmptyPage();
  void clampZeroPageSize();
  void fromHitValidAndInvalid();
  void snapWordEdgeNoText();
  void spanClampsOffPagePoints();
  void wordAtTabSeparated();
  void wordAtOnSpaceExpands();
  void wordAtHyphenated();
  void wordAtFirstAndLastGlyph();
  void wordAtEmptyPage();
  void lineAtCrlfSecondLine();
  void lineAtOutOfRange();
  void lineAtLastLineNoTrailingNewline();
  void crossPageBackwardThreePages();
  void spanMiddlePageOnly();
  void spanEmptyPageKeepsPageRange();
  void fromHitSetsAnchorPage();
  void selectAllSingleChar();
  void snapWordEdgeSecondWord();
  void clampWithinBoundsUnchanged();
  void spanFullPageViaCorners();
  void spanMultiLineSamePage();
  void spanMultiLineReverseMatchesForward();
  void lineAtSingleLineDocument();
  void wordAtUnicodeLetters();
  void isWordCharUnicodeLetters();
  void snapWordEdgeOutOfRangePage();
  void wordAtOnTabExpands();
  void crossPageAllEmptyPages();
  void selectAllMiddlePageOfDocument();
  void fromHitPreservesBoundingRect();
  void clampHeightZeroWidthNormal();
  void spanCrossPageMiddlePagesOnly();
  void wordAtSingleCharBetweenSpaces();
  void wordAtNegativePageIndex();
  void selectPageAllBeyondLastPage();
  void spanPartialThenFullNextPage();
  void lineAtFirstLineOfMany();
  void snapWordEdgeYIsWordCenter();
  void lineAtNegativePageIndex();
  void wordAtApostropheContraction();
  void wordAtDigitRun();
  void spanCrossPageReverseMiddlePagesOnly();
  void spanPerPageTextPopulated();
  void spanMultiLinePartialMiddle();
  void lineAtThirdLineOfMany();
  void wordAtDoubleSpaceBetweenWords();
  void clampExceedsBothAxes();
  void spanSingleCharPagesCrossDocument();
  void crossPageForwardPreservesPageIndices();
  void fromHitCopiesBounds();
  void selectPageAllSetsToCorner();
  void isWordCharApostropheAndDigits();
  void selectionStateDefaultsEmpty();
  void spanBackwardMiddlePagesOnly();
  void wordAtOutOfRangeHighPage();
  void snapWordEdgeNegativePageIndex();
  void spanMultiLineIncludesTrailingNewline();
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
      omapdf::selection::wordAt(0, {45, 6}, src);
  QCOMPARE(word.text, QStringLiteral("two"));
  const SelectionState line =
      omapdf::selection::lineAt(0, {5, 6}, src);
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
  const PagePoint mid{45, 6};
  const PagePoint start =
      omapdf::selection::snapWordEdge(0, mid, true, src);
  const PagePoint end =
      omapdf::selection::snapWordEdge(0, mid, false, src);
  QVERIFY(start.x < end.x);
  QCOMPARE(start.x, 0.0);
  QCOMPARE(end.x, 50.0);
}

void TextSelectionTest::reverseSinglePageDrag() {
  FakePdfTextSource src;
  src.addPage({200, 100}, QStringLiteral("hello world"));
  const SelectionState forward =
      omapdf::selection::span(0, {5, 6}, 0, {45, 6}, src);
  const SelectionState reverse =
      omapdf::selection::span(0, {45, 6}, 0, {5, 6}, src);
  QCOMPARE(reverse.text, forward.text);
  QCOMPARE(reverse.text, QStringLiteral("hello"));
}

void TextSelectionTest::zeroLengthSelection() {
  FakePdfTextSource src;
  src.addPage({200, 100}, QStringLiteral("hello"));
  const SelectionState state =
      omapdf::selection::span(0, {5, 6}, 0, {5, 6}, src);
  QCOMPARE(state.text, QStringLiteral("h"));
  QCOMPARE(state.pages.size(), size_t{1});
}

void TextSelectionTest::isWordCharBoundaries() {
  using omapdf::selection::isWordChar;
  QVERIFY(isWordChar(QLatin1Char('z')));
  QVERIFY(isWordChar(QLatin1Char('-')));
  QVERIFY(isWordChar(QLatin1Char(',')));
  QVERIFY(!isWordChar(QLatin1Char(' ')));
  QVERIFY(!isWordChar(QLatin1Char('\t')));
  QVERIFY(!isWordChar(QLatin1Char('\n')));
  QVERIFY(!isWordChar(QLatin1Char('\r')));
  QVERIFY(!isWordChar(QChar(0x00A0)));
  QVERIFY(!isWordChar(QChar()));
}

void TextSelectionTest::wordAtNbspBoundary() {
  FakePdfTextSource src;
  src.addPage({200, 50},
              QStringLiteral("a") + QChar(0x00A0) + QStringLiteral("b"));
  const SelectionState left = omapdf::selection::wordAt(0, {5, 6}, src);
  const SelectionState right = omapdf::selection::wordAt(0, {25, 6}, src);
  QCOMPARE(left.text, QStringLiteral("a"));
  QCOMPARE(right.text, QStringLiteral("b"));
}

void TextSelectionTest::wordAtPunctuationAttached() {
  FakePdfTextSource src;
  src.addPage({300, 50}, QStringLiteral("one,two"));
  const SelectionState word = omapdf::selection::wordAt(0, {25, 6}, src);
  QCOMPARE(word.text, QStringLiteral("one,two"));
}

void TextSelectionTest::lineAtSecondLine() {
  FakePdfTextSource src;
  src.addPage({400, 100}, QStringLiteral("first\nsecond"));
  const SelectionState line =
      omapdf::selection::lineAt(0, {5, kGlyphH + 6}, src);
  QCOMPARE(line.text, QStringLiteral("second"));
}

void TextSelectionTest::lineAtEmptyPage() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QString());
  QVERIFY(omapdf::selection::lineAt(0, {5, 6}, src).empty());
}

void TextSelectionTest::crossPageEmptyMiddle() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QString());
  src.addPage({200, 50}, QStringLiteral("ccc"));
  const SelectionState state =
      omapdf::selection::span(0, {5, 6}, 2, {15, 6}, src);
  QCOMPARE(state.pages.size(), size_t{3});
  QCOMPARE(state.text, QStringLiteral("aaa\ncc"));
}

void TextSelectionTest::crossPageAdjacentOnly() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aa"));
  src.addPage({200, 50}, QStringLiteral("bb"));
  const SelectionState state =
      omapdf::selection::span(0, {15, 6}, 1, {5, 6}, src);
  QCOMPARE(state.pages.size(), size_t{2});
  QCOMPARE(state.text, QStringLiteral("a\nb"));
}

void TextSelectionTest::backwardCrossPageTextOrder() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  const SelectionState state =
      omapdf::selection::span(1, {15, 6}, 0, {5, 6}, src);
  QCOMPARE(state.text, QStringLiteral("aaa\nbb"));
}

void TextSelectionTest::selectAllEmptyPage() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QString());
  QVERIFY(omapdf::selection::selectPageAll(0, src).empty());
}

void TextSelectionTest::clampZeroPageSize() {
  const PagePoint clamped = omapdf::selection::clamp({-3, 5}, {0, 0});
  QCOMPARE(clamped.x, 0.0);
  QCOMPARE(clamped.y, 1.0);
}

void TextSelectionTest::fromHitValidAndInvalid() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("abc"));
  const TextHit hit = src.selection(0, {5, 6}, {15, 6});
  const SelectionState state = omapdf::selection::fromHit(0, hit);
  QCOMPARE(state.text, QStringLiteral("ab"));
  QCOMPARE(state.pages.size(), size_t{1});
  QCOMPARE(state.pages[0].page, 0);
  QVERIFY(omapdf::selection::fromHit(0, TextHit{}).empty());
}

void TextSelectionTest::snapWordEdgeNoText() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QString());
  const PagePoint pt{50, 25};
  const PagePoint snapped =
      omapdf::selection::snapWordEdge(0, pt, true, src);
  QCOMPARE(snapped.x, pt.x);
  QCOMPARE(snapped.y, pt.y);
}

void TextSelectionTest::spanClampsOffPagePoints() {
  FakePdfTextSource src;
  src.addPage({100, 50}, QStringLiteral("hi"));
  const SelectionState state =
      omapdf::selection::span(0, {-50, -50}, 0, {500, 500}, src);
  QCOMPARE(state.text, QStringLiteral("hi"));
  QCOMPARE(state.pages[0].from.x, 0.0);
  QCOMPARE(state.pages[0].to.x, 100.0);
}


void TextSelectionTest::wordAtTabSeparated() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("one\ttwo"));
  const SelectionState word = omapdf::selection::wordAt(0, {45, 6}, src);
  QCOMPARE(word.text, QStringLiteral("two"));
}

void TextSelectionTest::wordAtOnSpaceExpands() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("one two"));
  const SelectionState word = omapdf::selection::wordAt(0, {35, 6}, src);
  QCOMPARE(word.text, QStringLiteral("one two"));
}

void TextSelectionTest::wordAtHyphenated() {
  FakePdfTextSource src;
  src.addPage({300, 50}, QStringLiteral("ab-cd"));
  const SelectionState word = omapdf::selection::wordAt(0, {15, 6}, src);
  QCOMPARE(word.text, QStringLiteral("ab-cd"));
}

void TextSelectionTest::wordAtFirstAndLastGlyph() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("xy"));
  const SelectionState first = omapdf::selection::wordAt(0, {5, 6}, src);
  const SelectionState last = omapdf::selection::wordAt(0, {15, 6}, src);
  QCOMPARE(first.text, QStringLiteral("xy"));
  QCOMPARE(last.text, QStringLiteral("xy"));
}

void TextSelectionTest::wordAtEmptyPage() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QString());
  QVERIFY(omapdf::selection::wordAt(0, {5, 6}, src).empty());
}

void TextSelectionTest::lineAtCrlfSecondLine() {
  FakePdfTextSource src;
  src.addPage({200, 100}, QStringLiteral("a\r\nb"));
  const SelectionState line =
      omapdf::selection::lineAt(0, {5, kGlyphH * 2 + 6}, src);
  QCOMPARE(line.text, QStringLiteral("b"));
}

void TextSelectionTest::lineAtOutOfRange() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("x"));
  QVERIFY(omapdf::selection::lineAt(5, {5, 6}, src).empty());
}

void TextSelectionTest::lineAtLastLineNoTrailingNewline() {
  FakePdfTextSource src;
  src.addPage({400, 100}, QStringLiteral("top\nbottom"));
  const SelectionState line =
      omapdf::selection::lineAt(0, {5, kGlyphH + 6}, src);
  QCOMPARE(line.text, QStringLiteral("bottom"));
}

void TextSelectionTest::crossPageBackwardThreePages() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  src.addPage({200, 50}, QStringLiteral("ccc"));
  const SelectionState state =
      omapdf::selection::span(2, {15, 6}, 0, {5, 6}, src);
  QCOMPARE(state.pages.size(), size_t{3});
  QCOMPARE(state.text, QStringLiteral("aaa\nbbb\ncc"));
  QCOMPARE(state.anchorPage, 2);
}

void TextSelectionTest::spanMiddlePageOnly() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  src.addPage({200, 50}, QStringLiteral("ccc"));
  const SelectionState state =
      omapdf::selection::span(1, {5, 6}, 1, {15, 6}, src);
  QCOMPARE(state.pages.size(), size_t{1});
  QVERIFY(!state.multiPage());
  QCOMPARE(state.text, QStringLiteral("bb"));
  QCOMPARE(state.pages[0].page, 1);
}

void TextSelectionTest::spanEmptyPageKeepsPageRange() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QString());
  const SelectionState state =
      omapdf::selection::span(0, {1, 1}, 0, {2, 2}, src);
  QVERIFY(state.text.isEmpty());
  QCOMPARE(state.pages.size(), size_t{1});
  QVERIFY(!state.empty());
}

void TextSelectionTest::fromHitSetsAnchorPage() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("x"));
  const TextHit hit = src.selection(0, {5, 6}, {5, 6});
  const SelectionState state = omapdf::selection::fromHit(3, hit);
  QCOMPARE(state.anchorPage, 3);
}

void TextSelectionTest::selectAllSingleChar() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("z"));
  const SelectionState all = omapdf::selection::selectPageAll(0, src);
  QCOMPARE(all.text, QStringLiteral("z"));
  QCOMPARE(all.anchorPage, 0);
}

void TextSelectionTest::snapWordEdgeSecondWord() {
  FakePdfTextSource src;
  src.addPage({400, 50}, QStringLiteral("alpha beta"));
  const PagePoint mid{75, 6};
  const PagePoint start =
      omapdf::selection::snapWordEdge(0, mid, true, src);
  const PagePoint end =
      omapdf::selection::snapWordEdge(0, mid, false, src);
  QCOMPARE(start.x, 60.0);
  QCOMPARE(end.x, 100.0);
}

void TextSelectionTest::clampWithinBoundsUnchanged() {
  FakePdfTextSource src;
  src.addPage({100, 50}, QStringLiteral("x"));
  const PageSize sz = src.pageSize(0);
  const PagePoint pt{40, 25};
  const PagePoint clamped = omapdf::selection::clamp(pt, sz);
  QCOMPARE(clamped.x, pt.x);
  QCOMPARE(clamped.y, pt.y);
}

void TextSelectionTest::spanFullPageViaCorners() {
  FakePdfTextSource src;
  src.addPage({100, 50}, QStringLiteral("full"));
  const SelectionState state =
      omapdf::selection::span(0, {0, 0}, 0, {100, 50}, src);
  QCOMPARE(state.text, QStringLiteral("full"));
  QCOMPARE(state.pages[0].from.x, 0.0);
  QCOMPARE(state.pages[0].to.x, 100.0);
}


void TextSelectionTest::spanMultiLineSamePage() {
  FakePdfTextSource src;
  src.addPage({200, 100}, QStringLiteral("ab\ncd"));
  const SelectionState state =
      omapdf::selection::span(0, {5, 6}, 0, {5, kGlyphH + 6}, src);
  QCOMPARE(state.pages.size(), size_t{1});
  QCOMPARE(state.text, QStringLiteral("ab\nc"));
}

void TextSelectionTest::spanMultiLineReverseMatchesForward() {
  FakePdfTextSource src;
  src.addPage({200, 100}, QStringLiteral("ab\ncd"));
  const SelectionState forward =
      omapdf::selection::span(0, {5, 6}, 0, {5, kGlyphH + 6}, src);
  const SelectionState reverse =
      omapdf::selection::span(0, {5, kGlyphH + 6}, 0, {5, 6}, src);
  QCOMPARE(reverse.text, forward.text);
}

void TextSelectionTest::lineAtSingleLineDocument() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("only"));
  const SelectionState line = omapdf::selection::lineAt(0, {25, 6}, src);
  QCOMPARE(line.text, QStringLiteral("only"));
}

void TextSelectionTest::wordAtUnicodeLetters() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("café"));
  const SelectionState word = omapdf::selection::wordAt(0, {25, 6}, src);
  QCOMPARE(word.text, QStringLiteral("café"));
}

void TextSelectionTest::isWordCharUnicodeLetters() {
  using omapdf::selection::isWordChar;
  QVERIFY(isWordChar(QChar(0x00E4)));
  QVERIFY(isWordChar(QChar(0x4E16)));
  QVERIFY(isWordChar(QChar(0x200B)));
}

void TextSelectionTest::snapWordEdgeOutOfRangePage() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("abc"));
  const PagePoint pt{12, 6};
  const PagePoint snapped =
      omapdf::selection::snapWordEdge(9, pt, true, src);
  QCOMPARE(snapped.x, pt.x);
  QCOMPARE(snapped.y, pt.y);
}

void TextSelectionTest::wordAtOnTabExpands() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("one\ttwo"));
  const SelectionState word = omapdf::selection::wordAt(0, {35, 6}, src);
  QCOMPARE(word.text, QStringLiteral("one\ttwo"));
}

void TextSelectionTest::crossPageAllEmptyPages() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QString());
  src.addPage({200, 50}, QString());
  src.addPage({200, 50}, QString());
  const SelectionState state =
      omapdf::selection::span(0, {1, 1}, 2, {2, 2}, src);
  QCOMPARE(state.pages.size(), size_t{3});
  QVERIFY(state.text.isEmpty());
  QVERIFY(!state.empty());
}

void TextSelectionTest::selectAllMiddlePageOfDocument() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  src.addPage({200, 50}, QStringLiteral("ccc"));
  const SelectionState all = omapdf::selection::selectPageAll(1, src);
  QCOMPARE(all.text, QStringLiteral("bbb"));
  QCOMPARE(all.anchorPage, 1);
}

void TextSelectionTest::fromHitPreservesBoundingRect() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("abcd"));
  const TextHit hit = src.selection(0, {5, 6}, {15, 6});
  const SelectionState state = omapdf::selection::fromHit(0, hit);
  QCOMPARE(state.pages[0].from.x, 0.0);
  QCOMPARE(state.pages[0].to.x, 20.0);
  QCOMPARE(state.pages[0].from.y, 0.0);
  QCOMPARE(state.pages[0].to.y, 12.0);
}

void TextSelectionTest::clampHeightZeroWidthNormal() {
  const PagePoint clamped = omapdf::selection::clamp({150, -3}, {100, 0});
  QCOMPARE(clamped.x, 100.0);
  QCOMPARE(clamped.y, 0.0);
}

void TextSelectionTest::spanCrossPageMiddlePagesOnly() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  src.addPage({200, 50}, QStringLiteral("ccc"));
  const SelectionState state =
      omapdf::selection::span(1, {5, 6}, 2, {15, 6}, src);
  QCOMPARE(state.pages.size(), size_t{2});
  QCOMPARE(state.text, QStringLiteral("bbb\ncc"));
}

void TextSelectionTest::wordAtSingleCharBetweenSpaces() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral(" a "));
  const SelectionState word = omapdf::selection::wordAt(0, {15, 6}, src);
  QCOMPARE(word.text, QStringLiteral("a"));
}

void TextSelectionTest::wordAtNegativePageIndex() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("x"));
  QVERIFY(omapdf::selection::wordAt(-1, {5, 6}, src).empty());
}

void TextSelectionTest::selectPageAllBeyondLastPage() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("x"));
  QVERIFY(omapdf::selection::selectPageAll(3, src).empty());
}

void TextSelectionTest::spanPartialThenFullNextPage() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bb"));
  const SelectionState state =
      omapdf::selection::span(0, {15, 6}, 1, {25, 6}, src);
  QCOMPARE(state.pages.size(), size_t{2});
  QCOMPARE(state.text, QStringLiteral("aa\nbb"));
}

void TextSelectionTest::lineAtFirstLineOfMany() {
  FakePdfTextSource src;
  src.addPage({400, 100}, QStringLiteral("first\nsecond\nthird"));
  const SelectionState line = omapdf::selection::lineAt(0, {25, 6}, src);
  QCOMPARE(line.text, QStringLiteral("first"));
}

void TextSelectionTest::snapWordEdgeYIsWordCenter() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("ab"));
  const PagePoint edge =
      omapdf::selection::snapWordEdge(0, {5, 2}, true, src);
  QCOMPARE(edge.y, 6.0);
}


void TextSelectionTest::lineAtNegativePageIndex() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("x"));
  QVERIFY(omapdf::selection::lineAt(-1, {5, 6}, src).empty());
}

void TextSelectionTest::wordAtApostropheContraction() {
  FakePdfTextSource src;
  src.addPage({300, 50}, QStringLiteral("don't"));
  const SelectionState word = omapdf::selection::wordAt(0, {35, 6}, src);
  QCOMPARE(word.text, QStringLiteral("don't"));
}

void TextSelectionTest::wordAtDigitRun() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("12345"));
  const SelectionState word = omapdf::selection::wordAt(0, {25, 6}, src);
  QCOMPARE(word.text, QStringLiteral("12345"));
}

void TextSelectionTest::spanCrossPageReverseMiddlePagesOnly() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  src.addPage({200, 50}, QStringLiteral("ccc"));
  const SelectionState state =
      omapdf::selection::span(2, {15, 6}, 1, {5, 6}, src);
  QCOMPARE(state.pages.size(), size_t{2});
  QCOMPARE(state.text, QStringLiteral("bbb\ncc"));
  QCOMPARE(state.pages[0].page, 1);
  QCOMPARE(state.pages[1].page, 2);
}

void TextSelectionTest::spanPerPageTextPopulated() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  src.addPage({200, 50}, QStringLiteral("ccc"));
  const SelectionState state =
      omapdf::selection::span(0, {5, 6}, 2, {15, 6}, src);
  QCOMPARE(state.pages[0].text, QStringLiteral("aaa"));
  QCOMPARE(state.pages[1].text, QStringLiteral("bbb"));
  QCOMPARE(state.pages[2].text, QStringLiteral("cc"));
}

void TextSelectionTest::spanMultiLinePartialMiddle() {
  FakePdfTextSource src;
  src.addPage({200, 100}, QStringLiteral("aa\nbb\ncc"));
  const SelectionState state =
      omapdf::selection::span(0, {5, kGlyphH + 6}, 0, {15, kGlyphH + 6}, src);
  QCOMPARE(state.text, QStringLiteral("bb"));
}

void TextSelectionTest::lineAtThirdLineOfMany() {
  FakePdfTextSource src;
  src.addPage({400, 120}, QStringLiteral("first\nsecond\nthird"));
  const SelectionState line =
      omapdf::selection::lineAt(0, {5, kGlyphH * 2 + 6}, src);
  QCOMPARE(line.text, QStringLiteral("third"));
}

void TextSelectionTest::wordAtDoubleSpaceBetweenWords() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("a  b"));
  const SelectionState word = omapdf::selection::wordAt(0, {5, 6}, src);
  QCOMPARE(word.text, QStringLiteral("a"));
}

void TextSelectionTest::clampExceedsBothAxes() {
  FakePdfTextSource src;
  src.addPage({100, 50}, QStringLiteral("x"));
  const PagePoint clamped =
      omapdf::selection::clamp({500, 500}, src.pageSize(0));
  QCOMPARE(clamped.x, 100.0);
  QCOMPARE(clamped.y, 50.0);
}

void TextSelectionTest::spanSingleCharPagesCrossDocument() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("x"));
  src.addPage({200, 50}, QStringLiteral("y"));
  src.addPage({200, 50}, QStringLiteral("z"));
  const SelectionState state =
      omapdf::selection::span(0, {5, 6}, 2, {5, 6}, src);
  QCOMPARE(state.text, QStringLiteral("x\ny\nz"));
}

void TextSelectionTest::crossPageForwardPreservesPageIndices() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aa"));
  src.addPage({200, 50}, QStringLiteral("bb"));
  const SelectionState state =
      omapdf::selection::span(0, {5, 6}, 1, {5, 6}, src);
  QCOMPARE(state.pages[0].page, 0);
  QCOMPARE(state.pages[1].page, 1);
}

void TextSelectionTest::fromHitCopiesBounds() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("ab"));
  const TextHit hit = src.selection(0, {5, 6}, {15, 6});
  const SelectionState state = omapdf::selection::fromHit(0, hit);
  QVERIFY(!state.pages[0].bounds.isEmpty());
  QCOMPARE(state.pages[0].bounds.size(), hit.bounds.size());
}

void TextSelectionTest::selectPageAllSetsToCorner() {
  FakePdfTextSource src;
  src.addPage({100, 50}, QStringLiteral("hi"));
  const SelectionState all = omapdf::selection::selectPageAll(0, src);
  QCOMPARE(all.pages[0].to.x, 100.0);
  QCOMPARE(all.pages[0].to.y, 50.0);
}

void TextSelectionTest::isWordCharApostropheAndDigits() {
  using omapdf::selection::isWordChar;
  QVERIFY(isWordChar(QLatin1Char('\'')));
  QVERIFY(isWordChar(QLatin1Char('7')));
}

void TextSelectionTest::selectionStateDefaultsEmpty() {
  SelectionState state;
  QVERIFY(state.empty());
  QVERIFY(!state.multiPage());
  QCOMPARE(state.anchorPage, -1);
}

void TextSelectionTest::spanBackwardMiddlePagesOnly() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("aaa"));
  src.addPage({200, 50}, QStringLiteral("bbb"));
  src.addPage({200, 50}, QStringLiteral("ccc"));
  const SelectionState state =
      omapdf::selection::span(2, {5, 6}, 1, {25, 6}, src);
  QCOMPARE(state.pages.size(), size_t{2});
  QCOMPARE(state.text, QStringLiteral("b\nc"));
}

void TextSelectionTest::wordAtOutOfRangeHighPage() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("x"));
  QVERIFY(omapdf::selection::wordAt(99, {5, 6}, src).empty());
}

void TextSelectionTest::snapWordEdgeNegativePageIndex() {
  FakePdfTextSource src;
  src.addPage({200, 50}, QStringLiteral("ab"));
  const PagePoint pt{5, 6};
  const PagePoint snapped =
      omapdf::selection::snapWordEdge(-1, pt, false, src);
  QCOMPARE(snapped.x, pt.x);
  QCOMPARE(snapped.y, pt.y);
}

void TextSelectionTest::spanMultiLineIncludesTrailingNewline() {
  FakePdfTextSource src;
  src.addPage({200, 100}, QStringLiteral("ab\ncd"));
  const SelectionState state =
      omapdf::selection::span(0, {5, 6}, 0, {15, kGlyphH + 6}, src);
  QCOMPARE(state.text, QStringLiteral("ab\ncd"));
}

QTEST_MAIN(TextSelectionTest)
#include "test_text_selection.moc"
