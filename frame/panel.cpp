// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "panel.h"
#include "private/panel_p.h"
#include "containmentitem.h"

#include "qmlengine.h"

#include <QLoggingCategory>

#include <QDir>
#include <QQmlContext>
#include <DIconTheme>

DS_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLog)

DPanel::DPanel(QObject *parent)
    : DContainment(*new DPanelPrivate(this), parent)
{
}

DPanel::~DPanel()
{

}

QQuickWindow *DPanel::window() const
{
    D_DC(DPanel);
    return qobject_cast<QQuickWindow *>(d->m_rootObject);
}

bool DPanel::load(const DAppletData &data)
{
    D_D(DPanel);
    return DContainment::load(data);
}

bool DPanel::init()
{
    D_D(DPanel);
    d->initDciSearchPaths();

    auto applet = this;

    std::unique_ptr<DQmlEngine> engine(new DQmlEngine(applet, applet));

    auto rootObject = engine->beginCreate();

    auto window = qobject_cast<QQuickWindow *>(rootObject);
    if (window) {
        applet->d_func()->setRootObject(window);
    }

    bool res = DContainment::init();

    if (res) {
        engine->completeCreate();
        d->m_engine = engine.release();
    }

    return res;
}

DPanel *DPanel::qmlAttachedProperties(QObject *object)
{
    auto applet = qobject_cast<DApplet *>(DContainmentItem::qmlAttachedProperties(object));
    while (applet) {
        if (auto panel = qobject_cast<DPanel *>(applet)) {
            return panel;
        }
        applet = applet->parentApplet();
    }
    return nullptr;
}

QQuickWindow *DPanel::popupWindow() const
{
    D_DC(DPanel);
    d->ensurePopupWindow();
    return d->m_popupWindow;
}

void DPanelPrivate::initDciSearchPaths()
{
    D_Q(DPanel);
    DGUI_USE_NAMESPACE;
    auto dciPaths = DIconTheme::dciThemeSearchPaths();
    QList<DApplet *> list = m_applets;
    list.append(q);
    for (const auto &item : list) {
        QDir root(item->pluginMetaData().pluginDir());
        if (root.exists("icons")) {
            dciPaths.push_back(root.absoluteFilePath("icons"));
        }
    }
    DIconTheme::setDciThemeSearchPaths(dciPaths);
}

void DPanelPrivate::ensurePopupWindow() const
{
    if (m_popupWindow)
        return;
    D_QC(DPanel);
    if (!q->window())
        return;

    auto object = DQmlEngine::createObject(QUrl("qrc:/ddeshell/qml/PanelPopupWindow.qml"));
     if (!object)
         return;
     const_cast<DPanelPrivate *>(this)->m_popupWindow = qobject_cast<QQuickWindow *>(object);
     if (m_popupWindow) {
         m_popupWindow->setTransientParent(q->window());
         Q_EMIT const_cast<DPanel *>(q)->popupWindowChanged();
     }
}

DS_END_NAMESPACE
