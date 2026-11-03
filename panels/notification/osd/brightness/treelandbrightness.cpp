// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "treelandbrightness.h"

#include "wayland-treeland-output-manager-unstable-v2-client-protocol.h"

#include <QGuiApplication>
#include <QScreen>

#include <wayland-client.h>

namespace osd {

TreelandPictureControl::TreelandPictureControl(struct ::treeland_output_picture_control_v2 *object, QObject *parent)
    : QObject(parent)
    , QtWayland::treeland_output_picture_control_v2(object)
{
}

TreelandPictureControl::~TreelandPictureControl()
{
    if (isInitialized()) {
        destroy();
    }
}

double TreelandPictureControl::brightness() const
{
    return m_brightness;
}

void TreelandPictureControl::treeland_output_picture_control_v2_brightness(wl_fixed_t brightness)
{
    m_brightness = wl_fixed_to_double(brightness);
    Q_EMIT brightnessChanged(m_brightness);
}

void TreelandPictureControl::treeland_output_picture_control_v2_result(uint32_t result)
{
    Q_EMIT resultReceived(result);
}

TreelandBrightness::TreelandBrightness(QObject *parent)
    : QWaylandClientExtensionTemplate<TreelandBrightness>(treeland_output_manager_v2_interface.version)
{
    setParent(parent);

    m_uuidManager = new ::TreelandOutputUuidManager(this);
    connect(m_uuidManager, &::TreelandOutputUuidManager::activeChanged, this, &TreelandBrightness::refresh);
    connect(this, &TreelandBrightness::activeChanged, this, &TreelandBrightness::refresh);
    connect(qApp, &QGuiApplication::primaryScreenChanged, this, &TreelandBrightness::refresh);
}

TreelandBrightness::~TreelandBrightness()
{
    delete m_control;
    m_control = nullptr;
    m_primaryScreen = nullptr;
    delete m_uuidRequest;
    m_uuidRequest = nullptr;
    if (isInitialized()) {
        destroy();
    }
}

void TreelandBrightness::refresh()
{
    if (!isActive() || !m_uuidManager || !m_uuidManager->isActive()) {
        delete m_control;
        m_control = nullptr;
        m_primaryScreen = nullptr;
        delete m_uuidRequest;
        m_uuidRequest = nullptr;
        m_currentUuid.clear();
        return;
    }

    QScreen *screen = qApp->primaryScreen();
    if (m_primaryScreen == screen && m_control && !m_currentUuid.isEmpty()) {
        return;
    }

    delete m_control;
    m_control = nullptr;
    m_primaryScreen = screen;
    m_currentUuid.clear();

    if (!screen) {
        return;
    }

    requestPrimaryUuid();
}

void TreelandBrightness::requestPrimaryUuid()
{
    if (!m_uuidManager || !m_uuidManager->isActive())
        return;

    if (!m_primaryScreen)
        return;

    auto *waylandScreen = m_primaryScreen->nativeInterface<QNativeInterface::QWaylandScreen>();
    if (!waylandScreen || !waylandScreen->output())
        return;

    struct wl_output *output = waylandScreen->output();

    delete m_uuidRequest;
    m_uuidRequest = nullptr;

    auto *raw = m_uuidManager->get_uuid(output);
    if (!raw)
        return;

    m_uuidRequest = new ::TreelandOutputUuid(raw, this);

    connect(m_uuidRequest, &::TreelandOutputUuid::uuidReceived, this, [this](const QString &uuid) {
        if (sender() != m_uuidRequest)
            return;

        if (uuid.isEmpty())
            return;

        m_currentUuid = uuid;

        delete m_uuidRequest;
        m_uuidRequest = nullptr;

        auto *raw = get_picture_control(uuid);
        if (!raw)
            return;

        m_control = new TreelandPictureControl(raw, this);
        connect(m_control, &TreelandPictureControl::brightnessChanged,
                this, &TreelandBrightness::brightnessChanged);
        connect(m_control, &TreelandPictureControl::resultReceived, this, [this](uint32_t result) {
            if (result == TreelandPictureControl::commit_result_invalid_output) {
                delete m_control;
                m_control = nullptr;
                m_currentUuid.clear();
            }
        });
    });
}

double TreelandBrightness::brightness() const
{
    return m_control ? m_control->brightness() : 0.0;
}

} // namespace osd
