// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "abstracttaskmanagerinterface.h"
#include "appitem.h"

#include "abstractwindow.h"
#include "abstractwindowmonitor.h"
#include "desktopfileamparser.h"
#include "desktopfileparserfactory.h"
#include "dockcombinemodel.h"
#include "dockglobalelementmodel.h"
#include "dsglobal.h"
#include "globals.h"
#include "hoverpreviewproxymodel.h"
#include "itemmodel.h"
#include "pluginfactory.h"
#include "taskmanager.h"
#include "taskmanageradaptor.h"
#include "taskmanagersettings.h"
#include "treelandwindowmonitor.h"

#include <QGuiApplication>
#include <QStringLiteral>
#include <QUrl>
#include <QStandardPaths>
#include <QProcess>

#include <appletbridge.h>
#include <DSGApplication>

#ifdef BUILD_WITH_X11
#include "x11windowmonitor.h"
#include "x11utils.h"
#endif

Q_LOGGING_CATEGORY(taskManagerLog, "dde.shell.dock.taskmanager", QtDebugMsg)

#define Settings TaskManagerSettings::instance()

#define DESKTOPFILEFACTORY DesktopfileParserFactory<    \
                            DesktopFileAMParser,        \
                            DesktopfileAbstractParser   \
                        >

namespace dock {

// 通过AM(Application Manager)匹配应用程序的辅助函数
static QString getDesktopIdByPid(const QStringList &identifies)
{
    if (identifies.isEmpty())
        return {};

    pid_t windowPid = identifies.last().toInt();
    if (windowPid <= 0)
        return {};

    auto appId = DSGApplication::getId(windowPid);
    if (appId.isEmpty()) {
        qCDebug(taskManagerLog) << "appId is empty, AM failed to identify window with pid:" << windowPid;
        return {};
    }
        
    return QString::fromUtf8(appId);
}

class BoolFilterModel : public QSortFilterProxyModel, public AbstractTaskManagerInterface
{
    Q_OBJECT
public:
    explicit BoolFilterModel(QAbstractItemModel *sourceModel, int role, QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
        , AbstractTaskManagerInterface(this)
        , m_role(role)
    {
        setSourceModel(sourceModel);
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (sourceRow >= sourceModel()->rowCount())
            return false;

        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        return !sourceModel()->data(index, m_role).toBool();
    }

private:
    int m_role;
};

TaskManager::TaskManager(QObject *parent)
    : DContainment(parent)
    , AbstractTaskManagerInterface(nullptr)
    , m_windowFullscreen(false)
{
    connect(Settings, &TaskManagerSettings::allowedForceQuitChanged, this, &TaskManager::allowedForceQuitChanged);
    connect(Settings, &TaskManagerSettings::windowSplitChanged, this, &TaskManager::windowSplitChanged);
}

bool TaskManager::load()
{
    auto platformName = QGuiApplication::platformName();
    if (QStringLiteral("wayland") == platformName) {
        m_windowMonitor.reset(new TreeLandWindowMonitor());
    }

#ifdef BUILD_WITH_X11
    else if (QStringLiteral("xcb") == platformName) {
        m_windowMonitor.reset(new X11WindowMonitor());
    }
#endif

    connect(m_windowMonitor.get(), &AbstractWindowMonitor::windowAdded, this, &TaskManager::handleWindowAdded);
    return true;
}

bool TaskManager::init()
{
    auto adaptor = new TaskManagerAdaptor(this);
    Q_UNUSED(adaptor)
    QDBusConnection::sessionBus().registerService("org.deepin.ds.Dock.TaskManager");
    QDBusConnection::sessionBus().registerObject("/org/deepin/ds/Dock/TaskManager", "org.deepin.ds.Dock.TaskManager", this);

    DApplet::init();

    DS_NAMESPACE::DAppletBridge bridge("org.deepin.ds.dde-apps");
    BoolFilterModel *leftModel = new BoolFilterModel(m_windowMonitor.data(), m_windowMonitor->roleNames().key("shouldSkip"), this);
    if (auto applet = bridge.applet()) {
        auto model = applet->property("appModel").value<QAbstractItemModel *>();
        Q_ASSERT(model);
        m_activeAppModel = new DockCombineModel(leftModel, model, TaskManager::IdentityRole, [](QVariant data, QAbstractItemModel *model) -> QModelIndex {
            auto roleNames = model->roleNames();
            QList<QByteArray> identifiedOrders = {MODEL_DESKTOPID, MODEL_STARTUPWMCLASS, MODEL_NAME, MODEL_ICONNAME};

            auto identifies = data.toStringList();
            for (auto id : identifies) {
                if (id.isEmpty()) {
                    continue;
                }

                for (auto identifiedOrder : identifiedOrders) {
                    auto res = model->match(model->index(0, 0), roleNames.key(identifiedOrder), id, 1, Qt::MatchFixedString | Qt::MatchWrap).value(0);
                    if (res.isValid()) {
                        qCDebug(taskManagerLog) << "matched" << res;
                        return res;
                    }
                }
            }

            // 尝试通过AM(Application Manager)匹配应用程序
            if (Settings->cgroupsBasedGrouping()) {
                auto desktopId = getDesktopIdByPid(identifies);
                if (!desktopId.isEmpty() && !Settings->cgroupsBasedGroupingSkipIds().contains(desktopId)) {
                    auto res = model->match(model->index(0, 0), roleNames.key(MODEL_DESKTOPID), desktopId, 1, Qt::MatchFixedString | Qt::MatchWrap).value(0);
                    if (res.isValid()) {
                        qCDebug(taskManagerLog) << "matched by AM desktop ID:" << desktopId << res;
                        return res;
                    }
                }
            }

            auto res = model->match(model->index(0, 0), roleNames.key(MODEL_DESKTOPID), identifies.value(0), 1, Qt::MatchEndsWith);
            qCDebug(taskManagerLog) << "matched" << res.value(0);
            return res.value(0);
        });

        m_dockGlobalElementModel = new DockGlobalElementModel(model, m_activeAppModel, this);
        m_itemModel = new DockItemModel(m_dockGlobalElementModel, this);

        // 初始化预览代理模型，基于合并后的数据
        m_hoverPreviewModel = new HoverPreviewProxyModel(this);
        m_hoverPreviewModel->setSourceModel(m_dockGlobalElementModel);

        connect(applet, SIGNAL(appModelReadyChanged(bool)), m_dockGlobalElementModel, SLOT(initDockedElements(bool)));
    }

    connect(m_windowMonitor.data(), &AbstractWindowMonitor::windowFullscreenChanged, this, [this] (bool isFullscreen) {
        m_windowFullscreen = isFullscreen;
        emit windowFullscreenChanged(isFullscreen);
    });

    connect(m_windowMonitor.data(), &AbstractWindowMonitor::previewShouldClear, this, [this]() {
        // 当预览窗口真正隐藏时，清空过滤条件
        if (m_hoverPreviewModel) {
            m_hoverPreviewModel->clearFilter();
        }
    });

    // 设置preview opacity
    DS_NAMESPACE::DAppletBridge appearanceBridge("org.deepin.ds.dde-appearance");
    auto appearanceApplet = appearanceBridge.applet();
    if (appearanceApplet) {
        modifyOpacityChanged();
        connect(appearanceApplet, SIGNAL(opacityChanged()), this, SLOT(modifyOpacityChanged()));
    }
    QTimer::singleShot(500, [this]() {
        if (m_windowMonitor)
            m_windowMonitor->start();
    });

    return true;
}

DockItemModel *TaskManager::dataModel() const
{
    return m_itemModel;
}

HoverPreviewProxyModel *TaskManager::hoverPreviewModel() const
{
    return m_hoverPreviewModel;
}

void TaskManager::requestActivate(const QModelIndex &index) const
{
    m_itemModel->requestActivate(index);
}

void TaskManager::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const
{
    m_itemModel->requestOpenUrls(index, urls);
}

void TaskManager::requestNewInstance(const QModelIndex &index, const QString &action) const
{
    m_itemModel->requestNewInstance(index, action);
}

void TaskManager::requestClose(const QModelIndex &index, bool force) const
{
    m_itemModel->requestClose(index, force);
}

void TaskManager::requestUpdateWindowIconGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate) const
{
    m_itemModel->requestUpdateWindowIconGeometry(index, geometry, delegate);
}

void TaskManager::requestPreview(const QModelIndex &index, QObject *relativePositionItem, int32_t previewXoffset, int32_t previewYoffset, uint32_t direction)
{
    if (!m_hoverPreviewModel) {
        qCWarning(taskManagerLog) << "TaskManager::requestPreview: m_hoverPreviewModel is null";
        return;
    }

    // Set the preview filter condition based on the incoming model index
    m_hoverPreviewModel->setFilterModelIndex(index);

    // Check if there are any windows after filtering
    if (m_hoverPreviewModel->rowCount() == 0) {
        qCDebug(taskManagerLog) << "TaskManager::requestPreview: No windows found for index";
        hideItemPreview();
        return;
    }

    m_windowMonitor->requestPreview(m_hoverPreviewModel, qobject_cast<QWindow *>(relativePositionItem), previewXoffset, previewYoffset, direction);
}

void TaskManager::requestWindowsView(const QModelIndexList &indexes) const
{
    m_itemModel->requestWindowsView(indexes);
}

void TaskManager::handleWindowAdded(QPointer<AbstractWindow> window)
{
    if (!window || window->shouldSkip() || window->getAppItem() != nullptr) return;

    // TODO: remove below code and use use model replaced.
    QModelIndexList res;
    if (m_activeAppModel) {
        res = m_activeAppModel->match(m_activeAppModel->index(0, 0), TaskManager::WinIdRole, window->id());
    }

    QSharedPointer<DesktopfileAbstractParser> desktopfile = nullptr;
    QString desktopId;
    if (res.size() > 0) {
        desktopId = res.first().data(m_activeAppModel->roleNames().key("desktopId")).toString();
        qCDebug(taskManagerLog()) << "identify by model:" << desktopId;
    }

    if (!desktopId.isEmpty()) {
        desktopfile = DESKTOPFILEFACTORY::createById(desktopId, "amAPP");
        qCDebug(taskManagerLog()) << "identify by AM:" << desktopId;
    }

    if (Settings->cgroupsBasedGrouping() && (desktopfile.isNull() || !desktopfile->isValied().first)) {
        desktopfile = DESKTOPFILEFACTORY::createByWindow(window);
        qCDebug(taskManagerLog()) << "identify by Fallback:" << desktopId;
    }

    auto appitem = desktopfile->getAppItem();

    if (appitem.isNull() || (appitem->hasWindow() && windowSplit())) {
        auto id = windowSplit() ? QString("%1@%2").arg(desktopfile->id()).arg(window->id()) : desktopfile->id();
        appitem = new AppItem(id);
    }

    appitem->appendWindow(window);
    appitem->setDesktopFileParser(desktopfile);

    ItemModel::instance()->addItem(appitem);
}

void TaskManager::dropFilesOnItem(const QString& itemId, const QStringList& urls)
{
    auto indexes = m_itemModel->match(m_itemModel->index(0, 0), TaskManager::ItemIdRole, itemId, 1, Qt::MatchExactly);
    if (indexes.isEmpty()) {
        return;
    }

    QList<QUrl> urlList;
    for (const QString &url : urls) {
        urlList.append(QUrl::fromLocalFile(url));
    }

    m_itemModel->requestOpenUrls(indexes.first(), urlList);
}

void TaskManager::hideItemPreview()
{
    m_windowMonitor->hideItemPreview();
}

bool TaskManager::allowForceQuit()
{
    return Settings->isAllowedForceQuit();
}

QString TaskManager::desktopIdToAppId(const QString& desktopId)
{
    return Q_LIKELY(desktopId.endsWith(".desktop")) ? desktopId.chopped(8) : desktopId;
}

bool TaskManager::requestDockByDesktopId(const QString& desktopID)
{
    if (desktopID.startsWith("internal/")) return false;
    QString appId = desktopIdToAppId(desktopID);
    // 检查应用是否已经在任务栏中，如果是则返回 false
    if (IsDocked(appId))
        return false;

    return RequestDock(appId);
}

bool TaskManager::requestUndockByDesktopId(const QString& desktopID)
{
    if (desktopID.startsWith("internal/")) return false;
    return RequestUndock(desktopIdToAppId(desktopID));
}

bool TaskManager::RequestDock(QString appID)
{
    auto desktopfileParser = DESKTOPFILEFACTORY::createById(appID, "amAPP");

    auto res = desktopfileParser->isValied();
    if (!res.first) {
        qCWarning(taskManagerLog) << res.second;
        return false;
    }

    QPointer<AppItem> appitem = desktopfileParser->getAppItem();
    if (appitem.isNull()) {
        appitem = new AppItem(appID);
        appitem->setDesktopFileParser(desktopfileParser);
        ItemModel::instance()->addItem(appitem);
    }
    appitem->setDocked(true);
    return true;
}

bool TaskManager::IsDocked(QString appID)
{
    auto desktopfileParser = DESKTOPFILEFACTORY::createById(appID, "amAPP");

    auto res = desktopfileParser->isValied();
    if (!res.first) {
        qCWarning(taskManagerLog) << res.second;
        return false;
    }

    QPointer<AppItem> appitem = desktopfileParser->getAppItem();
    if (appitem.isNull()) {
        return false;
    }
    return appitem->isDocked();
}

bool TaskManager::RequestUndock(QString appID)
{
    auto desktopfileParser = DESKTOPFILEFACTORY::createById(appID, "amAPP");
    auto res = desktopfileParser->isValied();
    if (!res.first) {
        qCWarning(taskManagerLog) << res.second;
        return false;
    }
    QPointer<AppItem> appitem = desktopfileParser->getAppItem();
    if (appitem.isNull()) {
        return false;
    }
    appitem->setDocked(false);
    return true;
}

bool TaskManager::windowSplit()
{
    return Settings->isWindowSplit();
}

bool TaskManager::windowFullscreen()
{
    return m_windowFullscreen;
}

void TaskManager::activateWindow(uint32_t windowID)
{
#ifdef BUILD_WITH_X11
    X11Utils::instance()->setActiveWindow(static_cast<xcb_window_t>(windowID));
#else
    qCWarning(taskManagerLog) << "activateWindow not supported on this platform";
    Q_UNUSED(windowID)
#endif
}

void TaskManager::saveDockElementsOrder(const QStringList &appIds)
{
    const QStringList &dockedElements = TaskManagerSettings::instance()->dockedElements();
    QStringList newDockedElements;
    for (const auto &appId : appIds) {
        auto desktopElement = QString("desktop/%1").arg(appId);
        if (dockedElements.contains(desktopElement) && !newDockedElements.contains(desktopElement)) {
            newDockedElements.append(desktopElement);
        }
    }
    TaskManagerSettings::instance()->setDockedElements(newDockedElements);
}

QString TaskManager::getTrashTipText()
{
    const auto count = queryTrashCount();
    return tr("%1 files").arg(count);
}

bool TaskManager::isTrashEmpty() const
{
    return queryTrashCount() == 0;
}

int TaskManager::queryTrashCount() const
{
    int count = 0;

    QProcess gio;
    gio.start("gio", QStringList() << "trash" << "--list");
    if (gio.waitForFinished(1000) && gio.exitStatus() == QProcess::NormalExit && gio.exitCode() == 0) {
        const QByteArray &out = gio.readAllStandardOutput();
        const QList<QByteArray> lines = out.split('\n');
        for (const QByteArray &l : lines) {
            if (!l.trimmed().isEmpty()) count++;
        }
        return count;
    }
    return count;
}

void TaskManager::modifyOpacityChanged()
{
    DS_NAMESPACE::DAppletBridge appearanceBridge("org.deepin.ds.dde-appearance");
    auto appearanceApplet = appearanceBridge.applet();
    if (appearanceApplet) {
        double opacity = appearanceApplet->property("opacity").toReal();
        if (auto x11Monitor = qobject_cast<X11WindowMonitor*>(m_windowMonitor.data())) {
            x11Monitor->setPreviewOpacity(opacity);
        }
    } else {
        qCWarning(taskManagerLog) << "modifyOpacityChanged: appearanceApplet is null";
    }
}

D_APPLET_CLASS(TaskManager)
}

#include "taskmanager.moc"
