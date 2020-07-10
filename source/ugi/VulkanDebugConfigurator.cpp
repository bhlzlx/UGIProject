#include <string>
#include <vector>
#include "VulkanDebugConfigurator.h"

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

namespace ugi {

    bool DebugReporterVk::setupDebugReport(VkInstance _inst)
    {
        vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_inst, "vkCreateDebugReportCallbackEXT");
        vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_inst, "vkDestroyDebugReportCallbackEXT");
        vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(_inst, "vkDebugReportMessageEXT");

        VkDebugReportFlagsEXT debugReportFlags =
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
            | VK_DEBUG_REPORT_ERROR_BIT_EXT
            | VK_DEBUG_REPORT_WARNING_BIT_EXT
            //|VK_DEBUG_REPORT_DEBUG_BIT_EXT
            //|VK_DEBUG_REPORT_INFORMATION_BIT_EXT
            ;
        VkDebugReportCallbackCreateInfoEXT DebugReportInfo = {
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
            nullptr,
            (VkDebugReportFlagsEXT)debugReportFlags,
            DebugReportCallback,
            nullptr
        };
        auto rst = vkCreateDebugReportCallbackEXT(_inst, &DebugReportInfo, nullptr, &m_debugReportCallback);
        return  rst == VK_SUCCESS;
    }

    bool DebugReporterVk::uninstallDebugReport(VkInstance _inst)
    {
        vkDestroyDebugReportCallbackEXT(_inst, m_debugReportCallback, nullptr);
        return true;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugReporterVk::DebugReportCallback(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char * pLayerPrefix, const char * pMsg, void * pUserData)
#ifdef __ANDROID__
    {
        if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            __android_log_print(ANDROID_LOG_ERROR,
                "AppName",
                "ERROR: [%s] Code %i : %s",
                pLayerPrefix, msgCode, pMsg);
        }
        else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
            __android_log_print(ANDROID_LOG_WARN,
                "AppName",
                "WARNING: [%s] Code %i : %s",
                pLayerPrefix, msgCode, pMsg);
        }
        else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
            __android_log_print(ANDROID_LOG_WARN,
                "AppName",
                "PERFORMANCE WARNING: [%s] Code %i : %s",
                pLayerPrefix, msgCode, pMsg);
        }
        else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
            __android_log_print(ANDROID_LOG_INFO,
                "AppName", "INFO: [%s] Code %i : %s",
                pLayerPrefix, msgCode, pMsg);
        }
        else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
            __android_log_print(ANDROID_LOG_VERBOSE,
                "AppName", "DEBUG: [%s] Code %i : %s",
                pLayerPrefix, msgCode, pMsg);
        }

        return VK_FALSE;
    }
#else
    {
        char debugInfo[2048];
        sprintf(debugInfo, "code : %d\n layer : %s\n message : %s\n", msgCode, pLayerPrefix, pMsg);
        //
        printf(debugInfo);
        //
        return 0;
    }
#endif
}
