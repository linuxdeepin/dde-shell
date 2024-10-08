// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appgroup.h"
#include <QLoggingCategory>
#include <algorithm>

Q_LOGGING_CATEGORY(appGroupLog, "org.deepin.ds.dde-apps.appgroup")

namespace apps {
static constexpr uint GROUP_MAX_ITEMS_PER_PAGE = 3 * 4;

AppGroup::AppGroup(const uint& id, QObject* parent)
    : QObject(parent)
    , m_id(id)
{
}

AppGroup::AppGroup(const uint& id, const QString &name, QObject* parent)
    : AppGroup(id, parent)
{
    m_name = name;
}

AppGroup::AppGroup(const uint& id, const QString &name, const QStringList &appItemIDs, const QList<int> &pages, QObject* parent)
    : AppGroup(id, name, parent)
{
    m_itemIds = appItemIDs;
    m_pages = pages;
}

uint AppGroup::id() const
{
    return m_id;
}

QString AppGroup::name() const
{
    return m_name;
}

void AppGroup::setName(const QString &name)
{
    if (name == m_name)
        return;

    m_name = name;
    Q_EMIT nameChanged();
}

QStringList AppGroup::appItems() const
{
    return m_itemIds;
}

void AppGroup::instertItem(const QString &itemId, int pos)
{
    // 插入到最后
    if (pos == -1) {
        auto lastPageCount = m_pages.last();
        m_itemIds.append(itemId);
        if (lastPageCount < GROUP_MAX_ITEMS_PER_PAGE) {
            lastPageCount +=1;
        } else {
            m_pages.append(1);
        }
    }

    auto actualPage = (uint)ceil((double)pos / GROUP_MAX_ITEMS_PER_PAGE);
    int actiualCount = 0;

    for (int i = 0; i <= std::min((long long)actualPage, m_pages.length() - 1); i++) {
        actiualCount += m_pages[i];
    }

    m_itemIds.insert(actiualCount, itemId);

    // 从逻辑页开始找到首个不满的页面 + 1
    bool added = false;
    for (; actualPage < m_pages.size(); actualPage++) {
        if (m_pages[actualPage] < GROUP_MAX_ITEMS_PER_PAGE) {
            m_pages[actualPage] += 1;
            added = true;
            break;
        }
    }

    // 从逻辑页面开始往后全满时，塞一个新的页面进去
    if (!added) {
        m_pages.append(1);
    }

    Q_EMIT appitemsChanged();
}

void AppGroup::removeItem(const QString &itemId)
{
    auto pageId = getPageById(itemId);
    m_itemIds.removeOne(itemId);

    m_pages[pageId] -= 1;

    if (0 == m_pages[pageId]) {
        m_pages.removeAt(pageId);
    }
}

int AppGroup::getPageById(const QString &appItem)
{
    auto index = m_itemIds.indexOf(appItem);
    if (-1 == index) {
        return -1;
    }

    auto count = 0;
    int res = 0;
    for (; res < m_pages.size(); res++) {
        count += m_pages[res];
        if (count > index) {
            break;
        }
    }

    return res;
}

void AppGroup::swap(const QString &itemA, const QString &itemB)
{
    auto indexA = m_itemIds.indexOf(itemA);
    auto indexB = m_itemIds.indexOf(itemB);

    if (indexA == -1 || indexB == -1) {
        qCDebug(appGroupLog) << itemA << " or " << itemB << " not contaned just return";
        return;
    }

    m_itemIds.swapItemsAt(indexA, indexB);
}

QList<int> AppGroup::pages()
{
    if (m_pages.length() == 0 && m_itemIds.length() != 0) {
        auto pageCount = (uint)ceil((double)m_itemIds.length() / GROUP_MAX_ITEMS_PER_PAGE);
        for (int i = 0; i < pageCount; i++) {
            m_pages.append(m_itemIds.length() > GROUP_MAX_ITEMS_PER_PAGE * (i +1) ?
                // 本页占满时，全部填充
                GROUP_MAX_ITEMS_PER_PAGE :
                // 未占满时，填充剩余部分
                m_itemIds.length() - GROUP_MAX_ITEMS_PER_PAGE * i);
        }
    }

    return m_pages;
}

void AppGroup::setPages(const QList<int> &pages)
{
    if (pages == m_pages)
        return;

    m_pages = pages;
    Q_EMIT pagesChanged();
}

}

