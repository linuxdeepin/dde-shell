// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "layershellextension.h"

namespace dock
{

TreeLandLayerShellExtensionManager::TreeLandLayerShellExtensionManager()
    : QWaylandClientExtensionTemplate<TreeLandLayerShellExtensionManager>(treeland_layer_shell_extension_manager_v1_interface.version)
{
}

struct ::treeland_layer_shell_extension_object_v1 *TreeLandLayerShellExtensionManager::getLayerShellExtensionObject(struct ::wl_surface *surface)
{
    return get_layer_shell_extension_object(surface);
}

TreeLandLayerShellExtensionObject::TreeLandLayerShellExtensionObject(struct ::treeland_layer_shell_extension_object_v1 *surface,
                                                                     struct ::wl_surface *nativeSurface)
    : QWaylandClientExtensionTemplate<TreeLandLayerShellExtensionObject>(treeland_layer_shell_extension_object_v1_interface.version)
    , m_nativeSurface(nativeSurface)
{
    init(surface);
}

TreeLandLayerShellExtensionObject::~TreeLandLayerShellExtensionObject()
{
    if (object()) {
        destroy();
    }
}

struct ::wl_surface *TreeLandLayerShellExtensionObject::nativeSurface() const
{
    return m_nativeSurface;
}

void TreeLandLayerShellExtensionObject::beginResize(struct ::wl_seat *seat,
                                                    uint32_t serial,
                                                    uint32_t edges,
                                                    int32_t minWidth,
                                                    int32_t minHeight,
                                                    int32_t maxWidth,
                                                    int32_t maxHeight)
{
    begin_resize(seat, serial, edges, minWidth, minHeight, maxWidth, maxHeight);
}

void TreeLandLayerShellExtensionObject::treeland_layer_shell_extension_object_v1_resizing(uint32_t resizing)
{
    Q_EMIT resizingChanged(resizing != 0);
}

}
