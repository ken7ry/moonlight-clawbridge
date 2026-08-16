# ClawBridge 指令手册

## 连接方式

```bash
nc -w 1 <mac-mini-ip> 9999
```

端口 `9999`，TCP 协议，Moonlight 串流中自动监听。

---

## 一、键盘 KB:<key>_<action>

### 格式

```
KB:<键名>_<动作>
```

**动作：** `_DOWN`（按下） `_UP`（松开）

### 字母键（A-Z）

| 指令 | 按键 |
|------|------|
| `KB:A_DOWN` / `KB:A_UP` | A |
| `KB:B_DOWN` / `KB:B_UP` | B |
| `KB:Z_DOWN` / `KB:Z_UP` | Z（其他字母同理） |

### 数字键（0-9）

| 指令 | 按键 |
|------|------|
| `KB:0_DOWN` / `KB:0_UP` | 数字 0 |
| `KB:1_DOWN` / `KB:1_UP` | 数字 1 |
| `KB:9_DOWN` / `KB:9_UP` | 数字 9（同理 2-8） |

### 功能键 F1-F24

| 指令 | 按键 |
|------|------|
| `KB:F1_DOWN` → `KB:F12_DOWN` | F1 - F12 |
| `KB:F13_DOWN` → `KB:F24_DOWN` | F13 - F24 |

### 特殊按键

| 指令 | 按键 | VK码 |
|------|------|------|
| `KB:ENTER_DOWN` | 回车 | 0x0D |
| `KB:ESC_DOWN` | Esc | 0x1B |
| `KB:TAB_DOWN` | Tab | 0x09 |
| `KB:SPACE_DOWN` | 空格 | 0x20 |
| `KB:BACKSPACE_DOWN` | 退格 | 0x08 |
| `KB:DELETE_DOWN` | 删除 | 0x2E |
| `KB:INSERT_DOWN` | 插入 | 0x2D |
| `KB:HOME_DOWN` | Home | 0x24 |
| `KB:END_DOWN` | End | 0x23 |
| `KB:PAGEUP_DOWN` | 上一页 | 0x21 |
| `KB:PAGEDOWN_DOWN` | 下一页 | 0x22 |
| `KB:CAPSLOCK_DOWN` | 大写锁定 | 0x14 |
| `KB:PRINTSCREEN_DOWN` | 截屏 | 0x2C |
| `KB:SCROLLLOCK_DOWN` | 滚动锁定 | 0x91 |
| `KB:PAUSE_DOWN` | 暂停 | 0x13 |

### 方向键

| 指令 | 按键 |
|------|------|
| `KB:KB_UP_DOWN` / `KB:KB_UP_UP` | ↑ 上 |
| `KB:KB_DOWN_DOWN` / `KB:KB_DOWN_UP` | ↓ 下 |
| `KB:KB_LEFT_DOWN` / `KB:KB_LEFT_UP` | ← 左 |
| `KB:KB_RIGHT_DOWN` / `KB:KB_RIGHT_UP` | → 右 |

### 修饰键

| 指令 | 按键 |
|------|------|
| `KB:LSHIFT_DOWN` | 左 Shift |
| `KB:RSHIFT_DOWN` | 右 Shift |
| `KB:LCTRL_DOWN` | 左 Ctrl |
| `KB:RCTRL_DOWN` | 右 Ctrl |
| `KB:LALT_DOWN` | 左 Alt（Option） |
| `KB:RALT_DOWN` | 右 Alt（Option） |
| `KB:LWIN_DOWN` / `KB:GUI_DOWN` | 左 Win（Command ⊞） |
| `KB:RWIN_DOWN` | 右 Win（Command ⊞） |

### 符号键

| 指令 | 按键 |
|------|------|
| `KB:MINUS_DOWN` | `-` 减号 |
| `KB:EQUALS_DOWN` | `=` 等号 |
| `KB:LBRACKET_DOWN` / `KB:LBRA_DOWN` | `[` 左方括号 |
| `KB:RBRACKET_DOWN` / `KB:RBRA_DOWN` | `]` 右方括号 |
| `KB:BACKSLASH_DOWN` | `\` 反斜杠 |
| `KB:SEMICOLON_DOWN` | `;` 分号 |
| `KB:APOSTROPHE_DOWN` | `'` 单引号 |
| `KB:COMMA_DOWN` | `,` 逗号 |
| `KB:PERIOD_DOWN` | `.` 句号 |
| `KB:SLASH_DOWN` | `/` 斜杠 |
| `KB:BACKTICK_DOWN` | `` ` `` 反引号 |

### 数字小键盘

| 指令 | 按键 |
|------|------|
| `KB:NUMPAD0_DOWN` / `KB:NP0_DOWN` | 小键盘 0 |
| `KB:NUMPAD1_DOWN` / `KB:NP1_DOWN` | 小键盘 1 |
| `KB:NUMPAD9_DOWN` / `KB:NP9_DOWN` | 小键盘 9 |
| `KB:NUMPADADD_DOWN` / `KB:NPADD_DOWN` | 小键盘 `+` |
| `KB:NUMPADENTER_DOWN` / `KB:NPENTER_DOWN` | 小键盘回车 |

### 常用组合键

```bash
# Ctrl+C 复制
echo "KB:LCTRL_DOWN" | nc -w 1 localhost 9999
echo "KB:C_DOWN" | nc -w 1 localhost 9999
echo "KB:C_UP" | nc -w 1 localhost 9999
echo "KB:LCTRL_UP" | nc -w 1 localhost 9999

# Ctrl+V 粘贴
echo "KB:LCTRL_DOWN" | nc -w 1 localhost 9999
echo "KB:V_DOWN" | nc -w 1 localhost 9999
echo "KB:V_UP" | nc -w 1 localhost 9999
echo "KB:LCTRL_UP" | nc -w 1 localhost 9999

# Alt+Tab 切换窗口
echo "KB:LALT_DOWN" | nc -w 1 localhost 9999
echo "KB:TAB_DOWN" | nc -w 1 localhost 9999
echo "KB:TAB_UP" | nc -w 1 localhost 9999
echo "KB:LALT_UP" | nc -w 1 localhost 9999

# Win+D 显示桌面
echo "KB:LWIN_DOWN" | nc -w 1 localhost 9999
echo "KB:D_DOWN" | nc -w 1 localhost 9999
echo "KB:D_UP" | nc -w 1 localhost 9999
echo "KB:LWIN_UP" | nc -w 1 localhost 9999

# Win+R 运行
echo "KB:LWIN_DOWN" | nc -w 1 localhost 9999
echo "KB:R_DOWN" | nc -w 1 localhost 9999
echo "KB:R_UP" | nc -w 1 localhost 9999
echo "KB:LWIN_UP" | nc -w 1 localhost 9999
```

---

## 二、鼠标按钮 MB:<btn>_<action>

### 按键映射

| 按键 | 数值 | 含义 |
|------|------|------|
| `1` 或 `LEFT` | 1 | 左键 |
| `2` 或 `MIDDLE` 或 `WHEEL` | 2 | 中键 |
| `3` 或 `RIGHT` | 3 | 右键 |
| `4` 或 `X1` 或 `BACK` | 4 | 侧键后退 |
| `5` 或 `X2` 或 `FORWARD` | 5 | 侧键前进 |

### 动作

- `_PRESS`：按下
- `_RELEASE`：松开

### 示例

| 指令 | 效果 |
|------|------|
| `MB:1_PRESS` | 左键按下 |
| `MB:1_RELEASE` | 左键松开 |
| `MB:LEFT_PRESS` | 左键按下（别名） |
| `MB:3_PRESS` | 右键按下 |
| `MB:3_RELEASE` | 右键松开 |
| `MB:2_PRESS` | 中键按下 |
| `MB:4_PRESS` | 后退 |
| `MB:5_PRESS` | 前进 |

---

## 三、鼠标相对移动 MV:<dx>,<dy>

### 格式

```
MV:<水平偏移>,<垂直偏移>
```

| 参数 | 含义 |
|------|------|
| `dx` | 水平偏移（像素），正数→右，负数→左 |
| `dy` | 垂直偏移（像素），正数→下，负数→上 |

### 示例

| 指令 | 效果 |
|------|------|
| `MV:100,0` | 向右移 100 像素 |
| `MV:0,50` | 向下移 50 像素 |
| `MV:0,-200` | 向上移 200 像素 |
| `MV:100,100` | 右下对角线移动 |

---

## 四、鼠标绝对定位 MP:<x>,<y>,<w>,<h>

### 格式

```
MP:<X坐标>,<Y坐标>,<屏幕宽>,<屏幕高>
```

| 参数 | 含义 |
|------|------|
| `x` | 目标 X 坐标（从屏幕左上角 0,0 起算） |
| `y` | 目标 Y 坐标 |
| `w` | 被控端屏幕宽度（像素） |
| `h` | 被控端屏幕高度（像素） |

### 示例

| 指令 | 效果 |
|------|------|
| `MP:960,540,1920,1080` | 移到 1080p 屏幕正中心 |
| `MP:0,0,1920,1080` | 移到屏幕左上角 |
| `MP:1919,1079,1920,1080` | 移到屏幕右下角 |
| `MP:1280,720,2560,1440` | 移到 2K 屏中心 |
| `MP:100,100,1920,1080` | 移到 (100,100) 位置 |

### 测试方法

1. 确保 Moonlight 串流中
2. 在 Mac Mini 终端执行：

```bash
# 移到屏幕中心（假设被控端 1080p）
echo "MP:960,540,1920,1080" | nc -w 1 localhost 9999

# 移到左上角
echo "MP:0,0,1920,1080" | nc -w 1 localhost 9999

# 移到右下角
echo "MP:1919,1079,1920,1080" | nc -w 1 localhost 9999
```

3. 看**被控 PC** 上鼠标是否移动到对应位置

### MV 对比 MP

| 特性 | MV（相对移动） | MP（绝对定位） |
|------|---------------|--------------|
| 需要知道当前位置 | ✅ 是 | ❌ 否 |
| 精确到达某坐标 | ❌ 不方便 | ✅ 直接指定 |
| 需要屏幕分辨率 | ❌ 不需要 | ✅ 需要 w,h |
| 适合场景 | 微调、拖动 | 跳转按钮、菜单 |

---

## 五、鼠标滚轮 SL:<dx>,<dy>

### 格式

```
SL:<水平滚轮>,<垂直滚轮>
```

| 指令 | 效果 |
|------|------|
| `SL:0,-120` | 向下滚动 |
| `SL:0,120` | 向上滚动 |
| `SL:120,0` | 向右水平滚动 |
| `SL:-120,0` | 向左水平滚动 |

---

## 六、文本输入 TXT:<text>

| 指令 | 效果 |
|------|------|
| `TXT:Hello World` | 英文输入 |
| `TXT:你好世界` | 中文输入 |
| `TXT:12345` | 数字输入 |

---

## 七、游戏手柄 GP:<掩码>_<action>

### 按键掩码

| 按键 | 十进制 | 十六进制 |
|------|--------|----------|
| A | 4096 | 0x1000 |
| B | 8192 | 0x2000 |
| X | 16384 | 0x4000 |
| Y | 32768 | 0x8000 |
| DPAD 上 | 1 | 0x0001 |
| DPAD 下 | 2 | 0x0002 |
| DPAD 左 | 4 | 0x0004 |
| DPAD 右 | 8 | 0x0008 |
| Back | 32 | 0x0020 |
| Start | 16 | 0x0010 |
| LB / L1 | 256 | 0x0100 |
| RB / R1 | 512 | 0x0200 |
| LS / L3 | 1024 | 0x0400 |
| RS / R3 | 2048 | 0x0800 |

### 动作：`_PRESS` / `_RELEASE`

### 示例

```bash
# 按下 A 键
echo "GP:4096_PRESS" | nc -w 1 localhost 9999
echo "GP:4096_RELEASE" | nc -w 1 localhost 9999

# 按下 B 键（十六进制写法）
echo "GP:0x2000_PRESS" | nc -w 1 localhost 9999
echo "GP:0x2000_RELEASE" | nc -w 1 localhost 9999

# A+B 同时按下 (4096+8192=12288)
echo "GP:12288_PRESS" | nc -w 1 localhost 9999
echo "GP:12288_RELEASE" | nc -w 1 localhost 9999
```

---

## 八、快捷键

Mac 上 Moonlight 快捷键前缀：**Ctrl + ⌥(Alt) + Shift + <键>**

| 快捷键 | 功能 |
|---------|------|
| `Ctrl+⌥+Shift+Q` | 退出串流 |
| `Ctrl+⌥+Shift+Z` | 释放鼠标（解除捕获） |
| `Ctrl+⌥+Shift+X` | 切换全屏 |
| `Ctrl+⌥+Shift+S` | 性能统计浮层 |
| `Ctrl+⌥+Shift+M` | 切换鼠标模式 |
| `Ctrl+⌥+Shift+C` | 切换鼠标指针显示 |
| `Ctrl+⌥+Shift+D` | 最小化窗口 |
| `Ctrl+⌥+Shift+V` | 粘贴文本到被控端 |
| `Ctrl+⌥+Shift+L` | 切换鼠标区域锁定 |
| `Ctrl+⌥+Shift+E` | 退出并关闭 Moonlight |

---

## 九、Shell 快捷函数

添加到 `~/.zshrc` 或 `~/.bashrc`：

```bash
# ClawBridge 快捷函数
CB_HOST="127.0.0.1"
cb() { echo "$1" | nc -w 1 "$CB_HOST" 9999; }
cbm() { echo "MV:$1,$2" | nc -w 1 "$CB_HOST" 9999; }
cbp() { echo "MP:$1,$2,$3,$4" | nc -w 1 "$CB_HOST" 9999; }
cbt() { echo "TXT:$1" | nc -w 1 "$CB_HOST" 9999; }

# 使用
source ~/.zshrc
cb "KB:ENTER_DOWN"          # 按 Enter
cbm 100 50                  # 鼠标相对移动
cbp 960 540 1920 1080       # 移到屏幕中心
cbt "Hello"                 # 输入文字
```

---

## 十、常见问题

**Q: 指令没反应？**
- 确认 Moonlight 正在串流（`lsof -i :9999` 能看到 LISTEN）
- KB 需要 `_DOWN` + `_UP` 配对
- MV 用**逗号**分隔，不是冒号

**Q: MP 绝对定位不准？**
- 确认 w,h 参数填的是**被控端**的屏幕分辨率
- 坐标会被缩放到被控端实际尺寸

**Q: 最小化 Moonlight 还能用吗？**
- 可以。TCP 服务器进程级别运行，跟窗口焦点无关。

**Q: 能从外网控制吗？**
- 需要 Tailscale/Zerotier 等内网穿透工具。
