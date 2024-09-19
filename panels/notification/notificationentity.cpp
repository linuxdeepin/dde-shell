// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationentity.h"

#include <QDBusArgument>
#include <QTimer>
#include <QLoggingCategory>
#include <QDateTime>
#include <QImage>
#include <QIcon>
#include <QTemporaryFile>
#include <QUrl>

#include <DIconTheme>

namespace notification {

Q_DECLARE_LOGGING_CATEGORY(notificationLog)

static inline void copyLineRGB32(QRgb *dst, const char *src, int width)
{
    const char *end = src + width * 3;
    for (; src != end; ++dst, src += 3) {
        *dst = qRgb(src[0], src[1], src[2]);
    }
}

static inline void copyLineARGB32(QRgb *dst, const char *src, int width)
{
    const char *end = src + width * 4;
    for (; src != end; ++dst, src += 4) {
        *dst = qRgba(src[0], src[1], src[2], src[3]);
    }
}

static QImage decodeImageFromDBusArgument(const QDBusArgument &arg)
{
    int width, height, rowStride, hasAlpha, bitsPerSample, channels;
    QByteArray pixels;
    char *ptr;
    char *end;

    arg.beginStructure();
    arg >> width >> height >> rowStride >> hasAlpha >> bitsPerSample >> channels >> pixels;
    arg.endStructure();
    //qDebug() << width << height << rowStride << hasAlpha << bitsPerSample << channels;

#define SANITY_CHECK(condition) \
if (!(condition)) { \
        qWarning() << "Sanity check failed on" << #condition; \
        return QImage(); \
}

    SANITY_CHECK(width > 0);
    SANITY_CHECK(width < 2048);
    SANITY_CHECK(height > 0);
    SANITY_CHECK(height < 2048);
    SANITY_CHECK(rowStride > 0);

#undef SANITY_CHECK

    QImage::Format format = QImage::Format_Invalid;
    void (*fcn)(QRgb *, const char *, int) = nullptr;
    if (bitsPerSample == 8) {
        if (channels == 4) {
            format = QImage::Format_ARGB32;
            fcn = copyLineARGB32;
        } else if (channels == 3) {
            format = QImage::Format_RGB32;
            fcn = copyLineRGB32;
        }
    }
    if (format == QImage::Format_Invalid) {
        qWarning() << "Unsupported image format (hasAlpha:" << hasAlpha << "bitsPerSample:" << bitsPerSample << "channels:" << channels << ")";
        return QImage();
    }

    QImage image(width, height, format);
    ptr = pixels.data();
    end = ptr + pixels.length();
    for (int y = 0; y < height; ++y, ptr += rowStride) {
        if (ptr + channels * width > end) {
            qWarning() << "Image data is incomplete. y:" << y << "height:" << height;
            break;
        }
        fcn((QRgb *)image.scanLine(y), ptr, width);
    }

    return image;
}

static QImage decodeImageFromBase64(const QString &arg)
{
    if (arg.startsWith("data:image/")) {
        // iconPath is a string representing an inline image.
        QStringList strs = arg.split("base64,");
        if (strs.length() == 2) {
            QByteArray data = QByteArray::fromBase64(strs.at(1).toLatin1());
            return QImage::fromData(data);
        }
    }
    return QImage();
}

static QIcon decodeIconFromPath(const QString &arg, const QString &fallback)
{
    DGUI_USE_NAMESPACE;
    const QUrl url(arg);
    const auto iconUrl = url.isLocalFile() ? url.toLocalFile() : url.url();
    QIcon icon = DIconTheme::findQIcon(iconUrl);
    if (!icon.isNull()) {
        return icon;
    }
    return DIconTheme::findQIcon(fallback, DIconTheme::findQIcon("application-x-desktop"));
}

static QString imagePathOfNotification(const QVariantMap &hints, const QString &appIcon, const QString &appName)
{
    static const QStringList HintsOrder {
            "desktop-entry",
            "image-data",
            "image-path",
            "image_path",
            "icon_data"
    };

    QImage img;
    QString imageData(appIcon);
    for (const auto &hint : HintsOrder) {
        const auto &source = hints[hint];
        if (source.isNull())
            continue;
        if (source.canConvert<QDBusArgument>()) {
            img = decodeImageFromDBusArgument(source.value<QDBusArgument>());
            if (!img.isNull())
                break;
        }
        imageData = source.toString();
    }
    if (img.isNull()) {
        img = decodeImageFromBase64(imageData);
    }
    if (!img.isNull()) {
        QTemporaryFile file("notification_icon");
        img.save(file.fileName());
        return file.fileName();
    }
    QIcon icon = decodeIconFromPath(imageData, appName);
    if (icon.isNull()) {
        qCWarning(notificationLog) << "Can't get icon for notification, appName:" << appName;
    }
    return icon.name();
}


NotificationEntity::NotificationEntity(const QString &appName, uint replacesId, const QString &appIcon,
                                       const QString &summary, const QString &body, const QStringList &actions,
                                       const QVariantMap &hints, int expireTimeout, QObject *parent)
    : QObject(parent)
    , m_text(body)
    , m_title(summary)
    , m_iconName(appIcon)
    , m_appName(appName)
    , m_actions(actions)
    , m_hints(hints)
    , m_replacesId(replacesId)
    , m_timeout(expireTimeout)
    , m_timeTip(tr("just now"))
    , m_ctime(QString::number(QDateTime::currentSecsSinceEpoch()))
{
    if (hints.contains("urgency")) {
        m_urgency = hints.value("urgency").toInt();
    }

    if (m_urgency != Critical && m_timeout >= 0) {
        auto timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(m_timeout == 0 ? TimeOutInterval : m_timeout);
        connect(timer, &QTimer::timeout, this, [this] {
            Q_EMIT notificationTimeout(this);
        });
        timer->start();
    }
}

QString NotificationEntity::body() const
{
    return m_text;
}

QString NotificationEntity::text() const
{
    return displayText();
}

QString NotificationEntity::title() const
{
    return m_title;
}

QString NotificationEntity::originIconName() const
{
    return m_iconName;
}

QString NotificationEntity::iconName() const
{
    return imagePathOfNotification(m_hints, m_iconName, m_appName);
}

QString NotificationEntity::appName() const
{
    return m_appName;
}

QStringList NotificationEntity::actions() const
{
    return m_actions;
}

QVariantMap NotificationEntity::hints() const
{
    return m_hints;
}

QString NotificationEntity::ctime() const
{
    return m_ctime;
}

QString NotificationEntity::timeTip() const
{
    return m_timeTip;
}

int NotificationEntity::level() const
{
    return m_level;
}

int NotificationEntity::id() const
{
    return m_id;
}

uint NotificationEntity::replacesId() const
{
    return m_replacesId;
}

int NotificationEntity::timeout() const
{
    return m_timeout;
}

int NotificationEntity::urgency() const
{
    return m_urgency;
}

uint NotificationEntity::storageId() const
{
    return m_storageId;
}

void NotificationEntity::setId(int id)
{
    m_id = id;
}

void NotificationEntity::setReplacesId(uint replacesId)
{
    m_replacesId = replacesId;
}

void NotificationEntity::setStorageId(uint storageId)
{
    m_storageId = storageId;
}

void NotificationEntity::setLevel(int newLevel)
{
    if (m_level == newLevel)
        return;
    m_level = newLevel;
    emit levelChanged();
}

void NotificationEntity::setEnablePreview(bool enable)
{
    m_showPreview = enable;
}

qint64 NotificationEntity::createdTimeSecs() const
{
    return m_ctime.toUInt();
}

void NotificationEntity::setTimeTip(const QString &timeTip)
{
    if (!timeTip.isEmpty() && timeTip != m_timeTip) {
        m_timeTip = timeTip;
        Q_EMIT timeTipChanged();
    }
}

QVariantMap NotificationEntity::toMap() const
{
    QVariantMap res;
    res["id"] = m_id;
    res["replacesId"] = m_replacesId;
    res["appName"] = m_appName;
    res["appIcon"] = m_iconName;
    res["summary"] = m_title;
    res["body"] = m_text;
    res["actions"] = m_actions;
    res["hints"] = m_hints;
    res["urgency"] = m_urgency;
    res["ctime"] = m_ctime;
    res["timeTip"] = m_timeTip;
    return res;
}

QString NotificationEntity::defaultActionText() const
{
    const auto index = defaultActionTextIndex();
    if (index < 0)
        return QString();
    return m_actions[index];
}

QString NotificationEntity::defaultActionId() const
{
    const auto index = defaultActionIdIndex();
    if (index < 0)
        return QString();
    return m_actions[index];
}

QString NotificationEntity::firstActionText() const
{
    if (!hasDisplayAction())
        return QString();
    return displayActions().at(1);
}

QString NotificationEntity::firstActionId() const
{
    if (!hasDisplayAction())
        return QString();
    return displayActions().at(0);
}

QStringList NotificationEntity::actionTexts() const
{
    QStringList res;
    const auto tmp = displayActions();
    for (int i = 3; i < tmp.count(); i += 2)
        res << tmp[i];
    return res;
}

QStringList NotificationEntity::actionIds() const
{
    QStringList res;
    const auto tmp = displayActions();
    for (int i = 2; i < tmp.count(); i += 2)
        res << tmp[i];
    return res;
}

int NotificationEntity::defaultActionIdIndex() const
{
    return m_actions.indexOf("default");
}

int NotificationEntity::defaultActionTextIndex() const
{
    const auto index = defaultActionIdIndex();
    if (index >= 0)
        return index + 1;
    return -1;
}

QStringList NotificationEntity::displayActions() const
{
    const auto defaultIndex = defaultActionIdIndex();
    if (defaultIndex >= 0) {
        auto tmp = m_actions;
        tmp.remove(defaultIndex, 2);
        return tmp;
    }
    return m_actions;
}

QString NotificationEntity::displayText() const
{
    return m_showPreview ? m_text : tr("1 new message");
}

bool NotificationEntity::hasDisplayAction() const
{
    const auto tmp = displayActions();
    return tmp.count() >= 2;
}

bool NotificationEntity::hasDefaultAction() const
{
    return defaultActionIdIndex() >= 0;
}

} // notification