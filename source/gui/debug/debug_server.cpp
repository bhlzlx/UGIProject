#include "debug/debug_server.h"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
#define INVALID_SOCKET_VAL INVALID_SOCKET
#define SOCKET_ERROR_VAL  SOCKET_ERROR
#define CLOSE_SOCKET      closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
#define INVALID_SOCKET_VAL (-1)
#define SOCKET_ERROR_VAL  (-1)
#define CLOSE_SOCKET      close
#endif

#include <core/ui/component.h>
#include <core/ui/stage.h>
#include <core/display_objects/display_components.h>
#include <core/display_objects/display_object_utility.h>

namespace gui {

    // ---------------------------------------------------------------------------
    // Static helpers
    // ---------------------------------------------------------------------------

    static const char* objectTypeToString(ObjectType t) {
        switch (t) {
        case ObjectType::Image:       return "Image";
        case ObjectType::MovieClip:   return "MovieClip";
        case ObjectType::Swf:         return "Swf";
        case ObjectType::Graph:       return "Graph";
        case ObjectType::Loader:      return "Loader";
        case ObjectType::Group:       return "Group";
        case ObjectType::Text:        return "Text";
        case ObjectType::RichText:    return "RichText";
        case ObjectType::InputText:   return "InputText";
        case ObjectType::Component:   return "Component";
        case ObjectType::List:        return "List";
        case ObjectType::Label:       return "Label";
        case ObjectType::Button:      return "Button";
        case ObjectType::ComboBox:    return "ComboBox";
        case ObjectType::ProgressBar: return "ProgressBar";
        case ObjectType::Slider:      return "Slider";
        case ObjectType::ScrollBar:   return "ScrollBar";
        case ObjectType::Tree:        return "Tree";
        case ObjectType::Loader3D:    return "Loader3D";
        case ObjectType::Root:        return "Root";
        default:                      return "Unknown";
        }
    }

    static std::string entityIdStr(entt::entity e) {
        std::ostringstream ss;
        ss << "e" << (uint32_t)entt::to_integral(e);
        return ss.str();
    }

    static const char* componentIcon(std::string const& name) {
        if (name == "basic_transform") return "📐";
        if (name == "children")        return "👶";
        if (name == "parent")          return "👆";
        if (name == "parent_batch")    return "📦";
        if (name == "batch_node")      return "🔀";
        if (name == "batch_data")      return "📊";
        if (name == "graphics")        return "🎨";
        if (name == "visible")         return "👁";
        if (name == "final_visible")   return "✅";
        if (name == "is_root")         return "🏠";
        if (name == "skew")            return "📐";
        if (name == "scale")           return "📏";
        if (name == "rotation")        return "🔄";
        if (name == "owner")           return "🔗";
        if (name == "image_desc_t")      return "🖼";
        if (name == "image_ext")       return "🎭";
        return "🔹";
    }

    // ---------------------------------------------------------------------------
    // Embedded HTML page (served at GET /)
    // ---------------------------------------------------------------------------

    static std::string getHtmlPage() {
        static const std::string page = []{
            std::string s;
            // Part 1: HTML head + CSS
            s += R"HTML(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>UGI UI Debugger</title>
<style>
:root {
    --bg: #1a1a2e; --surface: #16213e; --surface2: #0f3460;
    --text: #e0e0e0; --text2: #a0a0b0; --accent: #e94560;
    --accent2: #53a8b6; --border: #2a2a4a; --hover: #1f3060;
    --type-Component: #ff9800; --type-Image: #4caf50; --type-Text: #2196f3;
    --type-Button: #e94560; --type-List: #9c27b0; --type-Root: #00bcd4;
    --type-default: #607d8b; --tab-active: #e94560;
}
* { margin:0; padding:0; box-sizing:border-box; }
body {
    font-family: 'Segoe UI', system-ui, sans-serif;
    background: var(--bg); color: var(--text);
    height: 100vh; display: flex; overflow: hidden;
}
/* ---- left panel: tree ---- */
#tree-panel {
    width: 420px; min-width: 300px; background: var(--surface);
    border-right: 1px solid var(--border);
    display: flex; flex-direction: column; overflow: hidden;
}
#tabs {
    display: flex; background: var(--surface2);
    border-bottom: 1px solid var(--border);
}
#tabs button {
    flex: 1; padding: 10px 12px; background: none; border: none;
    color: var(--text2); cursor: pointer; font-size: 12px;
    font-weight: 600; border-bottom: 2px solid transparent;
    transition: all 0.15s;
}
#tabs button.active {
    color: var(--tab-active); border-bottom-color: var(--tab-active);
    background: var(--surface);
}
#tabs button:hover:not(.active) { color: var(--text); }
#tree-toolbar {
    padding: 8px 12px; display: flex; align-items: center;
    justify-content: space-between; gap: 8px;
    background: var(--surface); border-bottom: 1px solid var(--border);
}
#tree-toolbar h2 { font-size: 12px; font-weight: 600; white-space: nowrap; }
#tree-toolbar .controls { display: flex; gap: 4px; align-items: center; }
#tree-toolbar button {
    background: var(--hover); border: 1px solid var(--border);
    color: var(--text); padding: 3px 8px; border-radius: 3px;
    cursor: pointer; font-size: 11px;
}
#tree-toolbar button:hover { background: var(--accent); }
#auto-refresh { width: 14px; height: 14px; accent-color: var(--accent); }
#tree-container {
    flex: 1; overflow: auto; padding: 4px 0;
}
.tree-node { user-select: none; cursor: pointer; }
.tree-row {
    display: flex; align-items: center; padding: 2px 8px;
    font-size: 12px; line-height: 22px; white-space: nowrap;
    border-left: 3px solid transparent;
}
.tree-row:hover { background: var(--hover); }
.tree-row.selected { background: var(--surface2); border-left-color: var(--accent); }
.tree-toggle {
    width: 16px; text-align: center; color: var(--text2);
    font-size: 10px; flex-shrink: 0;
}
.tree-toggle.expandable { cursor: pointer; }
.tree-icon { width: 16px; text-align: center; margin-right: 4px; flex-shrink: 0; }
.tree-name { font-weight: 500; }
.tree-type {
    margin-left: 6px; font-size: 10px; padding: 1px 5px;
    border-radius: 3px; opacity: 0.85;
}
.tree-id {
    margin-left: auto; color: var(--text2);
    font-size: 10px; opacity: 0.6;
}
.tree-comps {
    margin-left: auto; color: var(--text2);
    font-size: 9px; opacity: 0.5; max-width: 120px;
    overflow: hidden; text-overflow: ellipsis;
}
.hidden-node { opacity: 0.45; }
/* ---- right panel: detail ---- */
#detail-panel {
    flex: 1; display: flex; flex-direction: column; overflow: hidden;
}
#detail-header {
    padding: 12px 16px; background: var(--surface2);
    font-size: 14px; font-weight: 600;
}
#detail-content { flex: 1; overflow: auto; padding: 16px; }
.prop-table { width: 100%; border-collapse: collapse; }
.prop-table td {
    padding: 3px 12px; font-size: 11px;
    border-bottom: 1px solid var(--border);
}
.prop-table td:first-child {
    color: var(--text2); width: 130px; white-space: nowrap;
}
.prop-table td:last-child { color: var(--text); word-break: break-all; }
.prop-section {
    font-size: 10px; font-weight: 700; color: var(--accent2);
    text-transform: uppercase; letter-spacing: 1px;
    margin: 12px 0 4px; padding-bottom: 2px;
    border-bottom: 1px solid var(--border);
}
.comp-tag {
    display: inline-block; font-size: 10px; padding: 1px 6px;
    margin: 1px 2px; border-radius: 3px;
    background: var(--surface2); color: var(--accent2);
}
#empty-hint { color: var(--text2); font-size: 14px; text-align: center; margin-top: 80px; }
.status-bar {
    padding: 4px 12px; font-size: 10px; color: var(--text2);
    background: var(--surface2); border-top: 1px solid var(--border);
    display: flex; justify-content: space-between;
}
</style>
)HTML";
            // Part 2: HTML body
            s += R"HTML(
</head>
<body>

<div id="tree-panel">
    <div id="tabs">
        <button id="tab-object" class="active" onclick="switchTab('object')">🧩 Object Tree</button>
        <button id="tab-display" onclick="switchTab('display')">🖥 Display Tree</button>
    </div>
    <div id="tree-toolbar">
        <h2 id="tree-title">Object Tree</h2>
        <div class="controls">
            <label style="font-size:10px;display:flex;align-items:center;gap:3px;">
                <input type="checkbox" id="auto-refresh" checked>auto
            </label>
            <button onclick="refresh()" title="Refresh (F5)">🔄</button>
            <button onclick="expandAll()">⊞</button>
            <button onclick="collapseAll()">⊟</button>
        </div>
    </div>
    <div id="tree-container"></div>
    <div class="status-bar">
        <span id="status">Connecting...</span>
        <span id="node-count"></span>
    </div>
</div>

<div id="detail-panel">
    <div id="detail-header">📋 Select a node</div>
    <div id="detail-content">
        <div id="empty-hint">Click a node in the tree to see its properties</div>
    </div>
</div>

<script>
// ---- state ----
let objectTree = null;
let displayTree = null;
let currentTab = 'object';   // 'object' | 'display'
let selectedPath = null;
let collapsed = {};
let autoRefresh = true;

const TYPE_CLASS = {
    Component:'type-Component', Image:'type-Image', MovieClip:'type-Image',
    Text:'type-Text', RichText:'type-Text', InputText:'type-Text',
    Button:'type-Button', List:'type-List', Root:'type-Root', Label:'type-Label'
};
const TYPE_ICON = {
    Root:'🏠', Component:'📦', Image:'🖼', MovieClip:'🎬', Text:'📝',
    RichText:'📄', InputText:'✏️', Button:'🔘', List:'📋', Label:'🏷',
    ComboBox:'📥', ProgressBar:'📊', Slider:'🎚', ScrollBar:'📜',
    Group:'📁', Graph:'📈', Loader:'⏳', Loader3D:'🧊', Tree:'🌲'
};
const COMP_NAMES = {
    'basic_transform':'tf','children':'ch','parent':'pa','parent_batch':'pb',
    'batch_node':'bn','batch_data':'bd','graphics':'gfx','visible':'vis',
    'final_visible':'fv','is_root':'root','skew':'sk','scale':'sc',
    'rotation':'rot','owner':'own','image_desc_t':'mesh','image_ext':'ext',
    'transform_dirty':'Δtf','batch_node_dirty':'Δbn','batch_dirty':'Δbd',
    'visible_dirty':'Δvis','mesh_dirty':'Δmesh','font_mesh':'fnt'
};

// ---- data fetching ----
async function fetchTree(endpoint) {
    const resp = await fetch(endpoint);
    return await resp.json();
}

async function refresh() {
    try {
        const [obj, disp] = await Promise.all([
            fetchTree('/api/tree'),
            fetchTree('/api/display-tree')
        ]);
        objectTree = obj;
        displayTree = disp;
        document.getElementById('status').textContent =
            'Updated: ' + new Date().toLocaleTimeString();
        updateNodeCount();
        renderTree();
    } catch(e) {
        document.getElementById('status').textContent = 'Error — retrying...';
    }
}

function updateNodeCount() {
    const data = currentTab === 'object' ? objectTree : displayTree;
    document.getElementById('node-count').textContent =
        'Nodes: ' + countNodes(data);
}

function countNodes(node) {
    if (!node) return 0;
    let n = 1;
    const kids = node.children || node._children;
    if (kids) kids.forEach(c => n += countNodes(c));
    return n;
}

// ---- tab switching ----
function switchTab(tab) {
    currentTab = tab;
    document.getElementById('tab-object').classList.toggle('active', tab==='object');
    document.getElementById('tab-display').classList.toggle('active', tab==='display');
    document.getElementById('tree-title').textContent =
        tab==='object' ? '🧩 Object Tree' : '🖥 Display Tree';
    selectedPath = null;
    document.getElementById('detail-header').textContent = '📋 Select a node';
    document.getElementById('detail-content').innerHTML =
        '<div id="empty-hint">Click a node in the tree to see its properties</div>';
    updateNodeCount();
    renderTree();
}

// ---- rendering ----
function renderTree() {
    const container = document.getElementById('tree-container');
    container.innerHTML = '';
    const data = currentTab === 'object' ? objectTree : displayTree;
    if (data) {
        if (currentTab === 'object') {
            renderObjectNode(container, data, '', 0);
        } else {
            renderDisplayNode(container, data, '', 0);
        }
    }
    if (selectedPath) {
        const row = document.querySelector('[data-path="' + CSS.escape(selectedPath) + '"]');
        if (row) row.classList.add('selected');
    }
}

// ---- Object tree rendering ----
function renderObjectNode(parent, node, path, depth) {
    const myPath = path ? path + '/' + depth : '0';
    const hasKids = node.children && node.children.length > 0;
    const isCollapsed = collapsed[myPath] === true;

    const div = document.createElement('div');
    div.className = 'tree-node';
    const row = document.createElement('div');
    row.className = 'tree-row';
    row.setAttribute('data-path', myPath);
    if (!node.visible) row.classList.add('hidden-node');

    // toggle
    const toggle = document.createElement('span');
    toggle.className = 'tree-toggle';
    if (hasKids) {
        toggle.classList.add('expandable');
        toggle.textContent = isCollapsed ? '▶' : '▼';
        toggle.onclick = (e) => { e.stopPropagation(); collapsed[myPath]=!collapsed[myPath]; renderTree(); };
    }
    row.appendChild(toggle);

    // indent via padding
    row.style.paddingLeft = (depth * 16 + 8) + 'px';

    // icon + name + type + id
    const icon = document.createElement('span');
    icon.className = 'tree-icon';
    icon.textContent = TYPE_ICON[node.type] || '❓';
    row.appendChild(icon);
    const nm = document.createElement('span');
    nm.className = 'tree-name';
    nm.textContent = node.name || '(unnamed)';
    row.appendChild(nm);
    const tb = document.createElement('span');
    tb.className = 'tree-type ' + (TYPE_CLASS[node.type]||'');
    tb.textContent = node.type;
    row.appendChild(tb);
    if (node.id) {
        const ids = document.createElement('span');
        ids.className = 'tree-id';
        ids.textContent = '#' + node.id;
        row.appendChild(ids);
    }
    row.onclick = () => selectObjectNode(node, myPath, row);
    div.appendChild(row);
    parent.appendChild(div);
    if (hasKids && !isCollapsed) {
        node.children.forEach((c,i) => renderObjectNode(parent, c, myPath, i));
    }
}

// ---- DisplayObject tree rendering ----
function renderDisplayNode(parent, node, path, depth) {
    const myPath = path ? path + '/' + depth : '0';
    const kids = node._children;
    const hasKids = kids && kids.length > 0;
    const isCollapsed = collapsed[myPath] === true;

    const div = document.createElement('div');
    div.className = 'tree-node';
    const row = document.createElement('div');
    row.className = 'tree-row';
    row.setAttribute('data-path', myPath);
    if (node.visible === false) row.classList.add('hidden-node');

    // toggle
    const toggle = document.createElement('span');
    toggle.className = 'tree-toggle';
    if (hasKids) {
        toggle.classList.add('expandable');
        toggle.textContent = isCollapsed ? '▶' : '▼';
        toggle.onclick = (e) => { e.stopPropagation(); collapsed[myPath]=!collapsed[myPath]; renderTree(); };
    }
    row.appendChild(toggle);

    // indent via padding
    row.style.paddingLeft = (depth * 16 + 8) + 'px';

    // entity ID
    const eid = document.createElement('span');
    eid.style.cssText = 'color:var(--accent2);font-size:11px;font-weight:600;margin-right:6px;';
    eid.textContent = node._entityId;
    row.appendChild(eid);

    // components as tiny tags
    if (node._components) {
        const comps = document.createElement('span');
        comps.className = 'tree-comps';
        comps.textContent = node._components.map(c => COMP_NAMES[c]||c).join(' ');
        row.appendChild(comps);
    }

    row.onclick = () => selectDisplayNode(node, myPath, row);
    div.appendChild(row);
    parent.appendChild(div);
    if (hasKids && !isCollapsed) {
        kids.forEach((c,i) => renderDisplayNode(parent, c, myPath, i));
    }
}
)HTML";
            // Part 3: JS selection + detail + helpers
            s += R"HTML(

// ---- selection / detail ----
function selectObjectNode(node, path, row) {
    document.querySelectorAll('.tree-row.selected').forEach(r => r.classList.remove('selected'));
    row.classList.add('selected');
    selectedPath = path;
    showObjectDetail(node);
}

function selectDisplayNode(node, path, row) {
    document.querySelectorAll('.tree-row.selected').forEach(r => r.classList.remove('selected'));
    row.classList.add('selected');
    selectedPath = path;
    showDisplayDetail(node);
}

function showObjectDetail(node) {
    document.getElementById('detail-header').textContent =
        '🧩 ' + (node.type||'?') + ' — ' + (node.name||'(unnamed)');
    const ct = document.getElementById('detail-content');
    let h = '';
    h += sec('Identity');
    h += tbl([
        ['Type','<b>'+(node.type||'')+'</b>'],['Name',esc(node.name)],['ID',esc(node.id)]
    ]);
    h += sec('Geometry');
    h += tbl([
        ['Position',fmt(node.x)+', '+fmt(node.y)],
        ['Size',fmt(node.width)+' × '+fmt(node.height)],
        ['Source Size',fmt(node.sourceWidth)+' × '+fmt(node.sourceHeight)],
        ['Raw Size',fmt(node.rawWidth)+' × '+fmt(node.rawHeight)],
        ['Init Size',fmt(node.initWidth)+' × '+fmt(node.initHeight)]
    ]);
    h += sec('Transform');
    h += tbl([
        ['Scale',fmt(node.scaleX)+', '+fmt(node.scaleY)],
        ['Skew',fmt(node.skewX)+', '+fmt(node.skewY)],
        ['Pivot',fmt(node.pivotX)+', '+fmt(node.pivotY)+(node.pivotAsAnchor?' (anchor)':'')],
        ['Rotation',fmt(node.rotation)+' rad'],
        ['Alpha',fmt(node.alpha)]
    ]);
    h += sec('Flags');
    h += tbl([
        ['Visible',flag(node.visible)],
        ['Touchable',flag(node.touchable)],
        ['Grayed',flag(node.grayed)],
        ['SortingOrder',node.sortingOrder!=null?node.sortingOrder:'—']
    ]);
    ct.innerHTML = h;
}

function showDisplayDetail(node) {
    document.getElementById('detail-header').textContent =
        '🖥 ' + (node._entityId||'?') + ' — ECS Entity';
    const ct = document.getElementById('detail-content');
    let h = '';

    // components list
    h += sec('Components (' + (node._components?node._components.length:0) + ')');
    h += '<div style="margin:4px 0;">';
    if (node._components) {
        node._components.forEach(c => {
            h += '<span class="comp-tag">' + esc(c) + '</span>';
        });
    }
    h += '</div>';

    // basic transform
    if (node.position || node.size || node.pivot) {
        h += sec('Basic Transform');
        h += tbl([
            ['Position',fmt(node.position?.x)+', '+fmt(node.position?.y)],
            ['Size',fmt(node.size?.width)+' × '+fmt(node.size?.height)],
            ['Pivot',fmt(node.pivot?.x)+', '+fmt(node.pivot?.y)]
        ]);
    }
    // visibility
    h += sec('Visibility');
    h += tbl([
        ['Visible',flag(node.visible)],
        ['Final Visible',flag(node.finalVisible)],
        ['Is Root',flag(node.isRoot)]
    ]);
    // batch
    if (node.isBatchNode !== undefined || node.parentBatch !== undefined) {
        h += sec('Batch');
        h += tbl([
            ['Is Batch Node',flag(node.isBatchNode)],
            ['Parent Batch',node.parentBatch||'—'],
            ['Inst Index',node.instIndex!=null?node.instIndex:'—'],
            ['Batch Children',node.batchChildren!=null?node.batchChildren:'—'],
            ['Sub Batch Nodes',node.subBatchNodes!=null?node.subBatchNodes:'—']
        ]);
    }
    // graphics
    if (node.hasGraphics) {
        h += sec('Graphics');
        h += tbl([
            ['Render Type',node.renderType||'—'],
            ['Has Texture',flag(node.hasTexture)]
        ]);
    }
    // transform
    if (node.scale || node.skew || node.rotation !== undefined) {
        h += sec('Transform');
        h += tbl([
            ['Scale',node.scale?fmt(node.scale.x)+', '+fmt(node.scale.y):'—'],
            ['Skew',node.skew?fmt(node.skew.x)+', '+fmt(node.skew.y):'—'],
            ['Rotation',node.rotation!=null?fmt(node.rotation)+' rad':'—']
        ]);
    }
    // dirty flags
    const dirties = ['transform_dirty','batch_node_dirty','batch_dirty','visible_dirty','mesh_dirty'];
    const activeDirties = dirties.filter(d => node[d]);
    if (activeDirties.length > 0) {
        h += sec('Dirty Flags');
        h += '<div style="margin:4px 0;">';
        activeDirties.forEach(d => { h += '<span class="comp-tag" style="color:#ff9800;">'+esc(d)+'</span>'; });
        h += '</div>';
    }
    ct.innerHTML = h;
}

// ---- helpers ----
function sec(title) { return '<div class="prop-section">'+esc(title)+'</div>'; }
function tbl(rows) {
    let s = '<table class="prop-table">';
    rows.forEach(r => { s += '<tr><td>'+esc(r[0])+'</td><td>'+r[1]+'</td></tr>'; });
    return s + '</table>';
}
function flag(v) { return (v?'🟢 true':'🔴 false'); }
function fmt(v) {
    if (v==null) return '—';
    if (Number.isInteger(v)) return String(v);
    if (typeof v === 'number') return parseFloat(v.toFixed(3)).toString();
    return String(v);
}
function esc(s) {
    if (!s) return '—';
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

// ---- expand/collapse ----
function expandAll() {
    collapsed = {};
    renderTree();
}
function collapseAll() {
    const data = currentTab === 'object' ? objectTree : displayTree;
    walk(data, '0');
    function walk(node, path) {
        if (!node) return;
        const kids = node.children || node._children;
        if (kids && kids.length > 0) {
            collapsed[path] = true;
            kids.forEach((c,i) => walk(c, path+'/'+i));
        }
    }
    renderTree();
}

// ---- init ----
document.getElementById('auto-refresh').onchange = function() { autoRefresh = this.checked; };
document.addEventListener('keydown', function(e) {
    if (e.key === 'F5') { e.preventDefault(); refresh(); }
});
refresh();
setInterval(function() { if (autoRefresh) refresh(); }, 2000);
</script>
</body>
</html>
)HTML";
            return s;
        }();
        return page;
    }

    // ---------------------------------------------------------------------------
    // HTTP response builder
    // ---------------------------------------------------------------------------

    static std::string httpResponse(int code, std::string const& contentType,
                                     std::string const& body) {
        const char* status = (code == 200) ? "200 OK" : "404 Not Found";
        std::ostringstream ss;
        ss << "HTTP/1.1 " << status << "\r\n";
        ss << "Content-Type: " << contentType << "; charset=utf-8\r\n";
        ss << "Content-Length: " << body.size() << "\r\n";
        ss << "Connection: close\r\n";
        ss << "Access-Control-Allow-Origin: *\r\n";
        ss << "\r\n";
        ss << body;
        return ss.str();
    }

    // ---------------------------------------------------------------------------
    // DebugServer
    // ---------------------------------------------------------------------------

    DebugServer& DebugServer::Instance() {
        static DebugServer s;
        return s;
    }

    DebugServer::DebugServer()
        : listenSocket_(INVALID_SOCKET_VAL)
        , running_(false)
        , port_(9876)
    {}

    DebugServer::~DebugServer() {
        stop();
    }

    bool DebugServer::start(uint16_t port) {
        if (running_.load(std::memory_order_acquire)) {
            return true;
        }

        port_ = port;

#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            printf("[DebugServer] WSAStartup failed\n");
            return false;
        }
#endif

        listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket_ == INVALID_SOCKET_VAL) {
            printf("[DebugServer] socket() failed\n");
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        int optval = 1;
        setsockopt((socket_t)listenSocket_, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&optval, sizeof(optval));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons(port_);

        if (bind((socket_t)listenSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VAL) {
            printf("[DebugServer] bind() failed on port %u\n", port_);
            CLOSE_SOCKET((socket_t)listenSocket_);
            listenSocket_ = INVALID_SOCKET_VAL;
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        if (listen((socket_t)listenSocket_, 5) == SOCKET_ERROR_VAL) {
            printf("[DebugServer] listen() failed\n");
            CLOSE_SOCKET((socket_t)listenSocket_);
            listenSocket_ = INVALID_SOCKET_VAL;
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        running_.store(true, std::memory_order_release);
        worker_ = std::thread(&DebugServer::workerLoop, this);

        printf("[DebugServer] Listening on http://localhost:%u\n", port_);
        return true;
    }

    void DebugServer::stop() {
        if (!running_.load(std::memory_order_acquire)) {
            return;
        }

        running_.store(false, std::memory_order_release);

        if (listenSocket_ != INVALID_SOCKET_VAL) {
            CLOSE_SOCKET((socket_t)listenSocket_);
            listenSocket_ = INVALID_SOCKET_VAL;
        }

        if (worker_.joinable()) {
            worker_.join();
        }

#ifdef _WIN32
        WSACleanup();
#endif

        printf("[DebugServer] Stopped\n");
    }

    // ---------------------------------------------------------------------------
    // Background accept loop
    // ---------------------------------------------------------------------------

    void DebugServer::workerLoop() {
        while (running_.load(std::memory_order_acquire)) {
            sockaddr_in clientAddr{};
#ifdef _WIN32
            int addrLen = sizeof(clientAddr);
#else
            socklen_t addrLen = sizeof(clientAddr);
#endif
            socket_t clientSock = accept((socket_t)listenSocket_,
                                         (sockaddr*)&clientAddr, &addrLen);

            if (!running_.load(std::memory_order_acquire)) {
                if (clientSock != INVALID_SOCKET_VAL) {
                    CLOSE_SOCKET(clientSock);
                }
                break;
            }

            if (clientSock == INVALID_SOCKET_VAL) {
                continue;
            }

            handleClient(clientSock);
        }
    }

    // ---------------------------------------------------------------------------
    // Per-client handler — detects HTTP vs plain-text protocol
    // ---------------------------------------------------------------------------

    void DebugServer::handleClient(uint64_t rawSock) {
        socket_t sock = (socket_t)rawSock;

        char buf[8192];
        memset(buf, 0, sizeof(buf));
        int n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            CLOSE_SOCKET(sock);
            return;
        }

        std::string request(buf, n);

        // Detect HTTP vs plain text
        if (request.rfind("GET ", 0) == 0 || request.rfind("POST ", 0) == 0) {
            // ---- HTTP mode ----
            size_t sp1 = request.find(' ');
            size_t sp2 = request.find(' ', sp1 + 1);
            std::string path = (sp1 != std::string::npos && sp2 != std::string::npos)
                                 ? request.substr(sp1 + 1, sp2 - sp1 - 1)
                                 : "/";

            std::string response;
            if (path == "/" || path == "/index.html") {
                response = httpResponse(200, "text/html", getHtmlPage());
            } else if (path == "/api/tree") {
                response = httpResponse(200, "application/json", buildTreeSnapshot());
            } else if (path == "/api/display-tree") {
                response = httpResponse(200, "application/json", buildDisplayTreeSnapshot());
            } else {
                response = httpResponse(404, "application/json",
                                        "{\"error\":\"not found\"}");
            }

            send(sock, response.data(), (int)response.size(), 0);
        } else {
            // ---- Plain-text protocol (telnet / nc) ----
            while (!request.empty() && (request.back() == '\n' || request.back() == '\r')) {
                request.pop_back();
            }

            std::string response;
            if (request == "TREE") {
                response = buildTreeSnapshot() + "\n";
            } else if (request == "DTREE") {
                response = buildDisplayTreeSnapshot() + "\n";
            } else if (request == "QUIT") {
                response = "{\"ok\":true}\n";
            } else if (!request.empty()) {
                response = "{\"error\":\"unknown command: " + escapeJson(request) + "\"}\n";
            }

            if (!response.empty()) {
                send(sock, response.data(), (int)response.size(), 0);
            }
        }

        CLOSE_SOCKET(sock);
    }

    // ---------------------------------------------------------------------------
    // Logical Object tree — walks Object / Component hierarchy
    // ---------------------------------------------------------------------------

    std::string DebugServer::buildTreeSnapshot() {
        auto* stage = Stage::Instance();
        auto* root  = stage ? stage->defaultRoot() : nullptr;
        if (!root) {
            return "{\"error\":\"no root\"}";
        }
        return serializeObject(root);
    }

    std::string DebugServer::serializeObject(Object* obj) {
        if (!obj) return "null";

        std::ostringstream ss;
        ss << "{";

        // ---- identity ----
        ss << "\"type\":\"" << objectTypeToString(obj->objectType()) << "\"";
        ss << ",\"name\":\"" << escapeJson(obj->name()) << "\"";
        ss << ",\"id\":\""   << escapeJson(obj->id())   << "\"";

        // ---- geometry ----
        ss << ",\"x\":"         << obj->x();
        ss << ",\"y\":"         << obj->y();
        ss << ",\"width\":"     << obj->width();
        ss << ",\"height\":"    << obj->height();
        ss << ",\"sourceWidth\":"  << obj->sourceWidth();
        ss << ",\"sourceHeight\":" << obj->sourceHeight();
        ss << ",\"rawWidth\":"     << obj->rawWidth();
        ss << ",\"rawHeight\":"    << obj->rawHeight();
        ss << ",\"initWidth\":"    << obj->initWidth();
        ss << ",\"initHeight\":"   << obj->initHeight();

        // ---- transform ----
        ss << ",\"scaleX\":"     << obj->scaleX();
        ss << ",\"scaleY\":"     << obj->scaleY();
        ss << ",\"skewX\":"      << obj->skewX();
        ss << ",\"skewY\":"      << obj->skewY();
        ss << ",\"pivotX\":"     << obj->pivot().x;
        ss << ",\"pivotY\":"     << obj->pivot().y;
        ss << ",\"pivotAsAnchor\":" << (obj->isPivotAsAnchor() ? "true" : "false");
        ss << ",\"rotation\":"   << obj->rotation();
        ss << ",\"alpha\":"      << obj->alpha();

        // ---- flags ----
        ss << ",\"visible\":"    << (obj->visible()   ? "true" : "false");
        ss << ",\"touchable\":"  << (obj->touchable() ? "true" : "false");
        ss << ",\"grayed\":"     << (obj->grayed()    ? "true" : "false");
        ss << ",\"sortingOrder\":" << obj->sortingOrder();

        // ---- children (Component only) ----
        Component* comp = dynamic_cast<Component*>(obj);
        if (comp && comp->numChildren() > 0) {
            ss << ",\"children\":[";
            for (int i = 0; i < comp->numChildren(); ++i) {
                if (i > 0) ss << ",";
                ss << serializeObject(comp->getChildAt(i));
            }
            ss << "]";
        }

        ss << "}";
        return ss.str();
    }

    // ---------------------------------------------------------------------------
    // ECS DisplayObject tree — walks entt::entity hierarchy
    // ---------------------------------------------------------------------------

    std::string DebugServer::buildDisplayTreeSnapshot() {
        auto* stage = Stage::Instance();
        auto* root  = stage ? stage->defaultRoot() : nullptr;
        if (!root) {
            return "{\"error\":\"no root\"}";
        }

        entt::entity rootEntity = root->getDisplayObject();
        if (rootEntity == entt::null) {
            return "{\"error\":\"no display root\"}";
        }

        return serializeDisplayEntity(rootEntity);
    }

    std::string DebugServer::serializeDisplayEntity(entt::entity entity) {
        if (entity == entt::null) return "null";

        std::ostringstream ss;
        ss << "{";

        // ---- entity identity ----
        ss << "\"_entityId\":\"" << entityIdStr(entity) << "\"";

        // ---- collect component names ----
        std::vector<std::string> comps;

        // Check each component
        if (reg.any_of<dispcomp::is_root>(entity))         comps.push_back("is_root");
        if (reg.any_of<dispcomp::visible>(entity))         comps.push_back("visible");
        if (reg.any_of<dispcomp::final_visible>(entity))   comps.push_back("final_visible");
        if (reg.any_of<dispcomp::children>(entity))        comps.push_back("children");
        if (reg.any_of<dispcomp::parent>(entity))          comps.push_back("parent");
        if (reg.any_of<dispcomp::parent_batch>(entity))    comps.push_back("parent_batch");
        if (reg.any_of<dispcomp::batch_node>(entity))      comps.push_back("batch_node");
        if (reg.any_of<dispcomp::batch_data>(entity))      comps.push_back("batch_data");
        if (reg.any_of<dispcomp::graphics>(entity))        comps.push_back("graphics");
        if (reg.any_of<dispcomp::basic_transform>(entity)) comps.push_back("basic_transform");
        if (reg.any_of<dispcomp::skew>(entity))            comps.push_back("skew");
        if (reg.any_of<dispcomp::scale>(entity))           comps.push_back("scale");
        if (reg.any_of<dispcomp::rotation>(entity))        comps.push_back("rotation");
        if (reg.any_of<dispcomp::owner>(entity))           comps.push_back("owner");
        if (reg.any_of<dispcomp::image_desc_t>(entity))      comps.push_back("image_desc_t");
        if (reg.any_of<dispcomp::image_ext>(entity))       comps.push_back("image_ext");
        if (reg.any_of<dispcomp::font_mesh>(entity))       comps.push_back("font_mesh");
        if (reg.any_of<dispcomp::transform_dirty>(entity)) comps.push_back("transform_dirty");
        if (reg.any_of<dispcomp::batch_need_rebuild>(entity)) comps.push_back("batch_node_dirty");
        if (reg.any_of<dispcomp::batch_dirty>(entity))     comps.push_back("batch_dirty");
        if (reg.any_of<dispcomp::visible_dirty>(entity))   comps.push_back("visible_dirty");
        if (reg.any_of<dispcomp::mesh_dirty>(entity))      comps.push_back("mesh_dirty");

        // components array
        ss << ",\"_components\":[";
        for (size_t i = 0; i < comps.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "\"" << comps[i] << "\"";
        }
        ss << "]";

        // ---- basic_transform ----
        if (reg.any_of<dispcomp::basic_transform>(entity)) {
            auto& tf = reg.get<dispcomp::basic_transform>(entity);
            ss << ",\"position\":{\"x\":" << tf.position.x << ",\"y\":" << tf.position.y << "}";
            ss << ",\"size\":{\"width\":" << tf.size.x << ",\"height\":" << tf.size.y << "}";
            ss << ",\"pivot\":{\"x\":" << tf.pivot.x << ",\"y\":" << tf.pivot.y << "}";
        }

        // ---- visibility ----
        ss << ",\"visible\":"      << (isVisible(entity)      ? "true" : "false");
        ss << ",\"finalVisible\":" << (isFinalVisible(entity) ? "true" : "false");
        ss << ",\"isRoot\":"       << (reg.any_of<dispcomp::is_root>(entity) ? "true" : "false");
        ss << ",\"isBatchNode\":"  << (isBatchNode(entity) ? "true" : "false");

        // ---- batch node info ----
        if (reg.any_of<dispcomp::batch_node>(entity)) {
            auto& bn = reg.get<dispcomp::batch_node>(entity);
            ss << ",\"batchChildren\":" << bn.children.size();
            ss << ",\"subBatchNodes\":" << bn.batchNodes.size();
        }

        // ---- parent_batch ----
        if (reg.any_of<dispcomp::parent_batch>(entity)) {
            auto& pb = reg.get<dispcomp::parent_batch>(entity);
            ss << ",\"parentBatch\":\"" << entityIdStr(pb.val) << "\"";
            ss << ",\"instIndex\":" << pb.instIndex;
        }

        // ---- graphics ----
        if (reg.any_of<dispcomp::graphics>(entity)) {
            auto& gfx = reg.get<dispcomp::graphics>(entity);
            ss << ",\"hasGraphics\":true";
            const char* rtype = "None";
            switch (gfx.graphics.renderItem.type) {
            case RenderItemType::Image: rtype = "Image"; break;
            case RenderItemType::Font:  rtype = "Font";  break;
            case RenderItemType::None:  rtype = "None";  break;
            default: rtype = "?"; break;
            }
            ss << ",\"renderType\":\"" << rtype << "\"";
            ss << ",\"hasTexture\":" << (gfx.graphics.texture ? "true" : "false");
        } else {
            ss << ",\"hasGraphics\":false";
        }

        // ---- scale/skew/rotation ----
        if (reg.any_of<dispcomp::scale>(entity)) {
            auto& sc = reg.get<dispcomp::scale>(entity);
            ss << ",\"scale\":{\"x\":" << sc.val.x << ",\"y\":" << sc.val.y << "}";
        }
        if (reg.any_of<dispcomp::skew>(entity)) {
            auto& sk = reg.get<dispcomp::skew>(entity);
            ss << ",\"skew\":{\"x\":" << sk.val.x << ",\"y\":" << sk.val.y << "}";
        }
        if (reg.any_of<dispcomp::rotation>(entity)) {
            auto& rot = reg.get<dispcomp::rotation>(entity);
            ss << ",\"rotation\":" << rot.val;
        }

        // ---- parent ----
        if (reg.any_of<dispcomp::parent>(entity)) {
            auto& p = reg.get<dispcomp::parent>(entity);
            ss << ",\"_parent\":\"" << entityIdStr(p.val) << "\"";
        }

        // ---- dirty flags ----
        ss << ",\"transform_dirty\":"  << (reg.any_of<dispcomp::transform_dirty>(entity)  ? "true" : "false");
        ss << ",\"batch_node_dirty\":" << (reg.any_of<dispcomp::batch_need_rebuild>(entity) ? "true" : "false");
        ss << ",\"batch_dirty\":"      << (reg.any_of<dispcomp::batch_dirty>(entity)      ? "true" : "false");
        ss << ",\"visible_dirty\":"    << (reg.any_of<dispcomp::visible_dirty>(entity)    ? "true" : "false");
        ss << ",\"mesh_dirty\":"       << (reg.any_of<dispcomp::mesh_dirty>(entity)       ? "true" : "false");

        // ---- children (recursive) ----
        auto* childrenComp = getChildren(entity);
        if (childrenComp && !childrenComp->val.empty()) {
            ss << ",\"_children\":[";
            for (size_t i = 0; i < childrenComp->val.size(); ++i) {
                if (i > 0) ss << ",";
                ss << serializeDisplayEntity(childrenComp->val[i]);
            }
            ss << "]";
        }

        ss << "}";
        return ss.str();
    }

    // ---------------------------------------------------------------------------
    // Simple JSON string escaping
    // ---------------------------------------------------------------------------

    std::string DebugServer::escapeJson(std::string const& s) {
        std::string result;
        result.reserve(s.size() + 8);
        for (char ch : s) {
            switch (ch) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += ch;     break;
            }
        }
        return result;
    }

} // namespace gui
