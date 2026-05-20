# Flashshot x64

**版本：0.1.0 beta** | **许可证：GNU General Public License v3.0** | **支持平台：Windows 10 / 11 (64-bit)**

Flashshot 是一款专为 Windows 平台设计的轻量级截图工具，支持全局热键、回放缓冲区（即时重放）、自定义通知及开机自启动。核心功能基于 Qt 6 与原生 Windows API 实现，具有低内存占用、高响应性能。

## 功能特性

- **全局热键截图**：支持字母、数字、功能键与修饰键（Ctrl / Alt / Shift / Win）组合，低延迟捕获全屏。
- **回放缓冲区（即时重放）**：后台持续抓屏并压缩存储，按快捷键保存最近 N 秒的连续帧序列。
- **可配置图片质量**：高 / 中 / 低三档 JPEG 压缩质量，平衡文件体积与画质。
- **自定义保存目录**：截图与回放序列分别存储，支持绝对路径。
- **系统托盘交互**：托盘图标悬停提示（运行状态、回放开关、开机自启），右键菜单提供常用功能。
- **通知气泡**：截图时右下角弹出黑色半透明通知，可附带图标与自定义音效。
- **剪贴板集成**：截图后自动将图像复制到系统剪贴板，便于即时粘贴使用。
- **开机自启动**：通过注册表实现当前用户自启动，无需管理员权限。
- **桌面快捷方式**：可选创建桌面快捷方式。
- **设置向导**：引导式配置热键、目录、质量、回放参数、通知与自启动。
- **日志系统**：环形缓冲区存储最近 5000 条日志，支持异常时自动导出或手动导出至 `Logs/` 目录。
- **单例运行**：保证同一时间只有一个实例运行，重复启动时仅弹出提示。
- **崩溃捕获**：安装 Qt 消息处理器与系统信号处理器，异常时自动保存诊断日志。

## 系统要求

- 操作系统：Windows 10 或 Windows 11（64 位）
- 依赖组件：无额外运行时要求（需随附 Qt 6 动态库）
- 管理员权限：键盘钩子需要以管理员身份运行程序（可执行文件已嵌入 `requireAdministrator` 清单）

## 编译与安装

### 源代码编译

1. **安装开发环境**
   
   - Qt 6.11.0 for MinGW 64-bit（`G:\Qt\6.11.0\mingw_64`）
   - MinGW 13.1.0（已随 Qt 安装：`G:\Qt\Tools\mingw1310_64`）
   - CMake 3.30.5 或更高（随 Qt 提供：`G:\Qt\Tools\CMake_64`）

2. **获取源代码**
   
   ```bash
   git clone <repository-url>
   cd Flashshot_x64
   ```

3. **生成构建文件**
   
   ```powershell
   mkdir build
   cd build
   G:\Qt\Tools\CMake_64\bin\cmake.exe .. -G "MinGW Makefiles" `
       -DCMAKE_MAKE_PROGRAM=G:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe `
       -DCMAKE_PREFIX_PATH=G:/Qt/6.11.0/mingw_64
   ```

4. **编译**
   
   ```powershell
   G:\Qt\Tools\CMake_64\bin\cmake.exe --build . --config Release
   ```

5. **生成结果**  
   可执行文件位于 `build/Flashshot_x64.exe`。

### 运行时部署

若需分发，请将以下文件置于同一目录：

- `Flashshot_x64.exe`

- `resources/Flashshot.png`（托盘与通知图标）

- `resources/Flashshot_noti.wav`（可选，通知音效）

- Qt 6 动态库（可使用 `windeployqt` 自动部署）：
  
  ```powershell
  G:\Qt\6.11.0\mingw_64\bin\windeployqt.exe Flashshot_x64.exe
  ```

## 使用说明

### 首次启动

以管理员身份运行 `Flashshot_x64.exe`，程序将自动弹出设置向导。按向导配置：

- **快捷键**：截图热键（如 `F12`）与回放热键（如 `Ctrl+Shift+PageUp`）。
- **保存目录**：截图存储路径，默认为 `%USERPROFILE%\Pictures\Flashshot_x64`。
- **图片质量**：高（95）、中（75）、低（50）。
- **回放参数**：回溯时长（秒）、抓帧间隔（毫秒）、缩放比例（减少内存占用）。
- **通知**：启用/禁用气泡通知、显示时长、提示音。
- **其他**：开机自启动、创建桌面快捷方式。

配置完成后程序最小化至系统托盘。

### 日常操作

| 动作     | 操作                                          |
| ------ | ------------------------------------------- |
| 截图     | 按下设置的截图热键（默认 `F12`）                         |
| 保存回放序列 | 按下回放热键（默认 `Ctrl+Shift+PageUp`），保存最近 N 秒的连续帧 |
| 打开配置向导 | 右键托盘图标 → “重新设置向导”                           |
| 打开截图目录 | 右键托盘图标 → “打开截图保存目录”                         |
| 导出日志   | 右键托盘图标 → “导出日志”                             |
| 启停回放功能 | 右键托盘图标 → “回放功能: 开/关”                        |
| 退出程序   | 右键托盘图标 → “退出”                               |

### 托盘图标悬停提示

鼠标悬停时显示当前状态，格式如下：

```
Flashshot x64 运行中
即时回放：开 / 关
开机自启：是 / 否
```

### 剪贴板

每次截图后，图像会自动复制到系统剪贴板（`CF_BITMAP` 格式），可直接粘贴至聊天软件、文档编辑器等。

## 配置文件与数据目录

程序数据存储在 `%USERPROFILE%\MomsterTech\Flashshot_x64\`：

- `config.ini`：用户配置（热键、目录、质量、回放参数、自启动等），使用 INI 格式。
- `Logs\`：日志文件，文件名格式 `flashshot_log_YYYYMMDD_hhmmss_<reason>.txt`。程序启动时会自动删除超过 24 小时的日志文件。
- （无其它临时文件）

示例 `config.ini` 片段：

```ini
[General]
hotkey=F12
replay_hotkey=Ctrl+Shift+PageUp
save_dir=C:/Users/xxx/Pictures/Flashshot_x64
quality=2
replay_enabled=false
replay_duration=10
replay_interval=500
replay_scale=50
autostart=false
desktop_shortcut=false
notifications_enabled=true
sound_enabled=true
notification_duration=1000
```

## 日志系统

- **环形缓冲区**：内存中保留最近 5000 条日志（`DEBUG` / `INFO` / `WARNING` / `CRITICAL` / `FATAL` 级别）。
- **自动导出**：
  - 程序正常退出时自动保存日志（`shutdown` 或 `normal_exit` 原因）。
  - 发生 `qFatal` 或系统信号（SIGSEGV 等）时自动保存。
- **手动导出**：托盘菜单 “导出日志”，立即将当前缓冲区写入文件。
- **自动清理**：每次启动时删除 `Logs/` 目录中最后修改时间超过 24 小时的日志文件。

## 常见问题

### 1. 程序无法安装键盘钩子，弹出错误提示

**原因**：未以管理员身份运行。  
**解决**：右键 `Flashshot_x64.exe` → “以管理员身份运行”。可执行文件已嵌入 `requireAdministrator` 清单，双击时会自动请求提权，请允许 UAC 对话框。

### 2. 截图热键无响应

**可能原因**：

- 程序未以管理员身份运行（见上条）。
- 热键被其他程序占用（如 NVIDIA ShadowPlay、Discord 等）。
- 设置的快捷键包含小键盘按键（当前版本对小键盘支持有限）。

**解决**：更改热键为其他组合，或关闭冲突的全局热键软件。

### 3. 回放缓冲区保存的图片数量较少

**原因**：回溯时长与抓帧间隔配置不当，或缩放比例过低导致过期帧被清除。  
**解决**：在设置向导中增大“回溯时长”或减小“抓帧间隔”，同时合理设置“采样比例”（不建议低于 25%）。

### 4. 通知气泡无图标或无声效

**原因**：资源文件未放置正确。  
**解决**：确保 `resources/Flashshot.png` 与 `resources/Flashshot_noti.wav` 位于可执行文件同级目录的 `resources` 子目录中。

### 5. 开机自启动未生效

**原因**：程序未以管理员身份运行，或注册表写入被安全软件拦截。  
**解决**：手动将快捷方式放入 `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup`，或在设置向导中重新勾选“开机自启动”并确认防火墙/安全软件允许。

### 6. 退出程序后进程残留

**原因**：`QThreadPool` 中的异步保存任务未完全结束，或键盘钩子线程未及时停止。  
**解决**：已在代码中添加 `QThreadPool::globalInstance()->waitForDone(1000)`，等待一秒。若仍残留，请更新至最新编译版本。

## 开发者信息

- **许可证**：GNU General Public License v3.0（详见 `LICENSE` 文件）
- **原作者**：Momster
- **联系方式**：参见源代码文件头部的版权声明。
- **商业授权**：如需将 Flashshot 用于闭源商业产品（即不遵守 GPL-3.0 条款），请联系作者获取商业许可证。

## 致谢

- Qt 项目 (LGPLv3)
- MinGW-w64 项目

---

最后更新：2026-05-20
