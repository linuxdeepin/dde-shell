// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandCompositor>

#define protected public
#include <private/qwaylandcompositor_p.h>
#undef protected
#include <private/qwlqtkey_p.h>
#include <private/qwlqttouch_p.h>

int main (int argc, char *argv[]) {
  new QtWayland::QtKeyExtensionGlobal(nullptr);
  return 0;
}
