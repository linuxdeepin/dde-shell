// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginmetadata.h"

#include <QLoggingCategory>
#include <QJsonObject>
#include <QJsonParseError>
#include <QFile>
#include <QFileInfo>
#include <QDir>

DS_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(dsLog, "dde.shell")

class DPluginMetaDataPrivate : public QSharedData
{
public:
    QVariantMap rootObject() const
    {
        return m_metaData["Plugin"].toMap();
    }
    QString m_pluginId;
    QVariantMap m_metaData;
    QString m_pluginDir;
};

DPluginMetaData::DPluginMetaData()
    : d(new DPluginMetaDataPrivate())
{
}

DPluginMetaData::DPluginMetaData(const DPluginMetaData &other)
    : d(other.d)
{
}

DPluginMetaData &DPluginMetaData::operator=(const DPluginMetaData &other)
{
    d = other.d;
    return *this;
}

bool DPluginMetaData::operator==(const DPluginMetaData &other) const
{
    return d->m_pluginId == other.pluginId();
}

DPluginMetaData::~DPluginMetaData()
{

}

bool DPluginMetaData::isValid() const
{
    return !d->m_pluginId.isEmpty();
}

QVariant DPluginMetaData::value(const QString &key, const QVariant &defaultValue) const
{
    if (!isValid())
        return defaultValue;

    auto root = d->rootObject();
    if (!root.contains(key))
        return defaultValue;

    return root.value(key);
}

QString DPluginMetaData::pluginId() const
{
    return d->m_pluginId;
}

QString DPluginMetaData::pluginDir() const
{
    return d->m_pluginDir;
}

DPluginMetaData DPluginMetaData::fromJsonFile(const QString &file)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        qCWarning(dsLog) << "Couldn't open" << file;
        return DPluginMetaData();
    }
    QJsonParseError error;
    const QJsonObject metaData = QJsonDocument::fromJson(f.readAll(), &error).object();
    if (error.error) {
        qCWarning(dsLog) << "error parsing" << file << error.errorString();
    }

    DPluginMetaData result;
    result.d->m_metaData = metaData.toVariantMap();
    result.d->m_pluginDir = QFileInfo(f).absoluteDir().path();
    auto root = result.d->rootObject();
    if (root.contains("Id")) {
        result.d->m_pluginId = root["Id"].toString();
    }
    return result;
}

DS_END_NAMESPACE
