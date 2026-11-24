// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mirrormodeclient.h"
#include "x11utils.h"

#include <memory>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusVariant>
#include <QLoggingCategory>

namespace dock {

namespace {
Q_LOGGING_CATEGORY(mirrorModeLog, "org.deepin.dde.shell.dock.taskmanager.mirrormode")

constexpr const char *kService = "org.kde.KWin";
constexpr const char *kPath = "/MirrorMode";
constexpr const char *kInterface = "org.kde.KWin.MirrorMode";
constexpr const char *kModeAtom = "_DEEPIN_NET_MIRROR_MODE";
// Everything here talks to a local peer; a hung KWin must not keep a call pending.
constexpr int kCallTimeoutMs = 1000;

// Single CARDINAL, see kwin's Workspace::updateMirrorModeProperty().
MirrorModePolicy fetchPolicy()
{
    X11Utils *utils = X11Utils::instance();
    xcb_connection_t *connection = utils->getXcbConnection();
    if (!connection) {
        return MirrorModePolicy::Off;
    }

    const xcb_atom_t atom = utils->getAtomByName(QString::fromLatin1(kModeAtom));
    if (atom == XCB_ATOM_NONE) {
        return MirrorModePolicy::Off;
    }

    const xcb_get_property_cookie_t cookie =
        xcb_get_property(connection, false, utils->getRootWindow(), atom, XCB_ATOM_CARDINAL, 0, 1);
    std::unique_ptr<xcb_get_property_reply_t> reply(xcb_get_property_reply(connection, cookie, nullptr));
    if (!reply || reply->type == XCB_ATOM_NONE || reply->format != 32
        || xcb_get_property_value_length(reply.get()) < static_cast<int>(sizeof(uint32_t))) {
        return MirrorModePolicy::Off;
    }

    return mirrorModePolicyFromRootProperty(*static_cast<const uint32_t *>(xcb_get_property_value(reply.get())));
}
} // namespace

MirrorModeClient::MirrorModeClient(QObject *parent)
    : QObject(parent)
{
    // The signal also fires when a window is added or removed, so the lists stay
    // in step with KWin without polling.
    QDBusConnection::sessionBus().connect(QString::fromLatin1(kService),
                                          QString::fromLatin1(kPath),
                                          QString::fromLatin1(kInterface),
                                          QStringLiteral("windowsChanged"),
                                          this,
                                          SLOT(handleWindowsChanged()));
}

MirrorModeClient::~MirrorModeClient()
{
    QDBusConnection::sessionBus().disconnect(QString::fromLatin1(kService),
                                             QString::fromLatin1(kPath),
                                             QString::fromLatin1(kInterface),
                                             QStringLiteral("windowsChanged"),
                                             this,
                                             SLOT(handleWindowsChanged()));
}

void MirrorModeClient::refresh()
{
    m_policy = fetchPolicy();

    // Supersede whatever is still on the wire: only this generation's answers are
    // applied, and changed() is emitted when both lists are in.
    const quint64 generation = ++m_generation;
    m_pendingLists = 0;
    requestWindowList(QStringLiteral("primaryWindows"), generation);
    requestWindowList(QStringLiteral("mirrorWindows"), generation);
}

void MirrorModeClient::requestWindowList(const QString &name, quint64 generation)
{
    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(kService),
                                                         QString::fromLatin1(kPath),
                                                         QStringLiteral("org.freedesktop.DBus.Properties"),
                                                         QStringLiteral("Get"));
    message << QString::fromLatin1(kInterface) << name;

    ++m_pendingLists;
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message, kCallTimeoutMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, name, generation](QDBusPendingCallWatcher *call) {
                call->deleteLater();
                if (generation != m_generation) {
                    return;
                }

                const QDBusMessage reply = call->reply();
                if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
                    // Keep the lists read last time: a failed read must not look like
                    // "KWin publishes no windows".
                    qCWarning(mirrorModeLog) << "failed to read" << name << ":" << reply.errorMessage();
                } else {
                    applyWindowList(name, reply.arguments().constFirst().value<QDBusVariant>().variant().toStringList());
                }

                if (--m_pendingLists <= 0) {
                    Q_EMIT changed();
                }
            });
}

void MirrorModeClient::applyWindowList(const QString &name, const QStringList &windows)
{
    if (name == QLatin1String("primaryWindows")) {
        m_primaryWindows = windows;
    } else if (name == QLatin1String("mirrorWindows")) {
        m_mirrorWindows = windows;
    }
}

void MirrorModeClient::handleWindowsChanged()
{
    refresh();
}

bool MirrorModeClient::isPrimary(uint32_t winId) const
{
    return winId != 0 && m_primaryWindows.contains(QString::number(winId));
}

bool MirrorModeClient::isListed(uint32_t winId) const
{
    return winId != 0 && m_mirrorWindows.contains(QString::number(winId));
}

MirrorModePolicy MirrorModeClient::policy() const
{
    return m_policy;
}

bool MirrorModeClient::isModeProperty(xcb_window_t window, xcb_atom_t atom) const
{
    X11Utils *utils = X11Utils::instance();
    return utils->getXcbConnection() && atom != XCB_ATOM_NONE && window == utils->getRootWindow()
        && atom == utils->getAtomByName(QString::fromLatin1(kModeAtom));
}

void MirrorModeClient::setListed(uint32_t winId, bool listed)
{
    if (winId == 0) {
        return;
    }
    if (m_pendingWrites.contains(winId)) {
        // The answer is on the wire and the lists will carry the result; a second write
        // for the same window would only race with it.
        qCInfo(mirrorModeLog) << "setListed" << winId << listed << "ignored, a write is already pending";
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(QString::fromLatin1(kService),
                                                         QString::fromLatin1(kPath),
                                                         QString::fromLatin1(kInterface),
                                                         listed ? QStringLiteral("approveWindow")
                                                                : QStringLiteral("removeWindow"));
    message << QVariant::fromValue<qulonglong>(winId);

    m_pendingWrites.insert(winId);
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message, kCallTimeoutMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, winId, listed](QDBusPendingCallWatcher *call) {
                call->deleteLater();
                m_pendingWrites.remove(winId);

                const QDBusMessage reply = call->reply();
                if (reply.type() != QDBusMessage::ReplyMessage) {
                    qCWarning(mirrorModeLog) << "setListed" << winId << listed << "failed:" << reply.errorMessage();
                } else if (reply.arguments().isEmpty() || !reply.arguments().constFirst().toBool()) {
                    // mirrormodedbus.cpp answers false only when the window is gone or
                    // cannot be listed, so that is the whole refusal signal KWin gives.
                    qCWarning(mirrorModeLog) << "setListed" << winId << listed << "was refused by KWin";
                }

                // The published lists, not the request, decide the button state.
                refresh();
            });
}

} // namespace dock
