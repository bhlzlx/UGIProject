#pragma once
#include "VulkanFunctionDeclare.h"
//
extern PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
extern PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
extern PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;

namespace ugi {

    class DebugReporterVk
    {
    private:
        VkDebugReportCallbackEXT m_debugReportCallback;
    public:
        DebugReporterVk()
        : m_debugReportCallback((VkDebugReportCallbackEXT)nullptr){
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
