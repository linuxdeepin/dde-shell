// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docksettings.h"
#include "dsglobal.h"
#include "constants.h"
#include "dockpanel.h"
#include "ddockapplet.h"
#include "dockdbusproxy.h"

#include <QObject>

#include <DWindowManagerHelper>
#include <DDciIcon>

DGUI_USE_NAMESPACE

namespace dock {
DockDBusProxy::DockDBusProxy(DockPanel* parent)
    : QObject(parent)
    , m_oldDockApplet(nullptr)
{
    registerPluginInfoMetaType();

    connect(DockSettings::instance(), &DockSettings::pluginsVisibleChanged, this, [this] (const QVariantMap &pluginsVisible) {
        for (auto applet : m_dockApplets) {
            QString itemKey = applet->itemKey();
            if (pluginsVisible.contains(itemKey)) {
                applet->setVisible(pluginsVisible[itemKey].toBool());
            } else {
                auto settingPluginsVisible = DockSettings::instance()->pluginsVisible();
                settingPluginsVisible[itemKey] = true;
                DockSettings::instance()->setPluginsVisible(settingPluginsVisible);
            }
        }
    });
    connect(parent, &DockPanel::rootObjectChanged, this, [this]() {
        auto pluginsVisible = DockSettings::instance()->pluginsVisible();
        for (auto applet : m_dockApplets) {
            QString itemKey = applet->itemKey();
            if (pluginsVisible.contains(itemKey)) {
                applet->setVisible(pluginsVisible[itemKey].toBool());
            } else {
                auto settingPluginsVisible = DockSettings::instance()->pluginsVisible();
                settingPluginsVisible[itemKey] = true;
                DockSettings::instance()->setPluginsVisible(settingPluginsVisible);
            }
        }
    });

    auto root = qobject_cast<DS_NAMESPACE::DContainment *>(this->parent());
    connect(root, &DS_NAMESPACE::DContainment::appletListChanged, this, &DockDBusProxy::onAppletListChanged);
    onAppletListChanged();
}

DockPanel* DockDBusProxy::parent() const
{
    return static_cast<DockPanel*>(QObject::parent());
}

QString DockDBusProxy::getAppID(const QString &desktopfile)
{
    const QString desktopLeft = "/applications/";
    const QString desktopSuffix = ".desktop";
    return desktopfile.mid(desktopfile.lastIndexOf(desktopLeft) + desktopLeft.size(), desktopfile.lastIndexOf(desktopSuffix) - desktopfile.lastIndexOf(desktopLeft) - desktopLeft.size());
}

QList<DS_NAMESPACE::DApplet *> DockDBusProxy::appletList(const QString &pluginId) const
{
    QList<DS_NAMESPACE::DApplet *> ret;
    auto root = qobject_cast<DS_NAMESPACE::DContainment *>(parent());

    QQueue<DS_NAMESPACE::DContainment *> containments;
    containments.enqueue(root);
    while (!containments.isEmpty()) {
        DS_NAMESPACE::DContainment *containment = containments.dequeue();
        for (const auto applet : containment->applets()) {
            if (auto item = qobject_cast<DS_NAMESPACE::DContainment *>(applet)) {
                containments.enqueue(item);
            }
            if (applet->pluginId() == pluginId)
                ret << applet;
        }
    }
    return ret;
}

DS_NAMESPACE::DApplet *DockDBusProxy::applet(const QString &pluginId) const
{
    const auto list = appletList(pluginId);
    if (!list.isEmpty())
        return list.first();
    return nullptr;
}

QRect DockDBusProxy::geometry()
{
    return parent()->window() ? parent()->window()->geometry() : QRect();
}

void DockDBusProxy::setPluginVisible(const QString &pluginId, const QVariantMap &pluginsVisible)
{
    auto it = std::find_if(m_dockApplets.begin(), m_dockApplets.end(), [ = ] (DS_NAMESPACE::DDockApplet *applet) {
        return pluginId == applet->pluginId();
    });

    if (it != m_dockApplets.end()) {
        auto applet = (*it);
        const auto &itemKey = applet->itemKey();
        if (pluginsVisible.contains(itemKey)) {
            applet->setVisible(pluginsVisible[itemKey].toBool());
        } else {
            auto settingPluginsVisible = DockSettings::instance()->pluginsVisible();
            settingPluginsVisible[itemKey] = true;
            DockSettings::instance()->setPluginsVisible(settingPluginsVisible);
        }
    }
}

QRect DockDBusProxy::frontendWindowRect()
{
    return parent()->frontendWindowRect();
}

Position DockDBusProxy::position()
{
    return parent()->position();
}

void DockDBusProxy::setPosition(Position position)
{
    parent()->setPosition(position);
}

HideMode DockDBusProxy::hideMode()
{
    return parent()->hideMode();
}

void DockDBusProxy::setHideMode(HideMode mode)
{
    parent()->setHideMode(mode);
}

HideState DockDBusProxy::hideState()
{
    return parent()->hideState();
}

uint DockDBusProxy::windowSizeEfficient()
{
    return parent()->dockSize();
}

void DockDBusProxy::setWindowSizeEfficient(uint size)
{
    qDebug() << size;
    parent()->setDockSize(size);
}

uint DockDBusProxy::windowSizeFashion()
{
    return parent()->dockSize();
}

void DockDBusProxy::setWindowSizeFashion(uint size)
{
    parent()->setDockSize(size);
}

int DockDBusProxy::displayMode()
{
    return parent()->itemAlignment();
}

void DockDBusProxy::setDisplayMode(int displayMode)
{
    parent()->setItemAlignment(static_cast<ItemAlignment>(displayMode));
}

bool DockDBusProxy::RequestDock(const QString &desktopFile, int index) {
    Q_UNUSED(index);
    QString id = getAppID(desktopFile);
    if (id.isEmpty())
        return false;

    auto appletItem = applet("org.deepin.ds.dock.taskmanager");
    if (nullptr == appletItem)
        return false;
    bool res = true;
    QMetaObject::invokeMethod(appletItem, "RequestDock", Qt::DirectConnection, Q_RETURN_ARG(bool, res), Q_ARG(QString, id));
    return res;
}

bool DockDBusProxy::IsDocked(const QString &desktopFile)
{
    QString id = getAppID(desktopFile);
    if (id.isEmpty())
        return false;

    auto appletItem = applet("org.deepin.ds.dock.taskmanager");
    if (nullptr == appletItem)
        return false;
    bool res = true;
    QMetaObject::invokeMethod(appletItem, "IsDocked", Qt::DirectConnection, Q_RETURN_ARG(bool, res), Q_ARG(QString, id));
    return res;
}

bool DockDBusProxy::RequestUndock(const QString &desktopFile)
{
    QString id = getAppID(desktopFile);
    auto appletItem = applet("org.deepin.ds.dock.taskmanager");
    if (nullptr == appletItem)
        return false;
    bool res = true;
    QMetaObject::invokeMethod(appletItem, "RequestUndock", Qt::DirectConnection, Q_RETURN_ARG(bool, res), Q_ARG(QString, id));
    return res;
}

void DockDBusProxy::onAppletListChanged()
{
    // for old dock
    QList<DS_NAMESPACE::DApplet *> list = appletList("org.deepin.ds.dock.tray");
    if (!m_oldDockApplet && !list.isEmpty()) m_oldDockApplet = list.first();

    // other dock plugin
    m_dockApplets.clear();
    auto root = qobject_cast<DS_NAMESPACE::DContainment *>(parent());

    QQueue<DS_NAMESPACE::DContainment *> containments;
    containments.enqueue(root);
    while (!containments.isEmpty()) {
        DS_NAMESPACE::DContainment *containment = containments.dequeue();
        for (const auto applet : containment->applets()) {
            if (auto item = qobject_cast<DS_NAMESPACE::DContainment *>(applet)) {
                containments.enqueue(item);
            }
            if (auto dockApplet = qobject_cast<DS_NAMESPACE::DDockApplet *>(applet)) {
                m_dockApplets << dockApplet;
            }
        }
    }
}

QStringList DockDBusProxy::GetLoadedPlugins()
{
    // TODO: implement this function
    return QStringList();
}

DockItemInfos DockDBusProxy::plugins()
{
    DockItemInfos iteminfos;
    if (m_oldDockApplet) {
        QMetaObject::invokeMethod(m_oldDockApplet, "plugins", Qt::DirectConnection, qReturnArg(iteminfos));
    }

    for (auto applet : m_dockApplets) {
        DockItemInfo info;
        info.name = applet->name();
        info.displayName = applet->displayName();
        info.itemKey = applet->itemKey();
        info.settingKey = applet->settingKey();
        info.visible = applet->visible();

        {
            const auto lightPalette = DGuiApplicationHelper::instance()->applicationPalette(DGuiApplicationHelper::LightType);
            auto lightPixmap = DDciIcon::fromTheme(applet->icon()).pixmap(
                qApp->devicePixelRatio(),
                30,
                DDciIcon::Light,
                DDciIcon::Normal,
                DDciIconPalette::fromQPalette(lightPalette)
                );
            QBuffer buffer(&info.iconLight);
            if (buffer.open(QIODevice::WriteOnly)) {
                lightPixmap.save(&buffer, "png");
            }
        }
        {
            const auto darkPalette = DGuiApplicationHelper::instance()->applicationPalette(DGuiApplicationHelper::DarkType);
            auto darkPixmap = DDciIcon::fromTheme("search").pixmap(
                qApp->devicePixelRatio(),
                30,
                DDciIcon::Dark,
                DDciIcon::Normal,
                DDciIconPalette::fromQPalette(darkPalette)
                );
            QBuffer buffer(&info.iconDark);
            if (buffer.open(QIODevice::WriteOnly)) {
                darkPixmap.save(&buffer, "png");
            }
        }

        iteminfos.append(info);
    }

    return iteminfos;
}

void DockDBusProxy::ReloadPlugins()
{
    parent()->ReloadPlugins();
}

void DockDBusProxy::callShow()
{
    parent()->callShow();
}

void DockDBusProxy::setItemOnDock(const QString &settingKey, const QString &itemKey, bool visible)
{
    auto it = std::find_if(m_dockApplets.begin(), m_dockApplets.end(), [ = ] (DS_NAMESPACE::DDockApplet *applet) {
        return itemKey == applet->itemKey();
    });

    if (it != m_dockApplets.end()) {
        auto applet = (*it);
        applet->setVisible(visible);
        auto pluginsVisible = DockSettings::instance()->pluginsVisible();
        pluginsVisible[itemKey] = visible;
        DockSettings::instance()->setPluginsVisible(pluginsVisible);
    }
}

void DockDBusProxy::setPluginVisible(const QString &pluginName, bool visible)
{
    // TODO: implement this function
    Q_UNUSED(pluginName)
    Q_UNUSED(visible)
}

bool DockDBusProxy::getPluginVisible(const QString &pluginName)
{
    // TODO: implement this function
    Q_UNUSED(pluginName)
    return true;
}

QString DockDBusProxy::getPluginKey(const QString &pluginName)
{
    // TODO: implement this function
    Q_UNUSED(pluginName)
    return QString();
}

void DockDBusProxy::resizeDock(int offset, bool dragging)
{
    Q_UNUSED(dragging)
    parent()->setDockSize(offset);
}

}

