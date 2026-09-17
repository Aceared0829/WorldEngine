# WorldEngine · 寰宇引擎

WorldEngine（寰宇引擎）是基于 [ezEngine](https://github.com/ezEngine/ezEngine) 开发的开源 C++ 游戏引擎项目。引擎代码统一使用 `W` 前缀，中文名称为 **寰宇引擎**。

本仓库保留上游引擎的模块化架构、编辑器、资源工具链和示例，并维护 WorldEngine 的命名、构建及资源格式迁移。现有功能主要继承自上游，不应将这些能力全部归因于本项目的独立开发。

## 命名

| 用途 | 名称或示例 |
| --- | --- |
| 英文名称 | WorldEngine |
| 中文名称 | 寰宇引擎 |
| C++ 类型 | `WWorld`、`WGameObject`、`WString` |
| 宏 / CMake 接口 | `W_CORE_DLL`、`W_create_target` |
| 编辑器 / 运行程序 | `WEditor.exe`、`WPlayer.exe` |
| 项目标记 / 场景 | `WProject`、`.WScene` |
| 归档标记 | `WEARCHIVE`、`WEARCHIVE-END` |

`WWorld` 中的 `W` 是引擎前缀，`World` 是类型自身的含义。普通的 Engine 概念、第三方标识符、版权署名和上游链接不做无差别替换。

## 现有组成

- C++ 基础库、反射、序列化、资源管理和模块化运行时。
- Qt 编辑器、场景与资产编辑、导入及资源转换工具。
- 渲染、动画、物理、音频、脚本等模块和可选插件。
- 引擎示例、自动化测试及独立命令行工具。

具体支持范围以源码、启用的构建选项和实际验证结果为准。上游的 [功能与使用文档](https://ezengine.net/pages/docs/docs-overview.html) 仍可作为参考，其中旧的 `ez` 命名需要对应到本仓库的 `W` 命名。

## 获取代码

```powershell
git clone --recurse-submodules https://github.com/Aceared0829/WorldEngine.git
cd WorldEngine
```

已有克隆可执行：

```powershell
git submodule update --init --recursive
```

资源内容和少量第三方构建接入使用本项目维护的子模块版本；预编译工具继续使用上游发布版本。请保留 `.gitmodules` 中的依赖关系，不要仅下载主仓库 ZIP。

## Windows 构建

推荐使用 Visual Studio 2026 的 C++ 桌面开发工具和 Windows SDK。仓库也保留其他平台配置；本次迁移以 Windows x64 Debug 为验证环境。

在仓库根目录执行：

```powershell
.\RunCMake.ps1 -Target vs2026x64 -SolutionName WorldEngine -WorkspaceDir worldengine-build
& .\Data\Tools\Precompiled\cmake\bin\cmake.exe --build Workspace/worldengine-build --config Debug -- '/m' '/p:CL_MPCount=16'
```

生成的解决方案为 `Workspace/worldengine-build/WorldEngine.slnx`，可供 Visual Studio 或支持该格式的 Rider 打开。程序输出位于 `Workspace/worldengine-build-output/Bin/WinVs2026Debug64/`。

启动编辑器：

```powershell
& .\Workspace\worldengine-build-output\Bin\WinVs2026Debug64\WEditor.exe
```

在编辑器中打开示例项目的 `WProject` 文件，例如 `Data/Samples/Testing Chambers/WProject`。

## 验证

```powershell
& .\Workspace\worldengine-build-output\Bin\WinVs2026Debug64\FoundationTest.exe -noGui
& .\Workspace\worldengine-build-output\Bin\WinVs2026Debug64\ToolsFoundationTest.exe -noGui
& .\Workspace\worldengine-build-output\Bin\WinVs2026Debug64\CoreTest.exe -noGui
```

迁移规则、二进制处理边界、脚本测试和验证记录见 [命名迁移说明](Utilities/Rebranding/README.md)。完整编译不等同于所有平台、所有插件和所有场景均已验证。

## 资源兼容

源码、反射名称、插件名称及资源扩展名已一起迁移。仓库内的项目标记、运行配置、内置压缩网格和预制体由格式感知的转换器处理；图片、音频、模型等普通媒体文件不做字节替换。

旧插件二进制需要重新编译，旧资源缓存需要重新生成。`WEARCHIVE` 与旧 `EZARCHIVE` 等长，但标识不同；旧归档不能直接按新格式加载，需要转换或重新打包。本仓库没有承诺与任意外部旧项目保持二进制兼容。

## 目录

| 目录 | 内容 |
| --- | --- |
| `Code/Engine` | 基础库与引擎核心模块 |
| `Code/Editor`、`Code/EditorPlugins` | 编辑器及编辑器插件 |
| `Code/EnginePlugins` | 运行时插件 |
| `Code/Tools` | 命令行和辅助工具 |
| `Code/UnitTests` | 自动化测试 |
| `Data` | 基础资源、示例和内容子模块 |
| `Utilities/Rebranding` | 命名与资源迁移工具 |
| `Documentation` | 文档及 API 文档配置 |

## 来源与许可

WorldEngine 基于 [ezEngine](https://github.com/ezEngine/ezEngine) 开发，原作者及贡献者的署名保留在代码和许可文件中。

引擎主仓库沿用 [MIT License](LICENSE.md)。第三方库、工具和资源各自的许可文件继续适用；重命名不改变这些文件的来源与许可声明。

## 开发目标
