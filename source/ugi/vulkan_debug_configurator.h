#pragma once
#include "vulkan_function_declare.h"
//
extern PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
extern PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
extern PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;

namespace ugi {

    class DebugReporterVk
    {
    private:
        VkDebugReportCallbackEXT _debugReportCallback;
    public:
        DebugReporterVk()
        : _debugReportCallback((VkDebugReportCallbackEXT)nullptr){
        }
        bool setupDebugReport( VkInstance _inst );
        bool uninstallDebugReport(VkInstance _inst);

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
            VkDebugReportFlagsEXT msgFlags,
            VkDebugReportObjectTypeEXT objType,
            uint64_t srcObject, size_t location,
            int32_t msgCode, const char * pLayerPrefix,
            const char * pMsg, void * pUserData);
    };

}
