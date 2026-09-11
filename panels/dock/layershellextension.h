// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "qwayland-treeland-layer-shell-extension-unstable-v1.h"

#include <QtWaylandClient/QWaylandClientExtension>

#include <QObject>

namespace dock
{

// Client wrapper for the treeland_layer_shell_extension_manager_v1 protocol global.
// Lets the dock opt in to compositor-driven interactive resize: the compositor
// owns the drag state machine and drives the surface size through the standard
// layer-surface configure events (see treeland-layer-shell-extension-unstable-v1.xml).
class TreeLandLayerShellExtensionManager : public QWaylandClientExtensionTemplate<TreeLandLayerShellExtensionManager>,
                                           public QtWayland::treeland_layer_shell_extension_manager_v1
{
    Q_OBJECT

public:
    explicit TreeLandLayerShellExtensionManager();

    // Creates an interactive object for an existing wl_surface and returns the
    // raw protocol object; the caller wraps it in a TreeLandLayerShellExtensionObject.
    struct ::treeland_layer_shell_extension_object_v1 *getLayerShellExtensionObject(struct ::wl_surface *surface);
};

// Client wrapper for the treeland_layer_shell_extension_object_v1 protocol object bound
// to the layer surface. This is the single entry point for every interactive
// operation on a treeland layer-shell surface (resize today, move later).
class TreeLandLayerShellExtensionObject : public QWaylandClientExtensionTemplate<TreeLandLayerShellExtensionObject>,
                                          public QtWayland::treeland_layer_shell_extension_object_v1
{
    Q_OBJECT

public:
    explicit TreeLandLayerShellExtensionObject(struct ::treeland_layer_shell_extension_object_v1 *surface, struct ::wl_surface *nativeSurface);
    ~TreeLandLayerShellExtensionObject();

    void beginResize(struct ::wl_seat *seat, uint32_t serial, uint32_t edges, int32_t minWidth, int32_t minHeight, int32_t maxWidth, int32_t maxHeight);

    // Returns the wl_surface this object was bound to. Does not validate
    // lifetime; callers must compare it against the window's current surface
    // and re-bind (a new object) when it changed, before issuing any request.
    struct ::wl_surface *nativeSurface() const;

Q_SIGNALS:
    void resizingChanged(bool resizing);

protected:
    void treeland_layer_shell_extension_object_v1_resizing(uint32_t resizing) override;

private:
    struct ::wl_surface *m_nativeSurface = nullptr;
};

}
