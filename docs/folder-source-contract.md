# 目录驻留与内容页面契约

## 职责

| 层 | 职责 |
| --- | --- |
| Dock 框架 `dock::FolderContentRegistry` | 解析目录引用，按来源加载内容插件，创建展示对象 |
| 框架 `PanelPopup` 与任务栏 `FolderPopup` | 统一弹窗生命周期、内容承载、尺寸约束、定位和打开关闭 |
| 任务栏 | 固定与取消固定、排序、承载插件图标、把点击交给内容容器 |
| 文管内容插件 | 目录读取与监听、导航、缩略图、文件打开、驻留图标外观、内容页面内部布局和交互 |


## 最小接口

接口定义位于 `panels/dock/frame/foldercontent.h`，使用 `dock` 命名空间，
由 `dde-shell-dock` 提供。外部插件通过 `find_package(DDEShellDock REQUIRED)`
并链接 `Dde::ShellDock` 使用安装后的 `<dock/foldercontent.h>`：

```cpp
namespace dock {
class FolderContentProvider {
public:
    virtual ~FolderContentProvider() = default;
    virtual QString normalizeId(const QString &id) const = 0;
    virtual FolderPresentation *create(const QString &id, QObject *parent) = 0;
};
}

// FolderPresentation 的公开属性
QString title;
QVariantList previewIcons; // 兼容预览数据，文管图标组件内部使用
QUrl iconUrl;              // 由插件提供的任务栏图标 QML Item 入口
QUrl contentUrl;           // 由插件提供的 QML Item 入口
// presentationChanged() 通知任务栏刷新图标和标题
```

`iconUrl` 由内容提供方提供任务栏目录图标组件；Shell 的任务栏 `FolderIcon.qml` 是唯一图标容器，
按内容来源加载提供方组件或绘制启动器目录预览。文管文件夹的图标预览布局和外观由文管插件实现，
任务栏不读取或解释文管的 `previewIcons`。加载插件图标时注入 `presentation` 属性并将图标尺寸绑定到
承载区域；图标缺失或加载失败时显示通用目录图标。
插件图标根项应为 `Item`，可以声明 `required property var presentation`，不应接管点击与拖拽。
`org.deepin.ds.FolderContentProvider/1.0` 的虚接口已冻结；新增可选能力应使用独立 IID，
不兼容修改需升级接口 IID。插件应使用已合入的 `dock::FolderContentProvider` 和
`dock::FolderPresentation`，不再使用开发阶段的 `ds` 接口。

这里没有内容条目模型、文件操作或业务菜单接口。任务栏的唯一 `FolderPopup.qml` 模板按内容来源
显示启动器目录网格，或加载 `contentUrl` 并注入 `presentation` 属性；页面使用插件派生类提供的
内部 API。需要关闭时，页面发出 `closeRequested()`。内容根项通过
`implicitWidth/implicitHeight` 请求尺寸，统一容器按可用区域约束。

文管的 `directoryModel`、`navigateTo`、`goBack`、`openFile` 等只供文管页面使用，
任务栏不读取这些属性，也不调用这些方法。

## 拖拽和持久化

拖拽 MIME 为 `text/x-dde-dock-dnd-folder`，内容为：

```text
folder/<source>/<base64url(UTF-8 id)，无填充>
```

当前文管的 `source` 为 `filemanager`，`id` 是规范化绝对目录的、完整编码的 `file:` URL。
任务栏验证引用结构并调用 provider 的 `normalizeId` 校验来源支持的对象。
provider 暂时缺失时，合法引用保持不变，可保存为占位驻留项；这不表示内容当前可用。
拖入期间复用任务栏现有预览排序逻辑；移出取消本次临时固定，提交使用 `CopyAction`，
源目录不会因固定而被移动。已有固定项的重复拖入会被拒绝。

设置只保存上述引用及顺序，不保存文件清单。恢复时即使插件缺失，也保留引用和取消固定入口。
插件和其它 Shell 插件一样安装到 `${libdir}/dde-shell/`，文件名遵循统一的
`org.deepin.ds.folderprovider.<source>.so` 规则，例如文管插件是
`org.deepin.ds.folderprovider.filemanager.so`。框架按来源拼出文件名后在该目录加载。

## 文件变化与生命周期

```text
文件系统变化 → 文管 DirectoryModel 监听并异步重新读取
                         ├─ 文管浏览页面绑定模型，更新内容
                         └─ presentationChanged → 文管图标组件刷新预览，任务栏刷新标题
```

文管插件使用 `QFileSystemWatcher`，合并短时间内的变化；后台读取结果带有代次，
过期读取结果不会覆盖新目录。目录监听在开始读取前建立。
原路径被删除后监听最近存在的父目录，路径重建时重新读取。

每个固定项拥有独立展示对象，取消固定后销毁对象及监听。浏览页面的导航模型与
固定目录的预览模型分开，进入子目录不会改变任务栏图标所代表的目录。

同目录重命名会自动跟随：目录监听发现固定路径消失后，用 `stat` 的设备号与 inode
在同一父目录中找回同名项，展示对象改用新路径，并发出 `identityChanged`，
任务栏据此用 `referenceWithId` 重写 `folder/<source>/<id>` 持久化值，
固定项不会退化为指向已失效路径的空文件夹。跨目录移动仍不跟随，
原路径不存在时显示空内容，固定项仍可移除，原路径重新创建后恢复。

固定目录被删除时会移除固定项：父目录仍然存在说明被删的是固定目录本身，
延迟 500 毫秒的容错窗口（覆盖删除后立即重建的情况）过后仍不存在，
展示对象发出 `contentLost`，任务栏据此从 `dockedElements()` 中删除该
`folder/<source>/<id>` 条目，避免长期保留一个永远为空的文件夹。
如果整个父目录层级都消失，只当作路径暂时不可达，固定项保留等原路径恢复。
当前插件缺失时显示通用不可用提示，安装插件后需重新加载 Shell 才能恢复已有展示对象。

## 验证

任务栏 `FolderPopup.qml` 统一处理普通目录和文管内容页面；`frame_qml` 覆盖共享弹窗生命周期。
文管插件的独立测试覆盖动态加载、真实 QML 页面、多个目录独立导航、文件增删改名、
目录删除后重建、固定目录重命名后跟随并上报新身份、固定目录被删除时上报内容丢失
而父目录整体消失时不上报；构建和手工拖拽步骤见文管
`folder-content/README.md`。
