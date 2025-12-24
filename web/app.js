/**
 * BYD 汽车信息查询系统
 * 基于关系表结构的查询系统
 */

// ==================== 
// 全局状态
// ====================
const AppState = {
    seriesList: [],
    techsList: [],
    modelsList: [],
    currentSeriesId: 0,
    currentEnergyType: '',
    selectedModelId: null,
};

// ==================== 
// 初始化
// ====================
document.addEventListener('DOMContentLoaded', async function () {
    initEventListeners();
    await loadInitialData();
});

// 初始化事件监听
function initEventListeners() {
    // 搜索框事件
    const searchInput = document.getElementById('global-search');
    if (searchInput) {
        searchInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') handleSearch();
        });
    }

    // 能源类型筛选按钮
    document.querySelectorAll('.filter-btn').forEach(btn => {
        btn.addEventListener('click', function() {
            document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
            this.classList.add('active');
            AppState.currentEnergyType = this.dataset.energy;
            loadModels();
        });
    });

    // 标签页切换
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', function() {
            const tab = this.dataset.tab;
            document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
            document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));
            this.classList.add('active');
            document.getElementById(`panel-${tab}`).classList.add('active');
        });
    });
}

// 加载初始数据
async function loadInitialData() {
    showLoading(true);
    try {
        await Promise.all([
            loadStats(),
            loadSeries(),
            loadTechs(),
            loadModels()
        ]);
        showToast('数据加载完成', 'success');
    } catch (e) {
        console.error(e);
        showToast('数据加载失败', 'error');
    }
    showLoading(false);
}

// ==================== 
// API 请求函数
// ====================

// 加载统计信息
async function loadStats() {
    try {
        const res = await fetch('/api/stats');
        const json = await res.json();
        if (json.ok) {
            document.getElementById('stat-series').textContent = json.series_count;
            document.getElementById('stat-models').textContent = json.model_count;
            document.getElementById('stat-techs').textContent = json.tech_count;
        }
    } catch (e) {
        console.error('加载统计失败:', e);
    }
}

// 加载系列列表
async function loadSeries() {
    try {
        const res = await fetch('/api/series');
        const json = await res.json();
        if (json.ok) {
            AppState.seriesList = json.data;
            renderSeriesList(json.data);
        }
    } catch (e) {
        console.error('加载系列失败:', e);
    }
}

// 加载技术列表
async function loadTechs() {
    try {
        const res = await fetch('/api/techs');
        const json = await res.json();
        if (json.ok) {
            AppState.techsList = json.data;
            renderTechList(json.data);
            renderTechCards(json.data);
        }
    } catch (e) {
        console.error('加载技术失败:', e);
    }
}

// 加载车型列表
async function loadModels() {
    showLoading(true);
    try {
        let url = '/api/models';
        const params = [];
        if (AppState.currentSeriesId > 0) {
            params.push(`series_id=${AppState.currentSeriesId}`);
        }
        if (AppState.currentEnergyType) {
            params.push(`energy_type=${encodeURIComponent(AppState.currentEnergyType)}`);
        }
        if (params.length > 0) {
            url += '?' + params.join('&');
        }

        const res = await fetch(url);
        const json = await res.json();
        if (json.ok) {
            AppState.modelsList = json.data;
            renderModelsTable(json.data);
        }
    } catch (e) {
        console.error('加载车型失败:', e);
    }
    showLoading(false);
}

// 搜索
window.handleSearch = async function() {
    const keyword = document.getElementById('global-search').value.trim();
    if (!keyword) {
        loadModels();
        return;
    }

    showLoading(true);
    try {
        const res = await fetch(`/api/search?q=${encodeURIComponent(keyword)}`);
        const json = await res.json();
        if (json.ok) {
            AppState.modelsList = json.data;
            renderModelsTable(json.data);
            showToast(`找到 ${json.data.length} 款相关车型`, 'success');
        } else {
            showToast(json.message || '搜索失败', 'error');
        }
    } catch (e) {
        console.error('搜索失败:', e);
        showToast('搜索失败', 'error');
    }
    showLoading(false);
};

// ==================== 
// 渲染函数
// ====================

// 渲染系列列表
function renderSeriesList(seriesList) {
    const container = document.getElementById('series-list');
    let html = `
        <label class="series-item ${AppState.currentSeriesId === 0 ? 'active' : ''}" data-series-id="0">
            <input type="radio" name="series" value="0" ${AppState.currentSeriesId === 0 ? 'checked' : ''}>
            <span class="series-name">全部系列</span>
        </label>
    `;
    
    seriesList.forEach(s => {
        html += `
            <label class="series-item ${AppState.currentSeriesId === s.series_id ? 'active' : ''}" data-series-id="${s.series_id}">
                <input type="radio" name="series" value="${s.series_id}" ${AppState.currentSeriesId === s.series_id ? 'checked' : ''}>
                <span class="series-name">${s.series_name}</span>
            </label>
        `;
    });
    
    container.innerHTML = html;

    // 绑定点击事件
    container.querySelectorAll('.series-item').forEach(item => {
        item.addEventListener('click', function() {
            container.querySelectorAll('.series-item').forEach(i => i.classList.remove('active'));
            this.classList.add('active');
            AppState.currentSeriesId = parseInt(this.dataset.seriesId);
            loadModels();
        });
    });
}

// 渲染技术列表（侧边栏）
function renderTechList(techsList) {
    const container = document.getElementById('tech-list');
    let html = '';
    techsList.forEach(t => {
        html += `
            <div class="tech-item" title="${t.intro}">
                <i class="fas fa-microchip"></i>
                <span>${t.tech_name}</span>
            </div>
        `;
    });
    container.innerHTML = html;
}

// 渲染技术卡片
function renderTechCards(techsList) {
    const container = document.getElementById('tech-cards');
    let html = '';
    techsList.forEach(t => {
        html += `
            <div class="tech-card">
                <div class="tech-card-icon">
                    <i class="fas fa-microchip"></i>
                </div>
                <div class="tech-card-content">
                    <h4>${t.tech_name}</h4>
                    <p>${t.intro}</p>
                </div>
            </div>
        `;
    });
    container.innerHTML = html;
}

// 渲染车型表格
function renderModelsTable(models) {
    const tbody = document.getElementById('models-tbody');
    const countEl = document.getElementById('model-count');
    
    countEl.textContent = `共 ${models.length} 款车型`;
    
    if (models.length === 0) {
        tbody.innerHTML = `
            <tr>
                <td colspan="9" class="empty-row">
                    <i class="fas fa-car-crash"></i>
                    <span>暂无符合条件的车型</span>
                </td>
            </tr>
        `;
        return;
    }

    let html = '';
    models.forEach(m => {
        const energyClass = m.energy_type === 'EV' ? 'energy-ev' : 'energy-phev';
        const techsHtml = m.techs.slice(0, 3).map(t => `<span class="tech-tag">${t}</span>`).join('');
        const moreTechs = m.techs.length > 3 ? `<span class="tech-more">+${m.techs.length - 3}</span>` : '';
        
        html += `
            <tr class="model-row" data-model-id="${m.model_id}" onclick="showModelDetail(${m.model_id})">
                <td class="model-name-cell">
                    <span class="model-name">${m.model_name}</span>
                </td>
                <td><span class="series-badge">${m.series_name}</span></td>
                <td class="price-cell">${m.price.toFixed(2)}</td>
                <td>${m.range_km}</td>
                <td><span class="energy-badge ${energyClass}">${m.energy_type}</span></td>
                <td>${m.body_type}</td>
                <td>${m.seats}座</td>
                <td>${m.launch_year}</td>
                <td class="tech-cell">${techsHtml}${moreTechs}</td>
            </tr>
        `;
    });
    
    tbody.innerHTML = html;
}

// 显示车型详情
window.showModelDetail = async function(modelId) {
    AppState.selectedModelId = modelId;
    
    // 高亮选中行
    document.querySelectorAll('.model-row').forEach(row => {
        row.classList.toggle('selected', parseInt(row.dataset.modelId) === modelId);
    });

    try {
        const res = await fetch(`/api/model?id=${modelId}`);
        const json = await res.json();
        
        if (json.ok) {
            renderDetailPanel(json.data);
            document.getElementById('detail-panel').classList.add('open');
            
            // 更新图谱选中状态 - 新增功能
            setGraphSelectedModel(json.data);
        }
    } catch (e) {
        console.error('获取详情失败:', e);
    }
};

// 渲染详情面板
function renderDetailPanel(model) {
    const container = document.getElementById('panel-content');
    
    const techTagsHtml = model.techs.map(t => `<span class="detail-tech-tag">${t}</span>`).join('');
    
    container.innerHTML = `
        <div class="model-detail">
            <div class="detail-header">
                <h2 class="model-name">${model.model_name}</h2>
                <span class="model-series">${model.series_name}</span>
            </div>

            <div class="detail-section">
                <h4><i class="fas fa-tag"></i> 基本信息</h4>
                <div class="info-grid">
                    <div class="info-item">
                        <span class="info-label">指导价</span>
                        <span class="info-value price">${model.price.toFixed(2)} 万元</span>
                    </div>
                    <div class="info-item">
                        <span class="info-label">续航里程</span>
                        <span class="info-value">${model.range_km} km</span>
                    </div>
                    <div class="info-item">
                        <span class="info-label">能源类型</span>
                        <span class="info-value">${model.energy_type === 'EV' ? '纯电动' : '插电混动'}</span>
                    </div>
                    <div class="info-item">
                        <span class="info-label">车身类型</span>
                        <span class="info-value">${model.body_type}</span>
                    </div>
                    <div class="info-item">
                        <span class="info-label">座位数</span>
                        <span class="info-value">${model.seats} 座</span>
                    </div>
                    <div class="info-item">
                        <span class="info-label">上市年份</span>
                        <span class="info-value">${model.launch_year} 年</span>
                    </div>
                </div>
            </div>

            <div class="detail-section">
                <h4><i class="fas fa-microchip"></i> 搭载技术</h4>
                <div class="tech-tags">
                    ${techTagsHtml}
                </div>
            </div>
        </div>
    `;
}

// 关闭详情面板
window.closeDetailPanel = function() {
    document.getElementById('detail-panel').classList.remove('open');
    document.querySelectorAll('.model-row').forEach(row => row.classList.remove('selected'));
    AppState.selectedModelId = null;
};

// ==================== 
// 工具函数
// ====================

// 显示加载状态
function showLoading(show) {
    const overlay = document.getElementById('loading-overlay');
    if (overlay) {
        overlay.classList.toggle('active', show);
    }
}

// 显示提示消息
function showToast(message, type = 'info') {
    const toast = document.getElementById('toast');
    if (!toast) return;
    
    toast.textContent = message;
    toast.className = `toast ${type} active`;
    
    setTimeout(() => {
        toast.classList.remove('active');
    }, 3000);
}

// ====================
// 关系图可视化 - 树状结构
// ====================

let graphChart = null;
let isGraphMinimized = false;
let isGraphFullscreen = false;

// 图视图状态
let graphViewState = {
    mode: 'all',           // 'all' | 'table' | 'tree'
    selectedModel: null,   // 当前选中的车型数据
    selectedModelId: null  // 当前选中的车型ID
};

// 初始化图窗口
function initGraphWindow() {
    console.log('========== 初始化图窗口 ==========');
    
    const graphWindow = document.getElementById('graph-window');
    const container = document.getElementById('graph-container');
    
    if (!graphWindow || !container) {
        console.error('找不到图窗口元素!');
        return;
    }
    
    const toggleBtn = document.getElementById('graph-toggle');
    const fullscreenBtn = document.getElementById('graph-fullscreen');
    
    if (toggleBtn) {
        toggleBtn.addEventListener('click', toggleGraph);
    }
    
    if (fullscreenBtn) {
        fullscreenBtn.addEventListener('click', toggleFullscreen);
    }
    
    // 视图切换标签事件
    document.querySelectorAll('.view-tab').forEach(tab => {
        tab.addEventListener('click', function() {
            const viewMode = this.dataset.view;
            switchGraphView(viewMode);
        });
    });
    
    // 加载图数据
    loadGraphData();
}

// 收起/展开图窗口
function toggleGraph() {
    const graphWindow = document.getElementById('graph-window');
    const toggleBtn = document.getElementById('graph-toggle');
    
    isGraphMinimized = !isGraphMinimized;
    graphWindow.classList.toggle('minimized', isGraphMinimized);
    
    if (toggleBtn) {
        toggleBtn.innerHTML = isGraphMinimized 
            ? '<i class="fas fa-chevron-up"></i>' 
            : '<i class="fas fa-chevron-down"></i>';
    }
    
    // 重新调整图表尺寸
    if (!isGraphMinimized && graphChart) {
        setTimeout(() => graphChart.resize(), 300);
    }
}

// 全屏切换
function toggleFullscreen() {
    const graphWindow = document.getElementById('graph-window');
    const fullscreenBtn = document.getElementById('graph-fullscreen');
    
    isGraphFullscreen = !isGraphFullscreen;
    graphWindow.classList.toggle('fullscreen', isGraphFullscreen);
    
    if (fullscreenBtn) {
        fullscreenBtn.innerHTML = isGraphFullscreen 
            ? '<i class="fas fa-compress"></i>' 
            : '<i class="fas fa-expand"></i>';
    }
    
    // 重新调整图表尺寸
    if (graphChart) {
        setTimeout(() => graphChart.resize(), 300);
    }
}

// 加载图数据
async function loadGraphData() {
    console.log('========== 开始加载图数据 ==========');
    try {
        const res = await fetch('/api/graph');
        console.log('API响应状态:', res.status);
        
        if (!res.ok) {
            console.error('API请求失败:', res.status, res.statusText);
            // 显示错误提示
            showGraphError('API 请求失败: ' + res.status);
            return;
        }
        
        const json = await res.json();
        console.log('API返回数据:', json);
        
        if (json.ok && json.nodes && json.links) {
            console.log('节点数:', json.nodes.length, '边数:', json.links.length);
            renderTreeGraph(json);
        } else if (json.ok && json.data) {
            console.log('节点数:', json.data.nodes.length, '边数:', json.data.links.length);
            renderTreeGraph(json.data);
        } else {
            console.error('API返回数据格式错误:', json);
            showGraphError('数据格式错误');
        }
    } catch (e) {
        console.error('加载关系图失败:', e);
        showGraphError('加载失败: ' + e.message);
    }
}

// 显示图错误信息
function showGraphError(msg) {
    const container = document.getElementById('graph-container');
    if (container) {
        container.innerHTML = '<div style="display:flex;align-items:center;justify-content:center;height:100%;color:#999;font-size:14px;">' + msg + '</div>';
    }
}

// ====================
// 视图切换功能
// ====================

// 切换图视图模式
function switchGraphView(mode) {
    graphViewState.mode = mode;
    
    // 更新标签激活状态
    document.querySelectorAll('.view-tab').forEach(tab => {
        tab.classList.toggle('active', tab.dataset.view === mode);
    });
    
    // 根据模式渲染
    if (mode === 'all') {
        // 全局视图 - 显示完整关系图
        renderTreeGraph(currentGraphData);
        hideSelectedItemInfo();
    } else if (graphViewState.selectedModel) {
        // 有选中车型时，显示对应视图
        if (mode === 'table') {
            renderModelTableView(graphViewState.selectedModel);
        } else if (mode === 'tree') {
            renderModelTreeView(graphViewState.selectedModel);
        }
    } else {
        // 没有选中车型，提示用户
        showGraphMessage('请先在表格中点击选择一个车型');
    }
}

// 当表格中点击车型时，更新图谱选中状态
function setGraphSelectedModel(modelData) {
    graphViewState.selectedModel = modelData;
    graphViewState.selectedModelId = modelData.model_id;
    
    // 显示选中项信息
    showSelectedItemInfo(modelData.model_name);
    
    // 如果当前不是全局视图，自动刷新当前视图
    if (graphViewState.mode !== 'all') {
        switchGraphView(graphViewState.mode);
    } else {
        // 在全局视图中高亮该车型
        highlightModelInGraph(modelData.model_id);
    }
}

// 清除图选择
window.clearGraphSelection = function() {
    graphViewState.selectedModel = null;
    graphViewState.selectedModelId = null;
    hideSelectedItemInfo();
    
    // 切换回全局视图
    switchGraphView('all');
};

// 显示选中项信息
function showSelectedItemInfo(name) {
    const el = document.getElementById('graph-selected-item');
    const nameEl = document.getElementById('selected-item-name');
    if (el && nameEl) {
        nameEl.textContent = name;
        el.style.display = 'flex';
    }
}

// 隐藏选中项信息
function hideSelectedItemInfo() {
    const el = document.getElementById('graph-selected-item');
    if (el) {
        el.style.display = 'none';
    }
}

// 显示提示消息
function showGraphMessage(msg) {
    const container = document.getElementById('graph-container');
    if (container) {
        container.innerHTML = `
            <div style="display:flex;flex-direction:column;align-items:center;justify-content:center;height:100%;color:#999;font-size:14px;text-align:center;padding:20px;">
                <i class="fas fa-hand-pointer" style="font-size:32px;margin-bottom:12px;color:#bbb;"></i>
                <span>${msg}</span>
            </div>
        `;
    }
}

// 渲染车型的表关联视图
function renderModelTableView(modelData) {
    const container = document.getElementById('graph-container');
    if (!container) return;
    
    // 获取关联的系列和技术
    const seriesInfo = AppState.seriesList.find(s => s.series_id === modelData.series_id);
    const techList = modelData.techs || [];
    
    let html = `
        <div style="padding:16px;height:100%;overflow:auto;">
            <h4 style="margin:0 0 12px 0;color:#1890ff;font-size:14px;">
                <i class="fas fa-car"></i> ${modelData.model_name} - 表关联
            </h4>
            
            <!-- 所属系列 -->
            <div style="margin-bottom:16px;">
                <div style="font-size:12px;color:#666;margin-bottom:6px;font-weight:600;">
                    <i class="fas fa-layer-group"></i> 所属系列
                </div>
                <div style="background:#e6f7ff;border:1px solid #91d5ff;border-radius:6px;padding:10px;">
                    <div style="font-weight:600;color:#1890ff;">${seriesInfo ? seriesInfo.series_name : '未知系列'}</div>
                    <div style="font-size:11px;color:#666;margin-top:4px;">${seriesInfo ? seriesInfo.intro : ''}</div>
                </div>
            </div>
            
            <!-- 基本信息 -->
            <div style="margin-bottom:16px;">
                <div style="font-size:12px;color:#666;margin-bottom:6px;font-weight:600;">
                    <i class="fas fa-info-circle"></i> 车型信息
                </div>
                <table style="width:100%;border-collapse:collapse;font-size:12px;">
                    <tr>
                        <td style="padding:6px 8px;background:#f5f5f5;border:1px solid #eee;width:80px;">价格</td>
                        <td style="padding:6px 8px;border:1px solid #eee;">${modelData.price} 万</td>
                        <td style="padding:6px 8px;background:#f5f5f5;border:1px solid #eee;width:80px;">续航</td>
                        <td style="padding:6px 8px;border:1px solid #eee;">${modelData.range_km} km</td>
                    </tr>
                    <tr>
                        <td style="padding:6px 8px;background:#f5f5f5;border:1px solid #eee;">能源</td>
                        <td style="padding:6px 8px;border:1px solid #eee;">${modelData.energy_type}</td>
                        <td style="padding:6px 8px;background:#f5f5f5;border:1px solid #eee;">座位</td>
                        <td style="padding:6px 8px;border:1px solid #eee;">${modelData.seats} 座</td>
                    </tr>
                    <tr>
                        <td style="padding:6px 8px;background:#f5f5f5;border:1px solid #eee;">车身</td>
                        <td style="padding:6px 8px;border:1px solid #eee;">${modelData.body_type}</td>
                        <td style="padding:6px 8px;background:#f5f5f5;border:1px solid #eee;">上市</td>
                        <td style="padding:6px 8px;border:1px solid #eee;">${modelData.launch_year}</td>
                    </tr>
                </table>
            </div>
            
            <!-- 搭载技术 -->
            <div>
                <div style="font-size:12px;color:#666;margin-bottom:6px;font-weight:600;">
                    <i class="fas fa-microchip"></i> 搭载技术 (${techList.length}项)
                </div>
                <div style="display:flex;flex-wrap:wrap;gap:6px;">
                    ${techList.map(t => `
                        <span style="padding:4px 10px;background:#fff7e6;border:1px solid #ffd591;border-radius:4px;font-size:11px;color:#d46b08;">
                            ${t}
                        </span>
                    `).join('')}
                </div>
            </div>
        </div>
    `;
    
    container.innerHTML = html;
}

// 渲染车型的树状图视图
function renderModelTreeView(modelData) {
    const container = document.getElementById('graph-container');
    if (!container || !currentGraphData) return;
    
    container.style.width = '100%';
    container.style.height = '350px';
    
    if (graphChart) {
        graphChart.dispose();
    }
    
    graphChart = echarts.init(container);
    
    const containerWidth = container.offsetWidth || 600;
    const containerHeight = container.offsetHeight || 350;
    
    // 获取该车型相关的数据
    const modelId = 'm_' + modelData.model_id;
    const seriesId = 's_' + modelData.series_id;
    
    // 找出关联的技术
    const relatedTechs = [];
    currentGraphData.links.forEach(link => {
        if (link.source === modelId && link.relation === 'equipped_with') {
            relatedTechs.push(link.target);
        }
    });
    
    // 找到系列、车型、技术节点
    const seriesNode = currentGraphData.nodes.find(n => n.id === seriesId);
    const modelNode = currentGraphData.nodes.find(n => n.id === modelId);
    const techNodes = currentGraphData.nodes.filter(n => relatedTechs.includes(n.id));
    
    if (!seriesNode || !modelNode) {
        showGraphError('未找到车型数据');
        return;
    }
    
    // 构建简化的树状图
    const nodes = [];
    const links = [];
    
    // 系列节点 - 顶部居中
    nodes.push({
        id: seriesNode.id,
        name: seriesNode.name,
        x: containerWidth / 2,
        y: 50,
        fixed: true,
        symbolSize: 40,
        category: 0,
        itemStyle: { color: '#1890ff', borderColor: '#0958d9', borderWidth: 3 },
        label: { show: true, fontSize: 13, fontWeight: 'bold', position: 'top', color: '#1890ff' }
    });
    
    // 车型节点 - 中间居中
    nodes.push({
        id: modelNode.id,
        name: modelNode.name,
        x: containerWidth / 2,
        y: containerHeight / 2,
        fixed: true,
        symbolSize: 50,
        category: 1,
        itemStyle: { color: '#52c41a', borderColor: '#237804', borderWidth: 4 },
        label: { show: true, fontSize: 14, fontWeight: 'bold', position: 'right', color: '#237804' }
    });
    
    // 系列 -> 车型 的边
    links.push({
        source: seriesNode.id,
        target: modelNode.id,
        relation: 'belongs_to',
        lineStyle: { color: '#91caff', width: 3, curveness: 0 }
    });
    
    // 技术节点 - 底部均匀分布
    const techCount = techNodes.length;
    techNodes.forEach((tech, i) => {
        const x = (containerWidth / (techCount + 1)) * (i + 1);
        nodes.push({
            id: tech.id,
            name: tech.name,
            x: x,
            y: containerHeight - 60,
            fixed: true,
            symbolSize: 28,
            category: 2,
            itemStyle: { color: '#fa8c16', borderColor: '#d46b08', borderWidth: 2 },
            label: { show: true, fontSize: 10, position: 'bottom', color: '#d46b08' }
        });
        
        // 车型 -> 技术 的边
        links.push({
            source: modelNode.id,
            target: tech.id,
            relation: 'equipped_with',
            lineStyle: { color: '#ffd591', width: 2, curveness: 0.2 }
        });
    });
    
    const categories = [
        { name: '系列', itemStyle: { color: '#1890ff' } },
        { name: '车型', itemStyle: { color: '#52c41a' } },
        { name: '技术', itemStyle: { color: '#fa8c16' } }
    ];
    
    const option = {
        title: {
            text: `${modelData.model_name} 关系树`,
            left: 'center',
            top: 10,
            textStyle: { fontSize: 14, color: '#333' }
        },
        tooltip: {
            trigger: 'item',
            formatter: function(params) {
                if (params.dataType === 'node') {
                    const layerNames = ['🏭 系列', '🚗 车型', '⚡ 技术'];
                    return `<b>${params.data.name}</b><br/>类型: ${layerNames[params.data.category]}`;
                }
                return '';
            }
        },
        series: [{
            type: 'graph',
            layout: 'none',
            data: nodes,
            links: links,
            categories: categories,
            roam: true,
            label: { show: true },
            lineStyle: { curveness: 0.2 }
        }]
    };
    
    graphChart.setOption(option);
}

// 在全局图中高亮指定车型
function highlightModelInGraph(modelId) {
    if (!graphChart || !currentGraphData) return;
    
    const targetId = 'm_' + modelId;
    
    // 找出相关的节点
    const relatedNodes = new Set([targetId]);
    const relatedLinks = [];
    
    currentGraphData.links.forEach(link => {
        if (link.source === targetId || link.target === targetId) {
            relatedNodes.add(link.source);
            relatedNodes.add(link.target);
            relatedLinks.push({ source: link.source, target: link.target });
        }
    });
    
    // 更新高亮
    highlightRelatedNodes(relatedNodes, relatedLinks);
}

// 构建树状数据结构
function buildTreeData(data) {
    console.log('构建树数据，输入:', data);
    
    // 分离各层节点
    const seriesNodes = data.nodes.filter(n => n.layer === 0);
    const modelNodes = data.nodes.filter(n => n.layer === 1);
    const techNodes = data.nodes.filter(n => n.layer === 2);
    
    console.log('系列节点:', seriesNodes.length, '车型节点:', modelNodes.length, '技术节点:', techNodes.length);
    
    // 创建 ID 到节点的映射
    const nodeMap = {};
    data.nodes.forEach(n => nodeMap[n.id] = { ...n, children: [] });
    
    // 创建系列到车型的映射
    const seriesModelMap = {}; // series_id -> [model_ids]
    const modelTechMap = {};   // model_id -> [tech_ids]
    
    data.links.forEach(link => {
        if (link.relation === 'belongs_to') {
            // series -> model (source是series, target是model)
            if (!seriesModelMap[link.source]) {
                seriesModelMap[link.source] = [];
            }
            seriesModelMap[link.source].push(link.target);
        } else if (link.relation === 'equipped_with') {
            // model -> tech
            if (!modelTechMap[link.source]) {
                modelTechMap[link.source] = [];
            }
            modelTechMap[link.source].push(link.target);
        }
    });
    
    console.log('系列-车型映射:', seriesModelMap);
    console.log('车型-技术映射:', modelTechMap);
    
    // 构建森林（多棵树）
    const trees = seriesNodes.map(series => {
        const seriesTree = {
            name: series.name,
            id: series.id,
            layer: 0,
            itemStyle: { color: '#1890ff' },
            label: { color: '#1890ff', fontWeight: 'bold', fontSize: 13 },
            children: []
        };
        
        // 获取该系列下的所有车型
        const modelIds = seriesModelMap[series.id] || [];
        modelIds.forEach(modelId => {
            const model = nodeMap[modelId];
            if (model) {
                const modelTree = {
                    name: model.name,
                    id: model.id,
                    layer: 1,
                    itemStyle: { color: '#52c41a' },
                    label: { color: '#52c41a', fontSize: 11 },
                    children: []
                };
                
                // 获取该车型搭载的所有技术
                const techIds = modelTechMap[modelId] || [];
                techIds.forEach(techId => {
                    const tech = nodeMap[techId];
                    if (tech) {
                        modelTree.children.push({
                            name: tech.name,
                            id: tech.id,
                            layer: 2,
                            itemStyle: { color: '#fa8c16' },
                            label: { color: '#fa8c16', fontSize: 10 }
                        });
                    }
                });
                
                seriesTree.children.push(modelTree);
            }
        });
        
        return seriesTree;
    });
    
    return trees;
}

// 全局变量 - 存储当前图数据
let currentGraphData = null;
let highlightedNodes = new Set();
let highlightedLinks = new Set();

// 渲染三层分布图
function renderTreeGraph(data) {
    console.log('========== 开始渲染图表 ==========');
    
    if (!data.nodes || !data.links) {
        console.error('数据格式错误：缺少 nodes 或 links');
        showGraphError('数据格式错误');
        return;
    }
    
    // 存储数据供点击使用
    currentGraphData = data;
    
    const container = document.getElementById('graph-container');
    if (!container) {
        console.error('图容器不存在');
        return;
    }
    
    // 设置容器尺寸
    container.style.width = '100%';
    container.style.height = '350px';
    
    if (typeof echarts === 'undefined') {
        console.error('ECharts 未加载');
        showGraphError('ECharts 未加载');
        return;
    }
    
    // 销毁旧图表
    if (graphChart) {
        graphChart.dispose();
    }
    
    graphChart = echarts.init(container);
    
    const containerWidth = container.offsetWidth || 800;
    const containerHeight = container.offsetHeight || 350;
    
    // 设置节点样式
    const categories = [
        { name: '系列', itemStyle: { color: '#1890ff' } },
        { name: '车型', itemStyle: { color: '#52c41a' } },
        { name: '技术', itemStyle: { color: '#fa8c16' } }
    ];
    
    // 按层分类节点
    const seriesNodes = data.nodes.filter(n => n.layer === 0);
    const modelNodes = data.nodes.filter(n => n.layer === 1);
    const techNodes = data.nodes.filter(n => n.layer === 2);
    
    console.log('节点数量 - 系列:', seriesNodes.length, '车型:', modelNodes.length, '技术:', techNodes.length);
    
    // 三层Y坐标 (顶部系列，中间车型，底部技术)
    const layerY = [40, 175, 310];
    
    const nodes = [];
    
    // 系列节点（顶层）- 大圆，显示名称
    seriesNodes.forEach((n, i) => {
        const x = (containerWidth / (seriesNodes.length + 1)) * (i + 1);
        nodes.push({
            id: n.id,
            name: n.name,
            x: x,
            y: layerY[0],
            fixed: true,
            symbolSize: 35,
            category: 0,
            layer: 0,
            itemStyle: { color: '#1890ff', borderColor: '#0958d9', borderWidth: 3 },
            label: { 
                show: true, 
                fontSize: 12,
                fontWeight: 'bold',
                position: 'top',
                color: '#1890ff',
                distance: 5
            }
        });
    });
    
    // 车型节点（中层）- 按系列分组布局
    // 首先建立系列到车型的映射
    const seriesModelMap = {};
    data.links.filter(l => l.relation === 'belongs_to').forEach(link => {
        if (!seriesModelMap[link.source]) {
            seriesModelMap[link.source] = [];
        }
        seriesModelMap[link.source].push(link.target);
    });
    
    // 按系列分组放置车型
    let modelIndex = 0;
    const modelPositions = {};
    seriesNodes.forEach((series, si) => {
        const modelsInSeries = seriesModelMap[series.id] || [];
        const seriesX = (containerWidth / (seriesNodes.length + 1)) * (si + 1);
        const modelSpread = containerWidth / (seriesNodes.length + 1) * 0.8;
        
        modelsInSeries.forEach((modelId, mi) => {
            const offsetX = (mi - (modelsInSeries.length - 1) / 2) * (modelSpread / Math.max(modelsInSeries.length, 1));
            modelPositions[modelId] = {
                x: seriesX + offsetX,
                y: layerY[1]
            };
        });
    });
    
    modelNodes.forEach((n) => {
        const pos = modelPositions[n.id] || { x: containerWidth / 2, y: layerY[1] };
        nodes.push({
            id: n.id,
            name: n.name,
            x: pos.x,
            y: pos.y,
            fixed: true,
            symbolSize: 16,
            category: 1,
            layer: 1,
            series_id: n.series_id,
            itemStyle: { color: '#52c41a', borderColor: '#389e0d', borderWidth: 1 },
            label: { 
                show: false,
                fontSize: 9,
                color: '#52c41a'
            }
        });
    });
    
    // 技术节点（底层）
    techNodes.forEach((n, i) => {
        const x = (containerWidth / (techNodes.length + 1)) * (i + 1);
        nodes.push({
            id: n.id,
            name: n.name,
            x: x,
            y: layerY[2],
            fixed: true,
            symbolSize: 25,
            category: 2,
            layer: 2,
            itemStyle: { color: '#fa8c16', borderColor: '#d46b08', borderWidth: 2 },
            label: { 
                show: true, 
                fontSize: 9,
                position: 'bottom',
                color: '#d46b08',
                distance: 3
            }
        });
    });
    
    // 边数据 - 使用柔和的颜色
    const links = data.links.map(link => ({
        source: link.source,
        target: link.target,
        relation: link.relation,
        lineStyle: {
            color: link.relation === 'belongs_to' ? '#91caff' : '#ffd591',
            width: 1,
            opacity: 0.5,
            curveness: 0.2
        }
    }));
    
    const option = {
        tooltip: {
            trigger: 'item',
            formatter: function(params) {
                if (params.dataType === 'node') {
                    const layerNames = ['🏭 系列', '🚗 车型', '⚡ 技术'];
                    return `<div style="font-weight:bold;margin-bottom:4px;">${params.data.name}</div>` +
                           `<div>类型: ${layerNames[params.data.layer]}</div>` +
                           `<div style="font-size:11px;color:#999;margin-top:4px;">点击查看关联关系</div>`;
                } else if (params.dataType === 'edge') {
                    const relName = params.data.relation === 'belongs_to' ? '包含' : '搭载';
                    return `关系: ${relName}`;
                }
                return '';
            }
        },
        series: [{
            type: 'graph',
            layout: 'none',
            data: nodes,
            links: links,
            categories: categories,
            roam: true,
            zoom: 1,
            label: {
                show: true,
                position: 'right',
                fontSize: 10
            },
            emphasis: {
                focus: 'adjacency',
                label: {
                    show: true,
                    fontSize: 12,
                    fontWeight: 'bold'
                },
                lineStyle: {
                    width: 3,
                    opacity: 1
                }
            },
            lineStyle: {
                curveness: 0.2
            }
        }]
    };
    
    graphChart.setOption(option);
    
    // 点击节点事件 - 高亮关联
    graphChart.on('click', function(params) {
        if (params.dataType === 'node' && params.data) {
            handleGraphNodeClick(params.data);
        }
    });
    
    // 双击重置
    graphChart.on('dblclick', function() {
        resetGraphHighlight();
    });
    
    // 点击空白处重置
    graphChart.getZr().on('click', function(e) {
        if (!e.target) {
            resetGraphHighlight();
        }
    });
    
    // 窗口大小变化时调整图表
    window.addEventListener('resize', () => {
        if (graphChart) graphChart.resize();
    });
    
    console.log('图表渲染完成，节点数:', nodes.length, '边数:', links.length);
}

// 处理图节点点击 - 高亮关联关系
function handleGraphNodeClick(nodeData) {
    console.log('点击节点:', nodeData);
    
    // 找出所有关联的节点和边
    const relatedNodes = new Set([nodeData.id]);
    const relatedLinks = [];
    
    if (currentGraphData) {
        currentGraphData.links.forEach(link => {
            if (link.source === nodeData.id) {
                relatedNodes.add(link.target);
                relatedLinks.push({ source: link.source, target: link.target });
            }
            if (link.target === nodeData.id) {
                relatedNodes.add(link.source);
                relatedLinks.push({ source: link.source, target: link.target });
            }
        });
        
        // 如果是系列节点，还要找出车型关联的技术
        if (nodeData.layer === 0) {
            const modelsInSeries = [];
            currentGraphData.links.forEach(link => {
                if (link.source === nodeData.id && link.relation === 'belongs_to') {
                    modelsInSeries.push(link.target);
                }
            });
            
            // 找出这些车型关联的技术
            currentGraphData.links.forEach(link => {
                if (modelsInSeries.includes(link.source) && link.relation === 'equipped_with') {
                    relatedNodes.add(link.target);
                    relatedLinks.push({ source: link.source, target: link.target });
                }
            });
        }
        
        // 如果是技术节点，找出使用该技术的车型的系列
        if (nodeData.layer === 2) {
            const modelsWithTech = [];
            currentGraphData.links.forEach(link => {
                if (link.target === nodeData.id && link.relation === 'equipped_with') {
                    modelsWithTech.push(link.source);
                    relatedNodes.add(link.source);
                }
            });
            
            // 找出这些车型所属的系列
            currentGraphData.links.forEach(link => {
                if (modelsWithTech.includes(link.target) && link.relation === 'belongs_to') {
                    relatedNodes.add(link.source);
                    relatedLinks.push({ source: link.source, target: link.target });
                }
            });
        }
    }
    
    // 更新图表高亮
    highlightRelatedNodes(relatedNodes, relatedLinks);
    
    // 显示关系信息面板
    showRelationInfo(nodeData, relatedNodes);
    
    // 原有的点击操作
    if (nodeData.layer === 0) {
        // 点击系列 - 筛选该系列的车型
        const seriesId = parseInt(nodeData.id.replace('s_', ''));
        AppState.currentSeriesId = seriesId;
        
        // 更新侧边栏选中状态
        document.querySelectorAll('.series-item').forEach(item => {
            item.classList.toggle('active', parseInt(item.dataset.seriesId) === seriesId);
            const radio = item.querySelector('input[type="radio"]');
            if (radio) radio.checked = parseInt(item.dataset.seriesId) === seriesId;
        });
        
        loadModels();
        showToast(`已筛选: ${nodeData.name} 系列`, 'info');
    } else if (nodeData.layer === 1) {
        // 点击车型 - 显示详情
        const modelId = parseInt(nodeData.id.replace('m_', ''));
        showModelDetail(modelId);
    } else if (nodeData.layer === 2) {
        // 点击技术 - 搜索搭载该技术的车型
        document.getElementById('global-search').value = nodeData.name;
        handleSearch();
        showToast(`正在搜索搭载"${nodeData.name}"的车型`, 'info');
    }
}

// 高亮相关节点和边
function highlightRelatedNodes(relatedNodes, relatedLinks) {
    if (!graphChart || !currentGraphData) return;
    
    // 获取当前option
    const option = graphChart.getOption();
    if (!option.series || !option.series[0]) return;
    
    // 更新节点样式
    const newNodes = option.series[0].data.map(node => {
        const isRelated = relatedNodes.has(node.id);
        return {
            ...node,
            itemStyle: {
                ...node.itemStyle,
                opacity: isRelated ? 1 : 0.2,
                borderWidth: isRelated ? 3 : 1
            },
            label: {
                ...node.label,
                show: isRelated || node.layer !== 1, // 高亮时显示车型标签
                fontWeight: isRelated ? 'bold' : 'normal'
            }
        };
    });
    
    // 更新边样式
    const newLinks = option.series[0].links.map(link => {
        const isRelated = relatedLinks.some(rl => 
            rl.source === link.source && rl.target === link.target
        );
        return {
            ...link,
            lineStyle: {
                ...link.lineStyle,
                opacity: isRelated ? 0.9 : 0.1,
                width: isRelated ? 2 : 1
            }
        };
    });
    
    graphChart.setOption({
        series: [{
            data: newNodes,
            links: newLinks
        }]
    });
}

// 显示关系信息
function showRelationInfo(nodeData, relatedNodes) {
    // 功能已移除 - 不再显示关联节点数量信息
}

// 重置图高亮
function resetGraphHighlight() {
    if (!graphChart || !currentGraphData) return;
    
    // 重新渲染恢复原样
    renderTreeGraph(currentGraphData);
    
    // 移除选中信息
    const selectedInfo = document.getElementById('selected-node-info');
    if (selectedInfo) {
        selectedInfo.remove();
    }
}

// 页面加载时初始化图窗口
document.addEventListener('DOMContentLoaded', function() {
    // 延迟初始化，确保其他组件加载完成
    setTimeout(initGraphWindow, 500);
});

// ====================
// 添加元素功能
// ====================

let currentAddType = 'model'; // 'model' 或 'tech'

// 显示添加模态框
window.showAddModal = function(type) {
    currentAddType = type;
    const modal = document.getElementById('add-modal');
    const title = document.getElementById('modal-title');
    const modelForm = document.getElementById('add-model-form');
    const techForm = document.getElementById('add-tech-form');
    
    if (type === 'model') {
        title.innerHTML = '<i class="fas fa-car"></i> 添加新车型';
        modelForm.style.display = 'block';
        techForm.style.display = 'none';
        // 填充系列下拉框
        populateSeriesSelect();
        // 填充技术复选框
        populateTechCheckboxes();
    } else {
        title.innerHTML = '<i class="fas fa-microchip"></i> 添加新技术';
        modelForm.style.display = 'none';
        techForm.style.display = 'block';
    }
    
    modal.classList.add('active');
};

// 关闭模态框
window.closeAddModal = function(event) {
    if (event && event.target !== event.currentTarget) return;
    document.getElementById('add-modal').classList.remove('active');
    // 重置表单
    document.getElementById('add-model-form').reset();
    document.getElementById('add-tech-form').reset();
};

// 填充系列下拉框
function populateSeriesSelect() {
    const select = document.getElementById('series-select');
    select.innerHTML = AppState.seriesList.map(s => 
        `<option value="${s.series_id}">${s.series_name}</option>`
    ).join('');
}

// 填充技术复选框
function populateTechCheckboxes() {
    const container = document.getElementById('tech-checkboxes');
    container.innerHTML = AppState.techsList.map(t => `
        <label class="tech-checkbox">
            <input type="checkbox" name="techs" value="${t.tech_id}">
            ${t.tech_name}
        </label>
    `).join('');
    
    // 复选框点击样式
    container.querySelectorAll('.tech-checkbox').forEach(label => {
        const checkbox = label.querySelector('input');
        checkbox.addEventListener('change', function() {
            label.classList.toggle('checked', this.checked);
        });
    });
}

// 提交添加
window.submitAdd = async function() {
    if (currentAddType === 'model') {
        await submitAddModel();
    } else {
        await submitAddTech();
    }
};

// 提交添加车型
async function submitAddModel() {
    const form = document.getElementById('add-model-form');
    const formData = new FormData(form);
    
    // 验证必填字段
    if (!formData.get('model_name') || !formData.get('series_id') || !formData.get('price') || !formData.get('energy_type')) {
        showToast('请填写必填字段', 'error');
        return;
    }
    
    // 收集选中的技术
    const techIds = [];
    form.querySelectorAll('input[name="techs"]:checked').forEach(cb => {
        techIds.push(parseInt(cb.value));
    });
    
    const data = {
        model_name: formData.get('model_name'),
        series_id: parseInt(formData.get('series_id')),
        price: parseFloat(formData.get('price')),
        range_km: parseFloat(formData.get('range_km')) || 0,
        energy_type: formData.get('energy_type'),
        body_type: formData.get('body_type') || '',
        seats: parseInt(formData.get('seats')) || 5,
        launch_year: formData.get('launch_year') || new Date().getFullYear().toString(),
        tech_ids: techIds
    };
    
    try {
        const res = await fetch('/api/model/add', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
        const json = await res.json();
        
        if (json.ok) {
            showToast('车型添加成功！', 'success');
            closeAddModal();
            // 刷新数据
            await Promise.all([loadModels(), loadStats(), loadGraphData()]);
        } else {
            showToast(json.message || '添加失败', 'error');
        }
    } catch (e) {
        console.error('添加车型失败:', e);
        showToast('添加失败: ' + e.message, 'error');
    }
}

// 提交添加技术
async function submitAddTech() {
    const form = document.getElementById('add-tech-form');
    const formData = new FormData(form);
    
    if (!formData.get('tech_name')) {
        showToast('请填写技术名称', 'error');
        return;
    }
    
    const data = {
        tech_name: formData.get('tech_name'),
        intro: formData.get('intro') || ''
    };
    
    try {
        const res = await fetch('/api/tech/add', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
        const json = await res.json();
        
        if (json.ok) {
            showToast('技术添加成功！', 'success');
            closeAddModal();
            // 刷新数据
            await Promise.all([loadTechs(), loadStats(), loadGraphData()]);
        } else {
            showToast(json.message || '添加失败', 'error');
        }
    } catch (e) {
        console.error('添加技术失败:', e);
        showToast('添加失败: ' + e.message, 'error');
    }
}
