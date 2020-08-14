#include <thread>
#include <mutex>

namespace ugi {

    class ITask {
    public:
        virtual bool execute();
    };

    class TextureUpdateTask {
    private:
        uint8_t*            _data;
        uint32_t            _length;
    public:
    };

    class WorkThread {
    public:
        struct Handle {
            uint32_t id;
        };
    private:
        std::thread     _thread;
    public:
        WorkThread() {
        }
        Handle postWork( ITask* task );
    };

}