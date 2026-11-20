// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "treelandoutputuuid.h"
#include "qwayland-treeland-output-manager-unstable-v2.h"

#include <QObject>
#include <QPointer>
#include <QtWaylandClient/QWaylandClientExtension>

struct wl_output;
struct treeland_output_picture_control_v2;
struct treeland_output_uuid_v1;

class QScreen;

namespace osd {

class TreelandPictureControl : public QObject, public QtWayland::treeland_output_picture_control_v2
{
    Q_OBJECT

public:
    explicit TreelandPictureControl(struct ::treeland_output_picture_control_v2 *object, QObject *parent = nullptr);
    ~TreelandPictureControl() override;

    double brightness() const;

Q_SIGNALS:
    void brightnessChanged(double brightness);
    void resultReceived(uint32_t result);

protected:
    void treeland_output_picture_control_v2_brightness(wl_fixed_t brightness) override;
    void treeland_output_picture_control_v2_result(uint32_t result) override;

private:
    double m_brightness = 0.0;
};

// Read-only Treeland brightness provider for the OSD. It binds a single
// treeland_output_picture_control_v2 to the primary output (identified by uuid)
// and caches the brightness reported by the compositor. It never commits
// brightness changes; dde-shortcut-tool is responsible for adjusting brightness,
// and this provider only reflects the resulting value.
class TreelandBrightness : public QWaylandClientExtensionTemplate<TreelandBrightness>, public QtWayland::treeland_output_manager_v2
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
    void requestPrimaryUuid();

    QPointer<TreelandPictureControl> m_control;
    QPointer<QScreen> m_primaryScreen;
    QString m_currentUuid;

    ::TreelandOutputUuidManager *m_uuidManager = nullptr;
    QPointer<::TreelandOutputUuid> m_uuidRequest;
};

} // namespace osd
