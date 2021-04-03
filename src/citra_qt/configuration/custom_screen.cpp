// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <utility>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QSpinBox>
#include <QtGlobal>
#include "citra_qt/configuration/custom_screen.h"
#include "common/diagnostic.h"
#include "ui_custom_screen.h"

CITRA_DIAGNOSTIC_IGNORE_SWITCH

constexpr unsigned to_underlying(RectangleSides s) {
    return static_cast<unsigned>(s);
}

constexpr RectangleSides operator|(RectangleSides l, RectangleSides r) {
    return RectangleSides{to_underlying(l) | to_underlying(r)};
}

constexpr RectangleSides operator&(RectangleSides l, RectangleSides r) {
    return RectangleSides{to_underlying(l) & to_underlying(r)};
}

CustomScreen::CustomScreen(QWidget* parent)
    : QFrame(parent), ui(std::make_unique<Ui::CustomScreen>()) {
    ui->setupUi(this);

    const auto value_changed{QOverload<int>::of(&QSpinBox::valueChanged)};

    connect(ui->left, value_changed, this, [this](int x) { move(x, y()); });
    connect(ui->top, value_changed, this, [this](int y) { move(x(), y); });
    connect(ui->width, value_changed, this, [this](int width) {
        resize(width, height());
        ui->width->setValue(this->width());
    });
    connect(ui->height, value_changed, this, [this](int height) {
        resize(width(), height);
        ui->height->setValue(this->height());
    });
}

QString CustomScreen::name() const {
    return ui->name->text();
}

void CustomScreen::setName(const QString& name) {
    ui->name->setText(name);
}

namespace {

bool IsClamped(int v, int lo, int hi) {
    return lo <= v && v < hi;
}

} // namespace

auto CustomScreen::SidesAt(QPoint pt) -> RectangleSides {
    return RectangleSides{
        (IsClamped(pt.x(), 0, lineWidth()) ? RectangleSides::left : RectangleSides{}) |
        (IsClamped(pt.y(), 0, lineWidth()) ? RectangleSides::top : RectangleSides{}) |
        (IsClamped(pt.x(), width() - lineWidth(), width()) ? RectangleSides::right
                                                           : RectangleSides{}) |
        (IsClamped(pt.y(), height() - lineWidth(), height()) ? RectangleSides::bottom
                                                             : RectangleSides{})};
}

void CustomScreen::mousePressEvent(QMouseEvent* event) {
    switch (event->buttons()) {
    case Qt::MouseButton::LeftButton:
        drag = {SidesAt(event->pos()), geometry(), event->globalPos()};
        if (drag.sides == RectangleSides{})
            setCursor(Qt::CursorShape::ClosedHandCursor);
        return event->accept();
    default:
        return event->ignore();
    }
}

void CustomScreen::mouseMoveEvent(QMouseEvent* event) {
    switch (event->buttons()) {
    case Qt::MouseButton::NoButton:
        switch (SidesAt(event->pos())) {
        case RectangleSides::left:
        case RectangleSides::right:
            setCursor(Qt::CursorShape::SizeHorCursor);
            break;
        case RectangleSides::top:
        case RectangleSides::bottom:
            setCursor(Qt::CursorShape::SizeVerCursor);
            break;
        case RectangleSides::top | RectangleSides::left:
        case RectangleSides::bottom | RectangleSides::right:
            setCursor(Qt::CursorShape::SizeFDiagCursor);
            break;
        case RectangleSides::top | RectangleSides::right:
        case RectangleSides::bottom | RectangleSides::left:
            setCursor(Qt::CursorShape::SizeBDiagCursor);
            break;
        default:
            setCursor(Qt::CursorShape::OpenHandCursor);
            break;
        }
        break;
    case Qt::MouseButton::LeftButton:
        drag(*this, event->globalPos());
        break;
    default:
        return event->ignore();
    }

    event->accept();
}

namespace {

bool HasSingleBit(unsigned x) {
    return x != 0 && (x & (x - 1)) == 0;
}

unsigned BitWidth(unsigned x) {
    return x == 0 ? 0 : 1 + static_cast<unsigned>(std::log2(x));
}

struct RectangleSideOffsets {
    int sides[4]{};

    int& operator[](RectangleSides s) {
        assert(HasSingleBit(to_underlying(s)));
        return sides[BitWidth(to_underlying(s)) - 1];
    }

    int& left() {
        return operator[](RectangleSides::left);
    }
    int& top() {
        return operator[](RectangleSides::top);
    }
    int& right() {
        return operator[](RectangleSides::right);
    }
    int& bottom() {
        return operator[](RectangleSides::bottom);
    }
};

RectangleSides prev(RectangleSides side) {
    assert(HasSingleBit(to_underlying(side)));
    return side == RectangleSides::left ? RectangleSides::bottom
                                        : RectangleSides{to_underlying(side) >> 1};
}

RectangleSides next(RectangleSides side) {
    assert(HasSingleBit(to_underlying(side)));
    return side == RectangleSides::bottom ? RectangleSides::left
                                          : RectangleSides{to_underlying(side) << 1};
}

bool HaveEqualOutwardsDirectionSign(RectangleSides l, RectangleSides r) {
    assert(l != r);
    return (l | r) == (RectangleSides::top | RectangleSides::left) ||
           (l | r) == (RectangleSides::bottom | RectangleSides::right);
}

void GrowAdjacents(double aspect_ratio, RectangleSideOffsets& offsets, RectangleSides side) {
    assert(HasSingleBit(to_underlying(side)));
    const int adjacent_growth{static_cast<int>(aspect_ratio * offsets[side])};
    RectangleSides adjacent_sides[2]{prev(side), next(side)};
    if (!HaveEqualOutwardsDirectionSign(side, adjacent_sides[0]))
        std::swap(adjacent_sides[0], adjacent_sides[1]);
    offsets[adjacent_sides[0]] = adjacent_growth / 2;
    offsets[adjacent_sides[1]] = -adjacent_growth / 2;
}

int FurthestOffset(RectangleSides side, int l, int r) {
    assert(HasSingleBit(to_underlying(side)));
    return (side & (RectangleSides::top | RectangleSides::left)) != RectangleSides{}
               ? std::min(l, r)
               : std::max(l, r);
}

constexpr RectangleSides vertical_sides{RectangleSides::left | RectangleSides::right};
constexpr RectangleSides horizontal_sides{RectangleSides::top | RectangleSides::bottom};

[[maybe_unused]] bool IsCorner(RectangleSides sides) {
    return HasSingleBit(to_underlying(sides & vertical_sides)) &&
           HasSingleBit(to_underlying(sides & horizontal_sides));
}

void GrowCorner(double aspect_ratio, RectangleSideOffsets& offsets, RectangleSides corner) {
    assert(IsCorner(corner));
    const RectangleSides vertical_side{corner & vertical_sides};
    const RectangleSides horizontal_side{corner & horizontal_sides};
    int& vertical_offset{offsets[vertical_side]};
    int& horizontal_offset{offsets[horizontal_side]};
    const int adjust{HaveEqualOutwardsDirectionSign(vertical_side, horizontal_side) ? 1 : -1};
    const int grown_vertical_offset{static_cast<int>(horizontal_offset * aspect_ratio) * adjust};
    const int grown_horizontal_offset{static_cast<int>(vertical_offset / aspect_ratio) * adjust};
    vertical_offset = FurthestOffset(vertical_side, vertical_offset, grown_vertical_offset);
    horizontal_offset = FurthestOffset(horizontal_side, horizontal_offset, grown_horizontal_offset);
}

} // namespace

void CustomScreen::Drag::operator()(CustomScreen& screen, QPoint move_position) const {
    const auto drag_offset{move_position - press_position};

    if (sides == RectangleSides{})
        return screen.move(original_screen_geometry.topLeft() + drag_offset);

    RectangleSideOffsets offsets{
        (sides & RectangleSides::left) != RectangleSides{} ? drag_offset.x() : 0,
        (sides & RectangleSides::top) != RectangleSides{} ? drag_offset.y() : 0,
        (sides & RectangleSides::right) != RectangleSides{} ? drag_offset.x() : 0,
        (sides & RectangleSides::bottom) != RectangleSides{} ? drag_offset.y() : 0};

    if (screen.ui->keep_aspect_ratio->isChecked()) {
        const double aspect_ratio{original_screen_geometry.width() * 1.0 /
                                  original_screen_geometry.height()};
        (HasSingleBit(to_underlying(sides)) ? GrowAdjacents : GrowCorner)(aspect_ratio, offsets,
                                                                          sides);
    }

    screen.move(std::min(original_screen_geometry.x() + offsets.left(),
                         original_screen_geometry.right() - screen.minimumWidth() + 1),
                std::min(original_screen_geometry.y() + offsets.top(),
                         original_screen_geometry.bottom() - screen.minimumHeight() + 1));
    screen.resize(original_screen_geometry.width() + offsets.right() - offsets.left(),
                  original_screen_geometry.height() + offsets.bottom() - offsets.top());
}

void CustomScreen::moveEvent(QMoveEvent*) {
    move(std::max(0, x()), std::max(0, y()));
    ui->left->setValue(x());
    ui->top->setValue(y());
}

void CustomScreen::resizeEvent(QResizeEvent*) {
    moveEvent(nullptr);
    ui->width->setValue(width());
    ui->height->setValue(height());

#if !defined(NDEBUG)
    const int num{width()};
    const int den{height()};
    const int gcd{std::gcd(num, den)};
    ui->keep_aspect_ratio->setText(
        tr("Keep aspect ratio (%1, %2:%3)").arg(1.0 * num / den).arg(num / gcd).arg(den / gcd));
#endif
}

CustomScreen::~CustomScreen() {}
