// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
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
#include "popupsortutils.h"
#include "taskmanager.h"
#include "taskmanageradaptor.h"
#include "taskmanagersettings.h"
#include "textcalculator.h"
#include "treelandwindowmonitor.h"

#include <QDesktopServices>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLocale>
#include <QMetaObject>
#include <QMimeDatabase>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QTemporaryFile>
#include <QUrl>
#include <QVariantList>
#include <QtQml/QtQml>

#include <DThumbnailProvider>

#include <libintl.h>

#include <appletbridge.h>
#include <DSGApplication>

#ifdef BUILD_WITH_X11
#include "x11windowmonitor.h"
#include "x11utils.h"
#endif

Q_LOGGING_CATEGORY(taskManagerLog, "org.deepin.dde.shell.dock.taskmanager", QtDebugMsg)

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

// 尝试通过 AM(Application Manager) 匹配应用程序
static QModelIndex tryMatchByApplicationManager(const QStringList &identifies,
                                                  QAbstractItemModel *model,
                                                  const QHash<int, QByteArray> &roleNames)
{
    Q_ASSERT(model);

    if (!Settings->cgroupsBasedGrouping()) {
        return QModelIndex();
    }

    auto desktopId = getDesktopIdByPid(identifies);
    if (desktopId.isEmpty() || Settings->cgroupsBasedGroupingSkipIds().contains(desktopId)) {
        return QModelIndex();
    }

    // 先在 model 中查找 desktopId 对应的 item
    auto res = model->match(model->index(0, 0), roleNames.key(MODEL_DESKTOPID),
                           desktopId, 1, Qt::MatchFixedString | Qt::MatchWrap).value(0);

    if (!res.isValid()) {
        return QModelIndex();
    }

    // 检查应用的 Categories 是否在跳过列表中
    auto skipCategories = Settings->cgroupsBasedGroupingSkipCategories();
    if (!skipCategories.isEmpty()) {
        QStringList categories = res.data(TaskManager::CategoriesRole).toStringList();
        if (!categories.isEmpty()) {
            // 检查是否有任何 skipCategory 在应用的 categories 中
            for (const QString &skipCategory : skipCategories) {
                if (categories.contains(skipCategory)) {
                    qCDebug(taskManagerLog) << "Skipping cgroups grouping for" << desktopId
                                           << "due to category:" << skipCategory;
                    return QModelIndex();
                }
            }
        }
    }

    qCDebug(taskManagerLog) << "matched by AM desktop ID:" << desktopId << res;
    return res;
}

static QStringList filteredIdentityCandidates(const QStringList &identifies, bool allowNumeric)
{
    QStringList filtered;
    for (const QString &identity : identifies) {
        if (identity.isEmpty() || filtered.contains(identity)) {
            continue;
        }

        bool isNumeric = false;
        identity.toLongLong(&isNumeric);
        if (!allowNumeric && isNumeric) {
            continue;
        }

        filtered.append(identity);
    }

    return filtered;
}

static QModelIndex matchLauncherAppByRoles(const QStringList &identifies,
                                           QAbstractItemModel *model,
                                           const QHash<int, QByteArray> &roleNames,
                                           const QList<QByteArray> &roleOrder)
{
    if (!model || identifies.isEmpty()) {
        return {};
    }

    const QModelIndex startIndex = model->index(0, 0);
    for (const QByteArray &roleName : roleOrder) {
        const int role = roleNames.key(roleName, -1);
        if (role < 0) {
            continue;
        }

        for (const QString &identity : identifies) {
            const QModelIndex result = model->match(startIndex,
                                                    role,
                                                    identity,
                                                    1,
                                                    Qt::MatchFixedString | Qt::MatchWrap).value(0);
            if (result.isValid()) {
                qCDebug(taskManagerLog) << "matched by role:" << roleName << identity << result;
                return result;
            }
        }
    }

    return {};
}

static QModelIndex matchLauncherApplication(const QStringList &identifies,
                                            QAbstractItemModel *model,
                                            const QHash<int, QByteArray> &roleNames)
{
    const QStringList exactCandidates = filteredIdentityCandidates(identifies, true);
    const QModelIndex exactResult = matchLauncherAppByRoles(exactCandidates,
                                                            model,
                                                            roleNames,
                                                            {MODEL_DESKTOPID, MODEL_STARTUPWMCLASS});
    if (exactResult.isValid()) {
        return exactResult;
    }

    const QModelIndex amMatchResult = tryMatchByApplicationManager(identifies, model, roleNames);
    if (amMatchResult.isValid()) {
        return amMatchResult;
    }

    const QStringList textCandidates = filteredIdentityCandidates(identifies, false);
    const QModelIndex fallbackResult = matchLauncherAppByRoles(textCandidates,
                                                               model,
                                                               roleNames,
                                                               {MODEL_NAME, MODEL_ICONNAME});
    if (fallbackResult.isValid()) {
        return fallbackResult;
    }

    const int desktopIdRole = roleNames.key(MODEL_DESKTOPID, -1);
    if (desktopIdRole < 0) {
        return {};
    }

    const QString fallbackIdentity = !textCandidates.isEmpty() ? textCandidates.constFirst()
                                                               : exactCandidates.value(0);
    if (fallbackIdentity.isEmpty()) {
        return {};
    }

    const QModelIndex result = model->match(model->index(0, 0),
                                            desktopIdRole,
                                            fallbackIdentity,
                                            1,
                                            Qt::MatchEndsWith).value(0);
    qCDebug(taskManagerLog) << "matched by desktopId suffix:" << fallbackIdentity << result;
    return result;
}

static QPair<QString, QString> splitDockElement(const QString &dockElement)
{
    const int separatorIndex = dockElement.indexOf(QLatin1Char('/'));
    if (separatorIndex <= 0) {
        return {};
    }

    return {dockElement.left(separatorIndex), dockElement.mid(separatorIndex + 1)};
}

static QString normalizedFolderPath(const QString &folderUrlOrPath)
{
    QUrl url(folderUrlOrPath);
    QString folderPath = url.isLocalFile() ? url.toLocalFile() : folderUrlOrPath;
    if (folderPath.isEmpty()) {
        return {};
    }

    QFileInfo fileInfo(folderPath);
    const QString canonicalPath = fileInfo.canonicalFilePath();
    if (!canonicalPath.isEmpty()) {
        return canonicalPath;
    }

    if (fileInfo.exists()) {
        return fileInfo.absoluteFilePath();
    }

    if (folderPath.startsWith(QLatin1Char('/'))) {
        return QDir::cleanPath(folderPath);
    }

    return {};
}

static bool isWithinBasePath(const QString &basePath, const QString &candidatePath)
{
    if (basePath.isEmpty() || candidatePath.isEmpty()) {
        return false;
    }

    if (basePath == candidatePath) {
        return true;
    }

    const QString prefix = basePath.endsWith(QLatin1Char('/')) ? basePath : basePath + QLatin1Char('/');
    return candidatePath.startsWith(prefix);
}

static QString localizedDesktopEntryText(QSettings &settings, const QString &key);

static QString displayNameForPath(const QString &path)
{
    const auto localizedStandardLocationName = [](QStandardPaths::StandardLocation location) {
        const auto englishDefaultName = [](QStandardPaths::StandardLocation standardLocation) -> QString {
            switch (standardLocation) {
            case QStandardPaths::HomeLocation:
                return QStringLiteral("Home");
            case QStandardPaths::DesktopLocation:
                return QStringLiteral("Desktop");
            case QStandardPaths::DocumentsLocation:
                return QStringLiteral("Documents");
            case QStandardPaths::DownloadLocation:
                return QStringLiteral("Download");
            case QStandardPaths::MoviesLocation:
                return QStringLiteral("Movies");
            case QStandardPaths::MusicLocation:
                return QStringLiteral("Music");
            case QStandardPaths::PicturesLocation:
                return QStringLiteral("Pictures");
            case QStandardPaths::TemplatesLocation:
                return QStringLiteral("Templates");
            case QStandardPaths::PublicShareLocation:
                return QStringLiteral("Public");
            case QStandardPaths::ApplicationsLocation:
                return QStringLiteral("Applications");
            default:
                return {};
            }
        };

        const QString localizedName = QStandardPaths::displayName(location);
        const QString englishName = englishDefaultName(location);
        if (!localizedName.isEmpty()
            && (englishName.isEmpty() || localizedName.compare(englishName, Qt::CaseInsensitive) != 0)) {
            return localizedName;
        }

        if (!englishName.isEmpty()) {
            const QByteArray englishNameUtf8 = englishName.toUtf8();
            const QString translatedName = QString::fromLocal8Bit(dgettext("xdg-user-dirs", englishNameUtf8.constData())).trimmed();
            if (!translatedName.isEmpty() && translatedName.compare(englishName, Qt::CaseInsensitive) != 0) {
                return translatedName;
            }
        }

        return QString();
    };

    QFileInfo fileInfo(path);
    const QString normalizedPath = QDir::cleanPath(fileInfo.absoluteFilePath().isEmpty() ? path : fileInfo.absoluteFilePath());
    const QString directoryEntryPath = QDir(normalizedPath).filePath(QStringLiteral(".directory"));
    if (QFileInfo::exists(directoryEntryPath)) {
        QSettings settings(directoryEntryPath, QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("Desktop Entry"));
        const QString localizedDirectoryName = localizedDesktopEntryText(settings, QStringLiteral("Name"));
        if (!localizedDirectoryName.isEmpty()) {
            return localizedDirectoryName;
        }
    }

    if (normalizedPath == QStringLiteral("/usr/share/applications")) {
        if (QLocale().language() == QLocale::Chinese) {
            return QStringLiteral("应用程序");
        }

        const QString applicationsName = QStandardPaths::displayName(QStandardPaths::ApplicationsLocation);
        if (!applicationsName.isEmpty()) {
            return applicationsName;
        }
    }

    const QList<QStandardPaths::StandardLocation> standardLocations = {
        QStandardPaths::HomeLocation,
        QStandardPaths::DesktopLocation,
        QStandardPaths::DocumentsLocation,
        QStandardPaths::DownloadLocation,
        QStandardPaths::MoviesLocation,
        QStandardPaths::MusicLocation,
        QStandardPaths::PicturesLocation,
        QStandardPaths::TemplatesLocation,
        QStandardPaths::PublicShareLocation,
    };
    for (const QStandardPaths::StandardLocation location : standardLocations) {
        const QString locationPath = QDir::cleanPath(QStandardPaths::writableLocation(location));
        if (!locationPath.isEmpty() && locationPath == normalizedPath) {
            const QString localizedName = localizedStandardLocationName(location);
            if (!localizedName.isEmpty()) {
                return localizedName;
            }
            break;
        }
    }

    QString name = fileInfo.fileName();
    if (name.isEmpty()) {
        name = fileInfo.absoluteFilePath();
    }
    if (name.isEmpty()) {
        name = path;
    }
    return name;
}

struct DesktopEntryMetadata
{
    QString displayName;
    QString iconName;
    bool hidden = false;
};

struct FilePresentationInfo
{
    QString displayName;
    QString iconName;
    bool hidden = false;
};

static bool isDesktopEntryFile(const QFileInfo &fileInfo)
{
    return fileInfo.isFile()
            && fileInfo.suffix().compare(QStringLiteral("desktop"), Qt::CaseInsensitive) == 0;
}

static QString localizedDesktopEntryText(QSettings &settings, const QString &key)
{
    if (key.isEmpty()) {
        return {};
    }

    QStringList localizedKeys;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &uiLanguage : uiLanguages) {
        QString normalizedLanguage = uiLanguage.trimmed();
        if (normalizedLanguage.isEmpty()) {
            continue;
        }

        normalizedLanguage.replace(QLatin1Char('-'), QLatin1Char('_'));

        const QString fullKey = QStringLiteral("%1[%2]").arg(key, normalizedLanguage);
        if (!localizedKeys.contains(fullKey)) {
            localizedKeys.append(fullKey);
        }

        const int separatorIndex = normalizedLanguage.indexOf(QLatin1Char('_'));
        if (separatorIndex > 0) {
            const QString baseLanguageKey = QStringLiteral("%1[%2]").arg(key, normalizedLanguage.left(separatorIndex));
            if (!localizedKeys.contains(baseLanguageKey)) {
                localizedKeys.append(baseLanguageKey);
            }
        }
    }

    for (const QString &localizedKey : localizedKeys) {
        const QString localizedValue = settings.value(localizedKey).toString().trimmed();
        if (!localizedValue.isEmpty()) {
            return localizedValue;
        }
    }

    return settings.value(key).toString().trimmed();
}

static DesktopEntryMetadata desktopEntryMetadataForFile(const QFileInfo &fileInfo)
{
    if (!isDesktopEntryFile(fileInfo)) {
        return {};
    }

    QSettings settings(fileInfo.absoluteFilePath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Desktop Entry"));

    DesktopEntryMetadata metadata;
    metadata.hidden = settings.value(QStringLiteral("Hidden")).toBool()
                      || settings.value(QStringLiteral("NoDisplay")).toBool();
    metadata.displayName = localizedDesktopEntryText(settings, QStringLiteral("Name"));
    metadata.iconName = settings.value(QStringLiteral("Icon")).toString().trimmed();
    return metadata;
}

static QString defaultFileIconName(const QFileInfo &fileInfo)
{
    if (fileInfo.isDir()) {
        return QStringLiteral("folder");
    }

    static QMimeDatabase mimeDatabase;
    const auto mimeType = mimeDatabase.mimeTypeForFile(fileInfo, QMimeDatabase::MatchDefault);
    if (!mimeType.iconName().isEmpty()) {
        return mimeType.iconName();
    }
    if (!mimeType.genericIconName().isEmpty()) {
        return mimeType.genericIconName();
    }

    return QStringLiteral("text-x-generic");
}

static FilePresentationInfo filePresentationInfo(const QFileInfo &fileInfo)
{
    FilePresentationInfo info;
    info.displayName = fileInfo.fileName();

    if (info.displayName.isEmpty()) {
        info.displayName = fileInfo.absoluteFilePath();
    }
    if (info.displayName.isEmpty()) {
        info.displayName = fileInfo.filePath();
    }

    if (fileInfo.isDir()) {
        info.iconName = QStringLiteral("folder");
        return info;
    }

    const DesktopEntryMetadata desktopEntry = desktopEntryMetadataForFile(fileInfo);
    info.hidden = desktopEntry.hidden;
    if (!desktopEntry.displayName.isEmpty()) {
        info.displayName = desktopEntry.displayName;
    }
    if (!desktopEntry.iconName.isEmpty()) {
        info.iconName = desktopEntry.iconName;
    }

    if (info.iconName.isEmpty()) {
        info.iconName = defaultFileIconName(fileInfo);
    }

    return info;
}

static QString fileIconName(const QFileInfo &fileInfo)
{
    return filePresentationInfo(fileInfo).iconName;
}

static QString thumbnailUrlForFile(const QFileInfo &fileInfo)
{
    if (fileInfo.isDir() || isDesktopEntryFile(fileInfo)) {
        return {};
    }

    auto *provider = Dtk::Gui::DThumbnailProvider::instance();
    if (!provider || !provider->hasThumbnail(fileInfo)) {
        return {};
    }

    const QString thumbnailPath = provider->thumbnailFilePath(fileInfo, Dtk::Gui::DThumbnailProvider::Small);
    if (!thumbnailPath.isEmpty() && QFileInfo::exists(thumbnailPath)) {
        return QUrl::fromLocalFile(thumbnailPath).toString();
    }

    provider->appendToProduceQueue(fileInfo, Dtk::Gui::DThumbnailProvider::Small);
    return {};
}

static bool launchDesktopEntryFile(const QFileInfo &fileInfo)
{
    if (!isDesktopEntryFile(fileInfo)) {
        return false;
    }

    const QString desktopFilePath = fileInfo.absoluteFilePath();
    if (desktopFilePath.isEmpty()) {
        return false;
    }

    return QProcess::startDetached(QStringLiteral("gio"),
                                   {QStringLiteral("launch"), desktopFilePath});
}

static QVariantMap popupEntry(const QString &entryId,
                              const QString &name,
                              const QString &iconName,
                              bool directory,
                              const QString &entryUrl = {},
                              const QString &thumbnailUrl = {})
{
    return {
        {QStringLiteral("entryId"), entryId},
        {QStringLiteral("name"), name},
        {QStringLiteral("iconName"), iconName},
        {QStringLiteral("directory"), directory},
        {QStringLiteral("entryUrl"), entryUrl},
        {QStringLiteral("thumbnailUrl"), thumbnailUrl},
    };
}

static qint64 dateTimeToSortValue(const QDateTime &dateTime)
{
    return dateTime.isValid() ? dateTime.toMSecsSinceEpoch() : 0;
}

static qint64 fileModifiedTimeForSort(const QFileInfo &fileInfo)
{
    return dateTimeToSortValue(fileInfo.lastModified());
}

static qint64 fileCreatedTimeForSort(const QFileInfo &fileInfo)
{
    const qint64 birthTime = dateTimeToSortValue(fileInfo.birthTime());
    if (birthTime > 0) {
        return birthTime;
    }

    const qint64 metadataTime = dateTimeToSortValue(fileInfo.metadataChangeTime());
    if (metadataTime > 0) {
        return metadataTime;
    }

    return fileModifiedTimeForSort(fileInfo);
}

static qint64 fileSizeForSort(const QFileInfo &fileInfo)
{
    return fileInfo.isDir() ? 0 : fileInfo.size();
}

static QString fileTypeSortText(const QFileInfo &fileInfo)
{
    if (fileInfo.isDir()) {
        return QStringLiteral("inode/directory");
    }

    static QMimeDatabase mimeDatabase;
    const auto mimeType = mimeDatabase.mimeTypeForFile(fileInfo, QMimeDatabase::MatchDefault);
    if (!mimeType.name().isEmpty()) {
        return mimeType.name();
    }

    return QStringLiteral("application/octet-stream");
}

static QVariantList directoryEntriesForPath(const QString &path, const PopupSortState &sortState)
{
    QList<PopupSortableEntry> entries;

    QDir directory(path);
    const QFileInfoList fileInfos = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                                            QDir::NoSort);
    for (const QFileInfo &fileInfo : fileInfos) {
        const QString entryPath = fileInfo.absoluteFilePath();
        const FilePresentationInfo presentation = filePresentationInfo(fileInfo);
        if (presentation.hidden) {
            continue;
        }

        PopupSortableEntry entry;
        entry.entryData = popupEntry(entryPath,
                                     presentation.displayName,
                                     presentation.iconName,
                                     fileInfo.isDir(),
                                     QUrl::fromLocalFile(entryPath).toString(),
                                     thumbnailUrlForFile(fileInfo));
        entry.name = presentation.displayName;
        entry.typeText = fileTypeSortText(fileInfo);
        entry.modifiedTime = fileModifiedTimeForSort(fileInfo);
        entry.createdTime = fileCreatedTimeForSort(fileInfo);
        entry.size = fileSizeForSort(fileInfo);
        entry.directory = fileInfo.isDir();
        entries.append(entry);
    }

    sortPopupEntries(&entries, sortState, true);

    QVariantList result;
    for (const PopupSortableEntry &entry : std::as_const(entries)) {
        result.append(entry.entryData);
    }

    return result;
}

static QStringList previewIconsForDirectory(const QString &path, int limit = 4)
{
    QStringList iconNames;

    QDir directory(path);
    const QFileInfoList fileInfos = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                                            QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &fileInfo : fileInfos) {
        const FilePresentationInfo presentation = filePresentationInfo(fileInfo);
        if (presentation.hidden) {
            continue;
        }

        iconNames.append(presentation.iconName);
        if (iconNames.size() >= limit) {
            break;
        }
    }

    return iconNames;
}

static QModelIndex findIndexByRole(QAbstractItemModel *model, int role, const QString &value)
{
    if (!model || value.isEmpty()) {
        return {};
    }

    return model->match(model->index(0, 0), role, value, 1, Qt::MatchExactly).value(0);
}

static int modelRole(QAbstractItemModel *model, const QByteArray &roleName, int fallbackRole)
{
    if (!model) {
        return fallbackRole;
    }

    return model->roleNames().key(roleName, fallbackRole);
}

static QModelIndex findIndexByNamedRole(QAbstractItemModel *model,
                                        const QByteArray &roleName,
                                        const QString &value,
                                        int fallbackRole)
{
    return findIndexByRole(model, modelRole(model, roleName, fallbackRole), value);
}

static bool isLauncherFolderId(const QString &launcherId)
{
    return launcherId.startsWith(QStringLiteral("internal/folder/")) ||
           launcherId.startsWith(QStringLiteral("internal/folders/")) ||
           launcherId.startsWith(QStringLiteral("internal/group/"));
}

static int launcherGroupNumber(const QString &launcherId)
{
    if (launcherId.isEmpty()) {
        return -1;
    }

    if (isLauncherFolderId(launcherId)) {
        return launcherId.section(QLatin1Char('/'), -1).toInt();
    }

    bool ok = false;
    const int groupId = launcherId.toInt(&ok);
    return ok ? groupId : -1;
}

static bool isLauncherRootFolderId(const QString &launcherId)
{
    return launcherGroupNumber(launcherId) == 0;
}

static QString alternateLauncherFolderId(const QString &launcherId)
{
    const QString singularPrefix = QStringLiteral("internal/folder/");
    const QString pluralPrefix = QStringLiteral("internal/folders/");
    const QString legacyPrefix = QStringLiteral("internal/group/");

    if (launcherId.startsWith(singularPrefix)) {
        return pluralPrefix + launcherId.mid(singularPrefix.size());
    }

    if (launcherId.startsWith(pluralPrefix)) {
        return singularPrefix + launcherId.mid(pluralPrefix.size());
    }

    if (launcherId.startsWith(legacyPrefix)) {
        return pluralPrefix + launcherId.mid(legacyPrefix.size());
    }

    return {};
}

static QString resolveLauncherGroupId(QAbstractItemModel *groupModel, const QString &groupId)
{
    if (!groupModel || !isLauncherFolderId(groupId)) {
        return groupId;
    }

    const QString suffix = groupId.section(QLatin1Char('/'), -1);
    const QStringList candidates = {
        groupId,
        alternateLauncherFolderId(groupId),
        QStringLiteral("internal/folders/%1").arg(suffix),
        QStringLiteral("internal/folder/%1").arg(suffix),
        QStringLiteral("internal/group/%1").arg(suffix),
        suffix
    };

    for (const QString &candidate : candidates) {
        if (!candidate.isEmpty() &&
            findIndexByNamedRole(groupModel, MODEL_DESKTOPID, candidate, TaskManager::DesktopIdRole).isValid()) {
            return candidate;
        }
    }

    return groupId;
}

static QStringList invokeLauncherGroupItems(QObject *groupModel, const QString &groupId)
{
    if (!groupModel || groupId.isEmpty()) {
        return {};
    }

    const QString resolvedGroupId = resolveLauncherGroupId(qobject_cast<QAbstractItemModel *>(groupModel), groupId);
    QStringList items;
    QMetaObject::invokeMethod(groupModel,
                              "groupItems",
                              Q_RETURN_ARG(QStringList, items),
                              Q_ARG(QString, resolvedGroupId));
    return items;
}

static QString invokeLauncherGroupDisplayName(QObject *groupModel, const QString &groupId)
{
    if (!groupModel || groupId.isEmpty()) {
        return {};
    }

    const QString resolvedGroupId = resolveLauncherGroupId(qobject_cast<QAbstractItemModel *>(groupModel), groupId);
    QString displayName;
    QMetaObject::invokeMethod(groupModel,
                              "groupDisplayName",
                              Q_RETURN_ARG(QString, displayName),
                              Q_ARG(QString, resolvedGroupId));
    return displayName;
}

static bool preferChineseLauncherGroupNames()
{
    return QLocale().language() == QLocale::Chinese;
}

static QString launcherGroupFallbackName()
{
    return preferChineseLauncherGroupNames() ? QStringLiteral("应用组") : TaskManager::tr("App Group");
}

static QString translatedLauncherCategoryName(int category)
{
    if (preferChineseLauncherGroupNames()) {
        switch (category) {
        case 0: return QStringLiteral("网络");
        case 1: return QStringLiteral("社交");
        case 2: return QStringLiteral("音乐");
        case 3: return QStringLiteral("视频");
        case 4: return QStringLiteral("图形图像");
        case 5: return QStringLiteral("游戏");
        case 6: return QStringLiteral("办公");
        case 7: return QStringLiteral("阅读");
        case 8: return QStringLiteral("编程开发");
        case 9: return QStringLiteral("系统管理");
        case 10: return QStringLiteral("其他");
        default: return {};
        }
    }

    switch (category) {
    case 0: return TaskManager::tr("Internet");
    case 1: return TaskManager::tr("Chat");
    case 2: return TaskManager::tr("Music");
    case 3: return TaskManager::tr("Video");
    case 4: return TaskManager::tr("Graphics");
    case 5: return TaskManager::tr("Game");
    case 6: return TaskManager::tr("Office");
    case 7: return TaskManager::tr("Reading");
    case 8: return TaskManager::tr("Development");
    case 9: return TaskManager::tr("System");
    case 10: return TaskManager::tr("Others");
    default: return {};
    }
}

static QString translatedLauncherCategoryName(const QString &groupName)
{
    if (!groupName.startsWith(QStringLiteral("internal/category/"))) {
        return {};
    }

    bool ok = false;
    const int category = groupName.section(QLatin1Char('/'), -1).toInt(&ok);
    if (!ok) {
        return {};
    }

    return translatedLauncherCategoryName(category);
}

static qint64 launcherInstalledTimeForSort(const QModelIndex &appIndex)
{
    const qint64 installedTime = appIndex.data(TaskManager::InstalledTimeRole).toLongLong();
    if (installedTime > 0) {
        return installedTime;
    }

    const QFileInfo fileInfo(appIndex.data(TaskManager::DesktopSourcePathRole).toString());
    if (!fileInfo.exists()) {
        return 0;
    }

    return fileCreatedTimeForSort(fileInfo);
}

static qint64 launcherModifiedTimeForSort(const QModelIndex &appIndex)
{
    const QFileInfo fileInfo(appIndex.data(TaskManager::DesktopSourcePathRole).toString());
    if (!fileInfo.exists()) {
        return 0;
    }

    return fileModifiedTimeForSort(fileInfo);
}

static qint64 launcherSizeForSort(const QModelIndex &appIndex)
{
    const QFileInfo fileInfo(appIndex.data(TaskManager::DesktopSourcePathRole).toString());
    if (!fileInfo.exists()) {
        return 0;
    }

    return fileInfo.size();
}

static QString launcherEntryTypeText(const QModelIndex &appIndex)
{
    const QString categoryName = translatedLauncherCategoryName(appIndex.data(TaskManager::DDECategoryRole).toInt());
    if (!categoryName.isEmpty()) {
        return categoryName;
    }

    const QStringList categories = appIndex.data(TaskManager::CategoriesRole).toStringList();
    if (!categories.isEmpty()) {
        return categories.constFirst();
    }

    return QStringLiteral("application");
}

static QString launcherGroupDisplayName(QAbstractItemModel *groupModel, const QString &groupId)
{
    QString groupName = invokeLauncherGroupDisplayName(groupModel, groupId);
    if (groupName.isEmpty()) {
        const QString resolvedGroupId = resolveLauncherGroupId(groupModel, groupId);
        const QModelIndex groupIndex = findIndexByNamedRole(groupModel, MODEL_DESKTOPID, resolvedGroupId, TaskManager::DesktopIdRole);
        groupName = groupIndex.data(modelRole(groupModel, MODEL_NAME, TaskManager::NameRole)).toString();
    }
    const QString categoryName = translatedLauncherCategoryName(groupName);
    if (!categoryName.isEmpty()) {
        return categoryName;
    }
    if (groupName.isEmpty()) {
        groupName = launcherGroupFallbackName();
    }
    return groupName;
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
        setDynamicSortFilter(false);
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
    qmlRegisterType<TextCalculator>("org.deepin.ds.dock.taskmanager", 1, 0, "TextCalculator");
    qmlRegisterUncreatableType<TextCalculatorAttached>("org.deepin.ds.dock.taskmanager", 1, 0, "TextCalculatorAttached", "TextCalculator Attached");

    connect(Settings, &TaskManagerSettings::allowedForceQuitChanged, this, &TaskManager::allowedForceQuitChanged);
    connect(Settings, &TaskManagerSettings::windowSplitChanged, this, &TaskManager::windowSplitChanged);

    if (auto *thumbnailProvider = Dtk::Gui::DThumbnailProvider::instance()) {
        const auto notifyThumbnailChanged = [this](const QString &sourceFilePath, const QString &) {
            if (!sourceFilePath.isEmpty()) {
                Q_EMIT popupEntryThumbnailChanged(QDir::cleanPath(sourceFilePath));
            }
        };
        connect(thumbnailProvider, &Dtk::Gui::DThumbnailProvider::thumbnailChanged, this, notifyThumbnailChanged);
        connect(thumbnailProvider, &Dtk::Gui::DThumbnailProvider::createThumbnailFinished, this, notifyThumbnailChanged);
    }

    m_trashCountProcess = new QProcess(this);
    m_trashCountProcess->setProgram(QStringLiteral("gio"));
    m_trashCountProcess->setArguments({QStringLiteral("trash"), QStringLiteral("--list")});
    connect(m_trashCountProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const int nextTrashCount = (exitStatus == QProcess::NormalExit && exitCode == 0)
                    ? trashCountFromOutput(m_trashCountProcess->readAllStandardOutput())
                    : m_cachedTrashCount;
                const bool stateChanged = !m_trashStateInitialized || m_cachedTrashCount != nextTrashCount;
                m_cachedTrashCount = nextTrashCount;
                m_trashStateInitialized = true;
                if (m_trashCountRefreshTimer.isValid()) {
                    m_trashCountRefreshTimer.restart();
                } else {
                    m_trashCountRefreshTimer.start();
                }
                if (stateChanged) {
                    emit trashStateChanged();
                }
            });
    connect(m_trashCountProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (!m_trashStateInitialized) {
            m_trashStateInitialized = true;
            if (m_trashCountRefreshTimer.isValid()) {
                m_trashCountRefreshTimer.restart();
            } else {
                m_trashCountRefreshTimer.start();
            }
            emit trashStateChanged();
        }
    });

    refreshTrashCount(true);
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
        auto groupModel = applet->property("appGroupModel").value<QAbstractItemModel *>();
        Q_ASSERT(model);
        m_launcherAppModel = model;
        m_launcherGroupModel = groupModel;
        if (m_launcherGroupModel) {
            const int desktopIdRole = modelRole(m_launcherGroupModel, MODEL_DESKTOPID, DesktopIdRole);
            const int nameRole = modelRole(m_launcherGroupModel, MODEL_NAME, NameRole);
            qCWarning(taskManagerLog) << "launcher group model rows" << m_launcherGroupModel->rowCount();
            for (int i = 0; i < m_launcherGroupModel->rowCount(); ++i) {
                const QModelIndex groupIndex = m_launcherGroupModel->index(i, 0);
                qCWarning(taskManagerLog) << "launcher group model item"
                                          << i
                                          << groupIndex.data(desktopIdRole).toString()
                                          << groupIndex.data(nameRole).toString();
            }
        }
        m_activeAppModel = new DockCombineModel(leftModel, model, TaskManager::IdentityRole, [](QVariant data, QAbstractItemModel *model) -> QModelIndex {
            return matchLauncherApplication(data.toStringList(), model, model->roleNames());
        });

        m_dockGlobalElementModel = new DockGlobalElementModel(model, m_activeAppModel, groupModel, this);
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
    dataModel()->requestActivate(index);
}

void TaskManager::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls) const
{
    dataModel()->requestOpenUrls(index, urls);
}

void TaskManager::requestNewInstance(const QModelIndex &index, const QString &action) const
{
    dataModel()->requestNewInstance(index, action);
}

void TaskManager::requestClose(const QModelIndex &index, bool force) const
{
    dataModel()->requestClose(index, force);
}

void TaskManager::requestUpdateWindowIconGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate) const
{
    dataModel()->requestUpdateWindowIconGeometry(index, geometry, delegate);
}

void TaskManager::requestPreview(const QModelIndex &index,
                                 QObject *relativePositionItem,
                                 int32_t previewXoffset,
                                 int32_t previewYoffset,
                                 uint32_t direction)
{
    if (!m_hoverPreviewModel) {
        qCWarning(taskManagerLog) << "TaskManager::requestPreview: m_hoverPreviewModel is null";
        return;
    }

    // Set the preview filter condition based on the incoming model index
    if (windowSplit()) {
        QString winId = index.data(TaskManager::WinIdRole).toString();
        m_hoverPreviewModel->setFilter(winId, HoverPreviewProxyModel::FilterByWinId);
    } else {
        QString appId = index.data(TaskManager::DesktopIdRole).toString();
        m_hoverPreviewModel->setFilter(appId, HoverPreviewProxyModel::FilterByAppId);
    }

    // Check if there are any windows after filtering
    if (m_hoverPreviewModel->rowCount() == 0) {
        qCDebug(taskManagerLog) << "TaskManager::requestPreview: No windows found for index";
        hideItemPreview();
        return;
    }

    m_windowMonitor->requestPreview(m_hoverPreviewModel,
                                    qobject_cast<QWindow *>(relativePositionItem),
                                    previewXoffset,
                                    previewYoffset,
                                    direction);
}

void TaskManager::requestWindowsView(const QModelIndexList &indexes) const
{
    dataModel()->requestWindowsView(indexes);
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

    if (desktopfile.isNull()) {
        desktopfile = DesktopfileParserFactory<DesktopfileAbstractParser>::createByWindow(window);
    }

    if (desktopfile.isNull()) {
        qCWarning(taskManagerLog()) << "Unable to create desktop file parser for window:" << window->id();
        return;
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

    dataModel()->requestOpenUrls(indexes.first(), urlList);
}

void TaskManager::hideItemPreview()
{
    m_windowMonitor->hideItemPreview();
}

bool TaskManager::allowForceQuit()
{
    return Settings->isAllowedForceQuit();
}

QString TaskManager::desktopIdToAppId(const QString& desktopId) const
{
    return Q_LIKELY(desktopId.endsWith(".desktop")) ? desktopId.chopped(8) : desktopId;
}

QString TaskManager::dockElementFromLauncherId(const QString &launcherId) const
{
    if (launcherId.isEmpty()) {
        return {};
    }

    if (isLauncherFolderId(launcherId)) {
        const QString resolvedGroupId = resolveLauncherGroupId(m_launcherGroupModel, launcherId);
        if (isLauncherRootFolderId(resolvedGroupId)) {
            return {};
        }
        return QStringLiteral("group/%1").arg(resolvedGroupId);
    }

    if (launcherId.startsWith(QStringLiteral("internal/"))) {
        return {};
    }

    return QStringLiteral("desktop/%1").arg(desktopIdToAppId(launcherId));
}

QString TaskManager::displayNameForDockElement(const QString &dockElement) const
{
    const auto [type, id] = splitDockElement(dockElement);
    if (type.isEmpty() || id.isEmpty()) {
        return {};
    }

    if (type == QStringLiteral("folder")) {
        return displayNameForPath(id);
    }

    if (type == QStringLiteral("group")) {
        QString groupName = invokeLauncherGroupDisplayName(m_launcherGroupModel, id);
        if (groupName.isEmpty()) {
            const QString resolvedGroupId = resolveLauncherGroupId(m_launcherGroupModel, id);
            const QModelIndex groupIndex = findIndexByNamedRole(m_launcherGroupModel,
                                                                MODEL_DESKTOPID,
                                                                resolvedGroupId,
                                                                DesktopIdRole);
            if (groupIndex.isValid()) {
                groupName = groupIndex.data(modelRole(m_launcherGroupModel, MODEL_NAME, NameRole)).toString();
            }
        }
        const QString categoryName = translatedLauncherCategoryName(groupName);
        if (!categoryName.isEmpty()) {
            return categoryName;
        }
        return groupName;
    }

    return {};
}

QString TaskManager::folderUrlToElementId(const QString &folderUrl) const
{
    const QString folderPath = normalizedFolderPath(folderUrl);
    if (folderPath.isEmpty()) {
        return {};
    }

    return QStringLiteral("folder/%1").arg(folderPath);
}

bool TaskManager::requestDockByDesktopId(const QString& desktopID)
{
    qCWarning(taskManagerLog) << "requestDockByDesktopId" << desktopID;
    if (isLauncherFolderId(desktopID)) {
        if (!m_launcherGroupModel || isLauncherRootFolderId(desktopID)) {
            qCWarning(taskManagerLog) << "reject launcher group dock request due to missing model or root group" << desktopID;
            return false;
        }

        const QString resolvedGroupId = resolveLauncherGroupId(m_launcherGroupModel, desktopID);
        const QString dockElement = dockElementFromLauncherId(desktopID);
        if (dockElement.isEmpty() || Settings->isDocked(dockElement)) {
            qCWarning(taskManagerLog) << "reject launcher group dock request due to empty/already docked element"
                                      << desktopID << resolvedGroupId << dockElement;
            return false;
        }

        const QModelIndex groupIndex = findIndexByNamedRole(m_launcherGroupModel, MODEL_DESKTOPID, resolvedGroupId, DesktopIdRole);
        if (!groupIndex.isValid()) {
            qCWarning(taskManagerLog) << "reject launcher group dock request due to invalid group index"
                                      << desktopID << resolvedGroupId;
            return false;
        }

        Settings->appendDockedElement(dockElement);
        qCWarning(taskManagerLog) << "accepted launcher group dock request" << desktopID << resolvedGroupId << dockElement;
        return true;
    }

    if (desktopID.startsWith("internal/")) return false;
    QString appId = desktopIdToAppId(desktopID);
    // 检查应用是否已经在任务栏中，如果是则返回 false
    if (IsDocked(appId)) {
        qCWarning(taskManagerLog) << "reject app dock request because already docked" << desktopID << appId;
        return false;
    }

    const bool ok = RequestDock(appId);
    qCWarning(taskManagerLog) << "app dock request result" << desktopID << appId << ok;
    return ok;
}

bool TaskManager::requestUndockByDesktopId(const QString& desktopID)
{
    if (isLauncherFolderId(desktopID)) {
        const QString dockElement = dockElementFromLauncherId(desktopID);
        if (dockElement.isEmpty()) {
            qCWarning(taskManagerLog) << "reject launcher group undock request due to empty element" << desktopID;
            return false;
        }

        Settings->removeDockedElement(dockElement);
        qCWarning(taskManagerLog) << "accepted launcher group undock request" << desktopID << dockElement;
        return true;
    }

    if (desktopID.startsWith("internal/")) return false;
    const QString appId = desktopIdToAppId(desktopID);
    const bool ok = RequestUndock(appId);
    qCWarning(taskManagerLog) << "app undock request result" << desktopID << appId << ok;
    return ok;
}

bool TaskManager::requestDockByFolderUrl(const QString &folderUrl)
{
    qCWarning(taskManagerLog) << "requestDockByFolderUrl" << folderUrl;
    const QString folderPath = normalizedFolderPath(folderUrl);
    if (folderPath.isEmpty()) {
        qCWarning(taskManagerLog) << "reject folder dock request due to empty normalized path" << folderUrl;
        return false;
    }

    QFileInfo folderInfo(folderPath);
    if (!folderInfo.exists() || !folderInfo.isDir()) {
        qCWarning(taskManagerLog) << "reject folder dock request due to invalid folder" << folderUrl << folderPath;
        return false;
    }

    const QString dockElement = QStringLiteral("folder/%1").arg(folderPath);
    if (Settings->isDocked(dockElement)) {
        qCWarning(taskManagerLog) << "reject folder dock request because already docked" << folderUrl << dockElement;
        return false;
    }

    Settings->appendDockedElement(dockElement);
    qCWarning(taskManagerLog) << "accepted folder dock request" << folderUrl << dockElement;
    return true;
}

bool TaskManager::requestUndockByFolderUrl(const QString &folderUrl)
{
    const QString folderPath = normalizedFolderPath(folderUrl);
    if (folderPath.isEmpty()) {
        qCWarning(taskManagerLog) << "reject folder undock request due to empty normalized path" << folderUrl;
        return false;
    }

    const QString dockElement = QStringLiteral("folder/%1").arg(folderPath);
    Settings->removeDockedElement(dockElement);
    qCWarning(taskManagerLog) << "accepted folder undock request" << folderUrl << dockElement;
    return true;
}

QVariantMap TaskManager::popupSortState(const QString &dockElement) const
{
    const auto [type, id] = splitDockElement(dockElement);
    Q_UNUSED(id)

    if (type == QStringLiteral("group") && !m_popupSortStates.contains(dockElement)) {
        return {
            {QStringLiteral("sortField"), QString()},
            {QStringLiteral("sortDescending"), false},
        };
    }

    const PopupSortState state = m_popupSortStates.value(dockElement, PopupSortState{});
    return {
        {QStringLiteral("sortField"), popupSortFieldToString(state.field)},
        {QStringLiteral("sortDescending"), state.order == Qt::DescendingOrder},
    };
}

QVariantMap TaskManager::cyclePopupSort(const QString &dockElement, const QString &fieldName)
{
    if (dockElement.isEmpty()) {
        return popupSortState(dockElement);
    }

    const auto [type, id] = splitDockElement(dockElement);
    Q_UNUSED(id)

    const PopupSortField selectedField = popupSortFieldFromString(fieldName);
    if (type == QStringLiteral("group") && !m_popupSortStates.contains(dockElement)) {
        PopupSortState nextState;
        nextState.field = selectedField;
        nextState.order = Qt::AscendingOrder;
        m_popupSortStates.insert(dockElement, nextState);
        return popupSortState(dockElement);
    }

    const PopupSortState currentState = m_popupSortStates.value(dockElement, PopupSortState{});
    const PopupSortState nextState = cyclePopupSortState(currentState, selectedField);
    m_popupSortStates.insert(dockElement, nextState);
    return popupSortState(dockElement);
}

QVariantMap TaskManager::popupDescriptor(const QString &dockElement, const QString &location) const
{
    const auto [type, id] = splitDockElement(dockElement);
    if (type.isEmpty() || id.isEmpty()) {
        return {};
    }

    if (type == QStringLiteral("group")) {
        QList<PopupSortableEntry> entries;
        for (const QString &appId : invokeLauncherGroupItems(m_launcherGroupModel, id)) {
            const QModelIndex appIndex = findIndexByNamedRole(m_launcherAppModel, MODEL_DESKTOPID, appId, DesktopIdRole);
            const QString iconName = appIndex.data(IconNameRole).toString().isEmpty() ?
                                         QString::fromLatin1(DEFAULT_APP_ICONNAME) :
                                         appIndex.data(IconNameRole).toString();
            const QString appName = appIndex.data(NameRole).toString().isEmpty() ?
                                        appId :
                                        appIndex.data(NameRole).toString();
            PopupSortableEntry entry;
            entry.entryData = popupEntry(appId, appName, iconName, false);
            entry.name = appName;
            entry.typeText = launcherEntryTypeText(appIndex);
            entry.modifiedTime = launcherModifiedTimeForSort(appIndex);
            entry.createdTime = launcherInstalledTimeForSort(appIndex);
            entry.size = launcherSizeForSort(appIndex);
            entries.append(entry);
        }

        const bool hasCustomSort = m_popupSortStates.contains(dockElement);
        const PopupSortState state = hasCustomSort ? m_popupSortStates.value(dockElement) : PopupSortState{};
        if (hasCustomSort) {
            sortPopupEntries(&entries, state, false);
        }

        QVariantList entryData;
        for (const PopupSortableEntry &entry : std::as_const(entries)) {
            entryData.append(entry.entryData);
        }

        return {
            {QStringLiteral("kind"), type},
            {QStringLiteral("title"), launcherGroupDisplayName(m_launcherGroupModel, id)},
            {QStringLiteral("location"), QString()},
            {QStringLiteral("parentLocation"), QString()},
            {QStringLiteral("canGoBack"), false},
            {QStringLiteral("entries"), entryData},
            {QStringLiteral("sortField"), hasCustomSort ? popupSortFieldToString(state.field) : QString()},
            {QStringLiteral("sortDescending"), hasCustomSort && state.order == Qt::DescendingOrder},
        };
    }

    if (type == QStringLiteral("folder")) {
        const QString rootLocation = normalizedFolderPath(id);
        if (rootLocation.isEmpty()) {
            return {};
        }

        QString currentLocation = location.isEmpty() ? rootLocation : normalizedFolderPath(location);
        if (currentLocation.isEmpty() || !isWithinBasePath(rootLocation, currentLocation) || !QFileInfo(currentLocation).isDir()) {
            currentLocation = rootLocation;
        }

        QString parentLocation = QFileInfo(currentLocation).dir().absolutePath();
        if (!isWithinBasePath(rootLocation, parentLocation)) {
            parentLocation = rootLocation;
        }

        const PopupSortState state = m_popupSortStates.value(dockElement, PopupSortState{});

        return {
            {QStringLiteral("kind"), type},
            {QStringLiteral("title"), displayNameForPath(currentLocation)},
            {QStringLiteral("location"), currentLocation},
            {QStringLiteral("parentLocation"), parentLocation},
            {QStringLiteral("canGoBack"), currentLocation != rootLocation},
            {QStringLiteral("entries"), directoryEntriesForPath(currentLocation, state)},
            {QStringLiteral("sortField"), popupSortFieldToString(state.field)},
            {QStringLiteral("sortDescending"), state.order == Qt::DescendingOrder},
        };
    }

    return {};
}

void TaskManager::activatePopupEntry(const QString &dockElement, const QString &entryId) const
{
    const auto [type, id] = splitDockElement(dockElement);
    Q_UNUSED(id)

    if (type == QStringLiteral("group")) {
        auto desktopfileParser = DESKTOPFILEFACTORY::createById(entryId, "amAPP");
        if (desktopfileParser && desktopfileParser->isValied().first) {
            desktopfileParser->launch();
        }
        return;
    }

    if (type == QStringLiteral("folder")) {
        const QFileInfo fileInfo(entryId);
        if (launchDesktopEntryFile(fileInfo)) {
            return;
        }

        QDesktopServices::openUrl(QUrl::fromLocalFile(entryId));
    }
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

    return desktopfileParser->isDocked();
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
        desktopfileParser->setDocked(false);
        return true;
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

void TaskManager::saveDockElementsOrder(const QStringList &dockElements)
{
    const QStringList &dockedElements = TaskManagerSettings::instance()->dockedElements();
    QStringList newDockedElements;
    for (const auto &dockElement : dockElements) {
        if (dockedElements.contains(dockElement) && !newDockedElements.contains(dockElement)) {
            newDockedElements.append(dockElement);
        }
    }
    TaskManagerSettings::instance()->setDockedElements(newDockedElements);
}

void TaskManager::moveItem(int from, int to)
{
    if (m_dockGlobalElementModel)
        m_dockGlobalElementModel->moveItem(from, to);
}

QString TaskManager::getTrashTipText()
{
    refreshTrashCount();
    return tr("%1 files").arg(m_cachedTrashCount);
}

bool TaskManager::isTrashEmpty() const
{
    refreshTrashCount();
    return m_cachedTrashCount == 0;
}

void TaskManager::refreshTrashCount(bool force) const
{
    if (!m_trashCountProcess || m_trashCountProcess->state() != QProcess::NotRunning) {
        return;
    }

    if (!force
        && m_trashStateInitialized
        && m_trashCountRefreshTimer.isValid()
        && m_trashCountRefreshTimer.elapsed() < 5000) {
        return;
    }

    m_trashCountProcess->start();
}

int TaskManager::trashCountFromOutput(const QByteArray &output)
{
    int count = 0;
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        if (!line.trimmed().isEmpty()) {
            ++count;
        }
    }
    return count;
}

QString TaskManager::managedTempDirectoryPath()
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (basePath.isEmpty()) {
        basePath = QDir::tempPath();
    }

    return QDir(basePath).filePath(QStringLiteral("dde-shell/taskmanager"));
}

QString TaskManager::normalizedManagedTempFilePath(const QString &pathOrUrl)
{
    if (pathOrUrl.trimmed().isEmpty()) {
        return {};
    }

    const QUrl url(pathOrUrl);
    const QString rawPath = url.isValid() && url.isLocalFile() ? url.toLocalFile() : pathOrUrl;
    return rawPath.isEmpty() ? QString() : QDir::cleanPath(rawPath);
}

void TaskManager::pruneManagedTempFiles() const
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (m_lastManagedTempPruneMs > 0 && nowMs - m_lastManagedTempPruneMs < 10LL * 60LL * 1000LL) {
        return;
    }

    m_lastManagedTempPruneMs = nowMs;
    const QDir directory(managedTempDirectoryPath());
    if (!directory.exists()) {
        return;
    }

    for (const QFileInfo &fileInfo : directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
        const QString cleanPath = QDir::cleanPath(fileInfo.absoluteFilePath());
        if (m_managedTempFiles.contains(cleanPath)) {
            continue;
        }

        if (fileInfo.lastModified().msecsTo(QDateTime::currentDateTime()) >= 6LL * 60LL * 60LL * 1000LL) {
            QFile::remove(cleanPath);
        }
    }
}

QString TaskManager::createManagedTempFilePath(const QString &prefix, const QString &suffix) const
{
    pruneManagedTempFiles();

    QDir directory(managedTempDirectoryPath());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        return {};
    }

    const QString safePrefix = prefix.trimmed().isEmpty() ? QStringLiteral("dde-shell-") : prefix.trimmed();
    const QString safeSuffix = suffix.trimmed();
    QTemporaryFile temporaryFile(directory.filePath(QStringLiteral("%1XXXXXX%2").arg(safePrefix, safeSuffix)));
    temporaryFile.setAutoRemove(false);
    if (!temporaryFile.open()) {
        return {};
    }

    const QString path = QDir::cleanPath(temporaryFile.fileName());
    temporaryFile.close();
    m_managedTempFiles.insert(path);
    return path;
}

void TaskManager::releaseManagedTempFile(const QString &pathOrUrl) const
{
    const QString path = normalizedManagedTempFilePath(pathOrUrl);
    if (path.isEmpty()) {
        return;
    }

    const QString managedDirectory = QDir(managedTempDirectoryPath()).absolutePath();
    const QString fileDirectory = QFileInfo(path).absolutePath();
    if (fileDirectory != managedDirectory && !fileDirectory.startsWith(managedDirectory + QLatin1Char('/'))) {
        return;
    }

    m_managedTempFiles.remove(path);
    QFile::remove(path);
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
