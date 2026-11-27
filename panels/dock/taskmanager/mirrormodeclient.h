// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "mirrormodepolicy.h"

#include <cstdint>
#include <xcb/xcb.h>

#include <QObject>
#include <QSet>
#include <QStringList>

namespace dock {

// Client side of the mirror mode window list KWin publishes at
// org.kde.KWin:/MirrorMode with the org.kde.KWin.MirrorMode interface.
// The object only exists in an X11 session; every call degrades to "nothing to
// see" when it is missing.
//
// Every call is asynchronous on purpose: the dock paints and handles input on the
// main thread, so neither a busy KWin nor a slow answer may block it.
class MirrorModeClient : public QObject
{
    Q_OBJECT

public:
    explicit MirrorModeClient(QObject *parent = nullptr);
    ~MirrorModeClient() override;

    // Re-reads the switch, the policy and both published window lists. The result
    // arrives through changed().
    void refresh();

    // Whether the window may be approved at all: it is listable and lives on the
    // output the mirror outputs show.
    bool isPrimary(uint32_t winId) const;
    // The raw listed flag; interpret it with mirrorModeIsApproved().
    bool isListed(uint32_t winId) const;
    // The switch and policy KWin publishes in the _DEEPIN_NET_MIRROR_MODE property.
    // Cached: it is read by every paint, and only a republish of the property or a
    // refresh() may change it.
    MirrorModePolicy policy() const;

    // Asks KWin to apply the listed flag through approveWindow()/removeWindow(). The
    // window lists KWin publishes afterwards are the authority on the result, so the
    // answer is only logged here and the button state comes back through changed().
    void setListed(uint32_t winId, bool listed);

    // True when the property change is the mirror mode switch being republished, so a
    // notify refreshes the cached state instead of polling it.
    bool isModeProperty(xcb_window_t window, xcb_atom_t atom) const;

Q_SIGNALS:
    // Emitted once the window lists have been re-read, never while they are half
    // updated.
    void changed();

private Q_SLOTS:
    void handleWindowsChanged();

private:
    void requestWindowList(const QString &name, quint64 generation);
    void applyWindowList(const QString &name, const QStringList &windows);

    QStringList m_primaryWindows;
    QStringList m_mirrorWindows;
    MirrorModePolicy m_policy = MirrorModePolicy::Off;
    QSet<uint32_t> m_pendingWrites;
    // Bumped by every refresh() so an answer to a superseded request is dropped.
    quint64 m_generation = 0;
    int m_pendingLists = 0;
};

} // namespace dock
