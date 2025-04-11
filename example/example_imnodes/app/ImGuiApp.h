// ImGuiApp.h
#pragma once
#ifndef IMGUIAPP_H
#define IMGUIAPP_H

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <stdexcept> // 用于异常处理

// Volk headers (可选，如果使用)
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION // 只在一个 .cpp 文件中定义
#include <volk.h>
#endif

// 前向声明 ImNodes (如果需要直接在头文件使用)
// struct ImNodesContext; // 如果只在 cpp 文件用，则不需要

// 定义一个运行时错误类，用于初始化失败
class AppInitError : public std::runtime_error
{
public:
    AppInitError(const std::string& message) : std::runtime_error(message) {}
};

class ImGuiApp
{
public:
    // 构造函数：初始化 SDL, Vulkan, ImGui
    ImGuiApp(const std::string& windowTitle, int width, int height);
    // 虚析构函数：确保派生类的析构函数被调用，并执行清理
    virtual ~ImGuiApp();

    // 禁止拷贝和赋值
    ImGuiApp(const ImGuiApp&) = delete;
    ImGuiApp& operator=(const ImGuiApp&) = delete;

    // 运行主循环
    void Run();

    // 加载字体 (TTF)
    // path: 字体文件路径
    // size: 字体大小（像素）
    // glyphRanges: (可选) 指定字形范围 (例如 ImGui::GetIO().Fonts->GetGlyphRangesChinese())
    // merge: (可选) 是否合并到上一个字体中
    // 返回加载的字体指针，如果失败则返回 nullptr
    ImFont* LoadFont(
        const std::string& path,
        float              size,
        const ImWchar*     glyphRanges = nullptr,
        bool               merge = false);

protected:
    // 用户必须实现的函数，用于每帧绘制 ImGui 界面
    virtual void OnFrame() = 0;

    // 可选：允许派生类访问清屏颜色
    ImVec4      m_clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    SDL_Window* GetSDLWindow() const { return m_window; } // 允许派生类访问窗口

private:
    // 初始化阶段
    void InitSDL(const std::string& windowTitle, int width, int height);
    void InitVulkan();
    void SetupVulkanWindow(int width, int height);
    void InitImGui();
    void UploadFonts(); // 将加载的字体上传到 GPU

    // 清理阶段
    void CleanupVulkanWindow();
    void CleanupVulkan();
    void CleanupImGui();
    void CleanupSDL();

    // 每帧逻辑
    void BeginFrame();
    void RenderFrame();
    void PresentFrame();
    void HandleResize(int newWidth, int newHeight);

    // Vulkan 辅助函数 (作为私有静态成员或移至单独的工具类)
    static void check_vk_result(VkResult err);
#ifdef APP_USE_VULKAN_DEBUG_REPORT
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(
        VkDebugReportFlagsEXT      flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t                   object,
        size_t                     location,
        int32_t                    messageCode,
        const char*                pLayerPrefix,
        const char*                pMessage,
        void*                      pUserData);
#endif
    static bool IsExtensionAvailable(
        const ImVector<VkExtensionProperties>& properties,
        const char*                            extension);

    // 成员变量
    SDL_Window* m_window = nullptr;
    bool        m_running = false;
    bool        m_minimized = false;

    // Vulkan Objects (替换原来的全局变量)
    VkAllocationCallbacks*   m_allocator = nullptr; // 通常为 nullptr，除非特殊分配
    VkInstance               m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice         m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                 m_device = VK_NULL_HANDLE;
    uint32_t                 m_queueFamily = (uint32_t)-1;
    VkQueue                  m_queue = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT m_debugReport = VK_NULL_HANDLE;
    VkPipelineCache          m_pipelineCache = VK_NULL_HANDLE; // ImGui 需要，但此例中可为 NULL
    VkDescriptorPool         m_descriptorPool = VK_NULL_HANDLE;

    ImGui_ImplVulkanH_Window m_mainWindowData;
    uint32_t                 m_minImageCount = 2;
    bool                     m_swapChainRebuild = false;

    // 字体加载相关
    bool m_fontsLoaded = false; // 标记字体是否已上传
};

#endif // IMGUIAPP_H