# chat-system — 分布式聊天系统

TCP 聊天服务端 + WeChat 风格 Qt6 客户端。

## 功能

- 注册 / 登录 (SHA256 密码哈希)
- 私聊消息（实时推送 + 离线缓存）
- 聊天记录持久化 (SQLite)
- 联系人管理（添加/删除）
- 管理员角色（查看所有用户）
- 记住上次登录账号

## 架构

```
┌─────────────────┐     TCP/JSON      ┌─────────────────────┐
│  chat-server     │◄────────────────►│  wechat-client (Qt6) │
│  epoll + SQLite  │                   │  深色侧边栏 + 气泡   │
└─────────────────┘                   └─────────────────────┘

协议: 4字节长度前缀 + JSON
```

## 数据库

```sql
users(id, username, password_hash, role, last_login)
messages(id, from_user, to_user, content, time)
contacts(username, contact_username)
```

默认管理员: admin / admin

## 构建

```bash
# 服务端
cd server && mkdir build && cd build
cmake .. && make -j$(nproc)
./chat-server 9000

# 客户端
cd client && mkdir build && cd build
cmake .. && make -j$(nproc)
./wechat-client
```

## 客户端界面

```
┌────────────┬──────────────────────────┐
│ 深色侧边栏  │  浅色聊天区               │
│ [#2E2E2E]  │  [#F5F5F5]               │
│            │                          │
│ 添加联系人  │  对方: 白色气泡            │
│ 搜索       │  自己: #95EC69 绿色气泡    │
│ Alice      │                          │
│ Bob        │  ┌──────────────────┐    │
│            │  │ 输入消息    [发送] │    │
│            │  └──────────────────┘    │
└────────────┴──────────────────────────┘
```
