// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifyentity.h"

#include <QDateTime>
#include <QLocale>
#include <QStringList>
#include <QLoggingCategory>
#include <QImage>
#include <QBuffer>
#include <QDBusArgument>
#include <QRegularExpression>

#include <unicode/reldatefmt.h>
#include <unicode/smpdtfmt.h>

#include <memory>

namespace notification {
Q_LOGGING_CATEGORY(notifyLog, "org.deepin.dde.shell.notification")

#define ACTION_SEGMENT ("|")
#define HINT_SEGMENT ("|")
#define KEY_VALUE_SEGMENT ("!!!")
#define LIST_VALUE_SEGMENT (":::")

static const uint NoReplaceId = 0;

class NotifyData : public QSharedData
{
public:
    explicit NotifyData()
        : QSharedData()
    {
    }

    QString appId;
    QString appName;
    QString appIcon;
    QString summary;
    QString body;
    QStringList actions;
    QVariantMap hints;
    uint bubbleId = 0;
    uint replacesId = NoReplaceId;
    int expireTimeout = 0;

    qint64 id = NotifyEntity::InvalidId;
    qint64 cTime = 0;
    int processedType = NotifyEntity::NotProcessed;
};

NotifyEntity::NotifyEntity()
    : d(new NotifyData())
{

}

NotifyEntity::NotifyEntity(qint64 id, const QString &appName)
    : NotifyEntity()
{
    d->id = id;
    d->appName = appName;
    d->cTime = QDateTime::currentMSecsSinceEpoch();
}

NotifyEntity::NotifyEntity(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary,
                           const QString &body, const QStringList &actions, const QVariantMap &hints,
                           int expireTimeout)
    : d(new NotifyData())
{
    d->appName = appName;
    d->replacesId = replacesId;
    d->appIcon = appIcon;
    d->summary = summary;
    d->body = body;
    d->actions = actions;
    d->hints = hints;
    d->expireTimeout = expireTimeout;
    d->cTime = QDateTime::currentMSecsSinceEpoch();
}

 NotifyEntity::NotifyEntity(const NotifyEntity &other)
    : d(other.d)
 {

 }

 NotifyEntity::NotifyEntity(NotifyEntity &&other) noexcept
 {
     d = other.d;
     other.d = nullptr;
 }

 NotifyEntity::~NotifyEntity()
 {

 }

 NotifyEntity &NotifyEntity::operator=(const NotifyEntity &other)
 {
     if (this != &other)
         d = other.d;

     return *this;
 }

 NotifyEntity &NotifyEntity::operator=(NotifyEntity &&other)
 {
     d.swap(other.d);
     return *this;
 }

bool NotifyEntity::operator==(const NotifyEntity &other) const
{
    if (d && other.d) {
        return id() == other.id();
    }
    return false;
}

bool NotifyEntity::operator!=(const NotifyEntity &other) const
{
    return !(operator==(other));
}

bool NotifyEntity::isValid() const
{
    return d && d->id != NotifyEntity::InvalidId && d->cTime > 0;
}

qint64 NotifyEntity::id() const
{
    return d->id;
}

void NotifyEntity::setId(qint64 id)
{
    d->id = id;
}

QString NotifyEntity::appName() const
{
    return d->appName;
}

void NotifyEntity::setAppName(const QString &appName)
{
    d->appName = appName;
}

QString NotifyEntity::appId() const
{
    return d->appId;
}

void NotifyEntity::setAppId(const QString &appId)
{
    d->appId = appId;
}

QString NotifyEntity::body() const
{
    return d->body;
}

void NotifyEntity::setBody(const QString &body)
{
    d->body = body;
}

QString NotifyEntity::summary() const
{
    return d->summary;
}

void NotifyEntity::setSummary(const QString &summary)
{
    d->summary = summary;
}

QString NotifyEntity::appIcon() const
{
    return d->appIcon;
}

void NotifyEntity::setAppIcon(const QString &appIcon)
{
    d->appIcon = appIcon;
}

QStringList NotifyEntity::actions() const
{
    return d->actions;
}

QString NotifyEntity::actionsString() const
{
    return convertActionsToString(d->actions);
}

void NotifyEntity::setActionString(const QString &actionString)
{
    d->actions = parseAction(actionString);
}

QVariantMap NotifyEntity::hints() const
{
    return d->hints;
}

QString NotifyEntity::hintsString() const
{
    return convertHintsToString(d->hints);
}

void NotifyEntity::setHintString(const QString &hintString)
{
    d->hints = parseHint(hintString);
}

uint NotifyEntity::replacesId() const
{
    return d->replacesId;
}

void NotifyEntity::setReplacesId(uint replacesId)
{
    d->replacesId = replacesId;
}

bool NotifyEntity::isReplace() const
{
    return d->replacesId != NoReplaceId;
}

qint64 NotifyEntity::cTime() const
{
    return d->cTime;
}

void NotifyEntity::setCTime(qint64 cTime)
{
    d->cTime = cTime;
}

bool NotifyEntity::processed() const
{
    return d->processedType == NotifyEntity::Processed;
}

int NotifyEntity::processedType() const
{
    return d->processedType;
}

void NotifyEntity::setProcessedType(int type)
{
    d->processedType = type;
}

QString NotifyEntity::convertActionsToString(const QStringList &actions)
{
    QString action;
    foreach (QString text, actions) {
        action += text;
        action += ACTION_SEGMENT;
    }

    if (!action.isEmpty())
        action = action.mid(0, action.length() - 1);

    return action;
}

uint NotifyEntity::bubbleId() const
{
    return d->bubbleId;
}

void NotifyEntity::setBubbleId(qint64 bubbleId)
{
    d->bubbleId = bubbleId;
}

// https://specifications.freedesktop.org/notification-spec/1.2/icons-and-images.html
QString NotifyEntity::bodyIcon() const
{
    const auto hints = this->hints();
    if (auto iter = hints.find("image-path"); iter != hints.end()) {
        const auto path = iter.value().toString();
        return path;
    }
    return QString();
}

static inline void copyLineRGB32(QRgb *dst, const char *src, int width)
{
    const char *end = src + width * 3;
    for (; src != end; ++dst, src += 3) {
        *dst = qRgb(static_cast<uchar>(src[0]), static_cast<uchar>(src[1]), static_cast<uchar>(src[2]));
    }
}

static inline void copyLineARGB32(QRgb *dst, const char *src, int width)
{
    const char *end = src + width * 4;
    for (; src != end; ++dst, src += 4) {
        *dst = qRgba(static_cast<uchar>(src[0]), static_cast<uchar>(src[1]), static_cast<uchar>(src[2]), static_cast<uchar>(src[3]));
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

#define SANITY_CHECK(condition) \
if (!(condition)) { \
    qWarning(notifyLog) << "Sanity check failed on" << #condition; \
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
        qWarning(notifyLog) << "Unsupported image format (hasAlpha:" << hasAlpha << "bitsPerSample:" << bitsPerSample << "channels:" << channels << ")";
        return QImage();
    }

    QImage image(width, height, format);
    ptr = pixels.data();
    end = ptr + pixels.length();
    for (int y = 0; y < height; ++y, ptr += rowStride) {
        if (ptr + channels * width > end) {
            qWarning(notifyLog) << "Image data is incomplete. y:" << y << "height:" << height;
            break;
        }
        fcn(reinterpret_cast<QRgb *>(image.scanLine(y)), ptr, width);
    }

    return image;
}

static QString decodeImageToBase64(const QImage &image, const char *format = "PNG")
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, format);

    return QString("data:image/%1;base64,%2").arg(QString::fromLatin1(format).toLower()).arg(QString::fromLatin1(ba.toBase64()));
}

static QString imagePathOfNotification(const QVariantMap &hints, const QString &appIcon)
{
    static const QStringList HintsOrder {
            "desktop-entry",
            "image-data",
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
        // check if imageData is a base64 image data.
        QRegularExpression dataUriPattern("^data:image/[a-zA-Z0-9+\\-]+;base64,");
        QRegularExpressionMatch match = dataUriPattern.match(imageData);
        if (match.hasMatch()) {
            return imageData;
        }
    } else {
        return decodeImageToBase64(img);
    }

    // ui can fallback to application-x-desktop icon.
    return {};
}

QString NotifyEntity::appIconResolved() const
{
    const QString iconFromHints = imagePathOfNotification(d->hints, d->appIcon);
    if (!iconFromHints.isEmpty())
        return iconFromHints;

    if (!d->appIcon.isEmpty()) {
        return d->appIcon;
    }

    return {};
}

QString NotifyEntity::convertHintsToString(const QVariantMap &map)
{
    QString text;

    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        QString key = it.key();
        text += key;
        text += KEY_VALUE_SEGMENT;
        QString value;
        if (it.value().typeId() == QMetaType::QStringList) {
            QStringList tmp = it.value().toStringList();
            value = tmp.join(LIST_VALUE_SEGMENT);
        } else {
            value = it.value().toString();
        }
        text += value;
        text += HINT_SEGMENT;
    }

    return text;
}

QStringList NotifyEntity::parseAction(const QString &action)
{
    if (action.isEmpty())
        return {};

    QStringList actions = action.split("|");
    const auto defaultIndex = actions.indexOf("default");
    Q_ASSERT((defaultIndex % 2) != 1);
    if (defaultIndex < 0)
        Q_ASSERT((actions.size() % 2) != 1);

    return actions;
}

QVariantMap NotifyEntity::parseHint(const QString &hint)
{
    if (hint.isEmpty())
        return {};

    QVariantMap map;
    QStringList keyValueList = hint.split("|");
    foreach (QString text, keyValueList) {
        QStringList list = text.split("!!!");
        if (list.size() != 2)
            continue;
        const QString &key = list[0];
        QVariant value;
        auto listValue = list[1].split(LIST_VALUE_SEGMENT);
        if (listValue.size() > 1) {
            value = QVariant::fromValue(listValue);
        } else {
            value = QVariant::fromValue(list[1]);
        }

        map.insert(key, value);
    }

    return map;
}

namespace {

QString toQString(const icu::UnicodeString &icuString)
{
    const UChar *ucharData = icuString.getBuffer();
    int32_t length = icuString.length();
    return QString(reinterpret_cast<const QChar *>(ucharData), length);
}

} // anonymous namespace

QString NotifyEntity::formatRelativeTime(qint64 ctimeMs)
{
    QDateTime time = QDateTime::fromMSecsSinceEpoch(ctimeMs);
    if (!time.isValid())
        return {};

    using namespace icu;
    static std::unique_ptr<RelativeDateTimeFormatter> formatter;
    static UErrorCode cachedStatus = U_ZERO_ERROR;
    if (!formatter) {
        cachedStatus = U_ZERO_ERROR;
        formatter = std::make_unique<RelativeDateTimeFormatter>(
            icu::Locale::getDefault(),
            nullptr,
            UDAT_STYLE_LONG,
            UDISPCTX_CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE,
            cachedStatus);
    }
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString result;

    QDateTime currentTime = QDateTime::currentDateTime();
    auto elapsedDay = time.daysTo(currentTime);

    if (elapsedDay == 0) {
        qint64 msec = QDateTime::currentMSecsSinceEpoch() - ctimeMs;
        auto minute = msec / 1000 / 60;
        if (minute <= 0) {
            return {};
        } else if (minute > 0 && minute < 60) {
            formatter->format(minute, UDAT_DIRECTION_LAST, UDAT_RELATIVE_MINUTES, result, status);
            return toQString(result);
        } else {
            const auto hour = minute / 60;
            formatter->format(hour, UDAT_DIRECTION_LAST, UDAT_RELATIVE_HOURS, result, status);
            return toQString(result);
        }
    } else if (elapsedDay >= 1 && elapsedDay < 2) {
        formatter->format(UDAT_DIRECTION_LAST, UDAT_ABSOLUTE_DAY, result, status);
        UnicodeString combinedString;
        UErrorCode timeStatus = U_ZERO_ERROR;
        SimpleDateFormat timeFormatter("HH:mm", icu::Locale::getDefault(), timeStatus);
        UnicodeString timeString;
        UDate udate = static_cast<UDate>(ctimeMs);
        timeFormatter.format(udate, timeString, timeStatus);
        formatter->combineDateAndTime(result, timeString, combinedString, status);
        return toQString(combinedString);
    } else if (elapsedDay >= 2 && elapsedDay < 7) {
        return QLocale::system().toString(time, "ddd hh:mm");
    } else {
        return time.toString(QLocale::system().dateFormat(QLocale::ShortFormat));
    }
}

}
