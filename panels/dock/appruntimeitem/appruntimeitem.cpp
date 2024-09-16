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
        , m_visible(true)
        , m_appruntimeVisible(false){}

    void AppRuntimeItem::toggleruntimeitem()
    {
        // 检查 XCB 线程是否已经存在
        if (!xcbThread) {
            // 创建并启动 XCB 事件处理线程
            xcbThread = new XcbThread();

            // 创建 QQmlApplicationEngine 实例
            engine = new QQmlApplicationEngine();
            const QUrl url(QStringLiteral("qrc:/ddeshell/package/ShowRuntimeMenu.qml"));

            // 创建 WindowManager 实例，用于管理窗口信息
            windowManager = new WindowManager();

            // 注册自定义类型
            qRegisterMetaType<xcb_window_t>("xcb_window_t");
            qRegisterMetaType<QVector<WindowInfo_1>>("QVector<WindowInfo_1>");
            qRegisterMetaType<WindowInfo_1>("WindowInfo_1");

            // 将 windowManager 注册到 QML 上下文
            engine->rootContext()->setContextProperty("windowManager", windowManager);

            // 连接信号与槽
            QObject::connect(xcbThread, &XcbThread::windowInfoChanged,
                             windowManager, &WindowManager::setWindowInfo_qiantai,
                             Qt::QueuedConnection);
            QObject::connect(xcbThread, &XcbThread::windowInfoChanged_qiantai,
                             windowManager, &WindowManager::setWindowInfo_qiantai,
                             Qt::QueuedConnection);
            QObject::connect(xcbThread, &XcbThread::windowInfoChanged_houtai,
                             windowManager, &WindowManager::setWindowInfo_houtai,
                             Qt::QueuedConnection);
            QObject::connect(xcbThread, &XcbThread::windowDestroyChanged,
                             windowManager, &WindowManager::WindowDetroyInfo,
                             Qt::QueuedConnection);

            // 启动 XCB 线程
            xcbThread->start();

            // 加载 QML 文件
            engine->load(url);
            if (engine->rootObjects().isEmpty()) {
                qFatal("Failed to load QML");
            }

            // 连接线程结束信号以清理资源
            QObject::connect(xcbThread, &QThread::finished, [this]() {
                xcbThread->deleteLater();
                xcbThread = nullptr;  // 清空指针
            });

            // 连接 engine 结束信号以清理资源
            QObject::connect(engine, &QQmlApplicationEngine::destroyed, [this]() {
                windowManager->deleteLater();
                windowManager = nullptr;  // 清空指针
                engine = nullptr;  // 清空指针
            });
        } else {
            // 如果窗口已经存在，检查窗口是否可见
            QObject *rootObject = engine->rootObjects().first();
            QQuickWindow *window = qobject_cast<QQuickWindow *>(rootObject);
            if (window) {
                // 如果窗口不可见，则显示它
                if (!window->isVisible()) {
                    window->setVisible(true);
                } else {
                    // 如果窗口可见，则隐藏它
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
        info.visible = m_visible;
        info.dccIcon = DCCIconPath + "appruntime.svg";
        return info;
    }
    void AppRuntimeItem::setVisible(bool visible)
    {
        if (m_visible != visible) {
            m_visible = visible;

            Q_EMIT visibleChanged(visible);
        }
    }

    void AppRuntimeItem::onappruntimeVisibleChanged(bool visible)
    {
        if (m_appruntimeVisible != visible) {
            m_appruntimeVisible = visible;
            Q_EMIT appruntimeVisibleChanged(visible);
        }
    }

    D_APPLET_CLASS(AppRuntimeItem)
}

#include "appruntimeitem.moc"
