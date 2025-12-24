#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "httplib.h"

using namespace std;

// =============================
// 核心表结构定义 (主表 + 关联表模式)
// =============================

// 1. 系列表 (Series) - 存储王朝/海洋等系列
struct Series {
    int series_id;          // 主键
    string series_name;     // 系列名称 (唯一, 非空)
    string intro;           // 系列介绍
};

// 2. 技术表 (Techs) - 存储DM-i、刀片电池等技术
struct Tech {
    int tech_id;            // 主键
    string tech_name;       // 技术名称 (唯一, 非空)
    string intro;           // 技术介绍
};

// 3. 车型表 (Models) - 绑定系列，确保每个车型必属一个系列
struct Model {
    int model_id;           // 主键
    string model_name;      // 车型名称 (唯一, 非空)
    int series_id;          // 外键 -> Series (非空)
    double price;           // 售价 (price > 0, 非空)
    double range_km;        // 续航里程
    string energy_type;     // 能源类型 (EV/PHEV/HEV)
    string body_type;       // 车身类型
    int seats;              // 座位数
    string launch_year;     // 上市年份
};

// 4. 车型-技术关联表 (ModelTech) - 确保每个车型绑定至少1个技术
struct ModelTech {
    int id;                 // 主键
    int model_id;           // 外键 -> Model
    int tech_id;            // 外键 -> Tech
    // model_id + tech_id 唯一约束
};

// =============================
// 数据管理器 (带完整性校验)
// =============================

const string DATA_FILE = "../data/byd_web_data.txt";

// 辅助函数：去除首尾空白
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// 辅助函数：分割字符串
vector<string> splitStr(const string& s, char delimiter) {
    vector<string> tokens;
    stringstream ss(s);
    string token;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

class CarDataManager {
public:
    // 数据存储 (模拟数据库表)
    unordered_map<int, Series> series_table;
    unordered_map<int, Model> models_table;
    unordered_map<int, Tech> techs_table;
    vector<ModelTech> model_tech_table;

    // 辅助索引 (用于唯一性校验)
    unordered_set<string> series_names;
    unordered_set<string> model_names;
    unordered_set<string> tech_names;
    unordered_set<string> model_tech_pairs; // "model_id_tech_id"

    mutable std::mutex mtx_;
    int next_mt_id = 1;

    // -------------------------
    // 约束校验与数据操作
    // -------------------------

    // 新增系列 (非空约束 + 唯一约束)
    bool addSeries(int id, const string& name, const string& intro, string& err) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (name.empty()) { err = "NOT NULL 约束失败: series_name 不能为空"; return false; }
        if (series_table.count(id)) { err = "主键约束失败: series_id 已存在"; return false; }
        if (series_names.count(name)) { err = "唯一约束失败: series_name 已存在"; return false; }

        series_table[id] = { id, name, intro };
        series_names.insert(name);
        return true;
    }

    // 新增技术 (非空约束 + 唯一约束)
    bool addTech(int id, const string& name, const string& intro, string& err) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (name.empty()) { err = "NOT NULL 约束失败: tech_name 不能为空"; return false; }
        if (techs_table.count(id)) { err = "主键约束失败: tech_id 已存在"; return false; }
        if (tech_names.count(name)) { err = "唯一约束失败: tech_name 已存在"; return false; }

        techs_table[id] = { id, name, intro };
        tech_names.insert(name);
        return true;
    }

    // 新增车型 (完整约束校验)
    bool addModel(int id, const string& name, int series_id, double price, 
                  double range_km, const string& energy_type, 
                  const string& body_type, int seats, const string& launch_year,
                  const vector<int>& tech_ids, string& err) {
        std::lock_guard<std::mutex> lk(mtx_);
        
        // 1. 非空约束
        if (name.empty()) { err = "NOT NULL 约束失败: model_name 不能为空"; return false; }
        if (energy_type.empty()) { err = "NOT NULL 约束失败: energy_type 不能为空"; return false; }

        // 2. 主键约束
        if (models_table.count(id)) { err = "主键约束失败: model_id 已存在"; return false; }

        // 3. 唯一约束
        if (model_names.count(name)) { err = "唯一约束失败: model_name 已存在"; return false; }

        // 4. 外键约束: series_id 必须在系列表存在
        if (series_table.find(series_id) == series_table.end()) {
            err = "外键约束失败: series_id " + to_string(series_id) + " 在系列表中不存在";
            return false;
        }

        // 5. CHECK约束: price > 0
        if (price <= 0) {
            err = "CHECK 约束失败: price 必须大于 0";
            return false;
        }

        // 6. 业务校验: 必须绑定至少1个技术
        if (tech_ids.empty()) {
            err = "业务约束失败: 车型必须绑定至少1个技术";
            return false;
        }

        // 7. 外键约束: 所有tech_id必须在技术表存在
        for (int tid : tech_ids) {
            if (techs_table.find(tid) == techs_table.end()) {
                err = "外键约束失败: tech_id " + to_string(tid) + " 在技术表中不存在";
                return false;
            }
        }

        // 入库
        models_table[id] = { id, name, series_id, price, range_km, energy_type, body_type, seats, launch_year };
        model_names.insert(name);

        // 插入关联表
        for (int tid : tech_ids) {
            string pair_key = to_string(id) + "_" + to_string(tid);
            if (!model_tech_pairs.count(pair_key)) {
                model_tech_table.push_back({ next_mt_id++, id, tid });
                model_tech_pairs.insert(pair_key);
            }
        }

        return true;
    }

    // 新增车型（无技术绑定版本，用于API添加后再单独绑定技术）
    bool addModel(int id, const string& name, int series_id, double price, 
                  double range_km, const string& energy_type, 
                  const string& body_type, int seats, const string& launch_year,
                  string& err) {
        std::lock_guard<std::mutex> lk(mtx_);
        
        if (name.empty()) { err = "NOT NULL 约束失败: model_name 不能为空"; return false; }
        if (energy_type.empty()) { err = "NOT NULL 约束失败: energy_type 不能为空"; return false; }
        if (models_table.count(id)) { err = "主键约束失败: model_id 已存在"; return false; }
        if (model_names.count(name)) { err = "唯一约束失败: model_name 已存在"; return false; }
        if (series_table.find(series_id) == series_table.end()) {
            err = "外键约束失败: series_id " + to_string(series_id) + " 在系列表中不存在";
            return false;
        }
        if (price <= 0) { err = "CHECK 约束失败: price 必须大于 0"; return false; }

        models_table[id] = { id, name, series_id, price, range_km, energy_type, body_type, seats, launch_year };
        model_names.insert(name);
        return true;
    }

    // 添加车型-技术关联
    bool addModelTech(int model_id, int tech_id) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (models_table.find(model_id) == models_table.end()) return false;
        if (techs_table.find(tech_id) == techs_table.end()) return false;
        
        string pair_key = to_string(model_id) + "_" + to_string(tech_id);
        if (model_tech_pairs.count(pair_key)) return true; // 已存在
        
        model_tech_table.push_back({ next_mt_id++, model_id, tech_id });
        model_tech_pairs.insert(pair_key);
        return true;
    }

    // -------------------------
    // 查询接口
    // -------------------------

    // 获取所有系列
    vector<Series> getAllSeries() {
        std::lock_guard<std::mutex> lk(mtx_);
        vector<Series> result;
        for (const auto& p : series_table) {
            result.push_back(p.second);
        }
        return result;
    }

    // 获取所有技术
    vector<Tech> getAllTechs() {
        std::lock_guard<std::mutex> lk(mtx_);
        vector<Tech> result;
        for (const auto& p : techs_table) {
            result.push_back(p.second);
        }
        return result;
    }

    // 获取所有车型 (带关联信息)
    struct ModelDetail {
        Model model;
        string series_name;
        vector<string> tech_names;
    };

    vector<ModelDetail> getAllModels(int filter_series_id = -1, const string& filter_energy = "") {
        std::lock_guard<std::mutex> lk(mtx_);
        vector<ModelDetail> result;

        for (const auto& p : models_table) {
            const Model& m = p.second;

            // 系列筛选
            if (filter_series_id > 0 && m.series_id != filter_series_id) continue;
            // 能源类型筛选
            if (!filter_energy.empty() && m.energy_type != filter_energy) continue;

            ModelDetail detail;
            detail.model = m;
            detail.series_name = series_table.count(m.series_id) ? series_table.at(m.series_id).series_name : "";

            // 获取关联技术
            for (const auto& mt : model_tech_table) {
                if (mt.model_id == m.model_id && techs_table.count(mt.tech_id)) {
                    detail.tech_names.push_back(techs_table.at(mt.tech_id).tech_name);
                }
            }
            result.push_back(detail);
        }

        // 按价格排序
        sort(result.begin(), result.end(), [](const ModelDetail& a, const ModelDetail& b) {
            return a.model.price < b.model.price;
        });

        return result;
    }

    // 获取单个车型详情
    ModelDetail getModelDetail(int model_id) {
        std::lock_guard<std::mutex> lk(mtx_);
        ModelDetail detail;
        
        if (!models_table.count(model_id)) return detail;
        
        const Model& m = models_table.at(model_id);
        detail.model = m;
        detail.series_name = series_table.count(m.series_id) ? series_table.at(m.series_id).series_name : "";

        for (const auto& mt : model_tech_table) {
            if (mt.model_id == m.model_id && techs_table.count(mt.tech_id)) {
                detail.tech_names.push_back(techs_table.at(mt.tech_id).tech_name);
            }
        }
        return detail;
    }

    // 搜索车型
    vector<ModelDetail> searchModels(const string& keyword) {
        std::lock_guard<std::mutex> lk(mtx_);
        vector<ModelDetail> result;

        for (const auto& p : models_table) {
            const Model& m = p.second;
            
            // 名称匹配
            bool match = m.model_name.find(keyword) != string::npos;
            
            // 系列名匹配
            if (!match && series_table.count(m.series_id)) {
                match = series_table.at(m.series_id).series_name.find(keyword) != string::npos;
            }
            
            // 技术名匹配
            if (!match) {
                for (const auto& mt : model_tech_table) {
                    if (mt.model_id == m.model_id && techs_table.count(mt.tech_id)) {
                        if (techs_table.at(mt.tech_id).tech_name.find(keyword) != string::npos) {
                            match = true;
                            break;
                        }
                    }
                }
            }

            if (match) {
                ModelDetail detail;
                detail.model = m;
                detail.series_name = series_table.count(m.series_id) ? series_table.at(m.series_id).series_name : "";
                for (const auto& mt : model_tech_table) {
                    if (mt.model_id == m.model_id && techs_table.count(mt.tech_id)) {
                        detail.tech_names.push_back(techs_table.at(mt.tech_id).tech_name);
                    }
                }
                result.push_back(detail);
            }
        }
        return result;
    }

    // 获取统计信息
    void getStats(int& series_count, int& model_count, int& tech_count) {
        std::lock_guard<std::mutex> lk(mtx_);
        series_count = series_table.size();
        model_count = models_table.size();
        tech_count = techs_table.size();
    }

    // -------------------------
    // 从文件加载数据
    // -------------------------
    bool loadData() {
        ifstream file(DATA_FILE);
        if (!file.is_open()) {
            cerr << "Warning: Cannot open data file " << DATA_FILE << endl;
            return false;
        }
        
        // 清空现有数据
        series_table.clear();
        models_table.clear();
        techs_table.clear();
        model_tech_table.clear();
        series_names.clear();
        model_names.clear();
        tech_names.clear();
        model_tech_pairs.clear();
        next_mt_id = 1;
        
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
            } else if (line == "[MODEL_TECH]") {
                currentSection = "MODEL_TECH";
                continue;
            }
            
            vector<string> parts = splitStr(line, ',');
            
            if (currentSection == "SERIES" && parts.size() >= 3) {
                int id = stoi(parts[0]);
                series_table[id] = { id, parts[1], parts[2] };
                series_names.insert(parts[1]);
            }
            else if (currentSection == "TECH" && parts.size() >= 3) {
                int id = stoi(parts[0]);
                techs_table[id] = { id, parts[1], parts[2] };
                tech_names.insert(parts[1]);
            }
            else if (currentSection == "MODEL" && parts.size() >= 9) {
                int id = stoi(parts[0]);
                Model m;
                m.model_id = id;
                m.model_name = parts[1];
                m.series_id = stoi(parts[2]);
                m.price = stod(parts[3]);
                m.range_km = stod(parts[4]);
                m.energy_type = parts[5];
                m.body_type = parts[6];
                m.seats = stoi(parts[7]);
                m.launch_year = parts[8];
                models_table[id] = m;
                model_names.insert(parts[1]);
            }
            else if (currentSection == "MODEL_TECH" && parts.size() >= 2) {
                int model_id = stoi(parts[0]);
                int tech_id = stoi(parts[1]);
                string pair_key = to_string(model_id) + "_" + to_string(tech_id);
                if (!model_tech_pairs.count(pair_key)) {
                    model_tech_table.push_back({ next_mt_id++, model_id, tech_id });
                    model_tech_pairs.insert(pair_key);
                }
            }
        }
        
        file.close();
        return true;
    }
    
    // -------------------------
    // 保存数据到文件
    // -------------------------
    bool saveData() {
        ofstream file(DATA_FILE);
        if (!file.is_open()) {
            cerr << "Error: Cannot write to data file " << DATA_FILE << endl;
            return false;
        }
        
        file << "# BYD汽车信息系统 - Web版本数据文件\n";
        file << "# 格式说明：\n";
        file << "# [SERIES] 系列数据: id,名称,简介\n";
        file << "# [TECH] 技术数据: id,名称,简介\n";
        file << "# [MODEL] 车型数据: id,名称,系列id,价格,续航,能源类型,车身类型,座位数,年份\n";
        file << "# [MODEL_TECH] 车型技术关联: 车型id,技术id\n\n";
        
        // 写入系列数据
        file << "[SERIES]\n";
        for (const auto& p : series_table) {
            const Series& s = p.second;
            file << s.series_id << "," << s.series_name << "," << s.intro << "\n";
        }
        file << "\n";
        
        // 写入技术数据
        file << "[TECH]\n";
        for (const auto& p : techs_table) {
            const Tech& t = p.second;
            file << t.tech_id << "," << t.tech_name << "," << t.intro << "\n";
        }
        file << "\n";
        
        // 写入车型数据
        file << "[MODEL]\n";
        for (const auto& p : models_table) {
            const Model& m = p.second;
            file << m.model_id << "," << m.model_name << "," << m.series_id << ","
                 << fixed << setprecision(2) << m.price << ","
                 << (int)m.range_km << "," << m.energy_type << ","
                 << m.body_type << "," << m.seats << "," << m.launch_year << "\n";
        }
        file << "\n";
        
        // 写入车型技术关联
        file << "[MODEL_TECH]\n";
        for (const auto& mt : model_tech_table) {
            file << mt.model_id << "," << mt.tech_id << "\n";
        }
        
        file.close();
        return true;
    }

    // -------------------------
    // 初始化数据 - 从文件加载
    // -------------------------
    void initData() {
        if (!loadData()) {
            cerr << "Data loading failed, please ensure data file exists: " << DATA_FILE << endl;
        } else {
            cout << "Data loaded from file: " << DATA_FILE << endl;
        }
    }
};

CarDataManager g_manager;

// =============================
// JSON 工具函数
// =============================
string escapeJson(const string& s) {
    stringstream ss;
    for (char c : s) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\n': ss << "\\n";  break;
            case '\r': ss << "\\r";  break;
            case '\t': ss << "\\t";  break;
            default:   ss << c;      break;
        }
    }
    return ss.str();
}

// =============================
// HTTP服务器
// =============================
int main() {
    g_manager.initData();
    
    int s_cnt, m_cnt, t_cnt;
    g_manager.getStats(s_cnt, m_cnt, t_cnt);
    cout << "Server started: " << s_cnt << " series, " << m_cnt << " models, " << t_cnt << " techs." << endl;

    httplib::Server svr;

    // 静态文件服务
    svr.set_mount_point("/", "../web");

    // 跨域设置
    svr.set_pre_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // API: 获取所有系列
    svr.Get("/api/series", [](const httplib::Request&, httplib::Response& res) {
        auto series = g_manager.getAllSeries();
        stringstream ss;
        ss << "{\"ok\":true,\"data\":[";
        bool first = true;
        for (const auto& s : series) {
            if (!first) ss << ",";
            ss << "{\"series_id\":" << s.series_id 
               << ",\"series_name\":\"" << escapeJson(s.series_name) << "\""
               << ",\"intro\":\"" << escapeJson(s.intro) << "\"}";
            first = false;
        }
        ss << "]}";
        res.set_content(ss.str(), "application/json");
    });

    // API: 获取所有技术
    svr.Get("/api/techs", [](const httplib::Request&, httplib::Response& res) {
        auto techs = g_manager.getAllTechs();
        stringstream ss;
        ss << "{\"ok\":true,\"data\":[";
        bool first = true;
        for (const auto& t : techs) {
            if (!first) ss << ",";
            ss << "{\"tech_id\":" << t.tech_id 
               << ",\"tech_name\":\"" << escapeJson(t.tech_name) << "\""
               << ",\"intro\":\"" << escapeJson(t.intro) << "\"}";
            first = false;
        }
        ss << "]}";
        res.set_content(ss.str(), "application/json");
    });

    // API: 获取车型列表 (支持筛选)
    svr.Get("/api/models", [](const httplib::Request& req, httplib::Response& res) {
        int series_id = -1;
        string energy = "";
        
        if (req.has_param("series_id")) {
            try { series_id = stoi(req.get_param_value("series_id")); } catch(...) {}
        }
        if (req.has_param("energy_type")) {
            energy = req.get_param_value("energy_type");
        }

        auto models = g_manager.getAllModels(series_id, energy);
        stringstream ss;
        ss << "{\"ok\":true,\"data\":[";
        bool first = true;
        for (const auto& md : models) {
            if (!first) ss << ",";
            ss << "{\"model_id\":" << md.model.model_id 
               << ",\"model_name\":\"" << escapeJson(md.model.model_name) << "\""
               << ",\"series_id\":" << md.model.series_id
               << ",\"series_name\":\"" << escapeJson(md.series_name) << "\""
               << ",\"price\":" << md.model.price
               << ",\"range_km\":" << md.model.range_km
               << ",\"energy_type\":\"" << escapeJson(md.model.energy_type) << "\""
               << ",\"body_type\":\"" << escapeJson(md.model.body_type) << "\""
               << ",\"seats\":" << md.model.seats
               << ",\"launch_year\":\"" << escapeJson(md.model.launch_year) << "\""
               << ",\"techs\":[";
            bool first_tech = true;
            for (const auto& tn : md.tech_names) {
                if (!first_tech) ss << ",";
                ss << "\"" << escapeJson(tn) << "\"";
                first_tech = false;
            }
            ss << "]}";
            first = false;
        }
        ss << "]}";
        res.set_content(ss.str(), "application/json");
    });

    // API: 获取单个车型详情
    svr.Get("/api/model", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("id")) {
            res.set_content("{\"ok\":false,\"message\":\"缺少 model_id 参数\"}", "application/json");
            return;
        }
        int model_id = 0;
        try { model_id = stoi(req.get_param_value("id")); } catch(...) {}

        auto detail = g_manager.getModelDetail(model_id);
        if (detail.model.model_id == 0) {
            res.set_content("{\"ok\":false,\"message\":\"车型不存在\"}", "application/json");
            return;
        }

        stringstream ss;
        ss << "{\"ok\":true,\"data\":{";
        ss << "\"model_id\":" << detail.model.model_id 
           << ",\"model_name\":\"" << escapeJson(detail.model.model_name) << "\""
           << ",\"series_id\":" << detail.model.series_id
           << ",\"series_name\":\"" << escapeJson(detail.series_name) << "\""
           << ",\"price\":" << detail.model.price
           << ",\"range_km\":" << detail.model.range_km
           << ",\"energy_type\":\"" << escapeJson(detail.model.energy_type) << "\""
           << ",\"body_type\":\"" << escapeJson(detail.model.body_type) << "\""
           << ",\"seats\":" << detail.model.seats
           << ",\"launch_year\":\"" << escapeJson(detail.model.launch_year) << "\""
           << ",\"techs\":[";
        bool first_tech = true;
        for (const auto& tn : detail.tech_names) {
            if (!first_tech) ss << ",";
            ss << "\"" << escapeJson(tn) << "\"";
            first_tech = false;
        }
        ss << "]}}";
        res.set_content(ss.str(), "application/json");
    });

    // API: 搜索车型
    svr.Get("/api/search", [](const httplib::Request& req, httplib::Response& res) {
        string keyword = req.get_param_value("q");
        if (keyword.empty()) {
            res.set_content("{\"ok\":false,\"message\":\"请输入搜索关键词\"}", "application/json");
            return;
        }

        auto models = g_manager.searchModels(keyword);
        stringstream ss;
        ss << "{\"ok\":true,\"data\":[";
        bool first = true;
        for (const auto& md : models) {
            if (!first) ss << ",";
            ss << "{\"model_id\":" << md.model.model_id 
               << ",\"model_name\":\"" << escapeJson(md.model.model_name) << "\""
               << ",\"series_id\":" << md.model.series_id
               << ",\"series_name\":\"" << escapeJson(md.series_name) << "\""
               << ",\"price\":" << md.model.price
               << ",\"range_km\":" << md.model.range_km
               << ",\"energy_type\":\"" << escapeJson(md.model.energy_type) << "\""
               << ",\"body_type\":\"" << escapeJson(md.model.body_type) << "\""
               << ",\"seats\":" << md.model.seats
               << ",\"launch_year\":\"" << escapeJson(md.model.launch_year) << "\""
               << ",\"techs\":[";
            bool first_tech = true;
            for (const auto& tn : md.tech_names) {
                if (!first_tech) ss << ",";
                ss << "\"" << escapeJson(tn) << "\"";
                first_tech = false;
            }
            ss << "]}";
            first = false;
        }
        ss << "]}";
        res.set_content(ss.str(), "application/json");
    });

    // API: 获取统计信息
    svr.Get("/api/stats", [](const httplib::Request&, httplib::Response& res) {
        int s_cnt, m_cnt, t_cnt;
        g_manager.getStats(s_cnt, m_cnt, t_cnt);
        stringstream ss;
        ss << "{\"ok\":true,\"series_count\":" << s_cnt 
           << ",\"model_count\":" << m_cnt 
           << ",\"tech_count\":" << t_cnt << "}";
        res.set_content(ss.str(), "application/json");
    });

    // API: 获取图结构数据 (三层架构: Series -> Model -> Tech)
    svr.Get("/api/graph", [](const httplib::Request&, httplib::Response& res) {
        auto series = g_manager.getAllSeries();
        auto techs = g_manager.getAllTechs();
        auto models = g_manager.getAllModels();

        stringstream ss;
        ss << "{\"ok\":true,";
        
        // 节点数据
        ss << "\"nodes\":[";
        bool first = true;
        
        // 顶层: 系列节点
        for (const auto& s : series) {
            if (!first) ss << ",";
            ss << "{\"id\":\"s_" << s.series_id << "\""
               << ",\"name\":\"" << escapeJson(s.series_name) << "\""
               << ",\"category\":0,\"layer\":0}";
            first = false;
        }
        
        // 中层: 车型节点
        for (const auto& md : models) {
            ss << ",{\"id\":\"m_" << md.model.model_id << "\""
               << ",\"name\":\"" << escapeJson(md.model.model_name) << "\""
               << ",\"series_id\":" << md.model.series_id
               << ",\"category\":1,\"layer\":1}";
        }
        
        // 底层: 技术节点
        for (const auto& t : techs) {
            ss << ",{\"id\":\"t_" << t.tech_id << "\""
               << ",\"name\":\"" << escapeJson(t.tech_name) << "\""
               << ",\"category\":2,\"layer\":2}";
        }
        ss << "],";
        
        // 边数据 (只允许相邻层: Series->Model, Model->Tech)
        ss << "\"links\":[";
        first = true;
        
        // Series -> Model 边
        for (const auto& md : models) {
            if (!first) ss << ",";
            ss << "{\"source\":\"s_" << md.model.series_id << "\""
               << ",\"target\":\"m_" << md.model.model_id << "\""
               << ",\"relation\":\"belongs_to\"}";
            first = false;
        }
        
        // Model -> Tech 边
        for (const auto& md : models) {
            for (const auto& tn : md.tech_names) {
                // 需要找到tech_id
                for (const auto& t : techs) {
                    if (t.tech_name == tn) {
                        ss << ",{\"source\":\"m_" << md.model.model_id << "\""
                           << ",\"target\":\"t_" << t.tech_id << "\""
                           << ",\"relation\":\"equipped_with\"}";
                        break;
                    }
                }
            }
        }
        ss << "]}";
        
        res.set_content(ss.str(), "application/json");
    });

    // =============================
    // POST API: 添加新数据
    // =============================
    
    // API: 添加新车型
    svr.Post("/api/model/add", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        
        try {
            // 简单解析JSON（不依赖外部库）
            string body = req.body;
            
            // 提取字段
            auto extractStr = [&](const string& key) -> string {
                string pattern = "\"" + key + "\":\"";
                size_t pos = body.find(pattern);
                if (pos == string::npos) return "";
                pos += pattern.length();
                size_t end = body.find("\"", pos);
                if (end == string::npos) return "";
                return body.substr(pos, end - pos);
            };
            
            auto extractNum = [&](const string& key) -> double {
                string pattern = "\"" + key + "\":";
                size_t pos = body.find(pattern);
                if (pos == string::npos) return 0;
                pos += pattern.length();
                size_t end = pos;
                while (end < body.length() && (isdigit(body[end]) || body[end] == '.' || body[end] == '-')) end++;
                return stod(body.substr(pos, end - pos));
            };
            
            auto extractIntArray = [&](const string& key) -> vector<int> {
                vector<int> result;
                string pattern = "\"" + key + "\":[";
                size_t pos = body.find(pattern);
                if (pos == string::npos) return result;
                pos += pattern.length();
                size_t end = body.find("]", pos);
                if (end == string::npos) return result;
                string arr = body.substr(pos, end - pos);
                stringstream ss(arr);
                string item;
                while (getline(ss, item, ',')) {
                    if (!item.empty()) {
                        try { result.push_back(stoi(item)); } catch (...) {}
                    }
                }
                return result;
            };
            
            string model_name = extractStr("model_name");
            int series_id = (int)extractNum("series_id");
            double price = extractNum("price");
            double range_km = extractNum("range_km");
            string energy_type = extractStr("energy_type");
            string body_type = extractStr("body_type");
            int seats = (int)extractNum("seats");
            string launch_year = extractStr("launch_year");
            vector<int> tech_ids = extractIntArray("tech_ids");
            
            // 验证
            if (model_name.empty()) {
                res.set_content("{\"ok\":false,\"message\":\"车型名称不能为空\"}", "application/json");
                return;
            }
            if (price <= 0) {
                res.set_content("{\"ok\":false,\"message\":\"价格必须大于0\"}", "application/json");
                return;
            }
            if (energy_type.empty()) {
                res.set_content("{\"ok\":false,\"message\":\"能源类型不能为空\"}", "application/json");
                return;
            }
            
            // 生成新ID
            int new_model_id = 9000 + rand() % 1000;
            
            // 添加车型
            string err;
            bool ok = g_manager.addModel(new_model_id, model_name, series_id, price, range_km,
                                          energy_type, body_type, seats, launch_year, err);
            
            if (!ok) {
                res.set_content("{\"ok\":false,\"message\":\"" + escapeJson(err) + "\"}", "application/json");
                return;
            }
            
            // 添加技术关联
            for (int tech_id : tech_ids) {
                g_manager.addModelTech(new_model_id, tech_id);
            }
            
            // 保存到文件
            g_manager.saveData();
            
            res.set_content("{\"ok\":true,\"message\":\"添加成功\",\"model_id\":" + to_string(new_model_id) + "}", "application/json");
            
        } catch (const exception& e) {
            res.set_content("{\"ok\":false,\"message\":\"解析错误\"}", "application/json");
        }
    });
    
    // API: 添加新技术
    svr.Post("/api/tech/add", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        
        try {
            string body = req.body;
            
            auto extractStr = [&](const string& key) -> string {
                string pattern = "\"" + key + "\":\"";
                size_t pos = body.find(pattern);
                if (pos == string::npos) return "";
                pos += pattern.length();
                size_t end = body.find("\"", pos);
                if (end == string::npos) return "";
                return body.substr(pos, end - pos);
            };
            
            string tech_name = extractStr("tech_name");
            string intro = extractStr("intro");
            
            if (tech_name.empty()) {
                res.set_content("{\"ok\":false,\"message\":\"技术名称不能为空\"}", "application/json");
                return;
            }
            
            // 生成新ID
            int new_tech_id = 200 + rand() % 100;
            
            // 添加技术
            string err;
            bool ok = g_manager.addTech(new_tech_id, tech_name, intro, err);
            
            if (!ok) {
                res.set_content("{\"ok\":false,\"message\":\"" + escapeJson(err) + "\"}", "application/json");
                return;
            }
            
            // 保存到文件
            g_manager.saveData();
            
            res.set_content("{\"ok\":true,\"message\":\"添加成功\",\"tech_id\":" + to_string(new_tech_id) + "}", "application/json");
            
        } catch (const exception& e) {
            res.set_content("{\"ok\":false,\"message\":\"解析错误\"}", "application/json");
        }
    });
    
    // OPTIONS 预检请求处理
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    // 启动服务器
    cout << "Starting server on http://localhost:8080 ..." << endl;
    cout.flush();
    
    if (!svr.bind_to_port("0.0.0.0", 8080)) {
        cerr << "Error: Cannot bind to port 8080" << endl;
        return 1;
    }
    
    cout << "Server is running. Press Ctrl+C to stop." << endl;
    cout.flush();
    
    svr.listen_after_bind();
    
    return 0;
}
