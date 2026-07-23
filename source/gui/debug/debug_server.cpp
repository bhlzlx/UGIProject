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
    // Minimal SHA1 (RFC 3174) — used only for WebSocket handshake
    // ---------------------------------------------------------------------------

    struct Sha1 {
        uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
        uint64_t total = 0;
        uint8_t  buf[64] = {};
        int      bufLen = 0;

        void update(void const* data, size_t len) {
            auto* p = (uint8_t const*)data;
            total += len;
            while (len > 0) {
                int space = 64 - bufLen;
                int n = (int)(len < (size_t)space ? len : (size_t)space);
                memcpy(buf + bufLen, p, n);
                bufLen += n; p += n; len -= n;
                if (bufLen == 64) { processBlock(); bufLen = 0; }
            }
        }

        void finish(uint8_t digest[20]) {
            uint64_t bits = total * 8;
            buf[bufLen++] = 0x80;
            if (bufLen > 56) { while (bufLen < 64) buf[bufLen++] = 0; processBlock(); bufLen = 0; }
            while (bufLen < 56) buf[bufLen++] = 0;
            for (int i = 0; i < 8; ++i) buf[56 + i] = (uint8_t)(bits >> (56 - i * 8));
            processBlock();
            for (int i = 0; i < 5; ++i)
                for (int j = 0; j < 4; ++j) digest[i * 4 + j] = (uint8_t)(h[i] >> (24 - j * 8));
        }

    private:
        void processBlock() {
            uint32_t w[80];
            for (int i = 0; i < 16; ++i) w[i] = ((uint32_t)buf[i*4]<<24)|((uint32_t)buf[i*4+1]<<16)|((uint32_t)buf[i*4+2]<<8)|buf[i*4+3];
            for (int i = 16; i < 80; ++i) { uint32_t t = w[i-3]^w[i-8]^w[i-14]^w[i-16]; w[i] = (t<<1)|(t>>31); }
            uint32_t a=h[0], b=h[1], c=h[2], d=h[3], e=h[4];
            for (int i = 0; i < 80; ++i) {
                uint32_t f, k;
                if (i<20)      { f=(b&c)|(~b&d);        k=0x5A827999; }
                else if (i<40) { f=b^c^d;               k=0x6ED9EBA1; }
                else if (i<60) { f=(b&c)|(b&d)|(c&d);   k=0x8F1BBCDC; }
                else           { f=b^c^d;               k=0xCA62C1D6; }
                uint32_t t = ((a<<5)|(a>>27)) + f + e + k + w[i];
                e=d; d=c; c=(b<<30)|(b>>2); b=a; a=t;
            }
            h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e;
        }
    };

    static std::string base64Encode(uint8_t const* data, size_t len) {
        static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve(((len + 2) / 3) * 4);
        for (size_t i = 0; i < len; i += 3) {
            uint32_t v = ((uint32_t)data[i] << 16) | ((i+1<len?data[i+1]:0) << 8) | (i+2<len?data[i+2]:0);
            out += tbl[(v>>18)&0x3F];
            out += tbl[(v>>12)&0x3F];
            out += (i+1<len) ? tbl[(v>>6)&0x3F]  : '=';
            out += (i+2<len) ? tbl[v&0x3F]        : '=';
        }
        return out;
    }

    // ---------------------------------------------------------------------------
    // WebSocket helpers
    // ---------------------------------------------------------------------------

    static const char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::string DebugServer::wsAcceptKey(std::string const& clientKey) {
        std::string combined = clientKey + WS_GUID;
        Sha1 sha1;
        sha1.update(combined.data(), combined.size());
        uint8_t digest[20];
        sha1.finish(digest);
        return base64Encode(digest, 20);
    }

    void DebugServer::sendWsFrame(uint64_t rawSock, std::string const& payload) {
        socket_t sock = (socket_t)rawSock;
        std::vector<uint8_t> frame;
        frame.reserve(2 + 8 + payload.size());
        frame.push_back(0x81); // FIN + text opcode
        size_t len = payload.size();
        if (len < 126) {
            frame.push_back((uint8_t)len);
        } else if (len <= 0xFFFF) {
            frame.push_back(126);
            frame.push_back((uint8_t)(len >> 8));
            frame.push_back((uint8_t)(len));
        } else {
            frame.push_back(127);
            for (int i = 7; i >= 0; --i) frame.push_back((uint8_t)(len >> (i*8)));
        }
        frame.insert(frame.end(), payload.begin(), payload.end());
        send(sock, (char const*)frame.data(), (int)frame.size(), 0);
    }

    bool DebugServer::tryWsHandshake(uint64_t rawSock, std::string const& request) {
        // Look for "Upgrade: websocket" header
        if (request.find("Upgrade: websocket") == std::string::npos)
            return false;

        // Extract Sec-WebSocket-Key
        auto pos = request.find("Sec-WebSocket-Key: ");
        if (pos == std::string::npos) return false;
        pos += 19;
        auto end = request.find("\r\n", pos);
        std::string key = request.substr(pos, end - pos);

        std::string accept = wsAcceptKey(key);
        std::string response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " + accept + "\r\n"
            "\r\n";
        socket_t sock = (socket_t)rawSock;
        send(sock, response.data(), (int)response.size(), 0);

        // Add to WS client list
        {
            std::lock_guard<std::mutex> lock(wsMutex_);
            wsClients_.push_back(rawSock);
            printf("[DebugServer] WebSocket client connected (total: %zu)\n", wsClients_.size());
        }
        return true;
    }

    void DebugServer::broadcastToWsClients() {
        std::string objJson = buildTreeSnapshot();
        std::string dispJson = buildDisplayTreeSnapshot();

        // Compact JSON: {"object":...,"display":...}
        std::string payload = "{\"object\":" + objJson + ",\"display\":" + dispJson + "}";

        std::lock_guard<std::mutex> lock(wsMutex_);
        for (auto it = wsClients_.begin(); it != wsClients_.end(); ) {
            socket_t sock = (socket_t)*it;
            // Send a ping-like check: try a 0-byte peek to detect dead socket
            char dummy;
            int n = recv(sock, &dummy, 1, MSG_PEEK);
            if (n == 0 || (n < 0
#ifdef _WIN32
                && WSAGetLastError() != WSAEWOULDBLOCK
#else
                && errno != EAGAIN && errno != EWOULDBLOCK
#endif
            )) {
                CLOSE_SOCKET(sock);
                it = wsClients_.erase(it);
                continue;
            }
            sendWsFrame(*it, payload);
            ++it;
        }
    }

    // ---------------------------------------------------------------------------
    // Simple redirect page (served at GET /)
    // ---------------------------------------------------------------------------

    static std::string getHtmlPage() {
        return R"HTML(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><title>UGI UI Debugger</title></head>
<body style="background:#1a1a2e;color:#e0e0e0;font-family:system-ui;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;">
<div style="text-align:center;">
<h1>🔧 UGI UI Debugger</h1>
<p>Open <code>bin/debug/debug.html</code> for the real-time WebSocket viewer.</p>
<p>HTTP API: <a href="/api/tree" style="color:#e94560;">/api/tree</a> &nbsp;|&nbsp; <a href="/api/display-tree" style="color:#53a8b6;">/api/display-tree</a></p>
</div>
</body></html>)HTML";
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
    // Receive & unmask a WebSocket text frame from client
    // Returns the unmasked payload, or empty string on error/close.
    // ---------------------------------------------------------------------------

    static std::string recvWsFrame(socket_t sock) {
        uint8_t hdr[2];
        int n = recv(sock, (char*)hdr, 2, 0);
        if (n <= 0) return {};

        bool fin   = (hdr[0] & 0x80) != 0;
        int opcode = hdr[0] & 0x0F;
        bool masked = (hdr[1] & 0x80) != 0;

        // Close frame → return empty to signal disconnect
        if (opcode == 0x8) return {};

        uint64_t payLen = hdr[1] & 0x7F;
        if (payLen == 126) {
            uint8_t ext[2];
            if (recv(sock, (char*)ext, 2, 0) != 2) return {};
            payLen = ((uint64_t)ext[0] << 8) | ext[1];
        } else if (payLen == 127) {
            uint8_t ext[8];
            if (recv(sock, (char*)ext, 8, 0) != 8) return {};
            payLen = 0;
            for (int i = 0; i < 8; ++i) payLen = (payLen << 8) | ext[i];
        }

        // Read mask key (required for client→server)
        uint8_t mask[4] = {};
        if (masked) {
            if (recv(sock, (char*)mask, 4, 0) != 4) return {};
        }

        // Read payload
        if (payLen > 65536) return {}; // sanity limit
        std::vector<uint8_t> payload((size_t)payLen);
        size_t total = 0;
        while (total < payLen) {
            n = recv(sock, (char*)payload.data() + total, (int)(payLen - total), 0);
            if (n <= 0) return {};
            total += n;
        }

        // Unmask
        if (masked) {
            for (size_t i = 0; i < (size_t)payLen; ++i)
                payload[i] ^= mask[i & 3];
        }

        return std::string(payload.begin(), payload.end());
    }

    // ---------------------------------------------------------------------------
    // Background accept loop (select-based, non-blocking)
    // ---------------------------------------------------------------------------

    void DebugServer::workerLoop() {
        while (running_.load(std::memory_order_acquire)) {
            fd_set readSet;
            FD_ZERO(&readSet);
            socket_t listenSock = (socket_t)listenSocket_;
            FD_SET(listenSock, &readSet);
            socket_t maxSock = listenSock;

            // Add WebSocket clients to the read set
            {
                std::lock_guard<std::mutex> lock(wsMutex_);
                for (auto raw : wsClients_) {
                    socket_t s = (socket_t)raw;
                    FD_SET(s, &readSet);
                    if (s > maxSock) maxSock = s;
                }
            }

            timeval tv = {0, 100000}; // 100 ms
            int ret = select((int)maxSock + 1, &readSet, nullptr, nullptr, &tv);

            if (ret > 0) {
                // New connection?
                if (FD_ISSET(listenSock, &readSet)) {
                    sockaddr_in clientAddr{};
#ifdef _WIN32
                    int addrLen = sizeof(clientAddr);
#else
                    socklen_t addrLen = sizeof(clientAddr);
#endif
                    socket_t clientSock = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);
                    if (clientSock != INVALID_SOCKET_VAL) {
                        handleClient(clientSock);
                    }
                }

                // Handle WebSocket client messages
                {
                    std::lock_guard<std::mutex> lock(wsMutex_);
                    for (auto it = wsClients_.begin(); it != wsClients_.end(); ) {
                        socket_t s = (socket_t)*it;
                        if (FD_ISSET(s, &readSet)) {
                            std::string msg = recvWsFrame(s);
                            if (msg.empty()) {
                                // Client disconnected or sent close frame
                                CLOSE_SOCKET(s);
                                it = wsClients_.erase(it);
                                printf("[DebugServer] WebSocket client disconnected (total: %zu)\n", wsClients_.size());
                                continue;
                            }
                            // Handle commands
                            if (msg == "tree") {
                                sendWsFrame(*it, buildTreeSnapshot());
                            } else if (msg == "display") {
                                sendWsFrame(*it, buildDisplayTreeSnapshot());
                            } else if (msg == "both") {
                                std::string payload = "{\"object\":" + buildTreeSnapshot()
                                                    + ",\"display\":" + buildDisplayTreeSnapshot() + "}";
                                sendWsFrame(*it, payload);
                            }
                        }
                        ++it;
                    }
                }
            }

            // Broadcast pending push (for pushUpdate() calls)
            if (pushPending_.exchange(false, std::memory_order_acquire)) {
                broadcastToWsClients();
            }
        }

        // Close all remaining WS clients on shutdown
        {
            std::lock_guard<std::mutex> lock(wsMutex_);
            for (auto raw : wsClients_) CLOSE_SOCKET((socket_t)raw);
            wsClients_.clear();
        }
    }

    // ---------------------------------------------------------------------------
    // Per-client handler — detects HTTP vs WebSocket vs plain-text protocol
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

        // Detect HTTP (includes WebSocket upgrade)
        if (request.rfind("GET ", 0) == 0 || request.rfind("POST ", 0) == 0) {
            size_t sp1 = request.find(' ');
            size_t sp2 = request.find(' ', sp1 + 1);
            std::string path = (sp1 != std::string::npos && sp2 != std::string::npos)
                                 ? request.substr(sp1 + 1, sp2 - sp1 - 1)
                                 : "/";

            // ---- WebSocket upgrade? ----
            if (path == "/ws" && tryWsHandshake(rawSock, request)) {
                return; // Socket stays open, handled by broadcast
            }

            // ---- HTTP mode ----
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

        // Only close non-WS sockets (WS sockets were kept open by tryWsHandshake)
        CLOSE_SOCKET(sock);
    }

    // ---------------------------------------------------------------------------
    // pushUpdate — called from render thread to request a broadcast
    // ---------------------------------------------------------------------------

    void DebugServer::pushUpdate() {
        pushPending_.store(true, std::memory_order_release);
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
        auto dobj = obj->getDisplayObject();
        ss << ",\"_entityVal\":" << (dobj ? (uint64_t)entt::to_integral(dobj.entity()) : 0ULL);

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
        ss << ",\"_entityVal\":" << (uint64_t)entt::to_integral(entity);

        // ---- collect component names ----
        std::vector<std::string> comps;

        // Check each component
        if (reg.any_of<dispcomp::is_root>(entity))         comps.push_back("is_root");
        if (reg.any_of<dispcomp::visible>(entity))         comps.push_back("visible");
        if (reg.any_of<dispcomp::final_visible>(entity))   comps.push_back("final_visible");
        if (reg.any_of<dispcomp::children>(entity))        comps.push_back("children");
        if (reg.any_of<dispcomp::parent>(entity))          comps.push_back("parent");
        if (reg.any_of<dispcomp::item_batch_info>(entity))    comps.push_back("parent_batch");
        if (reg.any_of<dispcomp::batch_node>(entity))      comps.push_back("batch_node");
        if (reg.any_of<dispcomp::batch_data>(entity))      comps.push_back("batch_data");
        if (reg.any_of<dispcomp::item_render_data>(entity))        comps.push_back("graphics");
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
        if (reg.any_of<dispcomp::item_batch_info>(entity)) {
            auto& pb = reg.get<dispcomp::item_batch_info>(entity);
            ss << ",\"parentBatch\":\"" << entityIdStr(pb.batchEntity) << "\"";
            ss << ",\"instIndex\":" << pb.instIndex;
        }

        // ---- graphics ----
        if (reg.any_of<dispcomp::item_render_data>(entity)) {
            auto& gfx = reg.get<dispcomp::item_render_data>(entity);
            ss << ",\"hasGraphics\":true";
            const char* rtype = "None";
            switch (gfx.meshData.type) {
            case UIMeshType::Image: rtype = "Image"; break;
            case UIMeshType::Font:  rtype = "Font";  break;
            case UIMeshType::None:  rtype = "None";  break;
            default: rtype = "?"; break;
            }
            ss << ",\"renderType\":\"" << rtype << "\"";
            ss << ",\"hasTexture\":" << (gfx.texture ? "true" : "false");
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
