// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "qwayland-treeland-layer-shell-extension-unstable-v1.h"

#include <QtWaylandClient/QWaylandClientExtension>

DS_BEGIN_NAMESPACE

class TreeLandLayerShellExtensionManager : public QWaylandClientExtensionTemplate<TreeLandLayerShellExtensionManager>,
                                         public QtWayland::treeland_layer_shell_extension_manager_v1
{
    Q_OBJECT
public:
    TreeLandLayerShellExtensionManager();
    ~TreeLandLayerShellExtensionManager() override;

    static TreeLandLayerShellExtensionManager *instance();
};

// Owned by the layer shell surface, and destroyed before its wl_surface.
class TreeLandLayerShellExtensionObject : public QObject,
                                        public QtWayland::treeland_layer_shell_extension_object_v1
{
    Q_OBJECT
public:
    explicit TreeLandLayerShellExtensionObject(struct ::treeland_layer_shell_extension_object_v1 *object);
    ~TreeLandLayerShellExtensionObject() override;

Q_SIGNALS:
    void resizingChanged(bool resizing);
    void resizeRejected(uint32_t error);

protected:
    void treeland_layer_shell_extension_object_v1_resizing(uint32_t resizing) override;
    void treeland_layer_shell_extension_object_v1_resize_rejected(uint32_t error) override;
};

DS_END_NAMESPACE
