#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

#include <entt/src/entt/entity/fwd.hpp>

namespace gui {

    /// <summary>
    /// TCP/HTTP debug server for widget tree inspection.
    /// Runs in a background thread so it never blocks the UI/render loop.
    ///
    /// HTTP (browser):
    ///   GET /              → interactive tree viewer page
    ///   GET /api/tree      → JSON: logical Object tree
    ///   GET /api/display-tree → JSON: ECS DisplayObject tree
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

        /// Convenience: get or create the single global instance.
        static DebugServer& Instance();

    private:
        void workerLoop();
        void handleClient(uint64_t clientSocket);

        // Logical Object tree
        std::string buildTreeSnapshot();
        std::string serializeObject(class Object* obj);

        // ECS DisplayObject tree
        std::string buildDisplayTreeSnapshot();
        std::string serializeDisplayEntity(entt::entity entity);

        std::string escapeJson(std::string const& s);

        std::thread     worker_;
        uint64_t        listenSocket_;   // SOCKET on Windows
        std::atomic<bool> running_;
        uint16_t        port_;
    };

} // namespace gui
