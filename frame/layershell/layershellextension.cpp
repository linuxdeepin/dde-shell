// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "layershellextension_p.h"

#include <QCoreApplication>
#include <QPointer>

DS_BEGIN_NAMESPACE

TreeLandLayerShellExtensionManager::TreeLandLayerShellExtensionManager()
    : QWaylandClientExtensionTemplate<TreeLandLayerShellExtensionManager>(1)
{
    initialize();
}

TreeLandLayerShellExtensionManager::~TreeLandLayerShellExtensionManager()
{
    if (isInitialized())
        destroy();
}

TreeLandLayerShellExtensionManager *TreeLandLayerShellExtensionManager::instance()
{
    static QPointer<TreeLandLayerShellExtensionManager> manager;
    if (!manager) {
        manager = new TreeLandLayerShellExtensionManager;
        manager->setParent(qApp);
    }
    return manager;
}

TreeLandLayerShellExtensionObject::TreeLandLayerShellExtensionObject(struct ::treeland_layer_shell_extension_object_v1 *object)
    : QtWayland::treeland_layer_shell_extension_object_v1(object)
{
}

TreeLandLayerShellExtensionObject::~TreeLandLayerShellExtensionObject()
{
    if (isInitialized())
        destroy();
}

void TreeLandLayerShellExtensionObject::treeland_layer_shell_extension_object_v1_resizing(uint32_t resizing)
{
    Q_EMIT resizingChanged(resizing != 0);
}

void TreeLandLayerShellExtensionObject::treeland_layer_shell_extension_object_v1_resize_rejected(uint32_t error)
{
    Q_EMIT resizeRejected(error);
}

DS_END_NAMESPACE
