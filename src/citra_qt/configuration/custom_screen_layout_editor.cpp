// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstddef>
#include <utility>
#include <QMainWindow>
#include <QPushButton>
#include "citra_qt/configuration/custom_screen_layout_editor.h"
#include "ui_custom_screen_layout_editor.h"

CustomScreenLayoutEditor::CustomScreenLayoutEditor(QWidget* parent)
    : QDialog(parent), ui(std::make_unique<Ui::CustomScreenLayoutEditor>()) {
    ui->setupUi(this);

    auto* const framebuffer = new QMainWindow;
    ui->scroll_area->setWidget(framebuffer);

    const auto set = [this](QRect RestorableScreen::*geometry) {
        return [this, geometry] {
            for (auto& s : screens)
                s.screen->setGeometry(s.*geometry);
        };
    };

    connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this,
            set(&RestorableScreen::reset_geometry));
    connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
            set(&RestorableScreen::default_geometry));
}

void CustomScreenLayoutEditor::addScreen(QString name, QRect default_geometry,
                                         QRect current_geometry) {
    auto s{std::make_unique<CustomScreen>(ui->scroll_area->widget())};
    s->setGeometry(current_geometry);
    s->setName(name);
    screens.emplace_back(RestorableScreen{std::move(s), default_geometry});
}

CustomScreen* CustomScreenLayoutEditor::getScreen(QString name) const {
    auto s{std::find_if(screens.begin(), screens.end(),
                        [&](auto& s) { return s.screen->name() == name; })};
    return s == screens.end() ? nullptr : s->screen.get();
}

CustomScreenLayoutEditor::~CustomScreenLayoutEditor() {}
