# WorldEngine 命名与资源迁移

正式名称为 **WorldEngine（寰宇引擎）**，代码前缀为 **W**。本次已实际应用源码、构建、路径与资产迁移，不再停留于预览。

## 规则与边界

| 内容 | 处理 |
| --- | --- |
| 产品名 | `ezEngine` → `WorldEngine` |
| 类型 / 宏 / CMake | `ezWorld` → `WWorld`；`EZ_CORE_DLL` → `W_CORE_DLL`；`ez_create_target` → `W_create_target` |
| 库、插件与工具 | `WFoundation.dll`、`WEditor.exe` 等 |
| 项目与资源路径 | `WProject`、`.WScene`、`.WPrefab`、`.WProfile` 等 |
| 字面量与特殊标识 | `_ezsv` → `_wsv`（避免 C++ 保留的下划线加大写字母形式）；依赖包和命令行 `-DEZ_` 参数同步迁移 |
| 归档魔数 | `WEARCHIVE` / `WEARCHIVE-END`，含终止符仍为 10 / 14 字节 |
| 资产 / 场景魔数 | `WEAsset` / `[WEBinaryScene]`，保持原字段长度 |
| 通信 FourCC | 保留 `EZBC`、`EZID`、`EZFS`、`EZIP` 等协议值 |
| 上游与供应商信息 | 保留 URL、仓库标识、版权、许可证及供应商算法标识 |

旧归档、插件及外部项目不自动获得兼容性：旧插件需要重编译，旧归档需要转换或重新打包，旧资产缓存需要重建。本次验证只覆盖明确运行的环境与测试。

## 原 742 个审查项如何处理

最初的保守预览把 653 个不透明文件和 89 个含特殊名称的文件列为审查项。这不是 742 个确认缺陷。

- 普通图片、声音、模型、字体及工具程序：保留内容字节，仅在需要时修改文件路径。
- 22 个项目标记：验证原始内容后转换成以空字符结束的 `WEditor Project File`。
- 20 个运行配置：按 ChunkStream 格式读取，重建名称、字符串长度和数据块长度，保留数值字段。
- 2 个内置网格：读取资产头和 zstd 分包，解压后只修改 Materials 块中的资源路径，重新计算块大小、压缩并执行解压往返校验。几何数据保持不变。
- 1 个内置预制体：转换定长标记和去重字符串表，保留索引及后续组件数据。
- Base64 数据：保留完整编码段，避免改坏 glTF/Substance 内嵌载荷。
- 特殊代码名称：显式映射字符串字面量后缀、包名、测试路径及大小写扩展名。
- 第三方接入：更新 Assimp、FMOD 和 DirectXTex 的引擎接入接口，保留供应商实现与署名。

## 工具

`rebrand_engine.py` 默认只生成预览。仅当审查项为零时，`--apply` 才会写入；必须指定仓库外的新备份目录。

```powershell
python Utilities/Rebranding/rebrand_engine.py --report Workspace/rebrand-preview.json
python Utilities/Rebranding/rebrand_engine.py --apply --backup D:/WorldEngine-migration-backup
python Utilities/Rebranding/rebrand_engine.py --restore D:/WorldEngine-migration-backup
python -B -m unittest discover -s Utilities/Rebranding -p 'test_*.py' -v
```

Python 需要 3.10+。压缩网格转换还需要本机已构建的 zstd DLL；转换器会在 `Workspace/*-output/Bin/Win*Debug64/` 中查找它。转换器只支持仓库中已核对的文件版本，未知格式会失败，不会按字节盲改。

`binary_formats.py` 实现格式转换；`finalize_product_name.py` 用初始备份精确追踪曾经映射为 Engine 的产品文字，避免误改普通 Engine 概念。

备份保存原始文件和 SHA-256 清单。恢复会拒绝覆盖后续编辑；空目录不会递归清理。整个工具不调用 Git reset，也不自动提交或推送。

## 本次验证与记录

- 迁移脚本的 8 项测试通过，包括备份恢复、后续编辑保护、碰撞拦截、格式长度、字符串表、媒体保护、编码和路径边界。
- 改名后的 Windows x64 Debug 完整构建已通过。
- FoundationTest、ToolsFoundationTest、CoreTest 全套测试已通过；Archive 测试另有独立的打包/解包验证。
- 项目目录已迁移至 `D:/WorldEngine`，在该位置使用新的 `Workspace/worldengine-build` 构建目录重新验证。
- Testing Chambers 完成资源转换，Main 与 Perf-AnimGraph 场景各运行 120 帧并生成截图，进程退出码均为 0。
- 首次冷缓存转换的引擎子进程出现脚本尚未加载的错误；缓存生成后重新转换，父子日志未再出现这些错误，两个场景运行也未复现。原素材导入及着色器警告仍保留。
- 本次产生的详细构建与运行日志位于被 Git 忽略的 `Workspace/` 下；最终验证汇总记录在 `validation.json`。

本次没有验证所有平台、全部图形后端或任意外部旧项目。新的解决方案为 `Workspace/worldengine-build/WorldEngine.slnx`，产物目录为 `Workspace/worldengine-build-output/Bin/WinVs2026Debug64/`。

## 子模块

本项目修改过的内容及接入配置发布到 `Aceared0829/WorldEngine-content`、`Aceared0829/WorldEngine-thirdparty-assimp`、`Aceared0829/WorldEngine-thirdparty-fmod`。主仓库记录可从这些远端取得的提交；预编译工具子模块仍使用上游版本。
