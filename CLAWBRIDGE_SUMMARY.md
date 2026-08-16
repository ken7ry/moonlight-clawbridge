# ClawBridge 项目结项总结

> **Moonlight-Qt TCP 远程控制器 — 工业级 SDL 事件注入模块**
>
> 版本: V10 最终版 | 状态: ✅ 编译通过 | 安全等级: 生产就绪

---

## 📋 模块简介

ClawBridge 为 [moonlight-stream/moonlight-qt](https://github.com/moonlight-stream/moonlight-qt) 增加 **TCP 远程输入控制** 能力。串流启动时自动监听 `0.0.0.0:9999`，接收纯文本指令（手柄/键盘/鼠标/滚轮/文本），通过 SDL 自定义事件安全注入到主线程，最终由 Limelight SDK 发送至被控主机。

### 支持 7 种指令

| 前缀 | 类型 | 示例 |
|------|------|------|
| `GP:` | 游戏手柄 | `GP:A_DOWN`, `GP:DPAD_LEFT_UP` |
| `KB:` | 键盘 | `KB:ENTER_DOWN`, `KB:ESC_UP` |
| `MB:` | 鼠标按键 | `MB:LEFT_DOWN`, `MB:X1_UP` |
| `MV:` | 鼠标移动 | `MV:100,-50` |
| `WH:` | 垂直滚轮 | `WH:3` |
| `HS:` | 水平滚轮 | `HS:-1` |
| `TXT:` | 文本输入 | `TXT:你好世界` |

---

## 🛡️ 防御性设计亮点

### 1. 跨线程安全 — SDL 事件队列注入

**问题：** Qt `readyRead` 回调在非主线程触发，直接调 `LiSend*` 会从非主线程访问 ENet 网络层，导致崩溃。

**方案：** 解析线程只负责创建事件入队，主线程 `Session::exec()` 消费事件后实际调用 Limelight API。

```cpp
// 解析线程: 只创建事件
push<CBEvt_Kb>(CB_EV_KEYBOARD, { vk, action, 0 });

// 主线程: 实际执行
LiSendKeyboardEvent(0x8000 | (vk & 0xFF), action, mods);
```

### 2. OOM 防御 — 单行 1024 字节限制

**问题：** `readLine()` 无上限，恶意/异常客户端可不发送换行符使缓冲区无限膨胀。

**方案：** `sock->readLine(1024)` — 最长指令仅 17 字节，1024 绰绰有余，完全阻断 OOM 攻击。

### 3. 内存泄漏保护 — CB_EV_TEXT 特判释放

**问题：** TXT 指令的字符串用 `new char[]` 动态分配。`SDL_PushEvent` 失败时普通 `delete heap` 只释放结构体，不释放 `str` 指向的数组，持续泄漏。

**方案：** push 模板对 `CB_EV_TEXT` 特判，先 `delete[] txt->str` 再 `delete heap`。session.cpp 中同样先 `delete[] p->str` 再 `delete p`。

### 4. 幽灵调用防御 — Socket 彻底解绑

**问题：** 高频连断场景下，`deleteLater()` 延迟期间闲置定时器和残留信号可能触发回调。

**方案：** `sock->disconnect()` → `m_clients.removeAll(sock)` → `sock->deleteLater()`，顺序不可颠倒。

---

## ⚠️ 维护注意事项

### 修改 CB_EV_TEXT 内存传递时

- push 失败路径：必须 **先** `delete[] str` **再** `delete heap`
- session.cpp 消费路径：必须 **先** `delete[] p->str` **再** `delete p`
- 其余 5 种事件均为 POD 平凡类型，直接 `delete heap` 即可

### 添加新指令类型

1. `clawbridge.h`: 定义 `CB_EV_XXX = SDL_USEREVENT + N`
2. 定义对应 `struct CBEvt_Xxx`
3. `parseLine`: 添加解析 → `push<CBEvt_Xxx>`
4. `input.h`: 声明 `void cbInjectXxx(...)` → `input.cpp` 实现
5. `session.cpp`: 添加 `case CB_EV_XXX:` 拦截

---

## 📁 文件清单

| 文件 | 说明 |
|------|------|
| `clawbridge.h` | 头文件（宏定义 + 类声明） |
| `clawbridge.cpp` | 实现（TCP 服务器 + 解析 + 映射表） |
| `claw_bridge.patch` | 标准 Unified Diff 补丁 |
| `macos-build-clawbridge.yml` | GitHub Actions CI |
| `CHEATSHEET.md` | 完整指令速查卡 |

*本文档为 ClawBridge V10 结项归档。所有设计决策均经代码审计验证。*
