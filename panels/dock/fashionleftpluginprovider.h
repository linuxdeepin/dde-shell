// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QElapsedTimer>
#include <QHash>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqml.h>

namespace dock {

class FashionLeftPluginProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString timeText READ timeText NOTIFY clockChanged FINAL)
    Q_PROPERTY(QString dateText READ dateText NOTIFY clockChanged FINAL)
    Q_PROPERTY(int notificationCount READ notificationCount NOTIFY notificationCountChanged FINAL)
    Q_PROPERTY(QString notificationCountText READ notificationCountText NOTIFY notificationCountChanged FINAL)
    Q_PROPERTY(int mailUnreadCount READ mailUnreadCount NOTIFY mailStateChanged FINAL)
    Q_PROPERTY(QString mailUnreadCountText READ mailUnreadCountText NOTIFY mailStateChanged FINAL)
    Q_PROPERTY(QString mailSummaryText READ mailSummaryText NOTIFY mailStateChanged FINAL)
    Q_PROPERTY(QString mailIconName READ mailIconName NOTIFY mailClientChanged FINAL)
    Q_PROPERTY(QString mailClientName READ mailClientName NOTIFY mailClientChanged FINAL)
    Q_PROPERTY(bool musicAvailable READ musicAvailable NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(QString musicTitleText READ musicTitleText NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(QString musicSubtitleText READ musicSubtitleText NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(QString musicAppName READ musicAppName NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(QUrl musicArtSource READ musicArtSource NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(QString musicPlayerIconName READ musicPlayerIconName NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(QUrl musicPlayerIconSource READ musicPlayerIconSource NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(bool musicHasArt READ musicHasArt NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(bool musicPlaying READ musicPlaying NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(bool musicCanGoPrevious READ musicCanGoPrevious NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(bool musicCanGoNext READ musicCanGoNext NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(bool musicCanTogglePlayback READ musicCanTogglePlayback NOTIFY musicStateChanged FINAL)
    Q_PROPERTY(int cpuUsage READ cpuUsage NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(int memoryUsage READ memoryUsage NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(QString downloadSpeedText READ downloadSpeedText NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(QString uploadSpeedText READ uploadSpeedText NOTIFY systemStatsChanged FINAL)
    Q_PROPERTY(int aiRunningCount READ aiRunningCount NOTIFY aiStateChanged FINAL)
    Q_PROPERTY(QString aiRunningCountText READ aiRunningCountText NOTIFY aiStateChanged FINAL)
    Q_PROPERTY(QString aiHeadlineText READ aiHeadlineText NOTIFY aiStateChanged FINAL)
    Q_PROPERTY(QString aiSummaryText READ aiSummaryText NOTIFY aiStateChanged FINAL)
    Q_PROPERTY(QVariantList aiToolEntries READ aiToolEntries NOTIFY aiStateChanged FINAL)
    Q_PROPERTY(QString aiPrimaryToolId READ aiPrimaryToolId NOTIFY aiStateChanged FINAL)
    Q_PROPERTY(QString weatherCityText READ weatherCityText NOTIFY weatherChanged FINAL)
    Q_PROPERTY(QString weatherTemperatureText READ weatherTemperatureText NOTIFY weatherChanged FINAL)
    Q_PROPERTY(QString weatherSummaryText READ weatherSummaryText NOTIFY weatherChanged FINAL)
    Q_PROPERTY(QUrl weatherIconSource READ weatherIconSource NOTIFY weatherChanged FINAL)
    Q_PROPERTY(QUrl messageIconSource READ messageIconSource CONSTANT FINAL)
    QML_NAMED_ELEMENT(FashionLeftPluginProvider)

public:
    explicit FashionLeftPluginProvider(QObject *parent = nullptr);

    QString timeText() const;
    QString dateText() const;

    int notificationCount() const;
    QString notificationCountText() const;
    int mailUnreadCount() const;
    QString mailUnreadCountText() const;
    QString mailSummaryText() const;
    QString mailIconName() const;
    QString mailClientName() const;
    bool musicAvailable() const;
    QString musicTitleText() const;
    QString musicSubtitleText() const;
    QString musicAppName() const;
    QUrl musicArtSource() const;
    QString musicPlayerIconName() const;
    QUrl musicPlayerIconSource() const;
    bool musicHasArt() const;
    bool musicPlaying() const;
    bool musicCanGoPrevious() const;
    bool musicCanGoNext() const;
    bool musicCanTogglePlayback() const;

    int cpuUsage() const;
    int memoryUsage() const;
    QString downloadSpeedText() const;
    QString uploadSpeedText() const;
    int aiRunningCount() const;
    QString aiRunningCountText() const;
    QString aiHeadlineText() const;
    QString aiSummaryText() const;
    QVariantList aiToolEntries() const;
    QString aiPrimaryToolId() const;

    QString weatherCityText() const;
    QString weatherTemperatureText() const;
    QString weatherSummaryText() const;
    QUrl weatherIconSource() const;
    QUrl messageIconSource() const;

    Q_INVOKABLE void openWeatherPage();
    Q_INVOKABLE void openWeatherPopup(int taskbarLeft, int taskbarTop, int activationX, int activationY);
    Q_INVOKABLE void openMailClient();
    Q_INVOKABLE void openMusicPlayer();
    Q_INVOKABLE void playPreviousTrack();
    Q_INVOKABLE void toggleMusicPlayback();
    Q_INVOKABLE void playNextTrack();
    Q_INVOKABLE void openAiClientHost();
    Q_INVOKABLE void openNotificationPage();
    Q_INVOKABLE void openSystemMonitorPage();

signals:
    void clockChanged();
    void notificationCountChanged();
    void mailStateChanged();
    void mailClientChanged();
    void musicStateChanged();
    void systemStatsChanged();
    void aiStateChanged();
    void weatherChanged();

private slots:
    void refreshClock();
    void refreshNotificationCount();
    void refreshMailState();
    void refreshMusicState();
    void refreshSystemStats();
    void refreshAiState();
    void refreshWeather();
    void onNotificationCountChanged(uint count);

private:
    static bool launchCommand(const QString &program, const QStringList &arguments = {});
    static bool showControlCenterPage(const QString &pagePath);
    static QString commandOutput(const QString &program, const QStringList &arguments, int timeoutMs = 1500);
    static QString executablePathForService(const QString &serviceName);
    static QStringList desktopSearchDirectories();
    static QString locateDesktopFile(const QString &desktopId);
    static QString locateDesktopFileByExecutable(const QString &executablePath);
    static QString desktopEntryValue(const QString &desktopFilePath, const QString &key);
    static QString desktopCommandExecutable(const QString &desktopFilePath);
    static bool launchDesktopEntry(const QString &desktopFilePath);
    static QString localizedDesktopEntryValue(const QString &desktopFilePath, const QString &key);
    static QString musicPlayerIconNameForDesktopEntry(const QString &desktopId,
                                                      const QString &appName,
                                                      const QString &serviceName);
    static QUrl iconSourceForName(const QString &iconName);
    static QStringList mailAccountIdsFromJson(const QString &jsonText);
    static int unreadCountFromJson(const QString &jsonText, bool *ok = nullptr);
    static bool readCpuTimes(quint64 *totalTime, quint64 *idleTime);
    static int systemMemoryUsagePercent();
    static QStringList preferredNetworkInterfaces();
    static QString weatherConfigPath();
    static QString weatherIconPathFor(const QString &iconName, bool isDay);
    static QString formatTransferRate(double bytesPerSecond);
    static quint64 totalInterfaceBytes(bool receiveBytes, const QStringList &preferredInterfaces = {});
    void ensureWeatherWatchPaths();
    void refreshMailClient();

    QString m_timeText;
    QString m_dateText;
    int m_notificationCount = 0;
    int m_mailUnreadCount = 0;
    QString m_mailSummaryText = QStringLiteral("邮箱信息不可用");
    QString m_mailDesktopId;
    QString m_mailDesktopFilePath;
    QString m_mailIconName = QStringLiteral("deepin-mail");
    QString m_mailClientName = QStringLiteral("邮箱");
    QString m_musicService;
    QString m_musicDesktopEntry;
    QString m_musicExecutablePath;
    QString m_musicTitleText = QStringLiteral("未检测到音乐");
    QString m_musicSubtitleText = QStringLiteral("打开播放器开始播放");
    QString m_musicAppName = QStringLiteral("音乐");
    QUrl m_musicArtSource;
    QString m_musicPlayerIconName = QStringLiteral("audio-x-generic");
    QUrl m_musicPlayerIconSource;
    bool m_musicAvailable = false;
    bool m_musicPlaying = false;
    bool m_musicCanRaise = false;
    bool m_musicCanGoPrevious = false;
    bool m_musicCanGoNext = false;
    bool m_musicCanTogglePlayback = false;
    int m_cpuUsage = 0;
    int m_memoryUsage = 0;
    QString m_downloadSpeedText = QStringLiteral("0kb/s");
    QString m_uploadSpeedText = QStringLiteral("0kb/s");
    int m_aiRunningCount = 0;
    QString m_aiHeadlineText = QStringLiteral("AI 后台任务");
    QString m_aiSummaryText = QStringLiteral("当前空闲");
    QVariantList m_aiToolEntries;
    QString m_aiPrimaryToolId;
    QString m_aiLastPrimaryToolId;
    qint64 m_aiPrimaryHostPid = 0;
    QString m_aiPrimaryHostDesktopFilePath;
    QString m_aiPrimaryHostExecutablePath;
    QString m_aiPrimaryHostAppName;
    QHash<QString, QString> m_aiObservedSessionStates;
    QHash<QString, qint64> m_aiLastSeenPidByTool;
    bool m_aiStateInitialized = false;
    QString m_weatherCityText;
    QString m_weatherTemperatureText = QStringLiteral("--°");
    QString m_weatherSummaryText = QStringLiteral("天气信息不可用");
    QUrl m_weatherIconSource;
    quint64 m_previousCpuTotalTime = 0;
    quint64 m_previousCpuIdleTime = 0;
    quint64 m_previousReceiveBytes = 0;
    quint64 m_previousTransmitBytes = 0;
    quint64 m_previousAggregateReceiveBytes = 0;
    quint64 m_previousAggregateTransmitBytes = 0;
    QElapsedTimer m_networkSampleTimer;
    QFileSystemWatcher *m_weatherWatcher = nullptr;
};

} // namespace dock
