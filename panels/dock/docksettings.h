// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "constants.h"
#include "dockabstractsettingsconfig.h"

#include <QObject>
#include <DConfig>
#include <QScopedPointer>

DCORE_USE_NAMESPACE

DS_BEGIN_NAMESPACE

namespace dock {

class DockSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(HideMode hidemode READ hideMode WRITE setHideMode NOTIFY hideModeChanged FINAL)
    Q_PROPERTY(Position position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(DisplayMode displayMode READ displayMode WRITE setDisplayMode NOTIFY displayModeChanged FINAL)

    Q_PROPERTY(uint windowSizeFashion READ windowSizeFashion WRITE setWindowSizeFashion NOTIFY windowSizeFashionChanged FINAL)
    Q_PROPERTY(uint windowSizeEfficient READ windowSizeEfficient WRITE setWindowSizeEfficient NOTIFY windowSizeEfficientChanged FINAL)

public:
    static DockSettings* instance();
    HideMode hideMode();
    Position position();
    DisplayMode displayMode();

    uint windowSizeFashion();
    uint windowSizeEfficient();
    

    void setHideMode(HideMode mode);
    void setPosition(Position position);
    void setDisplayMode(DisplayMode mode);

    void setWindowSizeFashion(uint size);
    void setWindowSizeEfficient(uint size);

    void updateDockSettingsBackend(DockAbstractConfig* backend);

private:
    explicit DockSettings(QObject *parent = nullptr);
    void init();

Q_SIGNALS:
    void hideModeChanged(HideMode mode);
    void displayModeChanged(DisplayMode mode);
    void positionChanged(Position position);
    void windowSizeFashionChanged(uint size);
    void windowSizeEfficientChanged(uint size);

private:
    QScopedPointer<DockAbstractConfig> m_dockConfig;
};
}
DS_END_NAMESPACE
