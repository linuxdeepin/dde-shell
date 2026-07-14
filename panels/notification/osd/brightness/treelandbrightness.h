// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "qwayland-treeland-output-manager-v1.h"

#include <QObject>
#include <QPointer>
#include <QtWaylandClient/QWaylandClientExtension>

struct wl_output;
struct treeland_output_color_control_v1;

namespace osd {

class TreelandColorControl : public QObject, public QtWayland::treeland_output_color_control_v1
{
    Q_OBJECT

public:
    explicit TreelandColorControl(struct ::treeland_output_color_control_v1 *object, QObject *parent = nullptr);
    ~TreelandColorControl() override;

    double brightness() const;

Q_SIGNALS:
    void brightnessChanged(double brightness);

protected:
    void treeland_output_color_control_v1_brightness(wl_fixed_t brightness) override;

private:
    double m_brightness = 0.0;
};

// Read-only Treeland brightness provider for the OSD. It binds a single
// treeland_output_color_control_v1 to the primary wl_output and caches the
// brightness reported by the compositor. It never commits brightness changes;
// dde-shortcut-tool is responsible for adjusting brightness, and this provider
// only reflects the resulting value.
class TreelandBrightness : public QWaylandClientExtensionTemplate<TreelandBrightness>, public QtWayland::treeland_output_manager_v1
{
    Q_OBJECT

public:
    explicit TreelandBrightness(QObject *parent = nullptr);
    ~TreelandBrightness() override;

    void refresh();

    double brightness() const;

Q_SIGNALS:
    void brightnessChanged(double brightness);

private:
    struct wl_output *primaryWlOutput() const;

    QPointer<TreelandColorControl> m_control;
    struct wl_output *m_output = nullptr;
};

} // namespace osd
