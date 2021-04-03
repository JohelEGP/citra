// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QFrame>
#include <QPoint>
#include <QRect>
#include <QString>

enum class RectangleSides : unsigned {
    left = (1 << 0),
    top = (1 << 1),
    right = (1 << 2),
    bottom = (1 << 3)
};

namespace Ui {
class CustomScreen;
}

class CustomScreen : public QFrame {
    Q_OBJECT

public:
    explicit CustomScreen(QWidget* parent = nullptr);
    ~CustomScreen();

    QString name() const;

    void setName(const QString&);

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

    void moveEvent(QMoveEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    RectangleSides SidesAt(QPoint);

    struct Drag {
        RectangleSides sides;
        QRect original_screen_geometry;
        QPoint press_position;

        void operator()(CustomScreen& screen, QPoint move_position) const;
    };

    friend Drag;

    std::unique_ptr<Ui::CustomScreen> ui;
    Drag drag;
};
