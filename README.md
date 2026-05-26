# chat-system — 分布式聊天系统

TCP 聊天服务端 + WeChat 风格 Qt6 桌面客户端，支持注册登录、私聊消息、联系人管理、聊天记录持久化、管理员角色。

---

## 目录

- [使用环境](#使用环境)
- [快速开始](#快速开始)
- [部署](#部署)
- [使用指南](#使用指南)
- [设计概念](#设计概念)
- [架构图](#架构图)
- [协议设计](#协议设计)
- [数据流图](#数据流图)
- [状态机](#状态机)
- [数据库设计](#数据库设计)
- [组件说明](#组件说明)
- [项目结构](#项目结构)

---

## 使用环境

| 项目 | 要求 |
|------|------|
| 操作系统 | Linux (服务端) / 跨平台 (客户端) |
| 编译器 | g++ 13+ 或 clang 16+ (C++20) |
| 构建工具 | CMake 3.16+ |
| 数据库 | SQLite 3 (内嵌，无需安装) |
| 客户端 Qt | Qt 6.10+ (Core, Widgets, Network) |

```bash
# Debian/Ubuntu
sudo apt install build-essential cmake qt6-base-dev libsqlite3-0
```

---

## 快速开始

```bash
git clone <repo-url>
cd chat-system

# 终端 1: 启动服务端
cd server && mkdir build && cd build
cmake .. && make -j$(nproc)
./chat-server 9000

# 终端 2: 启动客户端 (可以开多个窗口多用户聊天)
cd client && mkdir build && cd build
cmake .. && make -j$(nproc)
./wechat-client
```

默认管理员账号: `admin` / `admin`

---

## 部署

### 服务端

```bash
cd server/build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./chat-server 9000
```

服务端启动后会在当前目录创建 `chat.db` (SQLite 数据库文件)。

### Systemd 服务

```ini
# /etc/systemd/system/chat-server.service
[Unit]
Description=Chat Server
After=network.target

[Service]
Type=simple
WorkingDirectory=/opt/chat-system/server
ExecStart=/opt/chat-system/server/build/chat-server 9000
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

### 客户端

客户端是普通 Qt 桌面应用，直接运行即可。连接地址硬编码为 `127.0.0.1:9000`。

---

## 使用指南

### 注册和登录

```
 1. 启动客户端 → 看到登录窗口
 2. 首次使用 → 点击"没有账号？注册"
 3. 输入用户名 + 密码 → 注册
 4. 返回登录 → 输入账号密码 → 登录
 5. 勾选"记住登录" → 下次自动填充用户名
```

### 聊天

```
 1. 左侧深色面板 → 点击联系人
 2. 右侧聊天区 → 输入消息 → 点击"发送"或 Ctrl+Enter
 3. 自己的消息: 绿色气泡 (#95EC69)
 4. 对方的消息: 白色气泡
 5. 新消息提醒: 联系人变绿色并置顶
```

### 添加联系人

```
 1. 左侧顶部输入框 → 输入对方用户名
 2. 点击 "+" 按钮 → 双方互相添加为联系人
```

### 管理员功能

管理员 (admin) 登录后可以看到所有注册用户。普通用户只能看到自己添加的联系人。

---

## 设计概念

### 单 Reactor + epoll (服务端)

```
聊天场景的并发特点：
  - 连接数少（几十到几百个用户）
  - 每个连接消息频率低
  - 需要广播用户上下线

→ 单线程 epoll 完全够用
→ 无需线程池（消息处理逻辑轻量）
```

### JSON over TCP 协议

```
选择 JSON 的原因：
  - 可读性好，方便调试
  - Qt 内置 QJsonDocument 解析
  - 聊天消息量少，JSON 开销可忽略

选择 4 字节长度前缀的原因：
  - TCP 是字节流，需要帧边界
  - 比 HTTP 的 Content-Length 头更简单
  - 比 Protobuf 易调试
```

### 密码哈希

```
SHA256(password) → 64 位十六进制字符串
  - 自包含实现，零外部依赖
  - 单向不可逆
  - 生产环境建议加盐 (salt)
```

---

## 架构图

```
┌─────────────────────────────────────────────────────────┐
│                     chat-system                          │
│                                                          │
│  ┌──────────────────────┐    ┌───────────────────────┐  │
│  │   chat-server        │    │   wechat-client (Qt6)  │  │
│  │                      │    │                        │  │
│  │  Main Thread:        │    │  LoginWindow           │  │
│  │  ┌────────────────┐  │    │  ├── 登录/注册切换      │  │
│  │  │ epoll_wait     │  │    │  ├── 密码框            │  │
│  │  │   accept──→fd  │  │    │  └── 记住登录          │  │
│  │  │   EPOLLIN─→recv│  │    │                        │  │
│  │  │   error──→close│  │    │  MainWindow            │  │
│  │  └────────────────┘  │    │  ┌─────────┬─────────┐ │  │
│  │                      │    │  │ChatList │ChatView │ │  │
│  │  ChatDB (SQLite):    │    │  │ 深色侧栏│ 聊天区   │ │  │
│  │  ┌────────────────┐  │    │  │         │ ┌─────┐ │ │  │
│  │  │ users          │  │    │  │ 搜索     │ │气泡 │ │ │  │
│  │  │ messages       │  │    │  │ Alice   │ │气泡 │ │ │  │
│  │  │ contacts       │  │    │  │ Bob     │ │气泡 │ │ │  │
│  │  └────────────────┘  │    │  │         │ └─────┘ │ │  │
│  └──────────────────────┘    │  │         │ 输入框   │ │  │
│          │                   │  └─────────┴─────────┘ │  │
│          │ TCP :9000         │                        │  │
│          └───────────────────┤  NetworkClient         │  │
│                              │  QTcpSocket            │  │
│                              └───────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## 协议设计

### 帧格式

```
┌──────────────┬──────────────────────────┐
│ 4 bytes: len │ JSON payload (len bytes) │
└──────────────┴──────────────────────────┘
  big-endian     UTF-8, Compact format
  uint32_t
```

### 消息类型

#### 客户端 → 服务端

| type | 字段 | 说明 |
|------|------|------|
| `register` | username, password | 注册新用户 |
| `login` | username, password | 登录 |
| `msg` | to, content | 发送私聊消息 |
| `list` | — | 获取在线用户列表 |
| `history` | with | 获取与某人的聊天记录 |
| `add_contact` | contact | 添加联系人 |
| `get_contacts` | — | 获取联系人列表 |
| `admin_users` | — | 管理员获取所有用户 |

#### 服务端 → 客户端

| type | 字段 | 说明 |
|------|------|------|
| `login_ok` | username, role | 登录成功 |
| `register_ok` | — | 注册成功 |
| `error` | msg | 错误信息 |
| `msg` | from, to, content, time | 消息推送/回执 |
| `user_list` | users[] | 在线用户列表 |
| `contacts` | contacts[] | 联系人列表 |
| `history` | with, messages[] | 聊天记录 |
| `contact_added` | contact | 联系人添加成功 |
| `all_users` | users[] | 所有用户(管理员) |
| `kicked` | — | 被踢下线 |

### 消息示例

```
注册:
  → {"type":"register","username":"alice","password":"123"}

登录:
  → {"type":"login","username":"alice","password":"123"}
  ← {"type":"login_ok","username":"alice","role":"user"}

发送消息:
  → {"type":"msg","to":"bob","content":"Hello!"}
  ← {"type":"msg","from":"alice","to":"bob","content":"Hello!","time":"14:30"}

获取历史:
  → {"type":"history","with":"bob"}
  ← {"type":"history","with":"bob","messages":[
       {"from":"alice","to":"bob","content":"Hi","time":"14:29"},
       {"from":"bob","to":"alice","content":"Hey","time":"14:30"}
     ]}
```

---

## 数据流图

### 登录流程

```
  Client                           Server                  SQLite
    │                                │                       │
    │ connectToHost("127.0.0.1",9000)│                       │
    │───────────────────────────────→│                       │
    │                                │                       │
    │ {"type":"login",               │                       │
    │  "username":"alice",           │                       │
    │  "password":"xxx"}             │                       │
    │───────────────────────────────→│                       │
    │                                │ verifyPassword()      │
    │                                │──────────────────────→│
    │                                │ SELECT password_hash  │
    │                                │←──────────────────────│
    │                                │ SHA256(xxx) == stored?│
    │                                │                       │
    │                                │ 踢掉旧连接 (if any)    │
    │                                │                       │
    │                                │ updateLastLogin()     │
    │                                │──────────────────────→│
    │                                │←──────────────────────│
    │                                │                       │
    │ {"type":"login_ok",            │                       │
    │  "username":"alice",           │                       │
    │  "role":"user"}                │                       │
    │←───────────────────────────────│                       │
    │                                │                       │
    │ {"type":"contacts",            │                       │
    │  "contacts":["bob"]}           │                       │
    │←───────────────────────────────│                       │
```

### 消息发送流程

```
  Alice Client            Server              Bob Client      SQLite
      │                      │                     │            │
      │ {"type":"msg",       │                     │            │
      │  "to":"bob",         │                     │            │
      │  "content":"Hello"}  │                     │            │
      │─────────────────────→│                     │            │
      │                      │ saveMessage()       │            │
      │                      │────────────────────────────────→│
      │                      │←────────────────────────────────│
      │                      │                     │            │
      │                      │ 查找 bob 是否在线    │            │
      │                      │ users_["bob"] = fd  │            │
      │                      │                     │            │
      │                      │ {"type":"msg",      │            │
      │                      │  "from":"alice",    │            │
      │                      │  "content":"Hello", │            │
      │                      │  "time":"14:30"}    │            │
      │                      │────────────────────→│            │
      │                      │                     │            │
      │ {"type":"msg",       │                     │            │
      │  "from":"alice",     │                     │            │
      │  "to":"bob",         │                     │            │
      │  "content":"Hello",  │                     │            │
      │  "time":"14:30"}     │                     │            │
      │←─────────────────────│                     │            │
      │ (回执)               │                     │            │
```

---

## 状态机

### 服务端连接生命周期

```
                   ┌──────────┐
      accept ─────→│  连接    │
                   └────┬─────┘
                        │ 收到 "register"
                   ┌────▼─────┐
                   │  注册    │──→ 写入 users 表 → 返回 register_ok
                   └──────────┘
                        │ 收到 "login"
                   ┌────▼─────┐
                   │  鉴权    │──→ SHA256 比对 → login_ok / error
                   └────┬─────┘
                        │ 登录成功
                   ┌────▼─────┐
                   │  在线    │──→ msg / list / history / add_contact
                   └────┬─────┘
                        │ EPOLLHUP / 主动断开
                   ┌────▼─────┐
                   │  断开    │──→ users_.erase() → close(fd)
                   └──────────┘
```

### 客户端网络状态

```
                    ┌──────────┐
    启动 ──────────→│ 未连接   │
                    └────┬─────┘
                         │ connectToHost()
                    ┌────▼─────┐
                    │ 连接中   │
                    └────┬─────┘
                         │ connected()
                    ┌────▼─────┐
                    │ 已连接   │──→ login()
                    └────┬─────┘
                         │ login_ok
                    ┌────▼─────┐
                    │ 在线     │──→ sendMessage / requestHistory
                    └────┬─────┘
                         │ kicked / error
                    ┌────▼─────┐
                    │ 断开     │──→ 重新连接
                    └──────────┘
```

---

## 数据库设计

### ER 图

```
  ┌──────────┐       ┌──────────────┐       ┌──────────┐
  │  users   │       │  messages    │       │ contacts │
  ├──────────┤       ├──────────────┤       ├──────────┤
  │ id (PK)  │       │ id (PK)      │       │ username │
  │ username │──┐    │ from_user    │       │ contact  │
  │ password │  │    │ to_user      │       └──────────┘
  │ role     │  │    │ content      │
  │ last_login│ │    │ time         │
  └──────────┘  │    └──────────────┘
                │
                │  (username 关联 from_user / to_user)
                └────────────────────────────┘
```

### DDL

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,      -- SHA256 哈希
    role TEXT DEFAULT 'user',        -- 'admin' | 'user'
    last_login TEXT DEFAULT ''       -- ISO 时间戳
);

CREATE TABLE messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    from_user TEXT NOT NULL,
    to_user TEXT NOT NULL,
    content TEXT NOT NULL,
    time TEXT NOT NULL               -- "HH:MM" 格式
);

CREATE TABLE contacts (
    username TEXT NOT NULL,
    contact_username TEXT NOT NULL,
    UNIQUE(username, contact_username)
);
```

### 索引策略

```sql
-- messages 按对话查询 (主要查询)
-- 不做显式索引，SQLite 会在首次查询时自动创建

-- 聊天记录查询:
SELECT * FROM messages
WHERE (from_user=? AND to_user=?)
   OR (from_user=? AND to_user=?)
ORDER BY id DESC LIMIT 50;
```

---

## 组件说明

### 服务端

```
chat-server
├── ChatServer    主服务器类
│   ├── epoll 事件循环
│   ├── 客户端连接管理 (fd → Client)
│   ├── 用户在线映射 (username → fd)
│   ├── JSON 协议解析/构建
│   └── 消息路由 (在线直发 / 离线缓存)
│
├── ChatDB        数据库层
│   ├── registerUser()    插入 users 表
│   ├── verifyPassword()  SHA256 比对
│   ├── saveMessage()     插入 messages 表
│   ├── getHistory()      查询聊天记录
│   ├── addContact()      插入 contacts 表
│   ├── getContacts()     查询联系人
│   └── getAllUsers()     管理员查所有用户
│
├── sha256        自包含 SHA256 实现
└── sqlite3       SQLite 合并源码 (C)
```

### 客户端

```
wechat-client
├── NetworkClient  网络层
│   ├── QTcpSocket 连接管理
│   ├── 4字节长度前缀 + JSON 协议
│   ├── 粘包处理 (readBuffer 累积)
│   └── 信号: loginOk / messageReceived / contactsReceived ...
│
├── LoginWindow    登录/注册窗口
│   ├── QStackedWidget 切换登录/注册页
│   ├── QCheckBox 记住登录 (QSettings)
│   └── WeChat 绿色主题 (#07C160)
│
├── MainWindow     主聊天窗口
│   ├── 左侧: ChatList (深色边栏 #2E2E2E)
│   │   ├── 联系人搜索框
│   │   ├── 添加联系人 (+)
│   │   └── QListWidget 联系人列表
│   └── 右侧: ChatView (浅色聊天区 #F5F5F5)
│       ├── 标题栏 (联系人名称)
│       ├── 消息气泡 (自己绿色 #95EC69, 对方白色)
│       ├── QScrollArea 滚动消息
│       └── 输入框 + 发送按钮
│
└── ChatView       聊天视图
    ├── setPeer()        切换聊天对象
    ├── addMessage()     添加消息气泡
    ├── addBubble()      创建气泡 Widget
    ├── clearMessages()  清空消息
    └── onSend()         发送消息
```

---

## 项目结构

```
chat-system/
├── README.md
├── server/
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── server.h              # ChatServer 声明
│   │   ├── db.h                  # ChatDB 声明
│   │   ├── sha256.h              # SHA256 声明
│   │   └── sqlite3.h             # SQLite 头文件
│   └── src/
│       ├── main.cpp              # 服务端入口
│       ├── server.cpp            # ChatServer 实现
│       ├── db.cpp                # ChatDB 实现
│       ├── sha256.cpp            # SHA256 实现
│       └── sqlite3.c             # SQLite 合并源码
│
└── client/
    ├── CMakeLists.txt
    └── src/
        ├── main.cpp              # 客户端入口
        ├── networkclient.h/cpp   # 网络协议层
        ├── loginwindow.h/cpp     # 登录/注册窗口
        ├── mainwindow.h/cpp      # 主聊天窗口
        ├── chatlist.h/cpp        # 联系人列表 (深色侧栏)
        └── chatview.h/cpp        # 聊天视图 (消息气泡)
```
