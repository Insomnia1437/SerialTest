# SerialTest UI 重构建议与 Gemini 执行计划

分析日期：2026-10-31。代码基线：`908902e`（当前 `master`）。

当前状态：**仅完成静态分析和计划编写，所有实现阶段均未开始。没有在本机编译，也没有触发新的 CI。**

## 1. 建议先做什么

推荐保留 **Qt 6 Widgets + qmake + 现有通信功能**，先跑通可重复的三平台云端构建，再统一主题和尺寸，最后逐页修改布局。当前需求不需要重写整个应用，也没有证据表明 GitHub Actions 不适合构建这个项目。

这次的主要问题不是缺少新的 UI 框架，而是：

1. 桌面导航实际上是 Dock Tab，容易改错对象。
2. QDarkStyle 和后来追加的 QSS 在选择器、尺寸、字体上互相覆盖。
3. 数据、绘图、控制项页面把大量操作塞进同一横行，控件一放大就容易撑宽。
4. 缺少针对正常窗口尺寸、高 DPI、主题切换和 Windows 的验收闭环。
5. 构建环境不断变化，安装 Qt、编译代码、部署依赖和发布版本混在同一个工作流中。

先解决以上问题。通信层拆分、CMake 迁移、QML 重写属于后续独立项目。

## 2. 分析依据与确认程度

本次读取了 `src/SerialTest.pro`、主窗口、各 Tab 的 `.cpp/.h/.ui`、主题 QSS、设置初始化、WinRT 蓝牙代码、部署脚本、当前 workflow，以及仓库里已有的 `logs_new2/`～`logs_new5/`。第三方 QCustomPlot 不作为本轮修改对象。

没有启动旧应用、没有运行交互测试。下面区分代码可确认的事实和待运行验证的表现；不能据此宣称已经重现了用户遇到的所有 UI bug。

### 2.1 可以从代码确认的问题

| 位置 | 事实 | 对本轮重构的影响 |
| --- | --- | --- |
| `src/mainwindow.cpp`，构造函数、`dockInit()`、`showUpTab()` | 六个功能页先插入 `funcTab`，桌面初始化时又被放入六个 `QDockWidget`，随后隐藏 `funcTab` 和中央控件 | 可见的功能 Tab 来自 `tabifyDockWidget()`；只改 `src/ui/mainwindow.ui` 的 Tab 样式不够 |
| `MainWindow::onThemeChanged()` + 两份 QDarkStyle QSS | 新规则只有 `QTabBar::tab` 等通用选择器；旧规则含 `QTabBar::tab:top`、`:top:selected`、`:!selected:hover` 等更具体的选择器 | 新写的 padding、min-width、颜色可能被旧规则压过，不能假定“追加在后面就生效” |
| `MainWindow::onThemeChanged()` | `(none)` 先还原启动 QSS，随后仍进入浅色覆盖分支 | “None” 实际仍被套上应用的浅色外观，应明确恢复其语义 |
| 同上 | Tab、输入框、按钮写死 `font-size: 12px`，而设置页允许修改应用字体 | 样式优先于字体 API，造成部分控件不随用户字号设置变化 |
| 同上 + QDarkStyle | 输入框主体增加 padding、圆角，但下拉按钮和 SpinBox 按钮仍沿用旧的 12px 子控件尺寸 | 主体与箭头点击区没有一起设计，存在比例不协调的直接原因 |
| `src/ui/datatab.ui` | 接收区工具全在一行；底部四个参数组全在一行 | 放大按钮后可用宽度更紧张，需要分行，而不是继续压缩字体 |
| `src/ui/plottab.ui` | 绘图基础操作和高级参数各自集中在长横行；部分控件允许最低高度为 1 | 需要重新组织操作行并清理本页不合理的尺寸提示 |
| `src/ui/controlitem.ui` | 主控件、全部配置字段和配置按钮挤在同一横向结构 | 展开配置时特别容易撑宽；适合把配置移到同一个条目的第二行 |
| `src/ui/devicetab.ui` | 设备列表与参数栏左右并列，参数 `groupBox` 使用 `Maximum` 水平策略；串口参数没有滚动容器，BLE/网络参数已有 | 调整输入控件尺寸时，要同时调整参数栏策略和页面可用高度 |
| `src/adaptivestackedwidget.cpp` | `removeWidget()` 重复移除横向 policy，未移除纵向 policy；切页时对子页和自身连续 `adjustSize()` | 前者是明确笔误；后者与父布局共同控制尺寸，存在切页尺寸抖动风险 |
| `src/ui/datatab.ui`、`devicetab.ui`、`controlitem.ui` | 存在 Splitter 渐变、输入框透明背景等局部内联样式 | 局部 QSS 优先级会妨碍集中主题，需要在对应页面阶段处理 |
| `.github/workflows/release.yml` | Windows/macOS 使用 `*-latest`；直接安装 aqt 的 PR head；macOS 用系统 pip 并带失败重试分支 | 环境和安装器版本不可重复，历史修复不断叠加 |
| 同上 | PowerShell 把空格分隔的 `$env:QT_MODULES` 作为一个参数传给 `--modules` | 应显式传三个模块参数，不能照搬 Bash 的参数展开写法 |
| 同上、`pack/find_dlls.py` | MSVC 部署后额外执行依赖 `objdump`、`cp` 的扫描脚本 | 应让 MSVC 的主部署流程以 `windeployqt` 为准，不为 UI 重构再引入另一套部署工具 |
| 同上 | Linux 包只复制应用程序和文档，没有随包收集 Qt 运行库和平台插件 | 这是依赖外部 Qt 环境的构建产物，不能当成任意 Linux 机器解压即用的便携包 |
| `src/SerialTest.pro`、`README.md` | 工程最低 macOS 是 26.0，README 仍写 macOS 10.13 / Windows 7 等旧信息 | 产品兼容范围必须明确；不得用旧 README 承诺当前二进制能运行 |

Qt 的选择器优先级和字体优先关系见 [Qt 样式表语法](https://doc.qt.io/qt-6.11/stylesheet-syntax.html)。复杂控件需要同时考虑主体和子控件，见 [Qt 样式表定制说明](https://doc.qt.io/qt-6/stylesheet-customizing.html)。上表关于具体显示症状的判断仍需截图验证。

### 2.2 旧 CI 日志说明了什么

- `logs_new4/2_build-windows.txt`：Windows 已进入编译，出错文件是 `winrtbluetooth.cpp`；MSVC 的 `<experimental/coroutine>` 报 `STL1011/C2338`。日志中的编译器来自 Visual Studio 18，而 Qt kit 是 `msvc2022_64`。这说明至少存在工具链匹配问题，不能概括成“Windows 完全编译不了”。
- `logs_new5/3_build-windows.txt` 和 macOS 日志：安装 aqt 时出现 `Cannot import 'setuptools.build_meta'`，还未进入应用编译。
- `logs_new5/1_build-linux.txt`：Qt 安装阶段找不到指定模块，也没有进入应用编译。
- 当前 workflow 比上述日志又有变化，已加协程抑制宏并改为直接 aqt 安装。**旧日志不能证明当前 HEAD 仍会以同样方式失败，也不能证明最新修改已成功。**

本机未找到 `gh`；此次没有查询到当前 HEAD 的在线运行结果。Gemini 必须以新提交对应的 CI 为准，不要把历史日志修复建议当成已验证补丁。

### 2.3 暂不纳入本轮的代码问题

`MainWindow` 同时管理通信、收发缓存、主题和 Dock；`DeviceTab`、`Connection` 较大；部分数据路径调用 `processEvents()`。这些值得后续整理，但不能据此在 UI 阶段拆线程、重写协议、改缓存或增加抽象层。先把外观与布局做稳定，再单独评估有实测证据的性能问题。

## 3. GitHub Actions 是否够用

**够用，建议继续使用。** 用官方预编译 Qt kit 构建应用即可，不需要在 Actions 上从源码构建 Qt。qmake 仍有 [Qt 6.11 官方文档](https://doc.qt.io/qt-6.11/qmake-manual.html)，本项目无需为了 CI 改成 CMake。

GitHub 提供 Windows、macOS、Linux 的标准 runner；公开仓库的标准 runner 计算免费，私有仓库计算和存储有各自额度，不能把大规格 runner 或所有存储都视为免费。见 [runner 规格](https://docs.github.com/en/actions/reference/runners/github-hosted-runners) 和 [Actions 计费](https://docs.github.com/en/billing/concepts/product-billing/github-actions)。这里没有假定这个 fork 当前一定公开。

需要接受的限制：

| 工作 | 在 Actions 上怎么做 | 仍需怎样验收 |
| --- | --- | --- |
| 三平台编译、Qt 依赖部署 | 使用各平台 runner 和匹配的 Qt kit | 查看本次提交的三个 job 和产物 |
| 控件尺寸、切页、主题、基础布局 | Linux 虚拟显示中跑小型 Qt 测试入口并保存截图 | 检查截图，确认内容可见、对齐、配色 |
| Windows/macOS 原生表现、系统 DPI、浮动窗口 | 构建对应原生产物；后续可扩展原生截图 | 下载已编译应用运行即可，**不要求在本机编译** |
| 串口、蓝牙硬件真实通信 | 云端 runner 不提供用户的真实设备，不能用“无设备时不崩溃”冒充验证 | 用 CI 产物配合实体设备手动测试；或另建远程实体测试机 |
| macOS 签名/公证、Windows 发布签名 | 可以另设发布流程，但需要证书/账号等配置 | 属于发布任务；本轮先产出未签名测试包，不代替用户做该决定 |

最低成本路线：**Actions 编译 + Actions 截图 + 用户运行下载产物验收**。只有确实需要无人值守硬件测试时，再考虑非本机的 self-hosted runner；不要默认把用户电脑变成 runner。

## 4. 推荐路线与可选决定

以下阶段按 A 路线编写。用户说“按推荐路线执行”即可采用这些默认值；Gemini 不得自行切换到可选方案。

| 决定 | A：本计划采用 | B：可选，用户选择后另改计划 |
| --- | --- | --- |
| 导航架构 | 保留六个 Dock、浮动/回停靠能力、现有页索引，修复真实 Dock Tab 和布局 | 改回普通中央 `QTabWidget`，取消浮动/拆分停靠。更容易维护，但会移除现有功能；需先确认是否还要多页并排 |
| 构建系统 | qmake、C++17、现有 Qt 模块不变 | 单独迁移 CMake。对多目标测试组织有帮助，但不直接改善 UI，本轮不推荐混做 |
| Qt 版本 | 首轮固定现有 `6.11.1`，先修环境，不以追新代替诊断 | 安装器问题确实无法解决时，用户决定改用另一个明确补丁版本；先验证三平台 kit/模块可用性，不能随意升降版本 |
| macOS 最低版本 | 先保留工程里的 `26.0`，用 `macos-26` runner 建立基线 | 若需要更广用户群，可考虑 `13.0`，但必须另做旧系统运行验收；Qt 6.11 的平台下限不是应用已验证的兼容保证 |
| macOS 架构 | 首轮验证 arm64，产物名称明确写 arm64，编译参数显式指定 | 需要 Intel 时增加独立 x86_64 job/产物，再决定是否制作 universal；不能把当前包笼统宣传为全架构 |
| 外观实现 | 保留现有 QDarkStyle 资源作基础，在单一入口加载应用自己的配色/尺寸 QSS | 放弃 QDarkStyle 自写完整主题，或迁移 QML。前者需要完整重做子控件，后者还要迁移绘图与平台交互，本轮不推荐 |
| Linux 交付 | Ubuntu 24.04 环境的依赖型测试包，明确标注需要 Qt 运行环境 | 若需要发给普通 Linux 用户，再独立选择 AppImage/Flatpak；本轮不加入打包依赖 |

Qt 6.11 支持的 Windows/MSVC、macOS 下限见 [官方平台表](https://doc.qt.io/qt-6.11/supported-platforms.html)；runner 标签和架构见 [GitHub runner images](https://github.com/actions/runner-images)。版本与 runner 信息在执行前应复核，不能把 `*-latest` 当成固定工具链。

选择 B 导航方案时，需要重写下面阶段 4 和相应验收；不得仅把 `dockInit()` 删掉后继续套用 A 方案。旧的 `WindowState` 也不能再拿来恢复普通 Tab 布局，但串口历史、主题和其他偏好仍应保留。没有用户选择，不进行这项删改。

## 5. 全阶段共同要求

### 5.1 保留的功能与标识

- 页索引固定：0 Connect、1 Data、2 Plot、3 Control、4 File、5 Settings。
- 主题 ID 固定：`(none)`、`qdss_dark`、`qdss_light`；保留原有配置键、设置文件及导入导出格式。
- 保留控件 `objectName`、自动槽名和既有 signal/slot 连接；移动控件时移动原控件，不重新创建一套同名控件。
- 不改变串口/蓝牙/TCP/UDP 连接逻辑、收发内容、编码、文件传输和绘图解码。
- 不删除旧校验和错误提示；不新增无关的兜底、检查、日志或重试。
- Android 不在本轮构建支持范围；保留已有条件代码和文件，不顺手移植或删除。
- 不改 `src/qcustomplot.cpp/.h`、`demo/`、`doc/`、`fastlane/`、`pack/aur/`。

### 5.2 UI 的具体目标

以下统一配色/控件尺寸针对 Light/Dark；None 明确保留系统原生外观和尺寸，只验收无主题残留、功能与布局正常。为使新用户直接看到统一 UI，阶段 3 将无历史配置时的主题默认值改为 Light，已有用户明确保存的 None 仍保留。

数值均为 Qt 逻辑尺寸；以默认字体下的实际控件外框为验收依据，不能把 QSS 的内容区高度直接当成最终高度。

| 项目 | 目标 |
| --- | --- |
| 主功能 Tab | 默认字体下整体高度约 40～44，左右内容留白约 16～20；中文/英文都清晰，选中状态明显 |
| 单行输入、下拉、SpinBox、普通按钮 | 默认字体下外框约 32～36 高；统一圆角约 5～6；同一操作行垂直对齐 |
| ComboBox 下拉区、SpinBox 按钮列 | 保留约 24 的独立点击宽度；文本右侧给按钮留空间；箭头居中且两主题均可见 |
| 字体 | 不新增全局硬编码字体家族/像素字号；沿用系统字体和已有字号设置；10/12/14 pt 时可自然增高 |
| 间距 | 页外边距约 12，组间约 12，同行约 8，组内约 8；布局层负责，不在每个控件上叠 margin |
| 页面 | 默认窗口 1200×760 左右；最小正常验收尺寸 1000×700；最大化后编辑区/表格/曲线利用剩余空间 |
| 状态 | normal、hover、focus、disabled、selected/checked 外观完整；hover/focus 不能改变边框总厚度而导致跳动 |
| 数据区 | 收发文本和曲线优先获得空间，避免把编辑区包在整页滚动容器中 |
| 高 DPI | Windows 100%/150%/200%、macOS Retina；不手动再乘 `devicePixelRatio()` |

Qt 的 Widgets 几何尺寸通常按设备无关坐标工作，见 [Qt High DPI](https://doc.qt.io/qt-6.11/highdpi.html)。上述 DPI、字号、中英文、正常窗口缩放属于本任务范围，不是要忽略的罕见边界情况。无需为极小窗口、损坏配置或任意恶意输入扩展设计。

### 5.3 提交与门禁

实现时使用 `codex/ui-refactor` 或用户指定的开发分支，不直接推 `master`。每个阶段独立提交、推送并验证本次提交 SHA 的 CI；5A～5D 也分别提交。只暂存本阶段改动，不暂存已有的日志压缩包/目录。

每个阶段必须同时满足：允许的文件范围内完成任务、三平台必需构建全绿、该阶段测试通过。有 UI 变化的阶段还需提供截图并由用户确认；待确认时停止进入下一阶段。没有 `gh` 时提供提交 SHA 和 Actions 页面，请用户贴本次日志，不进行本机编译。

所有阶段均可更新本文件的进度记录；除此以外，严格遵守各阶段文件白名单。CI 报错若必须改白名单外源码，先提交错误位置、直接原因和拟修改文件，请用户决定，不自行扩展任务。

| 阶段 | 状态 | 完成时填写 |
| --- | --- | --- |
| 1 固定 CI 与构建基线 | 已完成 | SHA: 7e70728 / Run: 41184917133 / Windows x64, macOS arm64, Ubuntu 24.04 x86_64 产物上传成功 |
| 2 建立 UI 截图入口 | 已完成 | SHA: 4036a5b / Run: 41185669323 / 基线截图 24 张已生成（待用户视觉验收） |
| 3 整理主题加载 | 已完成 | SHA: a83fe07 / Run: 41186437261 / 主题集中到 Theme::styleSheet，循环切主题测试通过，None修复 |
| 4 统一控件与 Dock 导航 | 已完成 | SHA: 4e8a283 / Run: 41187184906 / 控件与Dock外观统一，42张截图生成（待用户视觉验收） |
| 5A 连接页与堆叠容器 | 已完成 | SHA: 76c838a / Run: 41188948212 / 连接页布局整理、堆叠尺寸与Splitter handle统一、协议切换测试通过，49张截图已生成（待用户视觉验收） |
| 5B 数据页 | 已完成 | SHA: 8b137f1 / Run: 41190412883 / 数据页双行工具条、2:1伸缩分隔条与2x2网格参数区整理完成，填充数据与交互测试通过，49张截图已生成（待用户视觉验收） |
| 5C 绘图页 | 未开始 | SHA / run / 截图验收 |
| 5D 控制、文件、设置页 | 未开始 | SHA / run / 截图验收 |
| 6 打包与跨平台验收 | 未开始 | SHA / run / 最终验收记录 |

## 6. Gemini 的逐阶段任务

### 阶段 1：固定 CI 与构建基线

**目的：先回答当前代码能否在固定的三平台环境中编译和部署。暂不修改 UI。**

文件白名单：

- 新增 `.github/workflows/build.yml`。
- 修改 `.github/workflows/release.yml` 的触发条件；此阶段不重写发布逻辑。
- `src/SerialTest.pro` 仅允许调整显式目标架构；保留 Qt 模块、C++17、macOS 26.0。

按顺序执行：

1. 创建开发分支。新增名为 `Build` 的普通工作流，支持分支 push、针对 master 的 PR、无发布参数的 `workflow_dispatch`。push 只匹配分支，不匹配 Tag。设置 `contents: read`，不包含发布 job。
2. 把原 `release.yml` 的分支 push/PR 触发移除，保留原有 Tag 和手动发布入口，避免每次开发提交重复跑两套三平台构建。不要触发它。
3. 新 Build 保留三个独立 job，固定 `windows-2022`（x64 + VS2022）、`macos-26`（arm64）、`ubuntu-24.04`（x86_64）。不要用 `windows-latest` 或 `macos-latest`，不要改回已淘汰 runner。
4. 使用 `actions/setup-python` 的明确主版本和 `python-version: '3.12'`；之后一律使用这个 Python 的 `python -m pip` / `python -m aqt`。确保该环境安装 `setuptools`、`wheel`，不再使用系统 pip 的 `--break-system-packages` 或双分支重试。
5. Qt 固定 `6.11.1`。aqt 暂固定到包含 Qt 6.11 Windows 修复的上游合并提交 `8c3695d4a4e1ceabf6a74dc6c79681656dc6b74b`，不要使用浮动 PR head 或 master。命令为 `python -m pip install "git+https://github.com/miurahr/aqtinstall.git@8c3695d4a4e1ceabf6a74dc6c79681656dc6b74b"`。这是当前安装方式的固定版本替换，不是新增运行时依赖。此提交是否能在本项目三平台安装成功，仍必须由本阶段 CI 验证。
6. 在 CI 安装步骤输出 Python、aqt 版本，并用 `list-qt` 确认对应 kit 和模块，然后安装。Windows 架构 `win64_msvc2022_64`、macOS `clang_64`、Linux `gcc_64`；三个附加模块是 `qt5compat qtserialport qtconnectivity`。**PowerShell 直接写三个独立参数，或传字符串数组，不能把含空格的 env 字符串当作一个模块。** 不安装全部 Qt 模块，不加不存在的 `qt_base` 模块名。
7. Windows 的 `msvc-dev-cmd` 指定 VS2022 和 x64；确认日志里的编译器来自 VS17/2022。保留现有协程抑制宏作为已知历史问题的临时配置，不升级 C++ 标准、不改 WinRT 源码。如果固定 VS 后仍失败，只按新日志修复；需要改源码时先报告范围。
8. 三平台全部改为源码外构建，目录如 `build/ci`。不复用用户机器已有的 `build/`，不上传对象文件。显式 Release，使用现有源码版 QCustomPlot，不另外下载/替换它。
9. Windows 用同一 Qt kit 的 `windeployqt --release` 部署，再压 zip；新 Build 不调用 `pack/find_dlls.py`，也不删除这个脚本。macOS 用同一 Qt kit 的 `macdeployqt` 产出 DMG。Linux 仍产依赖型 tar 包，名字写明 Ubuntu 24.04/x86_64。
10. 三个平台均上传 artifact；设置 7 天保留期，缺失预期文件直接失败。上传包中包含现有两份 LICENSE 和 README，不吞掉文件不存在错误。
11. 日志记录 `qmake -v`、实际编译器、CPU 架构、部署工具版本；安装成功后不再为无关 warning 修改源码。先不加编译缓存、Qt 源码构建或多 Qt 版本矩阵。

安装器修复依据：[aqt PR #1000](https://github.com/miurahr/aqtinstall/pull/1000)、[固定合并提交](https://github.com/miurahr/aqtinstall/commit/8c3695d4a4e1ceabf6a74dc6c79681656dc6b74b)。模块和架构查询语法见 [aqt CLI](https://aqtinstall.readthedocs.io/en/latest/cli.html)。不要未经验证就把固定提交替换为 PyPI 的旧版。

下面是 **CI runner 内**的构建命令形状，不在用户本机执行；部署路径按实际生成位置填写：

```text
Windows（cmd，MSVC shell 已初始化）：
  mkdir build\ci
  cd build\ci
  qmake ..\..\src\SerialTest.pro CONFIG+=release CONFIG-=debug_and_release "DEFINES+=_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS"
  nmake /nologo

macOS（runner 为 macos-26）：
  mkdir -p build/ci
  cd build/ci
  qmake ../../src/SerialTest.pro CONFIG+=release CONFIG+=app_bundle QMAKE_APPLE_DEVICE_ARCHS=arm64
  make -j2

Linux：
  mkdir -p build/ci
  cd build/ci
  qmake ../../src/SerialTest.pro CONFIG+=release
  make -j2
```

验收：三个 job 的 Qt 安装、编译、部署/归档、上传均成功；产物名称区分 Windows x64、macOS arm64、Ubuntu 24.04 x86_64；记录 SHA 与 run 链接。这一阶段通过不等于界面或硬件功能通过。

提交名：`阶段1：固定三平台 CI 构建基线`。

若固定安装器仍不能获取 kit/模块，保留失败日志停在本阶段，给用户两个选择：固定到另一个经查询可用的 aqt 提交，或改用另一个明确 Qt 补丁版本。不要连续猜版本、改操作系统、改应用源码。

### 阶段 2：建立 UI 截图与正常路径测试入口

**目的：先留下可比较的旧 UI 截图，后续每阶段复用这个入口。**

文件白名单：新增 `tests/ui_smoke.cpp`，修改 `src/SerialTest.pro` 和 `.github/workflows/build.yml`。此阶段不修改生产 C++ 和 `.ui`。

具体任务：

1. 在 `.pro` 增加仅由 `CONFIG+=ui_smoke` 启用的测试构建分支：移除 `main.cpp`，改用 `$$PWD/../tests/ui_smoke.cpp`，仅该分支增加 `QT += testlib`。其余源码、FORMS、RESOURCES 复用现有清单，正常应用构建行为不变。不为测试拆出公共库或迁移构建系统。
2. 测试入口创建 `QApplication`，用临时目录里的 INI 初始化 `MySettings`，然后创建真实 `MainWindow`。不读取用户的 `preference.ini`；不改生产设置路径。每个语言/缩放组合是独立进程，不重复创建 QApplication。
3. 用已有 `showUpTab(0..5)` 切页并保存真实窗口截图；测试应等待 Qt 事件/布局处理完成。不要只截图隐藏的 `funcTab`，也不要画一张模拟图冒充真实界面。
4. 使用 Qt Test 做少量有意义的正常路径检查：六页可切换；一个 Dock 能浮动并用 `dockBackButton` 回停靠；Data/Plot 的主要内容区可见。可通过现有 `objectName` 找控件，测试中查找失败就用 `QVERIFY` 报错，不增加生产空指针检查。
5. Linux job 在普通构建之后另建 `build/ui-smoke`。CI 安装 `xvfb`、`fonts-noto-cjk` 和 Qt xcb 平台插件所需的系统库，使用 `xvfb-run` 运行测试；这些只是 runner 测试依赖，不是应用新增依赖。缺库按 CI 日志补齐，不从源码构建 Qt。
6. 初始截图：1200×760、英文、三种主题、六页。添加一组简体中文/浅色截图。主题通过现有设置 ID 驱动，图表主题也应收到原有 `themeChanged` 信号。语言加载复用已有 `.qm` 资源，不改翻译文件。
7. 上传 `ui-screenshots`、Qt Test 结果和必要测试日志，测试失败时也上传已有截图。失败仍使 job 失败；不要 `continue-on-error`。基线截图可以有丑陋或拥挤，但测试不应把已知外观缺陷直接写成永久通过标准。

本计划只授权测试目标使用已有 Qt kit 的 `testlib`，不新增 Python GUI 库、截图服务或第三方测试框架。测试不连接实体串口/蓝牙，不发送数据，不扫描真实设备作为通过条件。Linux 无蓝牙适配器是 CI 的正常测试环境，不为它改生产设备探测逻辑。

验收：三个应用构建全绿，Linux 测试成功，截图能看清六个真实页面，用户查看基线截图后继续。该入口不能代替 Windows/macOS 的原生 DPI 验收。

提交名：`阶段2：增加云端 UI 截图与冒烟检查`。

### 阶段 3：把主题加载集中到一个入口

**目的：消除在主窗口中堆 QSS 的做法，恢复 None，并保证反复切主题没有残留。先不重新布局。**

文件白名单：`src/mainwindow.cpp`、`src/SerialTest.pro`、`src/settingstab.cpp`（仅 `Theme_Name` 的缺省值）；新增 `src/theme.h`、`src/theme.cpp`、`src/styles/styles.qrc`、`src/styles/common.qss`、`src/styles/light.qss`、`src/styles/dark.qss`；更新 `tests/ui_smoke.cpp`。

具体任务：

1. 新增简单的主题辅助函数，例如 `QString Theme::styleSheet(const QString &themeName, const QString &startupStyleSheet)`。它只组装并返回 QSS，不管理设置、不创建控件、不做插件化主题系统。
2. `MainWindow::onThemeChanged()` 保留为现有信号接收入口，调用这个函数后 `qApp->setStyleSheet()`；保留现有 `m_appDefaultQss` 的启动快照用途。
3. `(none)` 直接返回启动 QSS，不追加任何应用浅色/深色资源；不改系统 QStyle。`qdss_light` / `qdss_dark` 每次从对应原始 QDarkStyle 资源重新读取，再组合应用 QSS，绝不以当前 `qApp->styleSheet()` 作主题基底。
4. 把现有内嵌 QSS 移入上述资源。颜色差异分别放在 light/dark；共享的尺寸、边距和通用状态结构放在 common。组合顺序固定为“原始 QDarkStyle → 应用配色 → 应用通用尺寸”，不同层不重复声明同一类职责。暂时保持旧显示效果，阶段 4 再正式重定尺寸/选择器。
5. 将新 `.cpp/.h/.qrc` 加入 `.pro`。资源读取只写正常路径：`open()` 后 `readAll()`；不添加失败检查、警告、默认主题兜底或 try/catch。
6. 不编辑两份第三方 QDarkStyle QSS 和图片。保留主题 ID，保留 `SettingsTab` 向 `PlotTab` 发送主题信号的现有连接。
7. 测试同一进程 Light → Dark → None → Light。检查最终 Light 与首次 Light 的 QSS 一致，None 与启动快照一致；各次保存截图。不要增加“清空用户设置以修复主题”的操作。
8. `SettingsTab::loadPreference()` 中仅把 `Theme_Name` 缺省值从 `(none)` 改为 `qdss_light`；保留原配置键和已保存值，不新增配置迁移/兼容分支，不改其他默认设置。

验收：全平台构建和已有测试通过；None 不再强制浅色，主题往返不累计规则；全新配置默认 Light，已有主题偏好不变；没有新的全局字体规则；本阶段不重新布局页面。

提交名：`阶段3：集中主题加载并修复 None`。

### 阶段 4：统一控件尺寸与真实 Dock 导航

**目的：解决用户直接指出的 Tab 太小、输入/选择框难看，同时保持浮动功能。**

文件白名单：阶段 3 新增的三份应用 `.qss`；`src/mainwindow.cpp`、`src/ui/mainwindow.ui`、`tests/ui_smoke.cpp`；`.github/workflows/build.yml` 仅允许增加本节的截图运行组合。不改其他页布局。

具体任务：

1. 移除应用覆盖层中对 Tab、按钮、输入框写死的像素字号，保留已有 QWidget/QPlainTextEdit 字体设置机制。不要修改旧第三方 QSS，不添加 Qt QSS 不支持的 CSS `inherit`/变量语法，也不在每个控件上 `setFont()` 修补。
2. 按第 5.2 节的实际外框尺寸重写应用控件规则。输入框、ComboBox、SpinBox、按钮共享尺寸原则；可编辑 ComboBox 和 SpinBox 内部的 `QLineEdit` 不再重复画外框或叠加 padding。
3. 同时设计 ComboBox 的 `::drop-down`、`::down-arrow`、弹出列表、editable 状态；SpinBox 的 up/down 按钮和箭头；保留旧资源中的箭头图，不新引入图标库。明确选中、禁用、焦点状态的配色。
4. Tab 规则作用于真实主窗口下的 `QTabBar`，例如用 `QMainWindow#MainWindow QTabBar::tab:top` 及其 selected/hover 状态；不要仅写 `QTabWidget#funcTab`，不要依赖 Qt 私有控件名称。应用规则必须压过旧 QSS 的方向/状态规则，并固定各状态的总边框厚度和 padding。
5. 在 `dockInit()` 对所有 Dock 区域统一 `setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North)`，而不是只设置 Bottom 区域。保留六个 Dock 及其现有 `objectName`、`saveState()/restoreState()` 和索引，不创建另一条自绘导航栏来同步索引。
6. 保留现有自定义浮动标题栏与 Dock Back。停靠时不显示多余标题条，浮动时按钮可见、尺寸正常；不重写系统标题栏，不增加一个新的窗口管理器。
7. 主窗口默认设计尺寸调整到约 1200×760，保留可缩放。检查状态栏：对 `connArgsLabel` 设可缩短的水平策略、保留完整 tooltip，使 Rx/Tx 和连接按钮在 1000 宽下仍可操作；移除 `stateButton->setMinimumHeight(1)` 这一旧压缩设置。不要为状态栏再引入点击展开面板。
8. 应用配色以现有蓝色强调为基础，深色使用深灰背景、浅色使用浅灰背景和白色输入区。同一主题内正文、边框、禁用态一致，不给所有按钮都加粗或都涂成主操作色。
9. 更新截图测试，加入 1000×700、12/14 pt、`QT_SCALE_FACTOR=1.5` 和 `2` 的代表性组合。缩放由独立进程设置，只在 CI 测试中用环境变量，不修改生产 DPI 策略。

本阶段页面横排可能仍显拥挤；先记录截图中的具体页面问题，阶段 5 按页修。不能为了让截图勉强放下而把字号和 Tab 再缩回去。

验收：Light/Dark 中真实 Dock Tab 外框约 40～44，输入控件约 32～36；焦点/hover 不跳动；下拉与增减箭头可见可点；字体设置对目标控件生效；None 恢复原生外观；六页导航、浮动/回停靠、关闭后 Dock 布局恢复正常；三主题、中英文截图经用户确认。

提交名：`阶段4：统一控件尺寸与 Dock 导航外观`。

### 阶段 5A：连接页与堆叠页面尺寸

文件白名单：`src/ui/devicetab.ui`、`src/devicetab.cpp` 的 `initUI()` 布局设置、`src/adaptivestackedwidget.cpp/.h`、应用三份 `.qss`、`tests/ui_smoke.cpp`。

具体任务：

1. 保留左设备列表、右参数表单的结构，不拆分 `DeviceTab`，不改设备发现和连接逻辑。左侧获得主要伸缩空间；右侧参数组由 `Maximum` 改为 `Preferred`，正常宽度约 320～380，可随内容/字体调整，不设置固定宽度。
2. 在原 `SPArgsPage` 内加入 `QScrollArea`，把原串口参数布局移到内容控件中，设 `widgetResizable=true`；保留 `SPArgsPage` 以及所有串口输入控件的名字，不改变 `argsStack` 页索引。BLE 和网络沿用已有滚动区域。
3. 表单标签列与输入列对齐，输入列获得伸缩。串口的端口名、波特率可编辑属性、选项 userData、现有 validator 均不改变。底部 Open/Close 保持在参数滚动区之外，始终可操作。
4. 只修改已有表格列宽策略：名称/地址等主要文本列可伸缩，短数值/操作列按内容；保留用户调整列宽和表格滚动。不重写 QTableWidget 为 model/view，不改列表数据填充代码。
5. 移除 `BLECentralListSplitter` 的内联渐变样式，应用主题里统一 Splitter handle，保留双击恢复分配比例的事件逻辑。
6. `AdaptiveStackedWidget::removeWidget()` 的第二次横向 map 移除改为纵向 map 移除。`setCurrentWidgetPolicy()` 保留既有 policy 恢复逻辑，把对子页/自身的 `adjustSize()` 改为相应 `updateGeometry()`，尺寸交给父 Layout；不重写整个容器，不添加 null 检查、resize 定时器或缓存 sizeHint 系统。
7. 正常切换串口、TCP Client、TCP Server、UDP，确认两侧页面对应正确。BLE 在无设备 CI 中只用测试入口切换已有参数/列表页进行布局截图，明确标注“仅布局预览”，不伪造设备连接、不启用禁用功能。

验收：1000×700 和 1200×760 下参数可滚动、Open/Close 可见；协议切换不会强制改变主窗口尺寸；长设备名/地址可通过正常表格/下拉交互查看；文件页在切换发送/接收/说明时也没有因公共容器修改而回归。已有业务功能保持。

提交名：`阶段5A：整理连接页布局与堆叠尺寸`。

### 阶段 5B：数据页

文件白名单：`src/ui/datatab.ui`、`src/datatab.cpp` 的构造函数布局设置、应用三份 `.qss`、`tests/ui_smoke.cpp`。

目标结构：

```text
接收区：第一行 标题 + Hex/Latest/Timestamp/Realtime
        第二行 Update / Export / Copy / Clear，靠右排列
        接收文本，伸缩
---------------- 可拖动分隔条 ----------------
发送历史区：选项行 + 操作行 + 发送历史文本，伸缩
发送输入框（伸缩）                         Send
参数区：第1行 Suffix                 Repeat
        第2行 Flow control           Encoding
```

具体任务：

1. 移动原控件到上述布局，保留按钮、勾选项、标签和对象名。接收/发送历史仍在原 `dataTabSplitter` 中；默认比例约 2:1，用 stretch/初始 sizes 实现，用户拖动后不在每次 resize 时重置。
2. 底部四个已有 GroupBox 改成两行两列网格，禁止继续塞在同一行，也不引入自动响应式布局框架。
3. `sendEdit` 获得水平 stretch，Send 按钮按统一尺寸自然显示。参数区不抢占文本区的纵向伸缩。
4. 移除 `dataTabSplitter` 内联渐变，复用阶段 5A 的主题 handle；保留原双击恢复比例事件。
5. 保留编辑器字体设置，不给 QPlainTextEdit 套单行输入框的高度。保持发送回车、重复发送、Hex/文本切换、复制/清空和导出槽逻辑。
6. 用测试数据调用现有显示接口填充收发区；截图里有正常文本和十六进制内容。测试中不调用真实硬件发送；不修改缓存、时间戳或解码代码来配合截图。

验收：1000×700 下工具按钮、发送框和四组参数可访问；1200×760 和最大化时文本区占主要面积；复制/清空、文本/Hex 显示与切页仍工作；中英文及 14 pt 下操作行不重叠。

提交名：`阶段5B：整理数据页工具行与参数区`。

### 阶段 5C：绘图页

文件白名单：`src/ui/plottab.ui`、`src/plottab.cpp` 的构造函数布局设置、应用三份 `.qss`、`tests/ui_smoke.cpp`。

具体任务：

1. 曲线区保持顶端主要区域并获得纵向 stretch。
2. 基础操作拆成两行：第一行 Enabled、Latest、Clear、X 类型、数据数量；第二行 Legend、Fit X、Fit Y、Advanced、Tracer。移动原有对应控件与标签，不改其对象名和槽。
3. `plot_advancedWidget` 内从长横排改成标签/输入配对的两列网格：Frame Separator / Data Separator 一行，Clear Flag / Plot Style 一行。开关只控制原有高级区域可见性，不重建控件。
4. 清理本页几个高度为 1 的旧尺寸属性，让控件按统一 style 的 sizeHint 分配；不对曲线设置固定高度，不设置整个页面固定尺寸。
5. 不修改 `MyCustomPlot`、第三方 QCustomPlot、曲线缓存、解析正则或重绘频率。图表颜色继续通过已有 `PlotTab::onThemeChanged()` 工作。
6. 测试入口经现有 `newData()`/解码器接口提供固定的正常数值帧，截图能看到曲线；打开高级区检查其布局。测试用样例格式匹配现有解码方式，不为它另造协议。

验收：1000×700 下基础按钮与打开的高级区都可操作；最大化后曲线区变大；Fit X/Fit Y、图例、高级区开关和主题切换正常；不改变绘图数据含义。

提交名：`阶段5C：整理绘图页操作与高级参数`。

### 阶段 5D：控制、文件、设置页

文件白名单：`src/ui/ctrltab.ui`、`src/ui/controlitem.ui`、`src/ui/filetab.ui`、`src/ui/settingstab.ui`；对应 `src/ctrltab.cpp`、`src/controlitem.cpp`、`src/filetab.cpp`、`src/settingstab.cpp` **仅布局初始化、显示和字体应用相关部分**；应用三份 `.qss`、`tests/ui_smoke.cpp`。不改序列化、文件传输或线程代码。

具体任务：

1. 控制页顶端拆两行：添加四种控制项一行，导入/导出/清空一行。原列表滚动区域继续占剩余空间，不新建 item model。
2. 单个 `ControlItem` 改为两层：第一行原 `mainGrp` + 右侧 `confButton`；第二行原 `confGrp`。配置区用网格分组为数值范围、前缀、后缀等行，不再把全部字段和主控件横向并列。保留 `confGrp` 显隐逻辑和所有字段名、保存格式、发送行为。
3. 将 `nameEdit` 的局部透明背景样式移入应用主题的同名选择器；提供可辨认的 focus 状态。不要继续加新的控件内联 QSS。
4. 文件页保持“发送/接收模式与开始按钮 → 协议/参数 → 路径/浏览 → 大小/校验 → 进度/清空 → 日志”。路径框水平伸缩，日志区纵向伸缩；限速项与等待项分行，说明页沿用原堆叠容器。不重新启用原来未实现而隐藏的选项。
5. 设置页沿用现有滚动区域。主题、界面字体、数据字体三组按标签列/输入列/应用按钮列对齐；字体选择列获得 stretch；配置路径标签可换行或按布局显示，不撑大整页。保留原字体设置和配置写入逻辑，不改默认设置值。
6. 测试添加 Command/Slider/CheckBox/SpinBox 各一项，展开一项配置；检查调整数值/勾选/前后缀配置后原序列化往返不变。文件页只切发送/接收/说明，设置页只改主题/字体；不对真实配置执行清空、覆盖导入，不对真实连接发文件。

验收：控制项展开后列表不被强制撑宽，1000×700 下主要操作可访问；文件日志/进度清晰，模式切换不改窗口尺寸；设置页字体 10/12/14 pt 生效，浅色/深色/None 往返正常；原有控制项配置导入导出格式保留。

提交名：`阶段5D：整理控制文件与设置页布局`。

### 阶段 6：打包、翻译与跨平台最终验收

文件白名单：`.github/workflows/build.yml`、`.github/workflows/release.yml`、`tests/ui_smoke.cpp`、`README.md`、`src/i18n/SerialTest_zh_CN.ts`、`src/i18n/SerialTest_zh_TW.ts`、`src/i18n/SerialTest_nb_NO.ts` 及对应 `.qm`。此阶段不再重构应用源码。

具体任务：

1. 将阶段 1 已验证的普通 Build 增加 `workflow_call` 入口；Release 调用同一套构建/部署步骤，随后下载对应 artifacts 发布。移除 Release 中重复的旧构建 job，确保发布不会重新走 PR head/系统 pip/`*-latest`。保留原发布参数语义，不自动发布、不自动打 Tag。
2. Windows 包使用 `windeployqt --release`，检查 `platforms/qwindows.dll`、需要的 Qt DLL，以及 MSVC runtime 的交付方式。测试包明确记录运行时前提；不要承诺“任意干净 Windows 机器都能运行”。若实际缺 VC++ runtime，按 Qt 部署文档选择应用目录所需文件或官方 Redistributable 方案；不要复制任意 system32 DLL。
3. macOS 包明确 arm64 和最低 26.0，检查应用包内容和部署后的库路径。没有证书时记录为未签名测试包，不擅自设置签名 secrets 或加入公证操作。
4. Linux 包明确“Ubuntu 24.04 x86_64，需与构建所用 Qt 6.11.1 兼容的运行库及插件”，发行版自带 Qt 未必满足；不要继续写成通用便携包。想做 AppImage/Flatpak 时，停在可选决策，不把它算作本轮已完成交付。
5. CI 中对部署后的应用做短时存活检查：从包中解压/挂载的应用路径运行，检查没有提前退出，再由测试脚本结束这个测试进程。用独立测试配置启动；Linux 明确在保留 Qt 运行环境的 runner 上检查，不能据此证明包自包含。存活检查只发现明显启动失败，不能证明窗口可交互；交互仍由 UI 测试及人工验收覆盖。Windows/macOS 不以 Linux 的截图结果冒充原生启动结果，`--help` 也不能代替 GUI 进程的存活检查。
6. 如果阶段 3～5 新增了用户可见文本，CI 使用 Qt 的 `lupdate/lrelease` 更新翻译并生成 `.qm`，提交实际变动的 `.ts/.qm`；没有新增文本就跳过。若需要把 CI 生成的翻译文件带回仓库，通过 artifact 下载，不在本机运行构建工具。
7. README 更新当前构建入口、产物下载方式、三平台验证范围、未签名状态和 Linux 包性质；移除/修正不再适用的旧平台承诺。不要扩展修改 `doc/` 中旧文档。
8. 按第 7 节完成最终验收，记录真实通过、未测和不适用项。硬件未测就标未测，不为追求“全绿”伪造连接结果。

Windows 部署依据：[Qt 6.11 windeployqt 文档](https://doc.qt.io/qt-6.11/windows-deployment.html)。签名、Intel/macOS 旧版本、Linux 便携包、实体设备自动化都属于用户可选的后续工作。

验收：最终提交三平台构建/部署/启动检查全绿；截图检查通过；用户已使用 Windows 和 macOS 下载产物完成本节明确要求的人工 UI 验收。若暂时没有 Windows 环境，状态写“CI 编译/启动已验证，Windows 人工 UI/DPI 待验收”，不得宣称跨平台验收完成。

提交名：`阶段6：统一发布构建并记录跨平台验收`。

## 7. 最终验收清单

不跑所有条件的笛卡尔积。以下固定组合已经足够覆盖本轮正常使用路径：

| 运行环境 | 固定组合 | 验证内容 |
| --- | --- | --- |
| Linux Actions | 1200×760、英文、None/Light/Dark、六页 | 基本外观、切页、图表颜色、配置展开 |
| Linux Actions | 1000×700、简中、Light、12 pt、六页 | 窄窗口下常用控件不遮挡；滚动区内容可访问 |
| Linux Actions | 1200×760、Dark、14 pt、1.5/2 倍缩放、Connect/Data/Plot | 字体放大、Tab/箭头点击区、文本/曲线剩余面积 |
| Windows CI 产物 | 用户机器或远程 Windows，100%/150%/200%，Light/Dark | 原生控件、下拉弹出层、字体、浮动/回停靠与状态栏 |
| macOS CI 产物 | Retina，Light/Dark/None，正常与最大化 | 字体、Dock 浮动/回停靠、主题往返与窗口恢复 |

人工步骤（使用 CI 编译的包，不在本机编译）：

1. 用 `--config-path` 指向单独测试 INI 启动，保留真实个人配置。
2. 六页各切换一次；修改主题和界面字体后重新切换，确认没有旧色块、文字截断或按钮跳动。
3. 正常缩小到 1000×700，再最大化；表格/文本/曲线获得空间，底部按钮仍可访问。
4. 拖一个 Dock 浮动，点击 Dock Back 返回；另一次用浮动窗口关闭操作返回；关闭应用后重启，检查已有布局恢复。不要删除全部偏好来掩盖恢复问题。
5. 下拉框分别检查普通选择、可编辑波特率、字体选择；SpinBox 检查箭头与键盘操作；禁用态和焦点态可辨认。
6. 有硬件时再测一次串口、蓝牙正常连接/收发；无硬件则记录未测。TCP/UDP 可用本机或远程回环测试收发，不为此修改生产网络代码。

截图是视觉证据；Qt Test 检查的是正常交互和几何等可自动判定的结果。不要用跨平台像素完全一致作为通过条件，也不要为本轮 UI 增加模糊测试、损坏配置修复、自动重连或复杂异常恢复。

## 8. 直接给 Gemini 的起始指令

```text
先阅读 AGENTS.md 和 REFACTOR_PLAN.md。

执行推荐 A 路线，仅做阶段 1。现在不要开始 UI 修改。
不要在本机编译或安装 Qt，所有编译在 GitHub Actions。
先检查 git status，保留已有日志和其他用户文件。
严格遵守阶段 1 的文件白名单；不要防御性编程，不要新增边界处理。
使用开发分支，只提交本阶段明确修改的文件。
推送后验证本次提交 SHA 的 Windows/macOS/Linux 三个 job。
失败只按该次日志修复；如果必须改白名单外源码或更换方案，先说明具体问题让我决定。
本机没有 gh 时，给出提交 SHA 和 Actions 链接，让我贴该次 CI 状态/日志。
交付时列出文件、提交 SHA、CI 链接、产物和未验证项，然后停下。
```

后续每次将“阶段 1”替换为下一阶段名。UI 阶段补充：“上传并检查截图，待我确认视觉效果后再继续。”

## 9. 后续可选工作，不属于当前执行范围

- **普通 Tab 导航**：若不需要浮动/多页并排，采用第 4 节 B 方案可明显减少布局状态管理；先确定移除哪些交互，再写替代阶段。
- **更广 macOS 兼容 / Intel**：单独降低 deployment target、增加对应 runner 与真实旧系统测试，不与主题修改混合。
- **独立通信层拆分**：只有后续确有复用/测试需求时，再按串口、蓝牙、网络拆 `Connection`；当前先保留 API。
- **性能治理**：用户提供大流量卡顿复现后，测量收发/解码/绘图耗时，再讨论 `processEvents()` 和线程，不预先改线程模型。
- **CMake / QML**：CMake 是构建组织选择，QML 是 UI 重写选择；两者都不是“Tab 太小”的必要修复。
- **硬件远程测试机、签名、公证、Linux 便携包**：分别定预算和运行环境，不把本轮重构扩大为发布基础设施项目。
