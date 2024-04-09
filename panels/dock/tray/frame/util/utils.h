// Copyright (C) 2018 ~ 2020 Uniontech Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later


#ifndef UTILS
#define UTILS
#include <QPixmap>
#include <QImageReader>
#include <QApplication>
#include <QScreen>
#include <QDebug>

namespace Utils {

#define ICBC_CONF_FILE "/etc/deepin/icbc.conf"

const bool IS_WAYLAND_DISPLAY = !qgetenv("WAYLAND_DISPLAY").isEmpty();

inline bool isDraging()
{
    if (!qApp->property("isDraging").isValid())
        return false;

    return qApp->property("isDraging").toBool();
}

inline void setIsDraging(bool isDraging)
{
    qApp->setProperty("isDraging", isDraging);
}

/* convert 'some-key' to 'someKey' or 'SomeKey'.
 * the second form is needed for appending to 'set' for 'setSomeKey'
 */
inline QString qtify_name(const char *name)
{
    bool next_cap = false;
    QString result;

    while (*name) {
        if (*name == '-') {
            next_cap = true;
        } else if (next_cap) {
            result.append(QChar(*name).toUpper().toLatin1());
            next_cap = false;
        } else {
            result.append(*name);
        }

        name++;
    }

    return result;
}

inline QPixmap renderSVG(const QString &path, const QSize &size, const qreal devicePixelRatio)
{
    QImageReader reader;
    QPixmap pixmap;
    reader.setFileName(path);
    if (reader.canRead()) {
        reader.setScaledSize(size * devicePixelRatio);
        pixmap = QPixmap::fromImage(reader.read());
        pixmap.setDevicePixelRatio(devicePixelRatio);
    }
    else {
        pixmap.load(path);
    }

    return pixmap;
}

inline QScreen *screenAt(const QPoint &point) {
    for (QScreen *screen : qApp->screens()) {
        const QRect r { screen->geometry() };
        const QRect rect { r.topLeft(), r.size() * screen->devicePixelRatio() };
        if (rect.contains(point)) {
            return screen;
        }
    }

    return nullptr;
}

//!!! 注意:这里传入的QPoint是未计算缩放的
inline QScreen *screenAtByScaled(const QPoint &point) {
    for (QScreen *screen : qApp->screens()) {
        const QRect r { screen->geometry() };
        QRect rect { r.topLeft(), r.size() * screen->devicePixelRatio() };
        if (rect.contains(point)) {
            return screen;
        }
    }

    return nullptr;
}

/**
* @brief 比较两个插件版本号的大小
* @param pluginApi1 第一个插件版本号
* @param pluginApi2 第二个插件版本号
* @return 0:两个版本号相等,1:第一个版本号大,-1:第二个版本号大
*/
inline int comparePluginApi(const QString &pluginApi1, const QString &pluginApi2)
{
    // 版本号相同
    if (pluginApi1 == pluginApi2)
        return 0;

    // 拆分版本号
    QStringList subPluginApis1 = pluginApi1.split(".", Qt::SkipEmptyParts, Qt::CaseSensitive);
    QStringList subPluginApis2 = pluginApi2.split(".", Qt::SkipEmptyParts, Qt::CaseSensitive);
    for (int i = 0; i < subPluginApis1.size(); ++i) {
        auto subPluginApi1 = subPluginApis1[i];
        if (subPluginApis2.size() > i) {
            auto subPluginApi2 = subPluginApis2[i];

            // 相等判断下一个子版本号
            if (subPluginApi1 == subPluginApi2)
                continue;

            // 转成整形比较
            if (subPluginApi1.toInt() > subPluginApi2.toInt()) {
                return 1;
            } else {
                return -1;
            }
        }
    }

    // 循环结束但是没有返回,说明子版本号个数不同,且前面的子版本号都相同
    // 子版本号多的版本号大
    if (subPluginApis1.size() > subPluginApis2.size()) {
        return 1;
    } else {
        return -1;
    }
}

}

#endif // UTILS
