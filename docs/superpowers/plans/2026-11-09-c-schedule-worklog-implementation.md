# C 语言本地优先日程与工作记录软件 (Schedule & WorkLog) 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 构建一个基于 C 语言核心与现代 Apifox 质感 UI 的本地优先（Local-First）多项目日程记录与周总结桌面软件。

**Architecture:** 核心采用纯 C 语言（C99/C11）与嵌入式 SQLite3 引擎构建高性能本地数据层与业务引擎（项目/成员管理、极速记录、私密过滤、周报聚合）；通过轻量内置本地服务端与 Windows 原生 WebView2/Web 前端桥接，呈现媲美 Apifox 的高质感深浅色工作台。

**Tech Stack:** C (C99/C11), SQLite3 (`sqlite3.c`), Mongoose / micro-httpd (轻量嵌入式 C 服务), HTML5/CSS3/JavaScript (Apifox 风格现代交互), CMake / GCC / Clang / MSVC.

**Spec:** `docs/superpowers/specs/2026-11-09-schedule-worklog-design.md`

## Global Constraints

- 100% 本地单机运行，零外部云端依赖，启动即用。
- 实体主键统一采用 UUIDv4 字符串，包含 `created_at`, `updated_at`, `deleted_at` 软删除字段，确保未来平滑演进到在线多端同步。
- 权限隔离必须在底层 C 数据查询层物理执行：生成公开周报或项目全员视图时，必须过滤 `visibility = 'private'`。
- 支持 Windows 原生开箱即用，极速编译并可生成独立的执行文件。

## Review Focus

1. **私密项泄漏边界测试**：生成项目全员周报或导出团队总结时，标记为私密的记录项必须 100% 被过滤，绝不能出现在文本或 Markdown 导出中。
2. **待办转已办时间戳准确性**：跨天未完成的待办项在勾选完成时，必须记录勾选当下的实际完成时间戳，而不是创建时间。
3. **中文输入与 UTF-8 编码完整性**：任务描述包含中文、特殊符号及 `#标签` 时，SQLite 存储、C 语言提取与前端展示不能出现乱码。
4. **单机多项目成员切换边界**：切换当前登录成员身份时，待办列表和“我的已完成”必须即时按成员严格过滤更新。
5. **数据库零配置初始化**：首次运行无本地数据库文件时，系统自动完成 SQLite schema 建表并初始化「个人工作」默认项目与默认成员。

---

### Task 1: 工程骨架与 C 单元测试框架搭设

**Files:**
- Create: `include/common.h`
- Create: `src/common.c`
- Create: `tests/test_framework.h`
- Create: `tests/test_common.c`
- Create: `CMakeLists.txt`
- Create: `build.bat`

**Interfaces:**
- Produces:
  - `void generate_uuid_v4(char *out_uuid);`
  - `int64_t get_current_timestamp_ms(void);`
  - `void format_iso8601(int64_t ts_ms, char *out_str, size_t max_len);`

- [ ] **Step 1: 编写测试 common.c 的 UUID 与时间工具测试用例**
- [ ] **Step 2: 运行测试验证失败 (FAIL: function not defined)**
- [ ] **Step 3: 实现 `common.c` 中的 UUID 生成与时间戳格式化工具**
- [ ] **Step 4: 运行测试验证通过 (PASS)**
- [ ] **Step 5: 提交 Task 1 代码**

---

### Task 2: SQLite3 数据库驱动嵌入与 Schema 初始化

**Files:**
- Create: `include/db.h`
- Create: `src/db.c`
- Create: `src/sqlite3.c` (或系统/库提供的 sqlite3 引用)
- Create: `include/sqlite3.h`
- Test: `tests/test_db.c`

**Interfaces:**
- Consumes: `common.h`
- Produces:
  - `int db_init(const char *db_path);`
  - `void db_close(void);`
  - `sqlite3* db_get_handle(void);`
  - `int db_exec_migration(void);`

- [ ] **Step 1: 编写数据库初始化、表结构创建（Users, Projects, Members, ScheduleItems）的失败测试**
- [ ] **Step 2: 运行测试验证失败**
- [ ] **Step 3: 实现 `db.c` 中的数据库连接管理与 DDL 迁移逻辑**
- [ ] **Step 4: 运行测试验证建表成功且自检通过 (PASS)**
- [ ] **Step 5: 提交 Task 2 代码**

---

### Task 3: 项目（Project）与成员（Member）管理模块

**Files:**
- Create: `include/project.h`
- Create: `src/project.c`
- Test: `tests/test_project.c`

**Interfaces:**
- Consumes: `db.h`, `common.h`
- Produces:
  - `int project_create(const char *name, const char *desc, char *out_project_id);`
  - `int project_list(ProjectList *out_list);`
  - `int member_add(const char *project_id, const char *user_name, const char *role, char *out_user_id);`
  - `int member_list_by_project(const char *project_id, MemberList *out_list);`
  - `int user_set_current(const char *user_id);`
  - `int user_get_current(User *out_user);`

- [ ] **Step 1: 编写创建项目、添加成员、切换当前登录人的失败测试**
- [ ] **Step 2: 运行测试验证失败**
- [ ] **Step 3: 实现 `project.c` 业务逻辑（含默认“个人工作”项目自动初始化）**
- [ ] **Step 4: 运行测试验证项目与成员增删查逻辑全部通过 (PASS)**
- [ ] **Step 5: 提交 Task 3 代码**

---

### Task 4: 日程记录（ScheduleItem）与待办-已办流转引擎

**Files:**
- Create: `include/item.h`
- Create: `src/item.c`
- Test: `tests/test_item.c`

**Interfaces:**
- Consumes: `db.h`, `project.h`, `common.h`
- Produces:
  - `int item_create_done(const char *project_id, const char *user_id, const char *content, int is_private, char *out_item_id);`
  - `int item_create_todo(const char *project_id, const char *user_id, const char *content, int is_private, char *out_item_id);`
  - `int item_mark_done(const char *item_id);`
  - `int item_list_active_todos(const char *project_id, const char *user_id, ItemList *out_list);`
  - `int item_list_today_dones(const char *project_id, const char *user_id, ItemList *out_list);`

- [ ] **Step 1: 编写极速录入已完成项、待办转已办（验证自动打当前完成时间戳）及私密标志判定的失败测试**
- [ ] **Step 2: 运行测试验证失败**
- [ ] **Step 3: 实现 `item.c` 的状态流转与标签 `#tag` 提取逻辑**
- [ ] **Step 4: 运行测试验证通过 (PASS)**
- [ ] **Step 5: 提交 Task 4 代码**

---

### Task 5: 周总结（Weekly Summary）自动化聚合与报表生成引擎

**Files:**
- Create: `include/summary.h`
- Create: `src/summary.c`
- Test: `tests/test_summary.c`

**Interfaces:**
- Consumes: `item.h`, `project.h`, `db.h`
- Produces:
  - `int summary_generate_personal(const char *user_id, int year, int week_num, char **out_markdown);`
  - `int summary_generate_project(const char *project_id, int year, int week_num, char **out_markdown);`

- [ ] **Step 1: 编写周报聚合测试（重点验证按项目分类、按成员聚合、以及私密项在公开周报中被严格过滤）**
- [ ] **Step 2: 运行测试验证失败**
- [ ] **Step 3: 实现 `summary.c` 中的周时间区间计算与结构化 Markdown 字符串生成**
- [ ] **Step 4: 运行测试验证通过 (PASS)**
- [ ] **Step 5: 提交 Task 5 代码**

---

### Task 6: 本地轻量 API 服务与 IPC 数据通道

**Files:**
- Create: `include/server.h`
- Create: `src/server.c`
- Test: `tests/test_server.c`

**Interfaces:**
- Consumes: `project.h`, `item.h`, `summary.h`
- Produces:
  - `int server_start(int port);`
  - `void server_stop(void);`
  - 路由映射：
    - `GET  /api/projects`
    - `POST /api/projects`
    - `GET  /api/items` (query: `project_id`, `status`, `user_id`)
    - `POST /api/items`
    - `POST /api/items/:id/done`
    - `GET  /api/summary` (query: `type=personal|project`, `week=...`)

- [ ] **Step 1: 编写针对本地 REST API 路由响应的集成测试**
- [ ] **Step 2: 运行测试验证失败**
- [ ] **Step 3: 实现 `server.c` 轻量 HTTP/JSON 处理与路由分发**
- [ ] **Step 4: 运行测试验证接口返回标准 JSON 数据 (PASS)**
- [ ] **Step 5: 提交 Task 6 代码**

---

### Task 7: Apifox 风格现代前端交互界面与主题系统

**Files:**
- Create: `web/index.html`
- Create: `web/css/apifox-theme.css`
- Create: `web/js/app.js`

**Features:**
- 左侧树形导航：项目列表、+新建项目、成员卡片、视图切换（我的工作台 / 项目全员大盘 / 周报导出）。
- 顶部高频输入栏：文本框 + `#标签` 快速解析 + 私密切换器（🔒私密 / 🌐公开）+ 回车极速完成。
- 双栏流水看板：左栏今日待办（勾选动效流转），右栏带时间戳的今日已完成流水。
- 周报聚合弹窗/面板：支持查看 Markdown 预览，一键复制到剪贴板。
- 深色模式与浅色模式无缝切换。

- [ ] **Step 1: 搭建 Apifox 配色与响应式布局框架 (`web/index.html`, `web/css/apifox-theme.css`)**
- [ ] **Step 2: 编写 `web/js/app.js` 驱动与 `/api/` 的无感异步交互**
- [ ] **Step 3: 浏览器与原生 WebView 渲染联调验证**
- [ ] **Step 4: 提交 Task 7 前端代码**

---

### Task 8: 桌面宿主启动器与整体验收

**Files:**
- Create: `src/main.c`
- Create: `README.md`

**Workflow:**
- `main.c` 启动流程：
  1. 初始化本地 SQLite 数据库。
  2. 启动本地轻量 C 服务（绑定回环地址 127.0.0.1 随机端口或固定端口）。
  3. 调用 Windows 原生 WebView2（或启动默认浏览器全屏轻量窗口）加载 UI。
  4. 监听退出事件，释放资源平稳关闭。

- [ ] **Step 1: 实现 `src/main.c` 宿主主循环与优雅停机**
- [ ] **Step 2: 编写编译打包脚本 `build.bat` 生成独立执行文件**
- [ ] **Step 3: 端到端功能全覆盖测试（新建项目 -> 记录日程 -> 待办转已办 -> 私密保护 -> 一键生成周报）**
- [ ] **Step 4: 提交 Task 8 完整工程代码**
