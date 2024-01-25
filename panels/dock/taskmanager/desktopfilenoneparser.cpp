// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsglobal.h"
#include "desktopfilenoneparser.h"
#include "desktopfileabstractparser.h"

#include <string>
#include <fstream>
#include <unistd.h>

#include <QLoggingCategory>
#include <QCryptographicHash>
#include <utility>

Q_LOGGING_CATEGORY(nonedesktopfileLog, "dde.shell.dock.nonedesktopfile")

DS_BEGIN_NAMESPACE
namespace dock {
DesktopFileNoneParser::DesktopFileNoneParser(QString id, QObject* parent)
    : DesktopfileAbstractParser(id, parent)
    , m_id(id)
{
    qCDebug(nonedesktopfileLog()) << "create a none desktopfile object: " << m_id;
}

DesktopFileNoneParser::~DesktopFileNoneParser()
{
    qCDebug(nonedesktopfileLog()) << "destroy a none desktopfile object: " << m_id;
}

void DesktopFileNoneParser::launch()
{

}

void DesktopFileNoneParser::launchWithAction(const QString& action)
{

}

void DesktopFileNoneParser::requestQuit()
{

}


QString DesktopFileNoneParser::id()
{
    return m_id;
}

QString DesktopFileNoneParser::name()
{
    return "";
}

QString DesktopFileNoneParser::desktopIcon()
{
    return "application-default-icon";
}

QList<QPair<QString, QString>> DesktopFileNoneParser::actions()
{
    return QList<QPair<QString, QString>>();
}

QString DesktopFileNoneParser::genericName()
{
    return "";
}

QString DesktopFileNoneParser::identifyWindow(QPointer<AbstractWindow> window)
{
    auto res = window->title();
    do {
        if (window->pid() == 0) break;

        auto filePath = QStringLiteral("/proc/%1/cmdline").arg(QString::number(window->pid()));
        QString cmdline;
        std::ifstream fs(filePath.toStdString());
        if (!fs.is_open()) break;
        std::string tmp;
        while (std::getline(fs, tmp, '\0')) {
            cmdline.append(QString::fromStdString(tmp));
        }

        QByteArray encryText = QCryptographicHash::hash(cmdline.toLatin1(), QCryptographicHash::Md5);
        res = encryText.toHex();
    } while(false);

    return res;
}

QString DesktopFileNoneParser::type()
{
    return "none";
}

QString DesktopFileNoneParser::appType()
{
    return type();
}

std::pair<bool, QString> DesktopFileNoneParser::isValied()
{
    return std::make_pair(false, "no desktopfile backend");
}

}
DS_END_NAMESPACE
