// Android entry point — replaces WinMain
#include <android_native_app_glue.h>
#include <android/log.h>
#include <UGIApplication.h>
#include "AndroidArchive.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "UGI", __VA_ARGS__)

extern UGIApplication* GetApplication();
static UGIApplication* g_app = nullptr;

static void handleCmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("Window init");
            if (app->window && g_app) {
                auto* arch = comm::CreateAndroidArchive(
                    app->activity->assetManager,
                    std::string(app->activity->externalDataPath) + "/"
                );
                g_app->initialize(app->window, arch);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("Window term");
            break;
        case APP_CMD_WINDOW_RESIZED:
            if (g_app) g_app->resize(ANativeWindow_getWidth(app->window),
                                     ANativeWindow_getHeight(app->window));
            break;
    }
}

void android_main(struct android_app* state) {
    g_app = GetApplication();
    state->onAppCmd = handleCmd;

    while (1) {
        int events;
        struct android_poll_source* source;
        int ident = ALooper_pollAll(0, nullptr, &events, (void**)&source);
        if (ident >= 0 && source) {
            source->process(state, source);
        }
        if (state->destroyRequested) {
            if (g_app) g_app->release();
            return;
        }
        if (state->window && g_app) {
            g_app->tick();
        }
    }
}
