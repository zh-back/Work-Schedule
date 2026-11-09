# Schedule & WorkLog (Apifox Edition - C Core)

类似于 **Apifox** 体验的本地优先（Local-First）团队与个人日程记录与周总结桌面软件。

---

## 🌟 核心特色

1. **极致轻量、毫秒启动**：
   - 底层 100% 采用 **C 语言** (C99/C11) 与原生嵌入的 **SQLite3** 存储引擎编写。
   - 内存占用仅约 10MB，无需笨重庞大的 Node.js/Python 运行时。
2. **Apifox 级现代高质感交互**：
   - 经典左侧树形项目导航，右侧工作台双栏流转（今日待办 + 今日已完成时间轴）。
3. **极低摩擦的快速记录**：
   - 顶部单输入框，随手输入内容（支持 `#标签` 快速解析），按 **Enter** 即刻打上精确时间戳完成记录。
4. **自主可见性权限控制 (Privacy Control)**：
   - 记录时可自主切换为「🔒 仅自己可见 (私密)」或「🌐 项目公开」。
   - 私密条目在底层数据层严密防护，生成团队全员公开周报时物理过滤，绝不泄漏。
5. **周总结与周报一键提炼**：
   - 个人全项目周总结：一网打尽本周在所有项目完成的全部工作（含私密备忘）。
   - 项目团队全员周报：按成员自动分块聚合，支持一键复制标准格式 Markdown 周报。
6. **两阶段演进架构**：
   - Phase 1：纯单机离线运行，主键采用 UUIDv4 + 软删除 + 更新时间戳。
   - Phase 2：底层数据无缝兼容未来在线云端多端同步升级。

---

## 🛠️ 构建与运行

系统内置全自动编译脚本 `build.bat`（采用自带的轻量编译器工具链）：

### 1. 运行全套单元测试 (TDD)
```bash
build.bat test
```
将自动编译并运行 5 组针对通用工具、SQLite 数据库、项目管理、日程流转、周报聚合与本地 API 服务的自动化测试。

### 2. 编译独立可执行程序
```bash
build.bat
```
编译成功后，将在当前目录生成小巧干净的 `schedule_worklog.exe`。

### 3. 编译并直接运行
```bash
build.bat run
```
程序启动后会自动调起浏览器窗口进入 Apifox 风格工作台。

---

## 📁 目录结构

```
D:\schedule-worklog-c\
├── include/                # C 语言头文件
│   ├── common.h            # UUIDv4 与时间戳工具定义
│   ├── db.h                # SQLite 数据库与建表迁移定义
│   ├── project.h           # 项目与成员实体与接口
│   ├── item.h              # 待办/已办任务模型与流转接口
│   ├── summary.h           # 周总结与周报聚合引擎
│   ├── server.h            # 轻量嵌入式 HTTP API 服务
│   └── sqlite3.h           # SQLite3 官方头文件
├── src/                    # C 语言核心源码
│   ├── main.c              # 桌面宿主主入口
│   ├── common.c            # UUID 与时间工具实现
│   ├── db.c                # SQLite 数据层实现
│   ├── project.c           # 项目与成员业务逻辑
│   ├── item.c              # 快速记录与状态流转
│   ├── summary.c           # 周报聚合引擎与隐私过滤器
│   ├── server.c            # 本地 Winsock HTTP/JSON 服务
│   └── sqlite3.c           # SQLite3 官方 Amalgamation 源码
├── web/                    # Apifox 风格现代前端
│   ├── index.html          # 主界面 HTML
│   ├── css/
│   │   └── apifox-theme.css# Apifox 配色与布局样式
│   └── js/
│       └── app.js          # 前端控制器与 API 异步交互
├── tests/                  # 自动化测试用例
│   ├── test_framework.h    # 极简轻量 C 单元测试断言库
│   ├── test_common.c
│   ├── test_project.c
│   ├── test_item.c
│   ├── test_summary.c
│   └── test_server.c
├── docs/superpowers/
│   ├── specs/              # 产品设计规范文档
│   └── plans/              # 模块实施计划文档
├── build.bat               # 自动化构建与测试脚本
└── README.md
```
