#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <entt/src/entt/entity/fwd.hpp>

namespace gui {

    /// <summary>
    /// TCP/HTTP/WebSocket debug server for widget tree inspection.
    /// Runs in a background thread so it never blocks the UI/render loop.
    ///
    /// HTTP (browser):
    ///   GET /              → simple redirect page
    ///   GET /api/tree      → JSON: logical Object tree
    ///   GET /api/display-tree → JSON: ECS DisplayObject tree
    ///
    /// WebSocket (browser — open bin/debug/debug.html):
    ///   ws://localhost:9876/ws  → real-time tree updates (JSON push)
    ///
    /// Plain-text protocol (telnet / nc):
    ///   TREE   → JSON snapshot of the logical Object tree
    ///   DTREE  → JSON snapshot of the ECS DisplayObject tree
    ///   QUIT   → closes the connection
    ///
    /// Default port: 9876
    /// </summary>
    class DebugServer {
    public:
        DebugServer();
        ~DebugServer();

        /// Start listening on the given port. Safe to call multiple times (no-op if already running).
        bool start(uint16_t port = 9876);

        /// Shut down the server and join the worker thread.
        void stop();

        /// Whether the server is currently listening.
        bool isRunning() const { return running_.load(std::memory_order_relaxed); }

        /// Request a push update to all connected WebSocket clients.
        /// Thread-safe — call from any thread (typically the render thread).
        void pushUpdate();

        /// Convenience: get or create the single global instance.
        static DebugServer& Instance();

    private:
        void workerLoop();
        void handleClient(uint64_t clientSocket);

        // WebSocket helpers
        std::string wsAcceptKey(std::string const& clientKey);
        void        sendWsFrame(uint64_t sock, std::string const& payload);
        bool        tryWsHandshake(uint64_t sock, std::string const& request);
        void        broadcastToWsClients();

        // Logical Object tree
        std::string buildTreeSnapshot();
        std::string serializeObject(class Object* obj);

        // ECS DisplayObject tree
        std::string buildDisplayTreeSnapshot();
        std::string serializeDisplayEntity(entt::entity entity);

        std::string escapeJson(std::string const& s);

        std::thread             worker_;
        uint64_t                listenSocket_;   // SOCKET on Windows
        std::atomic<bool>       running_;
        std::atomic<bool>       pushPending_{false};
        uint16_t                port_;

        std::mutex              wsMutex_;
        std::vector<uint64_t>   wsClients_;      // raw SOCKET values
    };

} // namespace gui
