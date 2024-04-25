// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shell.h"

#include <QLoggingCategory>
#include <QQmlAbstractUrlInterceptor>
#include <qmlengine.h>

DS_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(dsLoaderLog)

class DtkInterceptor : public QObject, public QQmlAbstractUrlInterceptor
{
public:
    DtkInterceptor(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
    QUrl intercept(const QUrl &path, DataType type)
    {
        if (type != DataType::QmlFile)
            return path;
        if (path.path().endsWith("overridable/InWindowBlur.qml")) {
            qDebug() << "Override dtk's InWindowBlur";
            return QStringLiteral("qrc:/shell/override/dtk/InWindowBlur.qml");
        }

        return path;
    }
};

Shell::Shell(QObject *parent)
    : QObject(parent)
{

}

void Shell::installDtkInterceptor()
{
    auto engine = DQmlEngine().engine();
    engine->addUrlInterceptor(new DtkInterceptor(this));
}

DS_END_NAMESPACE
