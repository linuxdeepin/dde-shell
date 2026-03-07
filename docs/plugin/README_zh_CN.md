# DDE Shell 插件开发文档

欢迎使用 DDE Shell 插件开发文档。本目录包含为 DDE Shell 创建插件的全面指南。

## 文档文件

### API 参考
- **[API_zh_CN.md](./API_zh_CN.md)** (中文) - 完整的插件开发 API 参考
- **[API_en.md](./API_en.md)** (English) - API reference for plugin development

### 教程
- **[tutorial_zh_CN.md](./tutorial_zh_CN.md)** (中文) - 分步创建插件指南
- **[tutorial_en.md](./tutorial_en.md)** (English) - Step-by-step guide to creating plugins

---
## 插件类型

DDE Shell 支持三种主要类型的插件：

1. **小部件插件（Applet Plugin）** - 简单的单功能插件（小部件、指示器）
2. **容器插件（Containment Plugin）** - 管理多个子小部件的容器插件
3. **面板插件（Panel Plugin）** - 面板的专用容器（任务栏、通知中心）

## 快速入门

### 快速开始

如果您是插件开发的新手，请从教程开始：

```bash
# 1. 阅读教程
cat tutorial_zh_CN.md

# 2. 探索示例插件
ls ../../example/

# 3. 构建和测试示例
cd ../../example/applet-example
cmake -Bbuild
cmake --build build
```

### 插件开发流程

1. **选择插件类型** 根据您的需求
2. **创建目录结构** 包含所需文件
3. **实现插件类** 继承自基类
4. **编写 metadata.json** 包含插件信息
5. **创建 CMakeLists.txt** 用于构建
6. **构建和测试** 您的插件

## 示例

`example/` 目录包含可工作的插件示例：

| 示例 | 类型 | 描述 |
|------|------|------|
| `applet-example` | QML 小部件 | 基于 QML 的简单小部件 |
| `applet-widget-example` | Widget 小部件 | 基于 Qt 小部件的小部件 |
| `containment-example` | 容器 | 子小部件的容器 |
| `panel-example` | 面板 | 任务栏样式的面板 |

## 构建系统

插件使用 CMake 和自定义宏：

- `ds_install_package()` - 安装插件包
- `D_APPLET_CLASS()` - 注册插件类
- 链接到 `dde-shell-frame` 以使用基类

## 故障排除

### 插件未加载
1. 检查 `metadata.json` 语法是否有效
2. 验证插件 ID 是否唯一
3. 确保插件库已构建并安装
4. 查看日志以获取错误消息

### QML 未加载
1. 验证 metadata.json 中的 `Url` 指向现有的 QML 文件
2. 检查 QML 语法是否正确
3. 确保存在所有必需的导入
4. 查看 QML 控制台日志

## 最佳实践

- **使用唯一的插件 ID** 使用反向域表示法
- **遵循命名约定**（例如，`org.deepin.ds.myplugin`）
- **优雅地处理错误** 在 `load()` 和 `init()` 方法中
- **正确清理资源** 在析构函数中
- **使用 Qt 的父子系统** 进行自动清理
- **彻底测试** 部署前

## 支持

- 查阅 [API 参考](./API_zh_CN.md)
- 在 GitHub 上提交问题

## 许可证

DDE Shell 插件遵循 GPL-3.0-or-later 许可证，除非另有说明。

---

**祝插件开发愉快！🎉**
