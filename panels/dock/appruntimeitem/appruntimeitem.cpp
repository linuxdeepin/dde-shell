// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appruntimeitem.h"
#include "applet.h"
#include "pluginfactory.h"

#include "../constants.h"

#include <DDBusSender>
#include <DDciIcon>
#include <DGuiApplicationHelper>

#include <QGuiApplication>
#include <QBuffer>

DGUI_USE_NAMESPACE

namespace dock {
    AppRuntimeItem::AppRuntimeItem(QObject *parent)
        : DApplet(parent)
        , xcbThread(nullptr)
        , engine(nullptr)
        , windowManager(nullptr)
        , mVisible(true)
        , mAppRuntimeVisible(false){}

    void AppRuntimeItem::toggleruntimeitem()
    {
        // Check if the XCB thread already exists
        if (!xcbThread) {
            // Create and start the XCB event handling thread
            xcbThread = new XcbThread();

            // Create an instance of QQmlApplicationEngine
            engine = new QQmlApplicationEngine();
            const QUrl url(QStringLiteral("qrc:/ddeshell/package/ShowRuntimeMenu.qml"));

            // Create an instance of WindowManager to manage window information
            windowManager = new WindowManager();

            // Register custom types
            qRegisterMetaType<xcb_window_t>("xcb_window_t");
            qRegisterMetaType<QVector<AppRuntimeInfo>>("QVector<AppRuntimeInfo>");
            qRegisterMetaType<AppRuntimeInfo>("AppRuntimeInfo");

            // Register windowManager to the QML context
            engine->rootContext()->setContextProperty("windowManager", windowManager);

            // Connect signals and slots
            QObject::connect(xcbThread, &XcbThread::windowInfoChanged,
                             windowManager, &WindowManager::setWindowInfoForeground,
                             Qt::QueuedConnection);
            QObject::connect(xcbThread, &XcbThread::windowInfoChangedForeground,
                             windowManager, &WindowManager::setWindowInfoForeground,
                             Qt::QueuedConnection);
            QObject::connect(xcbThread, &XcbThread::windowInfoChangedBackground,
                             windowManager, &WindowManager::setWindowInfoBackground,
                             Qt::QueuedConnection);
            QObject::connect(xcbThread, &XcbThread::windowDestroyChanged,
                             windowManager, &WindowManager::WindowDetroyInfo,
                             Qt::QueuedConnection);

            // Start the XCB thread
            xcbThread->start();

            // Load the QML file
            engine->load(url);
            if (engine->rootObjects().isEmpty()) {
                qFatal("Failed to load QML");
            }

            // Connect thread finished signal to clean up resources
            QObject::connect(xcbThread, &QThread::finished, [this]() {
                xcbThread->deleteLater();
                xcbThread = nullptr;  // Clear pointer
            });

            // Connect engine destroyed signal to clean up resources
            QObject::connect(engine, &QQmlApplicationEngine::destroyed, [this]() {
                windowManager->deleteLater();
                windowManager = nullptr;  // Clear pointer
                engine = nullptr;  // Clear pointer
            });
        } else {
            // If the window already exists, check if the window is visible
            QObject *rootObject = engine->rootObjects().first();
            QQuickWindow *window = qobject_cast<QQuickWindow *>(rootObject);
            if (window) {
                // If the window is not visible, show it
                if (!window->isVisible()) {
                    window->setVisible(true);
                } else {
                    // If the window is visible, hide it
                    window->setVisible(false);
                }
            }
        }
    }
    DockItemInfo AppRuntimeItem::dockItemInfo()
    {
        DockItemInfo info;
        info.name = "appruntime";
        info.displayName = tr("App_runtime");
        info.itemKey = "appruntime";
        info.settingKey = "appruntime";
        info.visible = mVisible;
        info.dccIcon = DCCIconPath + "appruntime.svg";
        return info;
    }
    void AppRuntimeItem::setVisible(bool visible)
    {
        if (mVisible != visible) {
            mVisible = visible;

            Q_EMIT visibleChanged(visible);
        }
    }

    void AppRuntimeItem::onappruntimeVisibleChanged(bool visible)
    {
        if (mAppRuntimeVisible != visible) {
            mAppRuntimeVisible = visible;
            Q_EMIT appruntimeVisibleChanged(visible);
        }
        delete xcbThread;
        xcbThread = nullptr;
        delete engine;
        engine = nullptr;
    }

    D_APPLET_CLASS(AppRuntimeItem)
}

#include "appruntimeitem.moc"
