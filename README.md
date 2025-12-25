# BYD 汽车信息查询系统

一个基于 C++ 实现的 BYD 汽车知识图谱构建与查询系统，包含 Web 可视化界面和 CLI 终端版本。

## ✨ 功能特性

- **知识图谱构建**：使用链表 + 邻接表实现图结构，支持系列、车型、技术三层关联
- **多种查询方式**：支持按系列筛选、关键词搜索、能源类型过滤
- **数据可视化**：基于 ECharts 的关系图谱展示，支持全局视图、表关联、树状图三种模式
- **双版本支持**：
  - **Web 版**：现代化 UI，支持图谱可视化、筛选、搜索、数据管理
  - **CLI 版**：ASCII 图表展示，适合终端快速查询

## 📁 项目结构

```
├── data/
│   ├── byd_cli_data.txt    # CLI 版数据文件
│   └── byd_web_data.txt    # Web 版数据文件
├── src/
│   ├── main.cpp            # Web 服务端 (HTTP API + 静态文件服务)
│   ├── byd_cli.cpp         # CLI 终端版本
│   └── httplib.h           # cpp-httplib (header-only HTTP 库)
└── web/
    ├── index.html          # 前端页面
    ├── styles.css          # 样式文件
    └── app.js              # 前端逻辑
```

## 🛠️ 技术实现

### 数据结构

- **链表**：存储系列、技术、车型数据，支持动态增删
- **邻接表**：实现知识图谱，支持 BFS/DFS 遍历
- **关系模型**：
  - `Series` (系列) → `Model` (车型) → `Tech` (技术)
  - 支持外键约束、唯一约束、非空约束校验

### 后端 (C++)

- 使用 [cpp-httplib](https://github.com/yhirose/cpp-httplib) 提供 RESTful API
- 支持 CORS 跨域访问
- 线程安全的数据管理

### 前端

- 原生 JavaScript，无框架依赖
- ECharts 图表可视化
- 响应式布局

## 🚀 快速开始

### 编译运行

**Windows (MSVC / MinGW)**

```bash
# 编译 Web 服务端
g++ -std=c++17 -O2 -pthread -o byd_server.exe src/main.cpp -lws2_32

# 编译 CLI 版本
g++ -o byd_cli.exe byd_cli.cpp -std=c++17 -static -static-libgcc -static-libstdc++
```

**Linux / macOS**

```bash
g++ -std=c++17 -O2 -pthread -o byd_server src/main.cpp
g++ -std=c++17 -O2 -o byd_cli src/byd_cli.cpp
```

### 运行

```bash
# 启动 Web 服务
./byd_server
# 浏览器访问 http://localhost:8080

# 运行 CLI 版本
./byd_cli
```

> ⚠️ 注意：cpp-httplib 不支持 32 位 Windows，请使用 x64 编译。

## 📖 使用说明

### Web 版功能

| 功能 | 说明 |
|------|------|
| 车型列表 | 查看所有车型，支持按系列/能源类型筛选 |
| 搜索 | 支持车型名、系列名、技术名模糊搜索 |
| 详情面板 | 点击车型查看详细信息和搭载技术 |
| 关系图谱 | 可视化展示系列-车型-技术关联关系 |
| 数据管理 | 支持添加新车型和技术 |


###web版本预览
<img width="2550" height="1327" alt="image" src="https://github.com/user-attachments/assets/c6776e33-623f-4300-9340-4203b84d5be7" />

### CLI 版功能

```
1. 查看所有车型
2. 查看系列列表
3. 查看技术列表
4. 查看车型详情
5. 按系列筛选
6. 搜索车型
7. 显示关系图谱 (ASCII)
8. 添加新车型
9. 查看统计信息
0. 退出
```
###CLI版本预览
<img width="1366" height="804" alt="image" src="https://github.com/user-attachments/assets/af44de06-7d45-434e-8ee0-52b167ad39b1" />

## 📊 API 接口

| 接口 | 方法 | 说明 |
|------|------|------|
| `/api/series` | GET | 获取所有系列 |
| `/api/techs` | GET | 获取所有技术 |
| `/api/models` | GET | 获取车型列表 (支持 `series_id`, `energy_type` 筛选) |
| `/api/model?id=` | GET | 获取单个车型详情 |
| `/api/search?q=` | GET | 搜索车型 |
| `/api/stats` | GET | 获取统计信息 |
| `/api/graph` | GET | 获取关系图数据 |
| `/api/model/add` | POST | 添加新车型 |

## 📝 数据格式

数据文件采用分段 TXT 格式：

```
[SERIES]
1,王朝系列,传承中华文化的经典系列

[TECH]
101,DM-i超级混动,高效节能的插电混动技术

[MODEL]
1001,秦PLUS,1,9.98,120,PHEV,轿车,5,2023
```



