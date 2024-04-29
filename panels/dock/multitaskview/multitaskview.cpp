// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "applet.h"
#include "multitaskview.h"
#include "pluginfactory.h"

#include <QBuffer>

#include <DDciIcon>
#include <DDBusSender>
#include <DWindowManagerHelper>
#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE

namespace dock {

MultiTaskView::MultiTaskView(QObject *parent)
    : DApplet(parent)
    , m_iconName("deepin-multitasking-view")
{
    connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasCompositeChanged, this, &MultiTaskView::visibleChanged);
}

bool MultiTaskView::init()
{
    DApplet::init();
    return true;
}

void MultiTaskView::openWorkspace()
{
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

bool MultiTaskView::visible() const
{
    return m_visible && DWindowManagerHelper::instance()->hasComposite();
}

DockItemInfo MultiTaskView::dockItemInfo()
{
    DockItemInfo info;
    info.name = "multitasking-view";
    info.displayName = tr("Multitasking View");
    info.itemKey = "multitasking-view";
    info.settingKey = "multitasking-view";
    info.visible = visible();
    {
        const auto lightPalette = DGuiApplicationHelper::instance()->applicationPalette(DGuiApplicationHelper::LightType);
        auto lightPixmap = DDciIcon::fromTheme("dcc-view").pixmap(
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
        auto darkPixmap = DDciIcon::fromTheme("dcc-view").pixmap(
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

    return info;
}

void MultiTaskView::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;

        Q_EMIT visibleChanged();
    }
}

D_APPLET_CLASS(MultiTaskView)
}


#include "multitaskview.moc"
