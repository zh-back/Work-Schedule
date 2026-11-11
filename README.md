# Schedule & WorkLog (Apifox Edition - C Core)

类似于 **Apifox** 体验的本地优先（Local-First）团队与个人日程记录与周总结桌面软件。

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
