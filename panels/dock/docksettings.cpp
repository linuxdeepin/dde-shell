// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docksettings.h"
#include "constants.h"
#include "dockabstractsettingsconfig.h"
#include "docksettingsdconfig.h"
#include "dsglobal.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(dockSettingsLog, "dde.shell.dock.docksettings")

const static QString keyPosition             = "Position";
const static QString keyHideMode             = "Hide_Mode";
const static QString keyDisplayMode          = "Display_Mode";
const static QString keyWindowSizeFashion    = "Window_Size_Fashion";
const static QString keyWindowSizeEfficient  = "Window_Size_Efficient";

DS_BEGIN_NAMESPACE
namespace dock {

static QString hideMode2String(HideMode mode)
{
    switch (mode) {
    case HideMode::KeepShowing:
        return "keep-showing";
    case HideMode::KeepHidden:
        return "keep-hidden";
    case HideMode::SmartHide:
        return "smart-hide";
    default:
        return "keep-showing";
    }
}

static HideMode String2HideMode(const QString& modeStr)
{
    if ("keep-hidden" == modeStr)
        return HideMode::KeepHidden;
    if ("smart-hide" == modeStr)
        return HideMode::SmartHide;
    return HideMode::KeepShowing;
}

static QString displayMode2String(DisplayMode mode)
{
    switch (mode) {
    case DisplayMode::Fashion:
        return "fashion";
    case DisplayMode::Efficient:
        return "efficient";
    default:
        return "fashion";
    }
}

static DisplayMode Qstring2DisplayMode(const QString& modeStr)
{
    if ("fashion" == modeStr)
        return DisplayMode::Fashion;
    else if ("efficient" == modeStr)
        return DisplayMode::Efficient;
    return DisplayMode::Fashion;
}

static QString position2String(Position position)
{
    switch (position) {
        case Position::Top:
            return "top";
        case Position::Right:
            return "right";
        case Position::Left:
            return "left";
        case Position::Bottom:
            return "bottom";
        default:
            return "bottom";
    }
}

static Position String2Position(const QString& modeStr)
{
    if (modeStr == "left") return Position::Left;
    else if (modeStr == "right") return Position::Right;
    else if (modeStr == "top") return Position::Top;
    else if (modeStr == "bottom") return Position::Bottom;
    return Position::Bottom;
}

DockSettings* DockSettings::instance()
{
    static DockSettings* dockSettings = nullptr;
    if (!dockSettings) {
        dockSettings = new DockSettings();
    }

    return dockSettings;
}

DockSettings::DockSettings(QObject* parent)
    : QObject(parent)
    , m_dockConfig(new DockDconfig(this))
{
    init();
}

void DockSettings::init()
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        connect(m_dockConfig.data(), &DockAbstractConfig::valueChanged, this, [this](const QString& key){
            if (keyHideMode == key) {
                Q_EMIT hideModeChanged(hideMode());
            } else if (keyDisplayMode == key) {
                Q_EMIT displayModeChanged(displayMode());
            } else if (keyPosition == key) {
                Q_EMIT positionChanged(position());
            } else if (keyWindowSizeFashion == key) {
                Q_EMIT windowSizeFashionChanged(windowSizeFashion());
            } else if (keyWindowSizeEfficient == key) {
                Q_EMIT windowSizeEfficientChanged(windowSizeEfficient());
            }
        });
    } else {
        qCCritical(dockSettingsLog()) << "unable to create config for org.deepin.dde.dock";
    }
}

HideMode DockSettings::hideMode()
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        return String2HideMode(m_dockConfig->value(keyHideMode).toString());
    } else {
        qCCritical(dockSettingsLog()) << "unable get config for hidemode";
        return HideMode::KeepShowing;
    }
}

void DockSettings::setHideMode(HideMode mode)
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        m_dockConfig->setValue(keyHideMode, hideMode2String(mode));
    } else {
        qCCritical(dockSettingsLog()) << "unable set config for hidemode";
    }
}

Position DockSettings::position()
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        return String2Position(m_dockConfig->value(keyPosition).toString());
    } else {
        qCCritical(dockSettingsLog()) << "unable get config for hidemode";
        return Position::Bottom;
    }
}

void DockSettings::setPosition(Position position)
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        m_dockConfig->setValue(keyPosition, position2String(position));
    } else {
        qCCritical(dockSettingsLog()) << "unable set config for position";
    }
}

DisplayMode DockSettings::displayMode()
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        return Qstring2DisplayMode(m_dockConfig->value(keyDisplayMode).toString());
    } else {
        qCCritical(dockSettingsLog()) << "unable get config for displaymode";
        return DisplayMode::Fashion;
    }
}

void DockSettings::setDisplayMode(DisplayMode mode)
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        m_dockConfig->setValue(keyDisplayMode, displayMode2String(mode));
    } else {
        qCCritical(dockSettingsLog()) << "unable to set config for displaymode";
    }
}

uint DockSettings::windowSizeFashion()
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        return m_dockConfig->value(keyWindowSizeFashion).toUInt();
    } else {
        qCCritical(dockSettingsLog()) << "unable get dconfig for windowSizeFashion";
        return MIN_DOCK_SIZE;
    }
}

void DockSettings::setWindowSizeFashion(uint size)
{
    if(m_dockConfig && m_dockConfig->isValid()) {
        m_dockConfig->setValue(keyWindowSizeFashion, size);
    } else {
        qCCritical(dockSettingsLog()) << "unable to set dconfig for windowSizeFashion";
    }
}

uint DockSettings::windowSizeEfficient()
{
    if (m_dockConfig && m_dockConfig->isValid()) {
        return m_dockConfig->value(keyWindowSizeEfficient).toUInt();
    } else {
        qCCritical(dockSettingsLog()) << "unable get dconfig for windowSizeEfficient";
        return MIN_DOCK_SIZE;
    }
}

void DockSettings::setWindowSizeEfficient(uint size)
{
    if(m_dockConfig && m_dockConfig->isValid()) {
        m_dockConfig->setValue(keyWindowSizeEfficient, size);
    } else {
        qCCritical(dockSettingsLog()) << "unable to set dconfig for windowSizeEfficient";
    }
}

void DockSettings::updateDockSettingsBackend(DockAbstractConfig *backend)
{
    if (backend && backend->isValid()) {
        m_dockConfig.reset(backend);
    }
}

}
DS_END_NAMESPACE
