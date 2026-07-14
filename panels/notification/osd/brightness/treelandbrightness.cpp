// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandbrightness.h"

#include "wayland-treeland-output-manager-v1-client-protocol.h"

#include <QGuiApplication>
#include <QScreen>

#include <wayland-client.h>

namespace osd {

TreelandColorControl::TreelandColorControl(struct ::treeland_output_color_control_v1 *object, QObject *parent)
    : QObject(parent)
    , QtWayland::treeland_output_color_control_v1(object)
{
}

TreelandColorControl::~TreelandColorControl()
{
    if (isInitialized()) {
        destroy();
    }
}

double TreelandColorControl::brightness() const
{
    return m_brightness;
}

void TreelandColorControl::treeland_output_color_control_v1_brightness(wl_fixed_t brightness)
{
    m_brightness = wl_fixed_to_double(brightness);
    Q_EMIT brightnessChanged(m_brightness);
}

TreelandBrightness::TreelandBrightness(QObject *parent)
    : QWaylandClientExtensionTemplate<TreelandBrightness>(treeland_output_manager_v1_interface.version)
{
    setParent(parent);
    connect(this, &TreelandBrightness::activeChanged, this, &TreelandBrightness::refresh);
}

TreelandBrightness::~TreelandBrightness()
{
    delete m_control;
    m_control = nullptr;
    m_output = nullptr;
    if (isInitialized()) {
        destroy();
    }
}

void TreelandBrightness::refresh()
{
    if (!isActive()) {
        delete m_control;
        m_control = nullptr;
        m_output = nullptr;
        return;
    }

    struct wl_output *output = primaryWlOutput();
    if (m_control && m_output == output && output) {
        return;
    }

    delete m_control;
    m_control = nullptr;
    m_output = output;

    if (!output) {
        return;
    }

    auto *raw = get_color_control(output);
    if (!raw) {
        return;
    }

    m_control = new TreelandColorControl(raw, this);
    connect(m_control, &TreelandColorControl::brightnessChanged,
            this, &TreelandBrightness::brightnessChanged);
}

double TreelandBrightness::brightness() const
{
    return m_control ? m_control->brightness() : 0.0;
}

struct wl_output *TreelandBrightness::primaryWlOutput() const
{
    auto *screen = qApp->primaryScreen();
    if (!screen) {
        return nullptr;
    }

    auto *waylandScreen = screen->nativeInterface<QNativeInterface::QWaylandScreen>();
    return waylandScreen ? waylandScreen->output() : nullptr;
}

} // namespace osd
