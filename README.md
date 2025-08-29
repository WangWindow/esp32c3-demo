# ESP32-C3 Demo (ESP-IDF + PlatformIO)

一个基于 ESP32-C3 的最小演示工程，展示：

1. 启动后连接指定 WiFi（STA 模式，带重试与超时处理）
2. 打印一次网络信息（IP/MAC/信号强度/可用堆内存等）
3. 闪烁板载或外接 LED（当前使用 GPIO8）

## ✨ 功能概览
* WiFi STA 连接（最大重试次数可调 `WIFI_MAX_RETRY`）
* 连接成功 / 失败 / 超时日志输出
* 一次性打印网络参数（避免刷屏）
* LED 心跳闪烁（默认 500ms 周期）

## 🛠 硬件需求
* ESP32-C3-DevKitM-1 开发板
* 可选：外接 LED（若板载 LED 非 GPIO8，请按需改宏 `LED_GPIO`）

## ⚙️ 目录结构
```
.
├── platformio.ini          # PlatformIO 环境配置（espidf 框架）
├── src/main.c              # 主应用：WiFi + LED
├── include/                # 头文件放置目录（当前为空）
├── lib/                    # 自定义库目录（当前为空）
└── test/                   # 单元 / 集成测试目录（占位）
```

## 🚀 快速开始
确保已安装 VS Code + PlatformIO IDE 扩展，或直接使用命令行。

### 1. 克隆项目
```
git clone <your-repo-url> esp32c3-demo
cd esp32c3-demo
```

### 2. 修改 WiFi 配置
编辑 `src/main.c` 中：
```
#define WIFI_SSID "你的SSID"
#define WIFI_PASS "你的密码"
```
建议：不要把真实密码提交到公共仓库；可改为在编译期通过编译宏或 `sdkconfig`/Kconfig 管理。

### 3. 构建
```
pio run
```

### 4. 烧录
（自动检测串口，如需指定：`-p COMx`）
```
pio run -t upload
```

### 5. 串口监视
```
pio device monitor --baud 115200
```
看到 `WiFi connected` 与网络信息后，LED 将持续闪烁。

## 🔧 可调参数（`src/main.c`）
| 宏 | 说明 | 默认 |
| --- | --- | --- |
| WIFI_SSID / WIFI_PASS | WiFi 凭据 | 示例值（请替换） |
| WIFI_MAX_RETRY | 最大重试次数 | 5 |
| LED_GPIO | LED 所在 GPIO | 8 |
| BLINK_DELAY_MS | 闪烁周期（毫秒） | 500 |

## 📝 日志示例
```
I (1234) APP: WiFi start STA, connecting to: <SSID>
I (2345) APP: WiFi connected
I (2346) APP: Connected SSID: <SSID>
I (2347) APP: Channel: 1  RSSI: -45 dBm
I (2348) APP: IP: 192.168.1.123  GW: 192.168.1.1  MASK: 255.255.255.0
I (2349) APP: MAC: AA:BB:CC:DD:EE:FF
I (2350) APP: Free heap: 287612 bytes
```

## 🔐 安全提示
* 切勿在公共仓库保留真实 WiFi 密码。
* 可采用：
	* 使用 `build_flags` 传递：`-DWIFI_SSID=\"xxx\" -DWIFI_PASS=\"yyy\"`
	* 或运行时从 NVS / SPIFFS / 加密分区读取。

## 🧩 后续可扩展点（建议）
* Kconfig 菜单化 WiFi / LED 参数
* OTA 升级
* 使用事件组对更多状态进行同步
* 增加单元测试（Unity）

## 📄 许可证
根据需要添加（例如 MIT / Apache-2.0）。

---
如果你希望我继续添加 Kconfig、参数抽离或安全存储示例，请告诉我。
