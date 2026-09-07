// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dsglobal.h"
#include "constants.h"

#include <QObject>
#include <DConfig>
#include <QScopedPointer>
#include <optional>

DCORE_USE_NAMESPACE


namespace dock {

class DockSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint dockSize READ dockSize WRITE setDockSize NOTIFY dockSizeChanged FINAL)
    Q_PROPERTY(HideMode hidemode READ hideMode WRITE setHideMode NOTIFY hideModeChanged FINAL)
    Q_PROPERTY(Position position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(ItemAlignment itemAlignment READ itemAlignment WRITE setItemAlignment NOTIFY itemAlignmentChanged FINAL)
    Q_PROPERTY(IndicatorStyle indicatorStyle READ indicatorStyle WRITE setIndicatorStyle NOTIFY indicatorStyleChanged FINAL)
    Q_PROPERTY(QVariantMap pluginsVisible READ pluginsVisible WRITE setPluginsVisible NOTIFY pluginsVisibleChanged FINAL)
    Q_PROPERTY(QString cardCurrent READ cardCurrent WRITE setCardCurrent NOTIFY cardCurrentChanged FINAL)
    Q_PROPERTY(bool showInPrimary READ showInPrimary WRITE setShowInPrimary NOTIFY showInPrimaryChanged FINAL)
    Q_PROPERTY(bool locked READ locked WRITE setLocked NOTIFY lockedChanged FINAL)
    Q_PROPERTY(bool contextMenuEnabled READ contextMenuEnabled NOTIFY contextMenuEnabledChanged FINAL)
    Q_PROPERTY(bool fashionModeEnabled READ fashionModeEnabled NOTIFY fashionModeEnabledChanged FINAL)

public:
    static DockSettings* instance();

    uint dockSize();
    HideMode hideMode();
    Position position();
    ItemAlignment itemAlignment();
    IndicatorStyle indicatorStyle();
    QVariantMap pluginsVisible();
    QString cardCurrent() const;
    bool showInPrimary() const;
    bool locked() const;
    bool contextMenuEnabled() const;
    // 时尚模式是否允许开启，由 DConfig 的 enableFashionMode 控制，默认关闭
    bool fashionModeEnabled() const;

    void setDockSize(const uint& size);
    void setHideMode(const HideMode& mode);
    void setPosition(const Position& position);
    void setItemAlignment(const ItemAlignment& alignment);
    void setIndicatorStyle(const IndicatorStyle& style);
    void setPluginsVisible(const QVariantMap & pluginsVisible);
    void setCardCurrent(const QString &cardCurrent);
    void setShowInPrimary(bool newShowInPrimary);
    void setLocked(bool newLocked);

private:
    enum WriteJob {
        dockSizeJob = 0,
        hideModeJob = 1,
        positionJob = 2,
        itemAlignmentJob = 3,
        indicatorStyleJob = 4,
    };

    explicit DockSettings(QObject *parent = nullptr);
    void init();

    void addWriteJob(WriteJob job);
    void logDockConfig(std::optional<Position> pos, std::optional<ItemAlignment> align, const QString &reason) const;
    inline void checkWriteJob();

Q_SIGNALS:
    void dockSizeChanged(uint size);
    void hideModeChanged(HideMode mode);
    void positionChanged(Position position);
    void itemAlignmentChanged(ItemAlignment alignment);
    void indicatorStyleChanged(IndicatorStyle style);
    void pluginsVisibleChanged(const QVariantMap &pluginsVisible);
    void cardCurrentChanged(const QString &cardCurrent);

    void showInPrimaryChanged(bool showInPrimary);
    void lockedChanged(bool locked);
    void contextMenuEnabledChanged(bool enabled);
    void fashionModeEnabledChanged(bool enabled);

private:
    QScopedPointer<DConfig> m_dockConfig;
    QTimer* m_writeTimer;
    // The current card changes on every wheel/swipe event, debounce the writes
    // separately so they cannot delay the other settings in m_writeJob.
    QTimer* m_cardCurrentWriteTimer;
    QList<WriteJob> m_writeJob;

    uint m_dockSize;
    HideMode m_hideMode;
    Position m_dockPosition;
    ItemAlignment m_alignment;
    IndicatorStyle m_style;
    QVariantMap m_pluginsVisible;
    QString m_cardCurrent;
    bool m_showInPrimary;
    bool m_locked;
    bool m_contextMenuEnabled;
    bool m_fashionModeEnabled = false;
};
}
