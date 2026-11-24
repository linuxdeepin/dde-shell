// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "displaymodeapplet.h"

#include "pluginfactory.h"

#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusReply>
#include <QLoggingCategory>

#include <DConfig>
#include <DDBusSender>

Q_LOGGING_CATEGORY(osdDPLog, "org.deepin.dde.shell.osd.display")

namespace osd {

Q_DECLARE_LOGGING_CATEGORY(osdLog)

static DDBusSender displayInter()
{
    return DDBusSender().service("org.deepin.dde.Display1")
        .path("/org/deepin/dde/Display1")
        .interface("org.deepin.dde.Display1");
}

DPItem::DPItem(const QString &text, const QString &iconName, int mode, QObject *parent)
    : DPItem(text, "", iconName, mode, parent)
{

}

DPItem::DPItem(const QString &text, const QString &name, const QString &iconName, int mode, QObject *parent)
    : QObject(parent)
    , m_text(text)
    , m_name(name)
    , m_iconName(iconName)
    , m_mode(mode)
{

}

QString DPItem::text() const
{
    return m_text;
}

QString DPItem::name() const
{
    return m_name;
}

QString DPItem::iconName() const
{
    return m_iconName;
}

int DPItem::mode() const
{
    return m_mode;
}

QString DPItem::key() const
{
    return QString("%1-%2-%3").arg(m_text).arg(m_name).arg(m_mode);
}

DisPlayModeApplet::DisPlayModeApplet(QObject *parent)
    : DApplet(parent)
    , m_actionTimer(new QTimer(this))
{
    m_actionTimer->setSingleShot(true);
    m_actionTimer->setInterval(2000);
    QObject::connect(m_actionTimer, &QTimer::timeout, this, &DisPlayModeApplet::doAction);
}

int DisPlayModeApplet::state() const
{
    return m_state;
}

void DisPlayModeApplet::setState(int newState)
{
    if (m_state == newState)
        return;
    m_state = newState;
    emit stateChanged();
}

DPItem *DisPlayModeApplet::currentPlanItem() const
{
    return m_currentPlanItem;
}

QQmlListProperty<DPItem> DisPlayModeApplet::planItems()
{
    return QQmlListProperty<DPItem>(this, &m_planItems);
}

void DisPlayModeApplet::sync()
{
    if (!m_actionTimer->isActive())
        refreshDisplayState();

    setState(fetchState());
}

void DisPlayModeApplet::refreshDisplayState()
{
    m_planItemsValid = fetchPlanItems();
    const auto current = m_planItemsValid ? fetchCurrentPlanItem() : std::nullopt;
    setCurrentPlanItem(current.value_or(nullptr));
    Q_EMIT planItemsChanged();
}

void DisPlayModeApplet::next()
{
    if (!m_planItemsValid)
        refreshDisplayState();

    if (!m_planItemsValid || m_planItems.isEmpty())
        return;

    const auto nextIndex = (m_planItems.indexOf(m_currentPlanItem) + 1) % m_planItems.size();
    const auto item = m_planItems.at(nextIndex);
    setCurrentPlanItem(item);
    qCInfo(osdDPLog) << "next display mode" << item->mode() << item->name();
    m_actionTimer->start();
}

void DisPlayModeApplet::doAction()
{
    const auto item = currentPlanItem();
    if (!item)
        return;

    qCInfo(osdDPLog) << "Switch the mode" << item->mode() << item->name();

    QDBusReply<void> reply = displayInter().method("SwitchMode").arg(static_cast<uchar>(item->mode())).arg(item->name()).call();
    if (!reply.isValid()) {
        qCWarning(osdDPLog) << "Failed to call SwitchMode" << reply.error();
        return;
    }
}

void DisPlayModeApplet::setCurrentPlanItem(DPItem *newCurrentPlanItem)
{
    if (m_currentPlanItem == newCurrentPlanItem)
        return;
    m_currentPlanItem = newCurrentPlanItem;
    emit currentPlanItemChanged();
}

bool DisPlayModeApplet::fetchPlanItems()
{
    setCurrentPlanItem(nullptr);
    qDeleteAll(m_planItems);
    m_planItems.clear();

    QDBusReply<QStringList> listOutputNames = displayInter().method("ListOutputNames").call();
    if (!listOutputNames.isValid()) {
        qCWarning(osdDPLog) << "Failed to fetch ListOutputNames" << listOutputNames.error();
        return false;
    }
    auto outputNames = listOutputNames.value();

    m_planItems << new DPItem(tr("Duplicate"), "osd_multi_screen_copy", DPItem::Merge, this);
    m_planItems << new DPItem(tr("Extend"), "osd_multi_screen_extension", DPItem::Extend, this);
    if (outputNames.size() >= 2) {
        std::sort(outputNames.begin(), outputNames.end(), std::greater<QString>());
        for (int i = 0; i < outputNames.size(); i++) {
            const auto item = outputNames[i];
            const QString iconName = i < 2 ? QString("osd_multi_screen_only%1").arg(i + 1) : "osd_multi_screen_only_more";
            m_planItems << new DPItem(tr("Only on %1").arg(item), item, iconName, DPItem::Single, this);
        }
    }
    return true;
}

std::optional<DPItem *> DisPlayModeApplet::fetchCurrentPlanItem() const
{
    if (m_planItems.isEmpty())
        return nullptr;

    auto inter = displayInter();
    QDBusReply<QVariant> displayMode = inter.property("DisplayMode").get();
    if (!displayMode.isValid()) {
        qCWarning(osdDPLog) << "Failed to fetch DisplayMode" << displayMode.error();
        return std::nullopt;
    }
    auto mode = qdbus_cast<uchar>(displayMode);

    QDBusReply<QVariant> primaryScreen = inter.property("Primary").get();
    if (!primaryScreen.isValid()) {
        qCWarning(osdDPLog) << "Failed to fetch Primary" << primaryScreen.error();
        return std::nullopt;
    }
    auto screen = qdbus_cast<QString>(primaryScreen);

    auto it = std::find_if(m_planItems.begin(), m_planItems.end(), [this, mode, screen](const DPItem *item) {
        return (mode != DPItem::Single && mode == item->mode())
               || (mode == DPItem::Single && screen == item->name());
    });
    if (it != m_planItems.end())
        return *it;

    return nullptr;
}

int DisPlayModeApplet::fetchState() const
{
    DCORE_USE_NAMESPACE;

    int state = 1;
    QScopedPointer<DConfig> config(DConfig::create("org.deepin.dde.control-center", "org.deepin.dde.control-center", QString()));

    if (config->isValid()) {
        bool inHideModules = config->value("hideModule").toStringList().contains("display/mode");
        bool inDisableModules = config->value("disableModule").toStringList().contains("display/mode");
        if (!inHideModules && !inDisableModules) {
            state = 0;
        } else if (inDisableModules) {
            state = 2;
        }
    }
    return state;
}

D_APPLET_CLASS(DisPlayModeApplet)

}

#include "displaymodeapplet.moc"
