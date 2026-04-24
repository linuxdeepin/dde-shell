// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "multitaskview.h"
#include "pluginfactory.h"
#include "../constants.h"

#include <QBuffer>
#include <QProcess>

#include <DDciIcon>
#include <DDBusSender>
#include <DWindowManagerHelper>
#include <DGuiApplicationHelper>

#ifdef HAVE_DDE_API_EVENTLOGGER
#include <dde-api/eventlogger.hpp>
#endif

DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE

namespace {
constexpr qint64 EVENT_LOGGER_KWIN_MULTITASK_VIEW = 1000300000;
constexpr int EventLaunchTypeDockIcon = 2;

}

namespace dock {

constexpr int KWinOptimalPerformance = 4;
const QString windowEffectTypeKey = QStringLiteral("user_type");

MultiTaskView::MultiTaskView(QObject *parent)
    : DAppletDock(parent)
    , m_iconName("deepin-multitasking-view")
{
    connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasCompositeChanged, this, [this]() {
        setSupported(m_kWinEffect && DWindowManagerHelper::instance()->hasComposite());
    });
    auto platformName = QGuiApplication::platformName();
    if (QStringLiteral("wayland") == platformName) {
        m_multitaskview.reset(new TreeLandMultitaskview);
    } else {
        m_kWinCompositingConfig = DConfig::create("org.kde.kwin", "org.kde.kwin.compositing", QString(), this);
        m_kWinEffect = m_kWinCompositingConfig->value(windowEffectTypeKey).toInt() != KWinOptimalPerformance;

        connect(m_kWinCompositingConfig, &DConfig::valueChanged, this, [=] (const QString &key) {
            if (key == windowEffectTypeKey) {
                bool kWinEffect = m_kWinCompositingConfig->value(windowEffectTypeKey).toInt() != KWinOptimalPerformance;
                if (kWinEffect != m_kWinEffect) {
                    m_kWinEffect = kWinEffect;
                    setSupported(m_kWinEffect && DWindowManagerHelper::instance()->hasComposite());
                }
            }
        });
    }
}

bool MultiTaskView::init()
{
    setSupported(m_kWinEffect && DWindowManagerHelper::instance()->hasComposite());
    queryKwinVersion();
    DAppletDock::init();
    return true;
}

void MultiTaskView::queryKwinVersion()
{
    auto *process = new QProcess(this);
    process->start(QStringLiteral("dpkg-query"), { QStringLiteral("-W"), QStringLiteral("-f=${Version}"), QStringLiteral("kwin-x11") });
    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus status) {
        if (status == QProcess::NormalExit && exitCode == 0) {
            m_kwinVersion = QString::fromLocal8Bit(process->readAllStandardOutput()).trimmed();
        }
        process->deleteLater();
    });
}

void MultiTaskView::openWorkspace()
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    DDE_EventLogger::EventLogger::instance().writeEventLog(
        DDE_EventLogger::EventLoggerData(EVENT_LOGGER_KWIN_MULTITASK_VIEW, QStringLiteral("kwin_multitask_view"), {
            {QStringLiteral("launch_type"), EventLaunchTypeDockIcon},
            {QStringLiteral("kwin_version"), m_kwinVersion}
        }));
#endif
    if (m_multitaskview) {
        m_multitaskview->toggle();
        return;
    }

    DDBusSender()
        .service("com.deepin.wm")
        .path("/com/deepin/wm")
        .interface("com.deepin.wm")
        .method("ShowWorkspace")
        .call();
}

QString MultiTaskView::iconName() const
{
    return m_iconName;
}

void MultiTaskView::setIconName(const QString& iconName)
{
    if (iconName != m_iconName) {
        m_iconName = iconName;
        Q_EMIT iconNameChanged();
    }
}

DockItemInfo MultiTaskView::dockItemInfo()
{
    DockItemInfo info;
    info.name = "multitasking-view";
    info.displayName = tr("Multitasking View");
    info.itemKey = "multitasking-view";
    info.settingKey = "multitasking-view";
    info.visible = visible();
    info.dccIcon = DCCIconPath + "multitasking-view.svg";
    return info;
}

D_APPLET_CLASS(MultiTaskView)
}


#include "multitaskview.moc"
