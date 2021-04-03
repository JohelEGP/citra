// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include <QDialog>
#include <QRect>
#include <QString>
#include "citra_qt/configuration/custom_screen.h"

namespace Ui {
class CustomScreenLayoutEditor;
}

class CustomScreenLayoutEditor : public QDialog {
    Q_OBJECT

public:
    explicit CustomScreenLayoutEditor(QWidget* parent = nullptr);
    ~CustomScreenLayoutEditor();

    void addScreen(QString name, QRect default_geometry, QRect current_geometry);

    CustomScreen* getScreen(QString name) const;

private:
    struct RestorableScreen {
        std::unique_ptr<CustomScreen> screen;
        QRect default_geometry;
        QRect reset_geometry{screen->geometry()};
    };

    std::unique_ptr<Ui::CustomScreenLayoutEditor> ui;
    std::vector<RestorableScreen> screens;
};
