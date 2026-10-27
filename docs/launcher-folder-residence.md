# 启动器文件夹驻留

架构修订：统一数据源的目标职责、变化通知及跨进程契约见
[folder-source-contract.md](folder-source-contract.md)。本文记录当前启动器专项实现；
其中直接调用启动器后端的部分仍待按统一契约重构。

## 范围与数据所有权

本次接入启动器应用文件夹。文件管理器文件夹的数据源、文件监控和文件操作不在本次范围内；后续可复用 FolderIcon.qml / FolderPopup.qml，不另做一套布局。

- dde-shell 的 org.deepin.ds.dde-apps applet 暴露 appGroupModel（AppGroupManager），作为文件夹、顶层排序和分页的唯一可写模型。
- dde-launchpad 的 ItemArrangementProxyModel 只提供应用/文件夹联合列表与角色适配；创建、重命名、拖拽、分页、移到首位都转发给共享模型，不再读写独立 INI。
- DockGlobalElementModel 将应用文件夹映射成驻留项，不拥有另一份子项列表。驻留顺序与是否驻留仍归 TaskManagerSettings 管理，取消驻留不修改源文件夹。
- 文件夹身份为 internal/folders/N（N > 0）；0 仅代表启动器顶层，不允许驻留。ID 不是模型行号。读取旧数据时兼容 internal/folder/N、internal/group/N 和数字编号。
- folderInfo 返回 {id, name, contents, pageCount}，contents 中每项为 {id, name, iconName}。AppGroupManager 暴露原始分类名称，Dock 使用启动器翻译上下文解析默认分类名称。
- Dock 的应用文件夹接口使用 applicationFolder 命名：setApplicationFolderModel、containsApplicationFolder、syncApplicationFolders。名称直接读取共享模型的 name 角色，仅在请求 ApplicationFolderContentsRole 时调用 folderInfo 构造子项；文管目录的图标和页面通过 presentation 提供。
- 应用源 ready=false 时不清理已有排列；应用源不可用时保留文件夹身份。源恢复为 ready 后才处理应用删除与空文件夹清理。

## 公共模板与交互

FolderIcon 固定保留四个 2×2 预览位置。FolderPopup 接收名称和 contents，通过 activated/contextMenuRequested 输出业务事件，不直接操作应用或文件。最多五列、三行可见，超出后垂直滚动；名称最多两行。PanelPopup 不添加淡入淡出动画，保留原有开关行为；通过 closeOnInactive 在菜单、对话框或拖拽交互期间暂时禁止失焦关闭。

应用菜单单独位于 FolderApplicationMenu，调用启动器 DesktopIntegration 已有的桌面快捷方式、自启动、驻留与卸载接口。启动器原有右键菜单保持自己的文案、状态判断和交互；Dock 处理自己的原生菜单、卸载确认和焦点，不显示尚未支持的缩放操作。打开应用使用 dde-am。后续文件管理器数据源应提供自己的业务菜单和激活回调。

DockPanelPositioner 暴露当前屏幕 availableGeometry，使模板在任务栏四个方向、多屏和工作区偏移下限制尺寸与位置。不能使用 QML Screen.desktopAvailableWidth/Height 代替单屏工作区。

拖拽使用既有 text/x-dde-dock-dnd-appid/source MIME。进入任务栏时只对新驻留项做预览，离开时回滚预览，不移除本来已驻留的同源项。任务栏内部排序落地后保存混合应用/文件夹顺序。从任务栏拖出不会取消驻留，取消驻留只通过驻留项右键菜单的「移除驻留」完成。

## 驻留项右键菜单：固定项 + 插件扩展

这与**弹窗内应用图标**的菜单是两个入口：

- 任务栏文件夹驻留项：框架默认只提供「文件夹名称」（点击打开面板）、「移除驻留」。
- 启动器文件夹弹窗内的应用：由 FolderApplicationMenu.qml 提供应用操作，不会追加到驻留项菜单。

FolderMenuRegistry 负责组合固定项和插件扩展项，FolderContextMenu.qml 负责驻留项菜单的展示与事件分派。每次打开菜单时请求最新列表，只把自定义项送回注册插件；不能把未知项当作「打开文件夹」。启动器目前不注册额外选项，因此默认就是上述两项。

文件夹的 MenusRole 固定返回空列表，不再重复生成普通应用菜单使用的 JSON。共享模型在完整排列操作结束后发出 arrangementChanged，Dock 统一同步一次，并一起通知名称及子项变化。

### 插件后端契约

每种 folderType（对应 FolderTypeRole）可注册一个由插件持有的 QObject。folderType 表示来源：启动器为 launcher，文管为 filemanager，其它内容插件使用 folder/<source>/<id> 引用中的 source，不统一归为 content。当前接口用于同一 GUI 线程的 C++ 插件后端，不要求插件链接任务栏实现库。后端必须暴露：

```cpp
Q_INVOKABLE QVariantList folderMenuActions(const QString &folderId);
Q_INVOKABLE void triggerFolderMenuAction(const QString &folderId, const QString &actionId);
```

例如插件可返回：

```cpp
QVariantList MyFolderPlugin::folderMenuActions(const QString &folderId)
{
    return {QVariantMap{{"id", "show-properties"}, {"name", tr("Properties")},
                        {"enabled", canShowProperties(folderId)}}};
}

void MyFolderPlugin::triggerFolderMenuAction(const QString &folderId, const QString &actionId)
{
    if (actionId == "show-properties") showProperties(folderId);
}
```

每项格式：

| 字段 | 类型 / 默认值 | 含义 |
| --- | --- | --- |
| id | 非空字符串，必填 | 插件内唯一操作标识 |
| name | 非空字符串，必填 | 插件负责翻译的显示文本 |
| enabled | bool / true | 是否可点击 |
| visible | bool / true | 是否出现在本次菜单中 |
| checkable | bool / false | 是否显示勾选状态 |
| checked | bool / false | 当前状态；仅 checkable=true 时有效 |

框架固定项排在前面，插件项按返回顺序追加。不接受覆盖保留 ID（open-folder、undock-folder）、重复 ID 或缺少名称的项。当前扩展项是扁平操作列表，不包含子菜单。勾选项的状态由插件维护，点击回调后自行切换，下次打开重新读取。

### 注册与卸载

在 taskmanager applet 加载后，通过 DAppletBridge 获得对象并注册（以下 provider 为插件持有的后端 QObject）：

```cpp
DS_NAMESPACE::DAppletBridge bridge("org.deepin.ds.dock.taskmanager");
QObject *taskManager = bridge.applet();
bool registered = false;
if (taskManager) {
    QMetaObject::invokeMethod(taskManager, "registerFolderMenuProvider",
        Q_RETURN_ARG(bool, registered),
        Q_ARG(QString, QStringLiteral("launcher")), Q_ARG(QObject *, provider));
}
```

注册失败时应由插件处理，不能直接覆盖其他插件已注册的同类型 provider。插件主动停用时调用 unregisterFolderMenuProvider(folderType, provider)；注销只对该 provider 自己生效。框架使用 QPointer，不接管 provider 所有权，插件销毁后自动退回两项默认菜单。

原生菜单打开期间插件可能被卸载或改变可用状态，因此点击时会重新查询插件选项。已隐藏、禁用或移除的操作不执行；固定项不能通过插件回调入口执行。

## QML 驻留接口

QML 通过 Applet / TaskManager 附加类型使用小写接口：

- requestDockByDesktopId(id)：请求驻留。已驻留时返回 false 且不改变顺序；QML DropArea 用该返回值放行拖拽，因此已驻留项再次从启动器拖入会被拒绝，不会在拖拽过程中重排模型。
- requestUndockByDesktopId(id)：移除驻留，不删除源文件夹。

这些接口既接受应用 desktop ID，也接受 internal/folders/N。原有 IsDocked / RequestDock / RequestUndock 保留给 C++ 和 D-Bus；不要通过 QML 附加类型直接调用这些大写名称。在当前 Qt 6.8 中，这种查找会得到 undefined，即使方法已声明 Q_INVOKABLE。

## 配置迁移与部署

1. 两个仓库需要一起构建、安装；新启动器适配层依赖共享模型新增的方法。仅替换一侧不属于受支持组合。
2. 必须一并部署 applets/dde-apps/configs/org.deepin.ds.dde-apps.json，其中新增 ArrangementMigrated 标记。
3. 以 Groups 顶层记录的 arrangementVersion=1 判断迁移是否完成；旧 ArrangementMigrated 标记不能单独证明布局有效。首次迁移导入包含 toplevel 的 ~/.config/deepin/dde-launchpad/item-arrangement.ini，并保留原文件；INI 缺失或不完整时保留已有 Groups。之后使用共享 DConfig，避免旧文件覆盖新排列。
4. Dock 的 dockedElements 在已有 desktop/<appid> 条目之间保存 internal/folders/N。恢复时按配置中的相邻驻留项插入，避免文件夹先于应用加载造成顺序错乱。
5. 本次开发未安装到当前桌面或重启正在运行的 Shell。新增共享文件夹测试使用内存配置与临时目录，QML 测试使用 offscreen，不启动真实任务栏。
