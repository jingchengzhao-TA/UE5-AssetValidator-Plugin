# UE5 Asset Validator Plugin

> 基于 Unreal Engine 5.6 开发的纯 C++ 资产规范检查与自动修复编辑器插件。封装为独立 Editor Standalone Window Plugin,通过数据驱动的配置表、右键菜单集成与保存拦截机制,帮助美术团队在零代码改动的前提下灵活定义资产规范,并实现违规资产的一键修复与可跳转错误定位。

## ✨ 核心特性

- **数据驱动架构**:基于 UDataTable 的配置体系,美术/策划无需改代码即可拓展检查规则,彻底规避硬编码
- **多维度资产校验**:覆盖贴图(分辨率/压缩格式)、静态模型(正则命名/LOD0 顶点数)、场景 Actor(目录/蓝图类型白名单)
- **一键自动修复**:对违规贴图执行 `MaxTextureSize` / `CompressionSettings` 参数批量修正,并通过 `MarkPackageDirty()` 确保数据正确持久化到 `.uasset`
- **无感工作流集成**:右键菜单三档功能 + Ctrl+S 保存拦截,违规资产无法落盘
- **可跳转日志系统**:基于 `FMessageLog` + `FUObjectToken` 输出超链接日志,点击即定位违规资产
- **稳健的报错处理**:大体量诊断日志写入本地 UTF-8 txt,弹窗仅展示统计摘要,规避 UI 卡顿

## 🛠 技术栈

- **引擎版本**:Unreal Engine 5.6
- **开发语言**:C++ (Editor Standalone Window Plugin)
- **核心 API**:Slate / ToolMenus API / UDataTable / UE Reflection / FCoreUObjectDelegates / FMessageLog

## 🔑 核心技术实现

### 1. 数据驱动架构 (Data-Driven Design)

定义继承自 `FTableRowBase` 的核心配置结构体:`FTextureCheckRule`、`FStaticMeshCheckRule`、`FActorCheckRule`。

通过 `UDataTable` 在编辑器内创建配置实体,C++ 层利用 `StaticLoadObject` 和 `GetAllRows` 动态加载并读取规则,确保策划/美术可以在**不修改代码**的情况下自由拓展检查目录和规范参数。

### 2. 多维度资产检测

| 资产类型 | 核心 API | 检测维度 |
|---------|---------|---------|
| **贴图 (Texture)** | `Texture->Source.GetSize()` | 源文件尺寸、压缩格式 |
| **静态模型 (StaticMesh)** | `FRegexPattern` / `FRegexMatcher` / `GetNumVertices(0)` | 命名正则、LOD0 顶点超标 |
| **场景 Actor** | `TActorIterator<AActor>` / `GetFolderPath()` / `IsChildOf()` | 大纲目录、蓝图类型白名单 |

**贴图自动修复**:直接修改 `MaxTextureSize` 与 `CompressionSettings` 属性,并调用 `Texture->UpdateResource()` 使更改在编辑器内立即生效。

**Actor 视觉警示**:利用 `SetActorLabel()` 动态为不合格 Actor 追加 `[❌非法类型]` 醒目前缀,实现安全且直观的"爆红"效果(替代直接修改底层 Slate UI 的不稳定方案)。

### 3. 文件 I/O 与稳健的反馈机制

- 使用 `FFileHelper::SaveStringToFile` 配合 `FPaths::ProjectSavedDir()`,将详细诊断日志以 **UTF-8 编码**输出到工程目录的 `.txt` 文件中
- 使用 `FMessageDialog::Open` 仅弹出包含统计数量和 txt 路径的**简要提示框**
- **设计动机**:大体量报错若直接塞入对话框会导致 UI 卡顿,文件 + 摘要的分离方案确保工具在数千资产场景下依然流畅

### 4. 用户体验工作流

#### 无缝右键菜单集成
通过 `ToolMenus API` 扩展 `ContentBrowser.AssetContextMenu`,美术无需打开独立面板即可触发功能。

#### 工作流自动化拦截
绑定引擎底层委托 `FCoreUObjectDelegates::OnObjectSaved`,当用户按下 Ctrl+S 时**后台静默**触发检查,**违规资产无法落盘**——实现"无感介入"的高级工作流。

#### 高级可跳转日志
引入 `FMessageLog` 与 `FUObjectToken`,在引擎消息日志中输出带超链接的报错。点击日志即可瞬间让镜头**聚焦并高亮选中**违规资产,排错效率显著提升。

## 📄 License

MIT License

## 👤 作者

**赵景澄**
- B站作品集:https://www.bilibili.com/video/BV19mPTzMEXH/?spm_id_from=333.1387.list.card_archive.click&vd_source=81cfdf9b88cc99f5f3fd204ab3eefb4e
- Email:zjingcheng2022@163.com
