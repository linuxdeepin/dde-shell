# DDE Shell 插件 API 参考

**版本:** 1.0  
**生成时间:** 2026-03-07 10:30:00

## 概述

DDE Shell 使用基于插件的架构，面板和小部件可以在运行时动态加载。插件通过 `metadata.json` 文件被发现，并使用 Qt 的插件系统加载。

## 插件类型

### 1. 小部件插件（Applet Plugin）

单个功能插件（小部件、指示器等）的基类。

**基类:** `DApplet` (frame/applet.h)

**必需方法:**
```cpp
virtual bool load() override;
virtual bool init() override;
```

**主要属性:**
- `id`: 唯一实例标识符
- `pluginId`: 来自 metadata.json 的插件标识符
- `parent`: 父小部件（用于容器）
- `rootObject`: QML 根对象（用于 QML 插件）

**子类型:**

#### a. 基于 QML 的小部件
- 使用 QML 作为 UI
- metadata.json 指定 `Url` 到 QML 文件
- 自动加载 QML 并创建根对象

#### b. 基于小部件的小部件
- 使用 Qt 小部件（QWidget）作为 UI
- 在 `init()` 中手动创建小部件
- 使用 `DPlatformWindowHandle` 进行窗口管理

### 2. 容器插件（Containment Plugin）

管理多个子小部件的容器插件。

**基类:** `DContainment` (frame/containment.h)

**必需方法:**
```cpp
virtual bool load() override;
virtual bool init() override;
DApplet *createApplet(const DAppletData &data);
void removeApplet(DApplet *applet);
```

**主要属性:**
- `appletItems`: 子小部件的模型（用于 QML）
- `applets()`: 子小部件实例列表

**受保护方法:**
```cpp
virtual QObject *createProxyMeta() override;
```

**使用场景:** 面板、通知中心 - 多个项目的容器。

### 3. 面板插件（Panel Plugin）

用于面板/通知中心的专用容器。

**基类:** `DPanel` (继承自 `DContainment`)

**必需方法:**
```cpp
virtual bool load() override;
virtual bool init() override;
```

**使用场景:** 特定的面板实现（dock、notification）。

## 插件元数据

### metadata.json 格式

所有插件都需要 `package/metadata.json` 文件：

```json
{
  "Plugin": {
    "Version": "1.0",
    "Id": "org.deepin.ds.example.applet",
    "Url": "main.qml",
    "Parent": "org.deepin.ds.example.containment",
    "ContainmentType": "Panel|Containment|Applet"
  }
}
```

### 元数据字段

| 字段 | 类型 | 必需 | 描述 |
|-------|------|------|------|
| `Version` | 字符串 | 是 | 插件版本（如 "1.0"） |
| `Id` | 字符串 | 是 | 唯一插件标识符（反向域名风格） |
| `Url` | 字符串 | 条件* | QML 文件路径（QML 插件必需） |
| `Parent` | 字符串 | 条件* | 父插件 ID（用于子插件） |
| `ContainmentType` | 字符串 | 条件* | 插件类型："Panel"、"Containment" 或 "Applet" |

*根据插件类型条件必需

### ID 命名约定

使用反向域名表示法：`org.deepin.ds.<组件>.<名称>`

示例：
- `org.deepin.ds.dock.taskmanager`
- `org.deepin.ds.notification.center`
- `org.deepin.ds.example.applet`

## 插件注册

### C++ 类注册

使用 `D_APPLET_CLASS` 宏注册插件类：

```cpp
#include "pluginfactory.h"

// 您的插件类
class MyPlugin : public DApplet
{
    Q_OBJECT
public:
    explicit MyPlugin(QObject *parent = nullptr);
    virtual bool init() override;
};

// 注册插件
D_APPLET_CLASS(MyPlugin)

#include "myplugin.moc"
```

### 构建配置

#### 小部件的 CMakeLists.txt

```cmake
# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Widgets)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget)

add_library(ds-my-plugin SHARED
    myplugin.h
    myplugin.cpp
)

target_link_libraries(ds-my-plugin PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::Widgets
    Dtk${DTK_VERSION_MAJOR}::Widget
)

ds_install_package(PACKAGE org.deepin.ds.myplugin TARGET ds-my-plugin)
```

#### 仅 QML 插件的 CMakeLists.txt

```cmake
# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

ds_install_package(PACKAGE org.deepin.ds.myplugin)
```

## 核心 API

### DApplet API

```cpp
class DS_SHARE DApplet : public QObject, public DTK_CORE_NAMESPACE::DObject
{
    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString pluginId READ pluginId CONSTANT FINAL)
    Q_PROPERTY(QObject *rootObject READ rootObject NOTIFY rootObjectChanged)

    // 生命周期
    virtual bool load() override;
    virtual bool init() override;

    // 属性
    QString id() const;
    QString pluginId() const;
    QObject *rootObject() const;
    DApplet *parentApplet() const;

    // 元数据
    DPluginMetaData pluginMetaData() const;
    DAppletData appletData() const;
    void setAppletData(const DAppletData &data);
    void setRootObject(QObject *root);

signals:
    void rootObjectChanged();

protected:
    // 创建 QML 通信的代理对象
    virtual QObject *createProxyMeta();
};
```

### DContainment API

```cpp
class DS_SHARE DContainment : public DApplet
{
    Q_PROPERTY(DAppletItemModel *appletItems READ appletItemModel CONSTANT)

    // 小部件管理
    DApplet *createApplet(const DAppletData &data);
    void removeApplet(DApplet *applet);
    QList<DApplet *> applets() const;
    QList<QObject *> appletItems();
    DAppletItemModel *appletItemModel() const;
    DApplet *applet(const QString &id) const;

    // 生命周期（继承自 DApplet）
    virtual bool load() override;
    virtual bool init() override;

protected:
    virtual QObject *createProxyMeta() override;
};
```

### DPluginMetaData API

```cpp
class DS_SHARE DPluginMetaData : public QObject
{
    // 验证
    bool isValid() const;

    // 访问器
    QString pluginId() const;
    QString pluginDir() const;
    QString url() const;
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    // 静态创建器
    static DPluginMetaData fromJsonFile(const QString &file);
    static DPluginMetaData fromJsonString(const QByteArray &data);
    static DPluginMetaData rootPluginMetaData();
    static bool isRootPlugin(const QString &pluginId);
};
```

### DAppletBridge API

从 C++ 与小部件通信的桥接：

```cpp
#include "appletbridge.h"

// 创建到小部件的桥接
DAppletBridge bridge("org.deepin.ds.example.applet");

// 检查小部件是否可用
if (auto applet = bridge.applet()) {
    // 获取属性
    QString value = applet->property("mainText").toString();

    // 调用方法
    QMetaObject::invokeMethod(applet, "call", Qt::DirectConnection,
                              Q_RETURN_ARG(QString, result),
                              Q_ARG(const QString&, id));

    // 连接到信号
    QObject::connect(applet, SIGNAL(sendSignal(const QString&)),
                     this, SLOT(onReceivedSignal(const QString&)));
}
```

## QML 集成

### QML 导入

插件可以导入 DDE Shell QML 模块：

```qml
import QtQuick 2.11
import org.deepin.ds 1.0
```

### 可用的 QML 组件

#### AppletItem
QML 小部件的基础组件：

```qml
AppletItem {
    implicitWidth: 100
    implicitHeight: 100
    Rectangle {
        anchors.fill: parent
        // 您的内容在这里
    }
}
```

#### ContainmentItem
容器插件的基础组件：

```qml
ContainmentItem {
    implicitWidth: childrenRect.width
    implicitHeight: childrenRect.height
    RowLayout {
        Repeater {
            model: Containment.appletItems
            delegate: Control {
                contentItem: model.data
            }
        }
    }
}
```

## 插件生命周期

### 加载过程

1. **发现**: DPluginLoader 扫描插件目录查找 `metadata.json`
2. **解析**: DPluginMetaData 从 JSON 解析元数据
3. **实例化**: DAppletFactory 通过 Qt 插件系统创建插件实例
4. **load()**: 调用插件的 `load()` 方法
5. **init()**: 调用插件的 `init()` 方法
6. **QML 加载**（如适用）：加载 QML 文件并创建根对象

### 卸载过程

1. **关闭**: 插件清理被处理
2. **移除**: 小部件从父容器移除
3. **资源清理**: 所有资源被释放

## 最佳实践

### 1. 插件 ID
- 使用反向域名表示法
- 遵循模式：`org.deepin.ds.<组件>.<名称>`
- 保持所有插件中 ID 唯一

### 2. 错误处理
```cpp
bool MyPlugin::init()
{
    // 检查必需资源
    if (!requiredResourceAvailable()) {
        qWarning() << "必需资源不可用";
        return false;
    }

    // 调用父类 init
    return DApplet::init();
}
```

### 3. QML 性能
- 高效使用属性（避免过度重新计算）
- 缓存昂贵的操作
- 使用 Connections 进行响应式更新

### 4. 内存管理
- 使用 Qt 的父子系统
- 尽可能让 Qt 处理清理
- 小心显式的 `delete` 操作

### 5. 代理对象
- 使用 `createProxyMeta()` 向 QML 暴露方法
- 用 `Q_INVOKABLE` 标记方法
- 使用信号/属性进行响应式通信

## 示例

参见 `example/` 目录获取完整的插件示例：
- `applet-example`: 基于 QML 的小部件
- `applet-widget-example`: 基于小部件的小部件
- `containment-example`: 容器插件
- `panel-example`: 面板插件

## 故障排除

### 插件未加载
1. 检查 `metadata.json` 是有效的 JSON
2. 验证插件 ID 是唯一的
3. 检查插件库已构建并安装
4. 查看日志中的错误消息

### QML 未加载
1. 验证 metadata.json 中的 `Url` 指向存在的 QML 文件
2. 检查 QML 语法正确
3. 确保所有必需的导入都存在
4. 查看 QML 控制台日志

### 插件崩溃
1. 检查空指针访问
2. 验证所有必需资源都可用
3. 使用调试器追踪崩溃
4. 查看插件生命周期方法

## 另见

- [插件教程](./tutorial.md) - 创建插件的分步指南
- [示例插件](../../example/) - 完整的工作示例
