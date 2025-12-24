/**
 * BYD 汽车信息查询系统 - CLI 终端版本
 * 使用 ASCII 字符在终端中显示图表
 * 使用链表和邻接表实现知识图谱
 * 
 * 编译命令: g++ -o byd_cli.exe byd_cli.cpp -std=c++17 -static -static-libgcc -static-libstdc++
 * 运行命令: ./byd_cli.exe
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// =============================
// 链表节点定义
// =============================

// 通用链表节点模板
template<typename T>
struct ListNode {
    T data;
    ListNode* next;
    
    ListNode(const T& d) : data(d), next(nullptr) {}
};

// 链表类模板
template<typename T>
class LinkedList {
private:
    ListNode<T>* head;
    ListNode<T>* tail;
    int count;
    
public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}
    
    ~LinkedList() {
        clear();
    }
    
    // 在尾部添加元素
    void append(const T& data) {
        ListNode<T>* newNode = new ListNode<T>(data);
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        count++;
    }
    
    // 清空链表
    void clear() {
        ListNode<T>* current = head;
        while (current) {
            ListNode<T>* next = current->next;
            delete current;
            current = next;
        }
        head = tail = nullptr;
        count = 0;
    }
    
    // 获取链表大小
    int size() const { return count; }
    
    // 检查链表是否为空
    bool empty() const { return count == 0; }
    
    // 根据ID查找元素（假设T有id成员）
    T* findById(int id) {
        ListNode<T>* current = head;
        while (current) {
            if (current->data.id == id) {
                return &(current->data);
            }
            current = current->next;
        }
        return nullptr;
    }
    
    // 迭代器支持
    class Iterator {
    private:
        ListNode<T>* current;
    public:
        Iterator(ListNode<T>* node) : current(node) {}
        T& operator*() { return current->data; }
        T* operator->() { return &(current->data); }
        Iterator& operator++() { current = current->next; return *this; }
        bool operator!=(const Iterator& other) const { return current != other.current; }
    };
    
    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }
    
    // const迭代器支持
    class ConstIterator {
    private:
        const ListNode<T>* current;
    public:
        ConstIterator(const ListNode<T>* node) : current(node) {}
        const T& operator*() const { return current->data; }
        const T* operator->() const { return &(current->data); }
        ConstIterator& operator++() { current = current->next; return *this; }
        bool operator!=(const ConstIterator& other) const { return current != other.current; }
    };
    
    ConstIterator begin() const { return ConstIterator(head); }
    ConstIterator end() const { return ConstIterator(nullptr); }
};

// =============================
// 数据结构定义
// =============================

struct Series {
    int id;
    string name;
    string intro;
};

struct Tech {
    int id;
    string name;
    string intro;
};

struct Model {
    int id;
    string name;
    int series_id;
    double price;
    double range_km;
    string energy_type;
    string body_type;
    int seats;
    string launch_year;
    vector<int> tech_ids;
};

// =============================
// 邻接表实现的知识图谱
// =============================

// 图中节点的类型
enum class NodeType {
    BRAND,      // 品牌节点
    SERIES,     // 系列节点
    MODEL,      // 车型节点
    TECH        // 技术节点
};

// 边的类型（关系类型）
enum class EdgeType {
    HAS_SERIES,     // 品牌->系列
    BELONGS_TO,     // 车型->系列
    USES_TECH       // 车型->技术
};

// 图节点
struct GraphNode {
    int id;
    NodeType type;
    string name;
    
    GraphNode() : id(0), type(NodeType::MODEL) {}
    GraphNode(int _id, NodeType _type, const string& _name) 
        : id(_id), type(_type), name(_name) {}
};

// 邻接表的边节点（链表节点）
struct EdgeNode {
    int destId;         // 目标节点ID
    EdgeType edgeType;  // 边的类型
    EdgeNode* next;     // 下一条边
    
    EdgeNode(int dest, EdgeType type) : destId(dest), edgeType(type), next(nullptr) {}
};

// 邻接表节点（顶点 + 边链表）
struct AdjListNode {
    GraphNode vertex;       // 顶点信息
    EdgeNode* edgeHead;     // 边链表头
    AdjListNode* next;      // 下一个邻接表节点
    
    AdjListNode(const GraphNode& v) : vertex(v), edgeHead(nullptr), next(nullptr) {}
    
    // 添加一条边
    void addEdge(int destId, EdgeType type) {
        EdgeNode* newEdge = new EdgeNode(destId, type);
        newEdge->next = edgeHead;
        edgeHead = newEdge;
    }
    
    // 获取所有邻接节点ID
    vector<int> getNeighbors() const {
        vector<int> neighbors;
        EdgeNode* current = edgeHead;
        while (current) {
            neighbors.push_back(current->destId);
            current = current->next;
        }
        return neighbors;
    }
    
    // 获取特定类型的邻接节点ID
    vector<int> getNeighborsByType(EdgeType type) const {
        vector<int> neighbors;
        EdgeNode* current = edgeHead;
        while (current) {
            if (current->edgeType == type) {
                neighbors.push_back(current->destId);
            }
            current = current->next;
        }
        return neighbors;
    }
    
    // 析构函数，释放边链表
    ~AdjListNode() {
        EdgeNode* current = edgeHead;
        while (current) {
            EdgeNode* next = current->next;
            delete current;
            current = next;
        }
    }
};

// 基于邻接表的图类
class KnowledgeGraph {
private:
    AdjListNode* adjListHead;   // 邻接表链表头
    int nodeCount;              // 节点数量
    int edgeCount;              // 边数量
    
public:
    KnowledgeGraph() : adjListHead(nullptr), nodeCount(0), edgeCount(0) {}
    
    ~KnowledgeGraph() {
        clear();
    }
    
    // 清空图
    void clear() {
        AdjListNode* current = adjListHead;
        while (current) {
            AdjListNode* next = current->next;
            delete current;
            current = next;
        }
        adjListHead = nullptr;
        nodeCount = 0;
        edgeCount = 0;
    }
    
    // 添加节点
    void addNode(const GraphNode& node) {
        AdjListNode* newNode = new AdjListNode(node);
        newNode->next = adjListHead;
        adjListHead = newNode;
        nodeCount++;
    }
    
    // 查找邻接表节点
    AdjListNode* findAdjNode(int nodeId) {
        AdjListNode* current = adjListHead;
        while (current) {
            if (current->vertex.id == nodeId) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }
    
    // 添加边
    void addEdge(int srcId, int destId, EdgeType type) {
        AdjListNode* srcNode = findAdjNode(srcId);
        if (srcNode) {
            srcNode->addEdge(destId, type);
            edgeCount++;
        }
    }
    
    // 获取节点数量
    int getNodeCount() const { return nodeCount; }
    
    // 获取边数量
    int getEdgeCount() const { return edgeCount; }
    
    // 获取节点的所有邻居
    vector<int> getNeighbors(int nodeId) {
        AdjListNode* node = findAdjNode(nodeId);
        if (node) {
            return node->getNeighbors();
        }
        return vector<int>();
    }
    
    // 获取特定关系类型的邻居
    vector<int> getNeighborsByType(int nodeId, EdgeType type) {
        AdjListNode* node = findAdjNode(nodeId);
        if (node) {
            return node->getNeighborsByType(type);
        }
        return vector<int>();
    }
    
    // BFS遍历（用于查找路径）
    vector<int> bfsTraversal(int startId) {
        vector<int> visited;
        vector<int> queue;
        set<int> visitedSet;
        
        queue.push_back(startId);
        visitedSet.insert(startId);
        
        while (!queue.empty()) {
            int current = queue.front();
            queue.erase(queue.begin());
            visited.push_back(current);
            
            vector<int> neighbors = getNeighbors(current);
            for (int neighbor : neighbors) {
                if (visitedSet.find(neighbor) == visitedSet.end()) {
                    visitedSet.insert(neighbor);
                    queue.push_back(neighbor);
                }
            }
        }
        
        return visited;
    }
    
    // DFS遍历
    void dfsHelper(int nodeId, set<int>& visitedSet, vector<int>& result) {
        visitedSet.insert(nodeId);
        result.push_back(nodeId);
        
        vector<int> neighbors = getNeighbors(nodeId);
        for (int neighbor : neighbors) {
            if (visitedSet.find(neighbor) == visitedSet.end()) {
                dfsHelper(neighbor, visitedSet, result);
            }
        }
    }
    
    vector<int> dfsTraversal(int startId) {
        vector<int> result;
        set<int> visitedSet;
        dfsHelper(startId, visitedSet, result);
        return result;
    }
    
    // 获取节点信息
    GraphNode* getNode(int nodeId) {
        AdjListNode* node = findAdjNode(nodeId);
        if (node) {
            return &(node->vertex);
        }
        return nullptr;
    }
    
    // 遍历所有节点
    class Iterator {
    private:
        AdjListNode* current;
    public:
        Iterator(AdjListNode* node) : current(node) {}
        GraphNode& operator*() { return current->vertex; }
        Iterator& operator++() { current = current->next; return *this; }
        bool operator!=(const Iterator& other) const { return current != other.current; }
    };
    
    Iterator begin() { return Iterator(adjListHead); }
    Iterator end() { return Iterator(nullptr); }
};

// =============================
// 全局数据存储（使用链表）
// =============================

LinkedList<Series> g_series;
LinkedList<Tech> g_techs;
LinkedList<Model> g_models;

// 知识图谱
KnowledgeGraph g_graph;

// =============================
// 文件读写功能
// =============================

const string DATA_FILE = "../data/byd_cli_data.txt";

// 前向声明
void buildKnowledgeGraph();

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// 分割字符串
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    stringstream ss(s);
    string token;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

// 从文件加载数据
bool loadData() {
    ifstream file(DATA_FILE);
    if (!file.is_open()) {
        cerr << "  警告: 无法打开数据文件 " << DATA_FILE << "\n";
        return false;
    }
    
    g_series.clear();
    g_techs.clear();
    g_models.clear();
    g_graph.clear();
    
    string line;
    string currentSection;
    
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        if (line == "[SERIES]") {
            currentSection = "SERIES";
            continue;
        } else if (line == "[TECH]") {
            currentSection = "TECH";
            continue;
        } else if (line == "[MODEL]") {
            currentSection = "MODEL";
            continue;
        }
        
        vector<string> parts = split(line, ',');
        
        if (currentSection == "SERIES" && parts.size() >= 3) {
            Series s;
            s.id = stoi(parts[0]);
            s.name = parts[1];
            s.intro = parts[2];
            g_series.append(s);
        }
        else if (currentSection == "TECH" && parts.size() >= 3) {
            Tech t;
            t.id = stoi(parts[0]);
            t.name = parts[1];
            t.intro = parts[2];
            g_techs.append(t);
        }
        else if (currentSection == "MODEL" && parts.size() >= 10) {
            Model m;
            m.id = stoi(parts[0]);
            m.name = parts[1];
            m.series_id = stoi(parts[2]);
            m.price = stod(parts[3]);
            m.range_km = stod(parts[4]);
            m.energy_type = parts[5];
            m.body_type = parts[6];
            m.seats = stoi(parts[7]);
            m.launch_year = parts[8];
            
            // 解析技术ID列表 (用|分隔)
            vector<string> techIds = split(parts[9], '|');
            for (const auto& tid : techIds) {
                if (!tid.empty()) {
                    m.tech_ids.push_back(stoi(tid));
                }
            }
            g_models.append(m);
        }
    }
    
    file.close();
    
    // 构建知识图谱（邻接表）
    buildKnowledgeGraph();
    
    return true;
}

// 构建知识图谱（使用邻接表）
void buildKnowledgeGraph() {
    g_graph.clear();
    
    // 添加品牌节点 (ID: 0)
    g_graph.addNode(GraphNode(0, NodeType::BRAND, "BYD 比亚迪"));
    
    // 添加系列节点，并建立品牌->系列的边
    for (const auto& s : g_series) {
        g_graph.addNode(GraphNode(s.id, NodeType::SERIES, s.name));
        g_graph.addEdge(0, s.id, EdgeType::HAS_SERIES);
    }
    
    // 添加技术节点
    for (const auto& t : g_techs) {
        g_graph.addNode(GraphNode(t.id, NodeType::TECH, t.name));
    }
    
    // 添加车型节点，并建立车型->系列、车型->技术的边
    for (const auto& m : g_models) {
        g_graph.addNode(GraphNode(m.id, NodeType::MODEL, m.name));
        g_graph.addEdge(m.id, m.series_id, EdgeType::BELONGS_TO);
        
        // 添加车型到技术的边
        for (int techId : m.tech_ids) {
            g_graph.addEdge(m.id, techId, EdgeType::USES_TECH);
        }
    }
}

// 保存数据到文件
bool saveData() {
    ofstream file(DATA_FILE);
    if (!file.is_open()) {
        cerr << "  错误: 无法写入数据文件 " << DATA_FILE << "\n";
        return false;
    }
    
    file << "# BYD汽车信息系统 - CLI版本数据文件\n";
    file << "# 格式说明：\n";
    file << "# [SERIES] 系列数据: id,名称,简介\n";
    file << "# [TECH] 技术数据: id,名称,简介\n";
    file << "# [MODEL] 车型数据: id,名称,系列id,价格,续航,能源类型,车身类型,座位数,年份,技术id列表(用|分隔)\n\n";
    
    // 写入系列数据（遍历链表）
    file << "[SERIES]\n";
    for (const auto& s : g_series) {
        file << s.id << "," << s.name << "," << s.intro << "\n";
    }
    file << "\n";
    
    // 写入技术数据（遍历链表）
    file << "[TECH]\n";
    for (const auto& t : g_techs) {
        file << t.id << "," << t.name << "," << t.intro << "\n";
    }
    file << "\n";
    
    // 写入车型数据（遍历链表）
    file << "[MODEL]\n";
    for (const auto& m : g_models) {
        file << m.id << "," << m.name << "," << m.series_id << ","
             << fixed << setprecision(2) << m.price << ","
             << (int)m.range_km << "," << m.energy_type << ","
             << m.body_type << "," << m.seats << "," << m.launch_year << ",";
        
        // 写入技术ID列表
        for (size_t i = 0; i < m.tech_ids.size(); i++) {
            if (i > 0) file << "|";
            file << m.tech_ids[i];
        }
        file << "\n";
    }
    
    file.close();
    return true;
}

// 初始化数据 - 从文件加载
void initData() {
    if (!loadData()) {
        cerr << "  数据加载失败，请确保数据文件存在: " << DATA_FILE << "\n";
    }
}

// 辅助函数
// =============================

// 设置控制台编码（Windows）
void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
}

// 清屏
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// 获取字符串显示宽度（处理中文）
int getDisplayWidth(const string& s) {
    int width = 0;
    for (size_t i = 0; i < s.length(); ) {
        unsigned char c = s[i];
        if (c < 0x80) {
            width += 1;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            width += 2;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            width += 2;
            i += 3;
        } else {
            width += 2;
            i += 4;
        }
    }
    return width;
}

// 右填充字符串到指定显示宽度
string padRight(const string& s, int targetWidth) {
    int currentWidth = getDisplayWidth(s);
    int padding = targetWidth - currentWidth;
    if (padding <= 0) return s;
    return s + string(padding, ' ');
}

// 根据ID获取系列名称（链表查找）
string getSeriesName(int series_id) {
    Series* s = g_series.findById(series_id);
    if (s) return s->name;
    return "未知";
}

// 根据ID获取技术名称（链表查找）
string getTechName(int tech_id) {
    Tech* t = g_techs.findById(tech_id);
    if (t) return t->name;
    return "未知";
}

// =============================
// ASCII 图表绘制
// =============================

// 绘制水平分隔线
void drawLine(int width, char c = '-') {
    cout << "+" << string(width - 2, c) << "+" << endl;
}

// 绘制标题框
void drawTitle(const string& title) {
    int width = 60;
    cout << "\n";
    drawLine(width, '=');
    int padding = (width - 2 - getDisplayWidth(title)) / 2;
    cout << "|" << string(padding, ' ') << title << string(width - 2 - padding - getDisplayWidth(title), ' ') << "|" << endl;
    drawLine(width, '=');
}

// 绘制ASCII树状关系图（使用图的BFS遍历）
void drawRelationTree() {
    drawTitle("BYD 产品关系图谱 (ASCII Tree)");
    
    cout << "\n";
    cout << "                           ┌─────────────────────────┐\n";
    cout << "                           │       BYD 比亚迪        │\n";
    cout << "                           └────────────┬────────────┘\n";
    cout << "           ┌───────────────┬────────────┼────────────┬───────────────┐\n";
    cout << "           │               │            │            │               │\n";
    cout << "     ┌─────┴─────┐   ┌─────┴─────┐ ┌────┴────┐ ┌─────┴─────┐  ┌──────┴──────┐\n";
    cout << "     │ 王朝系列  │   │ 海洋系列  │ │腾势系列 │ │ 仰望系列  │  │ 方程豹系列  │\n";
    cout << "     └─────┬─────┘   └─────┬─────┘ └────┬────┘ └─────┬─────┘  └──────┬──────┘\n";
    cout << "           │               │            │            │               │\n";
    
    // 使用图的邻接表统计每个系列下的车型数量
    map<int, int> seriesModelCount;
    for (const auto& m : g_models) {
        // 使用邻接表查找车型所属系列
        vector<int> neighbors = g_graph.getNeighborsByType(m.id, EdgeType::BELONGS_TO);
        if (!neighbors.empty()) {
            seriesModelCount[neighbors[0]]++;
        }
    }
    
    cout << "     ";
    for (int i = 1; i <= 5; i++) {
        cout << "[" << seriesModelCount[i] << " 车型] ";
        if (i < 5) cout << "    ";
    }
    cout << "\n\n";
    
    // 显示技术层
    cout << "    ─────────────────────────────────────────────────────────────────────────\n";
    cout << "                              [ 核心技术平台 ]\n";
    cout << "    ─────────────────────────────────────────────────────────────────────────\n";
    cout << "     DM-i混动 | 刀片电池 | e平台3.0 | 云辇系统 | DiPilot | 易四方\n";
    cout << "    ─────────────────────────────────────────────────────────────────────────\n";
    
    // 显示图的统计信息
    cout << "\n  [知识图谱统计] 节点数: " << g_graph.getNodeCount() 
         << ", 边数: " << g_graph.getEdgeCount() << "\n";
}

// 绘制特定车型的关系树（使用邻接表查询）
void drawModelTree(const Model& model) {
    string seriesName = getSeriesName(model.series_id);
    
    cout << "\n";
    cout << "                    ┌─────────────────────────┐\n";
    cout << "                    │ " << padRight(seriesName, 21) << " │\n";
    cout << "                    └────────────┬────────────┘\n";
    cout << "                                 │\n";
    cout << "                                 ▼\n";
    cout << "                    ┌─────────────────────────┐\n";
    cout << "                    │ " << padRight(model.name, 21) << " │\n";
    cout << "                    │ " << padRight(to_string(model.price) + " 万 | " + model.energy_type, 21) << " │\n";
    cout << "                    └────────────┬────────────┘\n";
    cout << "                                 │\n";
    cout << "         ┌───────────┬──────────┼──────────┬───────────┐\n";
    cout << "         ▼           ▼          ▼          ▼           ▼\n";
    
    // 使用邻接表查询该车型使用的技术
    vector<int> techNeighbors = g_graph.getNeighborsByType(model.id, EdgeType::USES_TECH);
    vector<string> techNames;
    for (int tid : techNeighbors) {
        techNames.push_back(getTechName(tid));
    }
    
    // 最多显示5个技术在一行
    int showCount = min(5, (int)techNames.size());
    cout << "   ";
    for (int i = 0; i < showCount; i++) {
        string tn = techNames[i];
        if (tn.length() > 8) tn = tn.substr(0, 6) + "..";
        cout << "[" << tn << "]";
        if (i < showCount - 1) cout << " ";
    }
    if (techNames.size() > 5) {
        cout << " +" << (techNames.size() - 5) << " more";
    }
    cout << "\n\n";
}

// 绘制表格
void drawTable(const vector<vector<string>>& data, const vector<int>& widths) {
    // 绘制顶部边框
    cout << "+";
    for (int w : widths) cout << string(w + 2, '-') << "+";
    cout << endl;
    
    bool isHeader = true;
    for (const auto& row : data) {
        cout << "|";
        for (size_t i = 0; i < row.size() && i < widths.size(); i++) {
            cout << " " << padRight(row[i], widths[i]) << " |";
        }
        cout << endl;
        
        // 表头后绘制分隔线
        if (isHeader) {
            cout << "+";
            for (int w : widths) cout << string(w + 2, '=') << "+";
            cout << endl;
            isHeader = false;
        }
    }
    
    // 绘制底部边框
    cout << "+";
    for (int w : widths) cout << string(w + 2, '-') << "+";
    cout << endl;
}

// =============================
// 功能函数
// =============================

// 显示所有车型列表（遍历链表）
void showModelList() {
    drawTitle("车型列表");
    
    vector<vector<string>> tableData;
    tableData.push_back({"ID", "车型名称", "系列", "价格(万)", "续航", "能源", "车身"});
    
    // 遍历链表
    for (const auto& m : g_models) {
        tableData.push_back({
            to_string(m.id),
            m.name,
            getSeriesName(m.series_id),
            to_string(m.price).substr(0, to_string(m.price).find('.') + 3),
            to_string((int)m.range_km) + "km",
            m.energy_type,
            m.body_type
        });
    }
    
    drawTable(tableData, {6, 16, 12, 10, 8, 6, 8});
    cout << "\n共 " << g_models.size() << " 款车型\n";
}

// 显示系列列表（遍历链表）
void showSeriesList() {
    drawTitle("系列列表");
    
    vector<vector<string>> tableData;
    tableData.push_back({"ID", "系列名称", "介绍", "车型数"});
    
    // 遍历系列链表
    for (const auto& s : g_series) {
        int count = 0;
        // 遍历车型链表统计
        for (const auto& m : g_models) {
            if (m.series_id == s.id) count++;
        }
        tableData.push_back({
            to_string(s.id),
            s.name,
            s.intro,
            to_string(count)
        });
    }
    
    drawTable(tableData, {4, 14, 26, 8});
}

// 显示技术列表（遍历链表）
void showTechList() {
    drawTitle("核心技术列表");
    
    vector<vector<string>> tableData;
    tableData.push_back({"ID", "技术名称", "介绍"});
    
    // 遍历技术链表
    for (const auto& t : g_techs) {
        tableData.push_back({
            to_string(t.id),
            t.name,
            t.intro
        });
    }
    
    drawTable(tableData, {5, 22, 28});
    cout << "\n共 " << g_techs.size() << " 项核心技术\n";
}

// 显示车型详情（使用链表查找和邻接表查询）
void showModelDetail(int model_id) {
    // 使用链表查找
    Model* found = g_models.findById(model_id);
    
    if (!found) {
        cout << "未找到ID为 " << model_id << " 的车型\n";
        return;
    }
    
    drawTitle("车型详情: " + found->name);
    
    cout << "\n";
    cout << "  ┌────────────────────────────────────────────────────┐\n";
    cout << "  │ 基本信息                                           │\n";
    cout << "  ├────────────────────────────────────────────────────┤\n";
    cout << "  │ 车型名称: " << padRight(found->name, 39) << " │\n";
    cout << "  │ 所属系列: " << padRight(getSeriesName(found->series_id), 39) << " │\n";
    cout << "  │ 指导价格: " << padRight(to_string(found->price) + " 万元", 39) << " │\n";
    cout << "  │ 续航里程: " << padRight(to_string((int)found->range_km) + " km", 39) << " │\n";
    cout << "  │ 能源类型: " << padRight(found->energy_type == "EV" ? "纯电动" : "插电混动", 39) << " │\n";
    cout << "  │ 车身类型: " << padRight(found->body_type, 39) << " │\n";
    cout << "  │ 座位数量: " << padRight(to_string(found->seats) + " 座", 39) << " │\n";
    cout << "  │ 上市年份: " << padRight(found->launch_year + " 年", 39) << " │\n";
    cout << "  └────────────────────────────────────────────────────┘\n";
    
    // 使用邻接表查询搭载的技术
    cout << "\n  搭载技术 (通过邻接表查询):\n";
    vector<int> techNeighbors = g_graph.getNeighborsByType(model_id, EdgeType::USES_TECH);
    for (int tid : techNeighbors) {
        cout << "    • " << getTechName(tid) << "\n";
    }
    
    // 绘制关系树
    cout << "\n  关系结构:\n";
    drawModelTree(*found);
}

// 按系列筛选车型（遍历链表）
void filterBySeries() {
    cout << "\n请选择系列编号:\n";
    // 遍历系列链表
    for (const auto& s : g_series) {
        cout << "  " << s.id << ". " << s.name << "\n";
    }
    cout << "输入编号: ";
    
    int series_id;
    cin >> series_id;
    
    drawTitle("系列筛选: " + getSeriesName(series_id));
    
    vector<vector<string>> tableData;
    tableData.push_back({"ID", "车型名称", "价格(万)", "续航", "能源"});
    
    int count = 0;
    // 遍历车型链表进行筛选
    for (const auto& m : g_models) {
        if (m.series_id == series_id) {
            tableData.push_back({
                to_string(m.id),
                m.name,
                to_string(m.price).substr(0, to_string(m.price).find('.') + 3),
                to_string((int)m.range_km) + "km",
                m.energy_type
            });
            count++;
        }
    }
    
    if (count == 0) {
        cout << "该系列暂无车型\n";
    } else {
        drawTable(tableData, {6, 18, 10, 8, 6});
        cout << "\n共 " << count << " 款车型\n";
    }
}

// 搜索功能（使用链表和邻接表）
void searchModels() {
    cout << "\n请输入搜索关键词: ";
    string keyword;
    cin.ignore();
    getline(cin, keyword);
    
    drawTitle("搜索结果: " + keyword);
    
    vector<vector<string>> tableData;
    tableData.push_back({"ID", "车型名称", "系列", "价格(万)"});
    
    int count = 0;
    // 遍历车型链表进行搜索
    for (const auto& m : g_models) {
        // 搜索车型名、系列名、技术名
        bool match = (m.name.find(keyword) != string::npos);
        if (!match) {
            string sn = getSeriesName(m.series_id);
            match = (sn.find(keyword) != string::npos);
        }
        if (!match) {
            // 使用邻接表查询该车型使用的技术
            vector<int> techNeighbors = g_graph.getNeighborsByType(m.id, EdgeType::USES_TECH);
            for (int tid : techNeighbors) {
                if (getTechName(tid).find(keyword) != string::npos) {
                    match = true;
                    break;
                }
            }
        }
        
        if (match) {
            tableData.push_back({
                to_string(m.id),
                m.name,
                getSeriesName(m.series_id),
                to_string(m.price).substr(0, to_string(m.price).find('.') + 3)
            });
            count++;
        }
    }
    
    if (count == 0) {
        cout << "未找到匹配的车型\n";
    } else {
        drawTable(tableData, {6, 18, 14, 10});
        cout << "\n共找到 " << count << " 款车型\n";
    }
}

// 添加新车型（添加到链表并更新邻接表）
void addNewModel() {
    drawTitle("添加新车型");
    
    Model newModel;
    newModel.id = 9000 + rand() % 1000;
    
    cout << "\n请输入车型信息:\n";
    cin.ignore();
    
    cout << "  车型名称: ";
    getline(cin, newModel.name);
    
    cout << "\n  选择系列:\n";
    // 遍历系列链表
    for (const auto& s : g_series) {
        cout << "    " << s.id << ". " << s.name << "\n";
    }
    cout << "  系列编号: ";
    cin >> newModel.series_id;
    
    cout << "  指导价格(万): ";
    cin >> newModel.price;
    
    cout << "  续航里程(km): ";
    cin >> newModel.range_km;
    
    cout << "  能源类型(EV/PHEV): ";
    cin >> newModel.energy_type;
    
    cin.ignore();
    cout << "  车身类型: ";
    getline(cin, newModel.body_type);
    
    cout << "  座位数: ";
    cin >> newModel.seats;
    
    cin.ignore();
    cout << "  上市年份: ";
    getline(cin, newModel.launch_year);
    
    // 选择技术
    cout << "\n  选择搭载技术(输入ID,用空格分隔):\n";
    // 遍历技术链表
    for (const auto& t : g_techs) {
        cout << "    " << t.id << ". " << t.name << "\n";
    }
    cout << "  技术ID: ";
    string techInput;
    getline(cin, techInput);
    
    stringstream ss(techInput);
    int tid;
    while (ss >> tid) {
        newModel.tech_ids.push_back(tid);
    }
    
    if (newModel.tech_ids.empty()) {
        newModel.tech_ids.push_back(102); // 默认添加刀片电池
    }
    
    // 添加到链表
    g_models.append(newModel);
    
    // 更新邻接表：添加新车型节点和相关边
    g_graph.addNode(GraphNode(newModel.id, NodeType::MODEL, newModel.name));
    g_graph.addEdge(newModel.id, newModel.series_id, EdgeType::BELONGS_TO);
    for (int techId : newModel.tech_ids) {
        g_graph.addEdge(newModel.id, techId, EdgeType::USES_TECH);
    }
    
    // 保存到文件
    if (saveData()) {
        cout << "\n✓ 车型添加成功! ID: " << newModel.id << " (已保存到文件)\n";
    } else {
        cout << "\n✓ 车型添加成功! ID: " << newModel.id << " (警告: 保存到文件失败)\n";
    }
}

// 显示统计信息（遍历链表）
void showStats() {
    drawTitle("数据统计");
    
    cout << "\n";
    cout << "  ╔═══════════════════════════════════════════╗\n";
    cout << "  ║           BYD 汽车信息统计                ║\n";
    cout << "  ╠═══════════════════════════════════════════╣\n";
    cout << "  ║  系列总数:          " << setw(3) << g_series.size() << " 个               ║\n";
    cout << "  ║  车型总数:          " << setw(3) << g_models.size() << " 款               ║\n";
    cout << "  ║  技术总数:          " << setw(3) << g_techs.size() << " 项               ║\n";
    cout << "  ╠═══════════════════════════════════════════╣\n";
    
    // 遍历链表统计能源类型和价格
    int evCount = 0, phevCount = 0;
    double maxPrice = 0, minPrice = 9999;
    for (const auto& m : g_models) {
        if (m.energy_type == "EV") evCount++;
        else phevCount++;
        maxPrice = max(maxPrice, m.price);
        minPrice = min(minPrice, m.price);
    }
    
    cout << "  ║  纯电车型:          " << setw(3) << evCount << " 款               ║\n";
    cout << "  ║  混动车型:          " << setw(3) << phevCount << " 款               ║\n";
    cout << "  ║  价格区间:     " << setw(6) << fixed << setprecision(2) << minPrice << " - " << setw(6) << maxPrice << " 万   ║\n";
    cout << "  ╠═══════════════════════════════════════════╣\n";
    cout << "  ║  图节点数:          " << setw(3) << g_graph.getNodeCount() << " 个               ║\n";
    cout << "  ║  图边数量:          " << setw(3) << g_graph.getEdgeCount() << " 条               ║\n";
    cout << "  ╚═══════════════════════════════════════════╝\n";
    
    // ASCII 柱状图 - 各系列车型数量（遍历链表）
    cout << "\n  各系列车型数量:\n\n";
    
    for (const auto& s : g_series) {
        int count = 0;
        for (const auto& m : g_models) {
            if (m.series_id == s.id) count++;
        }
        cout << "  " << padRight(s.name, 14) << " |";
        for (int i = 0; i < count; i++) cout << "█";
        cout << " " << count << "\n";
    }
    cout << "                 +" << string(15, '-') << "\n";
}

// =============================
// 主菜单
// =============================

void showMenu() {
    cout << "\n";
    cout << "  ╔═══════════════════════════════════════════╗\n";
    cout << "  ║    BYD 汽车信息查询系统 - CLI 版本        ║\n";
    cout << "  ╠═══════════════════════════════════════════╣\n";
    cout << "  ║  1. 查看车型列表                          ║\n";
    cout << "  ║  2. 查看系列列表                          ║\n";
    cout << "  ║  3. 查看技术列表                          ║\n";
    cout << "  ║  4. 查看车型详情                          ║\n";
    cout << "  ║  5. 按系列筛选                            ║\n";
    cout << "  ║  6. 搜索车型                              ║\n";
    cout << "  ║  7. 查看关系图谱                          ║\n";
    cout << "  ║  8. 添加新车型                            ║\n";
    cout << "  ║  9. 查看统计信息                          ║\n";
    cout << "  ║  0. 退出程序                              ║\n";
    cout << "  ╚═══════════════════════════════════════════╝\n";
    cout << "\n  请输入选项: ";
}

// =============================
// 主函数
// =============================

int main() {
    setupConsole();
    initData();
    
    clearScreen();
    drawTitle("欢迎使用 BYD 汽车信息查询系统");
    cout << "\n  系统已加载 " << g_series.size() << " 个系列, " 
         << g_models.size() << " 款车型, " 
         << g_techs.size() << " 项核心技术\n";
    
    int choice;
    do {
        showMenu();
        cin >> choice;
        
        switch (choice) {
            case 1: showModelList(); break;
            case 2: showSeriesList(); break;
            case 3: showTechList(); break;
            case 4: {
                cout << "请输入车型ID: ";
                int id;
                cin >> id;
                showModelDetail(id);
                break;
            }
            case 5: filterBySeries(); break;
            case 6: searchModels(); break;
            case 7: drawRelationTree(); break;
            case 8: addNewModel(); break;
            case 9: showStats(); break;
            case 0: 
                cout << "\n  感谢使用，再见！\n\n";
                break;
            default:
                cout << "  无效选项，请重新输入\n";
        }
        
        if (choice != 0) {
            cout << "\n  按 Enter 继续...";
            cin.ignore();
            cin.get();
        }
        
    } while (choice != 0);
    
    return 0;
}
