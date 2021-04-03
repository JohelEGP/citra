// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cassert>
#include <QColorDialog>
#include <QPoint>
#include <QString>
#include "citra_qt/configuration/configure_enhancements.h"
#include "core/3ds.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_enhancements.h"
#include "video_core/renderer_opengl/post_processing_opengl.h"
#include "video_core/renderer_opengl/texture_filters/texture_filterer.h"

namespace {

const QString g_top_screen_name{QObject::tr("Top")};
const QString g_bottom_screen_name{QObject::tr("Bottom")};

void AddScreens(CustomScreenLayoutEditor& layout_editor) {
    layout_editor.addScreen(
        g_top_screen_name, {0, 0, Core::kScreenTopWidth, Core::kScreenTopHeight},
        {QPoint{Settings::values.custom_top_left, Settings::values.custom_top_top},
         QPoint{Settings::values.custom_top_right - 1, Settings::values.custom_top_bottom - 1}});
    layout_editor.addScreen(
        g_bottom_screen_name,
        {40, Core::kScreenTopHeight, Core::kScreenBottomWidth, Core::kScreenBottomHeight},
        {QPoint{Settings::values.custom_bottom_left, Settings::values.custom_bottom_top},
         QPoint{Settings::values.custom_bottom_right - 1,
                Settings::values.custom_bottom_bottom - 1}});
}

} // namespace

ConfigureEnhancements::ConfigureEnhancements(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureEnhancements>()),
      layout_editor(std::make_unique<CustomScreenLayoutEditor>(this)) {
    ui->setupUi(this);

    for (const auto& filter : OpenGL::TextureFilterer::GetFilterNames())
        ui->texture_filter_combobox->addItem(QString::fromStdString(filter.data()));

    SetConfiguration();

    ui->resolution_factor_combobox->setEnabled(Settings::values.use_hw_renderer);

    AddScreens(*layout_editor);

    connect(ui->render_3d_combobox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int currentIndex) {
                updateShaders(static_cast<Settings::StereoRenderOption>(currentIndex));
            });

    connect(ui->editor_button, &QPushButton::clicked, this,
            [this] { layout_editor->showMaximized(); });

    connect(ui->bg_button, &QPushButton::clicked, this, [this] {
        const QColor new_bg_color = QColorDialog::getColor(bg_color);
        if (!new_bg_color.isValid()) {
            return;
        }
        bg_color = new_bg_color;
        QPixmap pixmap(ui->bg_button->size());
        pixmap.fill(bg_color);
        const QIcon color_icon(pixmap);
        ui->bg_button->setIcon(color_icon);
    });

    ui->toggle_preload_textures->setEnabled(ui->toggle_custom_textures->isChecked());
    connect(ui->toggle_custom_textures, &QCheckBox::toggled, this, [this] {
        ui->toggle_preload_textures->setEnabled(ui->toggle_custom_textures->isChecked());
        if (!ui->toggle_preload_textures->isEnabled())
            ui->toggle_preload_textures->setChecked(false);
    });
}

void ConfigureEnhancements::SetConfiguration() {
    ui->resolution_factor_combobox->setCurrentIndex(Settings::values.resolution_factor);
    ui->render_3d_combobox->setCurrentIndex(static_cast<int>(Settings::values.render_3d));
    ui->factor_3d->setValue(Settings::values.factor_3d);
    updateShaders(Settings::values.render_3d);
    ui->toggle_linear_filter->setChecked(Settings::values.filter_mode);
    int tex_filter_idx = ui->texture_filter_combobox->findText(
        QString::fromStdString(Settings::values.texture_filter_name));
    if (tex_filter_idx == -1) {
        ui->texture_filter_combobox->setCurrentIndex(0);
    } else {
        ui->texture_filter_combobox->setCurrentIndex(tex_filter_idx);
    }
    ui->layout_combobox->setCurrentIndex(static_cast<int>(Settings::values.layout_option));
    ui->swap_screen->setChecked(Settings::values.swap_screen);
    ui->upright_screen->setChecked(Settings::values.upright_screen);
    ui->toggle_dump_textures->setChecked(Settings::values.dump_textures);
    ui->toggle_custom_textures->setChecked(Settings::values.custom_textures);
    ui->toggle_preload_textures->setChecked(Settings::values.preload_textures);
    bg_color = QColor::fromRgbF(Settings::values.bg_red, Settings::values.bg_green,
                                Settings::values.bg_blue);
    QPixmap pixmap(ui->bg_button->size());
    pixmap.fill(bg_color);
    const QIcon color_icon(pixmap);
    ui->bg_button->setIcon(color_icon);
}

void ConfigureEnhancements::updateShaders(Settings::StereoRenderOption stereo_option) {
    ui->shader_combobox->clear();

    if (stereo_option == Settings::StereoRenderOption::Anaglyph)
        ui->shader_combobox->addItem(QStringLiteral("dubois (builtin)"));
    else if (stereo_option == Settings::StereoRenderOption::Interlaced ||
             stereo_option == Settings::StereoRenderOption::ReverseInterlaced)
        ui->shader_combobox->addItem(QStringLiteral("horizontal (builtin)"));
    else
        ui->shader_combobox->addItem(QStringLiteral("none (builtin)"));

    ui->shader_combobox->setCurrentIndex(0);

    for (const auto& shader : OpenGL::GetPostProcessingShaderList(
             stereo_option == Settings::StereoRenderOption::Anaglyph)) {
        ui->shader_combobox->addItem(QString::fromStdString(shader));
        if (Settings::values.pp_shader_name == shader)
            ui->shader_combobox->setCurrentIndex(ui->shader_combobox->count() - 1);
    }
}

void ConfigureEnhancements::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureEnhancements::ApplyConfiguration() {
    Settings::values.resolution_factor =
        static_cast<u16>(ui->resolution_factor_combobox->currentIndex());
    Settings::values.render_3d =
        static_cast<Settings::StereoRenderOption>(ui->render_3d_combobox->currentIndex());
    Settings::values.factor_3d = ui->factor_3d->value();
    Settings::values.pp_shader_name =
        ui->shader_combobox->itemText(ui->shader_combobox->currentIndex()).toStdString();
    Settings::values.filter_mode = ui->toggle_linear_filter->isChecked();
    Settings::values.texture_filter_name = ui->texture_filter_combobox->currentText().toStdString();
    Settings::values.layout_option =
        static_cast<Settings::LayoutOption>(ui->layout_combobox->currentIndex());
    {
        const auto geometry = [this](const QString& screen_name) {
            const auto* const screen{layout_editor->getScreen(screen_name)};
            assert(screen);
            return screen->geometry();
        };
        const QRect top_screen{geometry(g_top_screen_name)};
        Settings::values.custom_top_left = top_screen.left();
        Settings::values.custom_top_top = top_screen.top();
        Settings::values.custom_top_right = top_screen.right() + 1;
        Settings::values.custom_top_bottom = top_screen.bottom() + 1;
        const QRect bottom_screen{geometry(g_bottom_screen_name)};
        Settings::values.custom_bottom_left = bottom_screen.left();
        Settings::values.custom_bottom_top = bottom_screen.top();
        Settings::values.custom_bottom_right = bottom_screen.right() + 1;
        Settings::values.custom_bottom_bottom = bottom_screen.bottom() + 1;
    }
    Settings::values.swap_screen = ui->swap_screen->isChecked();
    Settings::values.upright_screen = ui->upright_screen->isChecked();
    Settings::values.dump_textures = ui->toggle_dump_textures->isChecked();
    Settings::values.custom_textures = ui->toggle_custom_textures->isChecked();
    Settings::values.preload_textures = ui->toggle_preload_textures->isChecked();
    Settings::values.bg_red = static_cast<float>(bg_color.redF());
    Settings::values.bg_green = static_cast<float>(bg_color.greenF());
    Settings::values.bg_blue = static_cast<float>(bg_color.blueF());
}

ConfigureEnhancements::~ConfigureEnhancements() {}
