# 插件开发教程

**版本:** 1.0  
**生成时间:** 2026-03-07 10:30:00

## 概述

本教程将指导您逐步为 DDE Shell 创建插件。我们将涵盖三种类型的插件：
1. 基于 QML 的小部件（最简单）
2. 基于小部件的小部件（中级）
3. 容器插件（高级）

每个部分都包含完整的代码示例和说明。

---

## 前置条件

- CMake 3.16+
- Qt 6（或设置适当的 Qt 5）
- DTK6（深度工具包）
- C++ 和 QML 的基础知识
- C++ 编译器（GCC 或 Clang）
- 对 Qt 插件系统有基本了解

---

## 教程 1：创建基于 QML 的小部件

这是最简单的插件类型 - 仅包含 QML 的小部件。

### 步骤 1：创建目录结构

```bash
mkdir -p my-qml-applet/package
cd my-qml-applet
```

### 步骤 2：创建 metadata.json

创建 `package/metadata.json`：

```json
{
  "Plugin": {
    "Version": "1.0",
    "Id": "org.mycompany.myapplet",
    "Url": "main.qml"
  }
}
```

**说明：**
- `Version`：插件版本号
- `Id`：唯一标识符（使用反向域名表示法）
- `Url`：相对于 package 目录的 QML 文件路径

### 步骤 3：创建 QML 文件

创建 `package/main.qml`：

```qml
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import org.deepin.ds 1.0

AppletItem {
    objectName: "my applet"
    implicitWidth: 100
    implicitHeight: 100
    
    Rectangle {
        anchors.fill: parent
        color: "#2ecc71"
        radius: 8
        
        Text {
            anchors.centerIn: parent
            text: "你好，世界！"
            font.pixelSize: 14
            color: "white"
        }
    }
}
```

**说明：**
- `AppletItem`：QML 小部件的基础组件
- `implicitWidth/Height`：默认大小
- Rectangle：视觉容器
- Text：居中显示文本

### 步骤 4：创建 CMakeLists.txt

```cmake
# SPDX-FileCopyrightText: 2024 MyCompany
# SPDX-License-Identifier: GPL-3.0-or-later

ds_install_package(PACKAGE org.mycompany.myapplet)
```

**说明：**
- `ds_install_package`：安装插件包的自定义宏
- 仅 QML 的插件不需要 C++ 代码

### 步骤 5：构建和安装

```bash
# 从 dde-shell 根目录
cmake -Bbuild
cmake --build build
cmake --install build
```

### 步骤 6：测试您的插件

安装后，您的插件将被 DDE Shell 自动发现并加载。

---

## 教程 2：创建基于小部件的小部件

此插件使用 Qt 小部件作为 UI。

### 步骤 1：创建目录结构

```bash
mkdir -p my-widget-applet
cd my-widget-applet
```

### 步骤 2：创建头文件

创建 `mywidgetapplet.h`：

```cpp
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "applet.h"

DS_USE_NAMESPACE

class MyWidgetApplet : public DApplet
{
    Q_OBJECT
public:
    explicit MyWidgetApplet(QObject *parent = nullptr);
    virtual bool init() override;
};
```

**说明：**
- 继承自 `DApplet`
- 实现 `init()` 方法
- 使用 `DS_USE_NAMESPACE` 获取 DDE Shell 命名空间

### 步骤 3：创建实现文件

创建 `mywidgetapplet.cpp`：

```cpp
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mywidgetapplet.h"

#include "pluginfactory.h"

#include <DIconButton>
#include <DIconTheme>
#include <DLabel>
#include <dplatformwindowhandle.h>

#include <QHBoxLayout>
#include <QWidget>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

MyWidgetApplet::MyWidgetApplet(QObject *parent)
    : DApplet(parent)
{
}

bool MyWidgetApplet::init()
{
    // 创建主小部件
    auto widget = new QWidget();
    DPlatformWindowHandle handle(widget);
    widget->setFixedSize(QSize(200, 100));
    
    // 创建布局
    auto layout = new QHBoxLayout(widget);
    
    // 创建图标按钮
    auto btn = new DIconButton();
    btn->setIcon(DIconTheme::findQIcon("deepin-home"));
    btn->setIconSize(QSize(32, 32));
    layout->addWidget(btn);
    
    // 创建标签
    auto label = new DLabel("我的小部件");
    label->setStyleSheet("color: white; font-size: 14px;");
    layout->addWidget(label);
    
    // 显示小部件
    widget->show();
    
    // 调用父类 init
    return DApplet::init();
}

// 注册插件
D_APPLET_CLASS(MyWidgetApplet)

#include "mywidgetapplet.moc"
```

**说明：**
- 创建包含 DTK 组件的 QWidget
- 使用 `DPlatformWindowHandle` 进行窗口管理
- `D_APPLET_CLASS` 宏注册插件
- 必须在末尾包含 `.moc` 文件

### 步骤 4：创建 metadata.json

创建 `package/metadata.json`：

```json
{
  "Plugin": {
    "Version": "1.0",
    "Id": "org.mycompany.mywidgetapplet"
  }
}
```

**注意：** 基于小部件的插件不需要 `Url`。

### 步骤 5：创建 CMakeLists.txt

```cmake
# SPDX-FileCopyrightText: 2024 MyCompany
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Widgets)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget)

add_library(ds-mywidgetapplet SHARED
    mywidgetapplet.h
    mywidgetapplet.cpp
)

target_link_libraries(ds-mywidgetapplet PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::Widgets
    Dtk${DTK_VERSION_MAJOR}::Widget
)

ds_install_package(PACKAGE org.mycompany.mywidgetapplet TARGET ds-mywidgetapplet)
```

**说明：**
- 构建共享库
- 链接 DDE Shell 框架、Qt 和 DTK
- 使用 `ds_install_package` 安装插件

### 步骤 6：构建和安装

```bash
cmake -Bbuild
cmake --build build
cmake --install build
```

---

## 教程 3：创建容器插件

这是最复杂的插件类型 - 它管理其他小部件。

### 步骤 1：创建目录结构

```bash
mkdir -p my-containment/package
cd my-containment
```

### 步骤 2：创建头文件

创建 `mycontainment.h`：

```cpp
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "containment.h"

DS_USE_NAMESPACE

class MyContainment : public DContainment
{
    Q_OBJECT
public:
    explicit MyContainment(QObject *parent = nullptr);
    ~MyContainment();

    virtual bool load() override;
protected:
    virtual QObject *createProxyMeta() override;
};
```

**说明：**
- 继承自 `DContainment`
- 实现 `load()` 和 `createProxyMeta()`
- 可以管理子小部件

### 步骤 3：创建实现文件

创建 `mycontainment.cpp`：

```cpp
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mycontainment.h"

#include "pluginfactory.h"
#include "appletproxy.h"
#include <QDebug>

MyContainment::MyContainment(QObject *parent)
    : DContainment(parent)
{
    qDebug() << "MyContainment 已创建";
}

MyContainment::~MyContainment()
{
    qDebug() << "MyContainment 已销毁";
}

bool MyContainment::load()
{
    DCORE_USE_NAMESPACE;
    
    // 获取所有可用的小部件
    auto applets = DPluginLoader::instance()->plugins();
    
    // 过滤特定小部件（示例）
    QList<DAppletData> groups;
    for (const auto &plugin : applets) {
        if (plugin.pluginId().contains("example")) {
            groups << DAppletData::fromPluginMetaData(plugin);
        }
    }
    
    // 设置小部件数据
    auto data = appletData();
    data.setGroupList(groups);
    setAppletData(data);
    
    // 调用父类 load
    return DContainment::load();
}

QObject *MyContainment::createProxyMeta()
{
    // 您可以在这里创建用于 QML 通信的代理对象
    return new QObject(this);
}

// 注册插件
D_APPLET_CLASS(MyContainment)

#include "mycontainment.moc"
```

**说明：**
- `load()`：设置子小部件
- `createProxyMeta()`：为 QML 创建代理
- 使用 `DPluginLoader` 发现小部件
- 过滤并分组小部件

### 步骤 4：创建 QML 文件

创建 `package/main.qml`：

```qml
// SPDX-FileCopyrightText: 2024 MyCompany
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.deepin.ds 1.0

ContainmentItem {
    id: root
    objectName: "my containment"
    implicitWidth: childrenRect.width
    implicitHeight: childrenRect.height
    
    RowLayout {
        spacing: 10
        
        Repeater {
            model: Containment.appletItems
            delegate: Control {
                contentItem: model.data
                
                Rectangle {
                    width: 80
                    height: 80
                    color: "#3498db"
                    radius: 8
                    
                    Text {
                        anchors.centerIn: parent
                        text: model.data.pluginId
                        color: "white"
                        font.pixelSize: 10
                    }
                }
            }
        }
    }
}
```

**说明：**
- `ContainmentItem`：容器 UI 的基础
- `Repeater`：迭代子小部件
- `Control`：委托给小部件内容

### 步骤 5：创建 metadata.json

创建 `package/metadata.json`：

```json
{
  "Plugin": {
    "Version": "1.0",
    "Id": "org.mycompany.mycontainment",
    "Url": "main.qml",
    "ContainmentType": "Containment"
  }
}
```

**说明：**
- `ContainmentType`："Containment" 表示这是一个容器
- `Url`：指向 QML 文件

### 步骤 6：创建 CMakeLists.txt

```cmake
# SPDX-FileCopyrightText: 2024 MyCompany
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(ds-mycontainment SHARED
    mycontainment.h
    mycontainment.cpp
)

target_link_libraries(ds-mycontainment PRIVATE
    dde-shell-frame
)

ds_install_package(PACKAGE org.mycompany.mycontainment TARGET ds-mycontainment)
```

### 步骤 7：构建和安装

```bash
cmake -Bbuild
cmake --build build
cmake --install build
```

---

## 高级主题

### 添加 D-Bus 支持

通过 D-Bus 暴露功能：

```cpp
#include <QDBusConnection>

bool MyPlugin::init()
{
    QDBusConnection::sessionBus().registerObject(
        "/org/mycompany/MyPlugin",
        this
    );
    
    return DApplet::init();
}
```

### 处理配置

使用 DConfig 进行插件设置：

```cpp
#include <DConfig>

bool MyPlugin::init()
{
    auto config = DConfig::create("org.mycompany.myplugin");
    QString setting = config->value("mySetting").toString();
    
    return DApplet::init();
}
```

### 信号/槽通信

C++ 和 QML 之间：

```cpp
// C++ 端
class MyPlugin : public DApplet
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    
signals:
    void statusChanged();
    
public:
    QString status() const { return m_status; }
    
private:
    QString m_status;
};
```

```qml
// QML 端
Connections {
    target: root
    function onStatusChanged() {
        console.log("状态改变：", root.status)
    }
}
```

---

## 调试

### 启用插件日志

```cpp
#include <QDebug>

bool MyPlugin::init()
{
    qDebug() << "MyPlugin::init() 被调用";
    qInfo() << "插件 ID：" << pluginId();
    return DApplet::init();
}
```

### 常见问题

1. **插件未加载**
   - 检查 `metadata.json` 语法
   - 验证插件 ID 是唯一的
   - 检查日志中的错误消息

2. **QML 未显示**
   - 验证 metadata.json 中的 `Url` 指向存在的 QML 文件
   - 检查 QML 语法错误
   - 查看 QML 控制台输出

3. **启动时崩溃**
   - 检查空指针访问
   - 验证所有资源都可用
   - 使用调试器追踪

---

## 下一步

---

## 总结

您已学会如何创建：
1. ✅ 基于 QML 的小部件（最简单）
2. ✅ 基于小部件的小部件（中级）
3. ✅ 容器插件（高级）

每个教程都提供了完整的、可工作的代码，您可以将其作为自己插件的起点。

祝插件开发愉快！
